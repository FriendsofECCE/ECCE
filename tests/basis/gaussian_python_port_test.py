#!/usr/bin/env python3
"""Pin stdout, stderr and exit status to captured Perl translator results."""

import difflib
import json
from pathlib import Path
import subprocess
import sys


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def main():
    cases = json.loads((HERE / 'gaussian_perl_reference.json').read_text())
    failures = 0
    for case in cases:
        script = ROOT / 'scripts/parsers' / ('std2Gaussian-' + case['version'])
        result = subprocess.run([sys.executable, str(script)] + case.get('args', []),
                                input=case['input'].encode(), stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=20)
        expected = (case['status'], case['stdout'].encode(), case['stderr'].encode())
        actual = (result.returncode, result.stdout, result.stderr)
        if actual != expected:
            failures += 1
            print('FAIL: Gaussian %s %s' % (case['version'], case['name']))
            print('status: expected %s, got %s' % (expected[0], actual[0]))
            for label, want, got in zip(('stdout', 'stderr'), expected[1:], actual[1:]):
                print(''.join(difflib.unified_diff(
                    want.decode().splitlines(True), got.decode().splitlines(True),
                    'Perl ' + label, 'Python ' + label)), end='')
    print('%s: %d Gaussian translator byte comparisons; %d failures' %
          ('FAIL' if failures else 'PASS', len(cases), failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
