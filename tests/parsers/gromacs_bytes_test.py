#!/usr/bin/env python3
"""GROMACS descriptor blocks and malformed-byte sites against Perl goldens."""
import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

from eccejobmonitor_sim import read_desc, replay

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
SCRIPTS = REPO / 'scripts/parsers'
FIX = HERE / 'fixtures/gromacs'
GOLDEN = HERE / 'expected/gromacs-bytes.json'
TOKENS = [('nbsp', b'\xc2\xa0'), ('digit', b'\xd9\xa3'), ('word', b'\xc3\x85'),
          ('overflow', b'*****'), ('prefix', b'3x'), ('empty', b''), ('stray', b'\xff')]


def pipeline_cases():
    desc = read_desc(str(SCRIPTS / 'gromacs.desc'))
    for stem in ('water_em', 'water_em_bytes', 'water_md', 'water_npt'):
        result = replay(desc, str(FIX / (stem + '.log')))
        assert result.runaway is None, stem
        for entry in desc.live_entries():
            for i, block in enumerate(result.delivered_for(entry)):
                data = block.text.encode('utf-8').replace(b'\xef\xbf\xbd', b'\xff')
                yield '{}-{}-{}'.format(stem, entry.script, i), entry.script, data
    for script, path in [
        ('gromacs.geomtrace', REPO / 'data/client/examples/genchem/gases-liquids-solids/gromacs-ar-liquid/1-dynamics/ecce-parse-traj.txt'),
        ('gromacs.md', FIX / 'nacl_dissolving_ecce-parse-md.txt'),
        ('gromacs.pull', FIX / 'pull.txt'),
    ]:
        assert any(e.script == script and 'file' in e.rules for e in desc.entries)
        yield 'file-' + script, script, path.read_bytes()


def site_cases():
    sites = []
    for script in ('gromacs.energy', 'gromacs.mdavg'):
        head = b'Energies (kJ/mol)\n'
        names = b'      Potential    Temperature\n'
        base = head + names + b'            1.0            2.0\n\n'
        step = (head + b'@Step Time\n' + names + b'1.0 2.0\n\n'
                if script == 'gromacs.energy' else b'@Step Time\n' + base)
        sites.extend([
            (script, 'step-space', step),
            (script, 'energies-leading', b'@' + base),
            (script, 'energies-space', base.replace(b'Energies (', b'Energies@(')),
            (script, 'blank', base.replace(b'\n\n', b'\n@\n')),
            (script, 'header-alpha', head + b'@\n1.0\n\n'),
            (script, 'header-leading', head + b'@' + names + b'1.0 2.0\n\n'),
            (script, 'header-trailing', head + b'Potential@\n1.0\n\n'),
            (script, 'value-leading', head + names + b'@1.0 2.0\n\n'),
            (script, 'value-separator', head + names + b'1.0@2.0\n\n'),
            (script, 'value-one', head + names + b'@ 2.0\n\n'),
            (script, 'value-two', head + names + b'1.0 @\n\n'),
            (script, 'virial', head + b'@Total Virial\n1.0\n' + names + b'1.0 2.0\n\n'),
        ])
        if script == 'gromacs.energy':
            sites.extend([
                (script, 'step-middle', head + b'Step@Time\n' + names + b'1.0 2.0\n\n'),
                (script, 'step-trailing', head + b'Step Time@\n' + names + b'1.0 2.0\n\n'),
            ])
        for length in range(len(base)+1):
            yield '{}-cut-{}'.format(script, length), script, base[:length]
    sites.extend([
        ('gromacs.fmax', 'leading', b'@Maximum force = 1.0\n'),
        ('gromacs.fmax', 'space-before-equal', b'Maximum force@= 1.0\n'),
        ('gromacs.fmax', 'space-after-equal', b'Maximum force =@1.0\n'),
        ('gromacs.fmax', 'value', b'Maximum force = @\n'),
        ('gromacs.time', 'leading', b'@3 1.00000\n'),
        ('gromacs.time', 'step', b'@ 1.00000\n'),
        ('gromacs.time', 'separator', b'3@1.00000\n'),
        ('gromacs.time', 'time-integer', b'3 @.00000\n'),
        ('gromacs.time', 'time-fraction', b'3 1.@\n'),
        ('gromacs.time', 'trailing', b'3 1.00000@\n'),
        ('gromacs.geomtrace', 'end', b'H 1 2 3\n@ECCE-TRAJ end\nH 4 5 6\n'),
        ('gromacs.geomtrace', 'end-suffix', b'H 1 2 3\nECCE-TRAJ end@\nH 4 5 6\n'),
        ('gromacs.geomtrace', 'box-leading', b'@ECCE-TRAJ box 1 2 3 90 90 90\nH 1 2 3\n'),
        ('gromacs.geomtrace', 'box-space', b'ECCE-TRAJ box@1 2 3 90 90 90\nH 1 2 3\n'),
        ('gromacs.geomtrace', 'box-trailing', b'ECCE-TRAJ box 1 2 3 90 90 90@\nH 1 2 3\n'),
        ('gromacs.geomtrace', 'coord-leading', b'@H 1 2 3\n'),
        ('gromacs.geomtrace', 'coord-symbol', b'@ 1 2 3\n'),
        ('gromacs.geomtrace', 'coord-separator', b'H@1 2 3\n'),
        ('gromacs.geomtrace', 'coord-trailing', b'H 1 2 3@\n'),
    ])
    for i in range(6):
        row = [b'1', b'2', b'3', b'90', b'90', b'90']
        row[i] = b'@'
        sites.append(('gromacs.geomtrace', 'box-number-' + str(i),
                      b'ECCE-TRAJ box ' + b' '.join(row) + b'\nH 1 2 3\n'))
    for i in range(3):
        row = [b'1', b'2', b'3']
        row[i] = b'@'
        sites.append(('gromacs.geomtrace', 'coord-number-' + str(i), b'H ' + b' '.join(row) + b'\n'))
    for i in range(3):
        parts = [b'H', b'1', b'2', b'3']
        template = b' '.join(parts[:i+1]) + b'@' + b' '.join(parts[i+1:]) + b'\n'
        sites.append(('gromacs.geomtrace', 'coord-space-' + str(i), template))
    for i in range(3):
        parts = [b'ECCE-PULL', b'1', b'2', b'3']
        template = b' '.join(parts[:i+1]) + b'@' + b' '.join(parts[i+1:]) + b'\n'
        sites.append(('gromacs.pull', 'space-' + str(i), b'ECCE-PULL 0 1 2\n' + template))
    for i in range(5):
        parts = [b'1', b'2', b'3', b'90', b'90', b'90']
        template = b'ECCE-TRAJ box ' + b' '.join(parts[:i+1]) + b'@' + b' '.join(parts[i+1:]) + b'\nH 1 2 3\n'
        sites.append(('gromacs.geomtrace', 'box-space-' + str(i), template))
    for marker in ('ECCE-MDSPEED', 'ECCE-MDMSD'):
        for site, template in [('leading', b'@' + marker.encode() + b' record\n'),
                               ('separator', marker.encode() + b'@record\n'),
                               ('value', marker.encode() + b' @\n'),
                               ('trailing', marker.encode() + b' record@\n')]:
            sites.append(('gromacs.md', marker + '-' + site, template))
    for i in range(3):
        row = [b'1.0', b'2.0', b'3.0']
        row[i] = b'@'
        sites.append(('gromacs.pull', 'value-' + str(i), b'ECCE-PULL 0 1 2\nECCE-PULL ' + b' '.join(row) + b'\n'))
    for site, template in [('leading', b'@ECCE-PULL 1 2 3\n'),
                           ('separator', b'ECCE-PULL@1 2 3\n'),
                           ('trailing', b'ECCE-PULL 1 2 3@\n')]:
        sites.append(('gromacs.pull', site, b'ECCE-PULL 0 1 2\n' + template))
    for script, site, template in sites:
        for token, value in TOKENS:
            yield 'site-{}-{}-{}'.format(script, site, token), script, template.replace(b'@', value)


def digest(data):
    return dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def execute(name, script, data, directory, interpreter, artifacts):
    with tempfile.TemporaryDirectory(prefix='ecce-gromacs-parser-') as work:
        # Equal relative names preserve Perl diagnostics containing __FILE__.
        Path(work, script).write_bytes((directory / script).read_bytes())
        helper = directory / 'xtb_parse.py'
        if helper.exists():
            Path(work, 'xtb_parse.py').write_bytes(helper.read_bytes())
        proc = subprocess.run([interpreter, script, '.', 'MD', 'MD', 'GROMACS', '0'],
                              input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              cwd=work, timeout=30,
                              env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
        files = {str(p.relative_to(work)): p.read_bytes() for p in Path(work).rglob('*')
                 if p.is_file() and p.name not in (script, 'xtb_parse.py')}
        if artifacts:
            dest = artifacts / name
            dest.mkdir(parents=True, exist_ok=True)
            for filename, content in [('stdin', data), ('stdout', proc.stdout), ('stderr', proc.stderr),
                                      ('rc', str(proc.returncode).encode('ascii') + b'\n')]:
                (dest / filename).write_bytes(content)
            for filename, content in files.items():
                path = dest / 'files' / filename
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(content)
        return dict(stdout=digest(proc.stdout), stderr=digest(proc.stderr), rc=proc.returncode,
                    files={key: digest(value) for key, value in files.items()}), proc.stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--capture-perl', type=Path)
    ap.add_argument('--artifacts', type=Path)
    ap.add_argument('--output', type=Path, default=GOLDEN)
    args = ap.parse_args()
    expected = {} if args.capture_perl else json.loads(GOLDEN.read_text(encoding='utf-8'))
    actual, aggregate = {}, {}
    for name, script, data in list(pipeline_cases()) + list(site_cases()):
        result, stdout = execute(name, script, data, args.capture_perl or SCRIPTS,
                                 'perl' if args.capture_perl else sys.executable, args.artifacts)
        actual[name] = result
        if not args.capture_perl:
            assert result == expected[name], '{}: actual={!r}, expected={!r}'.format(name, result, expected[name])
        for stem in ('water_em', 'water_em_bytes'):
            if name.startswith(stem + '-'):
                aggregate.setdefault(stem, {}).setdefault(script, bytearray()).extend(stdout)
        print('PASS {}: stdout/stderr/rc/files'.format(name))
    assert aggregate['water_em'] == aggregate['water_em_bytes'], 'byte decoys changed clean values'
    print('PASS water_em and water_em_bytes: identical per-script values')
    if args.capture_perl:
        args.output.write_text(json.dumps(actual, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    else:
        assert set(actual) == set(expected), 'golden case inventory differs'
    print('PASS {} cases'.format(len(actual)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
