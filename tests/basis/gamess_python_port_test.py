#!/usr/bin/env python3
"""Compare exact bytes, diagnostics and status with frozen Perl output (#868).

Single-element cases avoid Perl's random hash traversal order. References
have no update mode; their whitespace and legacy quirks are intentional.
Run with ECCE_TEST_PARSERS to check an installed parsers directory.
"""

import difflib
import json
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
PARSERS = Path(os.environ.get('ECCE_TEST_PARSERS', str(HERE.parents[1] / 'scripts/parsers')))


def main():
    refs = json.loads((HERE / 'gamess_perl_reference.json').read_text())
    failures = 0
    for ref in refs:
        command = [str(PARSERS / ('std2' + ref['code']))]
        if os.name == 'nt':
            command.insert(0, sys.executable)
        result = subprocess.run(command + ref['args'], input=ref['input'].encode('latin1'),
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20,
                                env=dict(os.environ, ECCE_HOME=str(HERE.parents[1]),
                                         PYTHONDONTWRITEBYTECODE="1"))
        expected = (ref['status'], ref['stdout'].encode('latin1'),
                    ref['stderr'].replace('{parsers}', str(PARSERS)).encode('latin1'))
        actual = (result.returncode, result.stdout, result.stderr)
        if actual != expected:
            failures += 1
            print('FAIL: %s %s %r' % (ref['code'], ref['name'], ref['args']))
            print(''.join(difflib.unified_diff(repr(expected).splitlines(True),
                                             repr(actual).splitlines(True))))
    print('%s: %d byte/status/diagnostic comparisons with Perl' %
          ('FAIL' if failures else 'PASS', len(refs)))
    return bool(failures)


if __name__ == '__main__':
    sys.exit(main())
