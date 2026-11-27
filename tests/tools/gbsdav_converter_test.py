#!/usr/bin/env python3
"""Compare a local conversion with a captured run; never contacts a server.

The expected JSON was captured from Perl. Use --script to check another
implementation. The capture includes every output byte and both streams.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


MASTER = '''NAME="Local Basis"
GENR=F
SHARED=T
CATEGORY=ORBITAL
TYPE="MB"
SPHR=N
INFO=
"Local information NA
continued NA "
FILE="LOCAL.BAS"
ATMN=1 118 ENDATMNOS
!
NAME="Local (+*)"
GENR=F
SHARED=T
CATEGORY=ORBITAL
TYPE="MB"
SPHR=Y
FILE="LOCAL.BAS"
ATMN=1 ENDATMNOS
FIL1="HIDDEN.BAS"
ATM1=1 ENDATMNOS
ECPFILE="LOCAL.POT"
! Revision History
! heading one
! heading two
! heading three
! heading four
!  1/ 2/26 DFF EN Local revision
! continuation
'''
BASIS = '''NAME="Local Basis"
REFERENCE=
"Local reference
continued"
Z=1
NUMEXP=2 S
  1.00D+00  0.5
  0.25      0.4
Z=118
NUMEXP=1 SP
  2.0 0.1 0.2
! File Revision History
! 1/ 2/26 initial entry
!        continuation is omitted
!
'''
POTENTIAL = '''NAME="Local ECP"
REFERENCE=
"Potential reference
continued"
Z=1
NCORE=0
LMAX=1
L=0 TYPE="S"
NUMEXP=1
  2 1.0 -0.5
!
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--script', default='scripts/gbsDAVConverter')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--capture')
    mode.add_argument('--compare', default=str(Path(__file__).with_name(
        'gbsdav_converter.expected.json')))
    args = parser.parse_args()
    script = Path(args.script).resolve()
    root = Path(tempfile.mkdtemp(prefix='.gbsdav-test-', dir=Path.cwd()))
    try:
        source = root / 'source'
        dest = root / 'dest'
        template = root / 'data/admin/basissets'
        source.mkdir()
        template.mkdir(parents=True)
        (template / 'rev.txt').write_bytes(b'Local revision header\n')
        (source / 'MASTER.TBL').write_bytes(MASTER.encode('ascii'))
        (source / 'LOCAL.BAS').write_bytes(BASIS.encode('ascii'))
        (source / 'HIDDEN.BAS').write_bytes(b'NAME="Hidden auxiliary"\n')
        (source / 'LOCAL.POT').write_bytes(POTENTIAL.encode('ascii'))
        env = dict(os.environ, ECCE_HOME=str(root))
        # The single type group avoids Perl's randomized hash iteration order.
        command = ([sys.executable] if script.read_bytes().startswith(
            b'#!/usr/bin/env python3') else ['perl']) + [str(script)]
        snapshots = {}
        for name, options, response in [
                ('help', ['-h'], b''),
                ('decline', ['-s', 'source', '-d', 'dest'], b'no\n'),
                ('convert', ['-s', 'source', '-d', 'dest'], b'y\n'),
                ('cleanup', ['-s', 'source', '-d', 'dest'], b'n\n')]:
            result = subprocess.run(command + options, input=response, env=env, cwd=str(root),
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    timeout=15)
            snapshots[name] = {
                'returncode': result.returncode,
                'stdout': result.stdout.decode('latin-1').replace(str(root), '<fixture>'),
                'stderr': result.stderr.decode('latin-1').replace(str(root), '<fixture>'),
                'files': {str(p.relative_to(dest)): p.read_bytes().decode('latin-1')
                          for p in sorted(dest.glob('*')) if p.is_file()}}
            assert result.returncode == 0, snapshots[name]
        files = snapshots['convert']['files']
        assert set(files) == {'MASTER.TBL', 'bsb.rev', 'gbsNameConv',
                              'LOCAL.BAS', 'LOCAL.BAS.meta', 'LOCALPS-AGG.BAS',
                              'LOCALPS-AGG.BAS.meta', 'LOCAL.POT', 'LOCAL.POT.meta'}, files
        assert files['LOCALPS-AGG.BAS'] == ''
        assert 'atom=Og' not in files['LOCAL.BAS']
        if args.capture:
            Path(args.capture).write_text(json.dumps(snapshots, indent=2) + '\n')
        else:
            expected = json.loads(Path(args.compare).read_text())
            assert snapshots == expected, 'Output differs from captured conversion'
        failure = subprocess.run(command + ['-s', 'missing', '-d', 'dest'],
                                 input=b'y\n', env=env, cwd=str(root),
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
        assert failure.returncode == 2, failure
        # Source line numbers and input-handle annotations are interpreter diagnostics.
        assert re.match(rb'Unable to open source MASTER.TBL at .* line \d+', failure.stderr)
        print('PASS: help, decline, local BAS/POT/aggregate conversion, cleanup; '
              'exit codes, stdout, stderr and all output files; missing-source error')
    finally:
        shutil.rmtree(str(root))


if __name__ == '__main__':
    main()
