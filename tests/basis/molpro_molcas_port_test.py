#!/usr/bin/env python3
"""Compare translator bytes/status/diagnostics with captured Perl results."""

import difflib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
PARSERS = ROOT / 'scripts' / 'parsers'
REFERENCE = HERE / 'molpro_molcas_reference.json'


def run(case, seed=0):
    env = dict(os.environ, ECCE_HOME=str(ROOT), PERL_HASH_SEED=str(seed),
               PERL_PERTURB_KEYS='0', PYTHONDONTWRITEBYTECODE='1')
    if case['code'] == 'molpro_int_lib':
        if (PARSERS / 'wrmolpro_int_libGBS.pm').exists():
            command = ['perl', '-I', str(PARSERS), '-e',
                       'require "rdStandardGBS.pm"; '
                       'require "wrmolpro_int_libGBS.pm"; '
                       'writemolpro_int_lib(readStandardBS());']
        else:
            command = [sys.executable, '-c',
                       'from rdStandardGBS import readStandardBS; '
                       'from wrmolpro_int_libGBS import writemolpro_int_lib; '
                       'writemolpro_int_lib(readStandardBS())']
            env['PYTHONPATH'] = str(PARSERS)
    else:
        path = PARSERS / ('std2' + case['code'])
        interpreter = 'perl' if b'perl' in path.read_bytes().splitlines()[0] else sys.executable
        command = [interpreter, str(path)] + case.get('args', [])
    proc = subprocess.run(command, input=case['input'].encode('ascii'),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=20, env=env)
    return [proc.returncode, proc.stdout.decode('ascii'), proc.stderr.decode('ascii')]


def main():
    cases = json.loads(REFERENCE.read_text())
    failed = 0
    reordered = 0
    for case in cases:
        actual = run(case)
        if actual not in case['results']:
            # Perl randomizes atom order. Compare whole, unmodified atom
            # blocks when no captured run happens to use Python's order.
            def atom_order(result):
                status, stdout, stderr = result
                prefix = 'basis={\n' if stdout.startswith('basis={\n') else ''
                suffix = next((s for s in ('}\n', '$END\n') if stdout.endswith(s)), '')
                body = stdout[len(prefix):len(stdout) - len(suffix) if suffix else None]
                blocks = re.split(r'(?m)(?=^! |^\* [A-Z]+\n|^ECP,atom #,)', body)
                return [status, prefix + ''.join(sorted(blocks)) + suffix, stderr]
            if any(atom_order(actual) == atom_order(result) for result in case['results']):
                reordered += 1
                continue
            failed += 1
            print('FAIL ' + case['name'])
            expected = case['results'][0]
            for index in (1, 2):
                sys.stdout.writelines(difflib.unified_diff(
                    expected[index].splitlines(True), actual[index].splitlines(True),
                    fromfile='Perl', tofile='current'))
            if actual[0] != expected[0]:
                print('status: expected %s, got %s' % (expected[0], actual[0]))
    print('%s: MOLPRO/MOLCAS Perl reference (%d/%d cases; byte diffs %s)' % (
        'FAIL' if failed else 'PASS', len(cases) - failed, len(cases),
        'found' if failed else 'empty'))
    print('%d cases required atom-block ordering normalization' % reordered)
    return bool(failed)


if __name__ == '__main__':
    sys.exit(main())
