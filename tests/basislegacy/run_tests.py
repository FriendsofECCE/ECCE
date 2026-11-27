#!/usr/bin/env python3
"""Byte-level reference cases captured from the three Perl exporters.

Single-centre cases avoid Perl's randomized element order. Run through Slurm
with python3 tests/basislegacy/run_tests.py; no services or GUI are needed.
"""
import json
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def main():
    cases = json.loads((HERE / 'reference.json').read_text())
    env = dict(os.environ, ECCE_HOME=str(ROOT), PYTHONDONTWRITEBYTECODE='1')
    failures = 0
    for case in cases:
        script = ROOT / 'scripts' / 'parsers' / ('std2Gaussian-' + case['code'])
        command = ([sys.executable] if script.read_bytes().startswith(
            b'#!/usr/bin/env python3') else ['perl']) + [str(script)] + case['args']
        result = subprocess.run(command, input=case['input'].encode(),
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                env=env)
        actual = [result.returncode, result.stdout.decode(), result.stderr.decode()]
        expected = [case['status'], case['stdout'], case['stderr']]
        if actual != expected:
            failures += 1
            print('FAIL: {} {}: {!r} != {!r}'.format(
                case['code'], case['name'], actual, expected))
    print('{}: {} legacy Gaussian exporter reference cases'.format(
        'FAIL' if failures else 'PASS', len(cases)))
    return bool(failures)


if __name__ == '__main__':
    sys.exit(main())
