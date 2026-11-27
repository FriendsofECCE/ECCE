#!/usr/bin/env python3
"""Byte/status/diagnostic references captured from the Perl exporters (#868).

No update mode: these hashes pin the Perl output, including trailing spaces.
The readable expected/*.txt files exercise the ordinary basis decisions.
"""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
PARSERS = Path(os.environ.get('ECCE_TEST_PARSERS', str(HERE.parents[1] / 'scripts' / 'parsers')))
sys.path.insert(0, str(PARSERS))


def compare_references():
    refs = json.loads((HERE / 'python_port_reference.json').read_text())
    failures = []
    for ref in refs:
        data = ((HERE / 'fixtures' / ref['fixture']).read_bytes() if ref['fixture']
                else ref.get('input', '').encode('latin1'))
        command = [str(PARSERS / ('std2' + ref['code']))]
        if os.name == 'nt':
            command.insert(0, sys.executable)
        result = subprocess.run(command + ref['args'], input=data, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=20,
                                env=dict(os.environ, ECCE_HOME=str(HERE.parents[1])))
        actual = (result.returncode, hashlib.sha256(result.stdout).hexdigest(), result.stderr)
        stderr = ref['stderr'].replace('{parsers}', str(PARSERS)).encode('latin1')
        expected = (ref['status'], ref['stdout_sha256'], stderr)
        if actual != expected:
            failures.append('%s %s %s: %r != %r' %
                            (ref['code'], ref['fixture'], ref['args'], actual, expected))
    return failures, len(refs)


def reader_agreement():
    from rdStandardGBS import readStandardBS
    from wrORCAGBS import NameToBasis
    import io
    import re
    failures = []
    refs = json.loads((HERE / 'python_port_reference.json').read_text())
    inputs = {((HERE / 'fixtures' / ref['fixture']).read_text() if ref['fixture']
               else ref.get('input', '')) for ref in refs}
    for data in inputs:
        legacy = subprocess.run(
            ['perl', '-I', str(PARSERS), '-MJSON::PP', '-e',
             'require "rdStandardGBS.pm"; print JSON::PP->new->encode(readStandardBS());'],
            input=data.encode('latin1'), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            timeout=20)
        if legacy.returncode or readStandardBS(io.StringIO(data)) != json.loads(legacy.stdout):
            failures.append('shared reader disagrees with Perl for input %r' % data[:100])
    legacy_names = dict(re.findall(
        r'\$NameToBasis\{"([^"]+)"\}\s*=\s*"([^"]+)"',
        (PARSERS / 'wrORCAGBS.pm').read_text()))
    if NameToBasis != legacy_names:
        failures.append('ORCA export name table disagrees with the Perl importer')
    return failures, len(inputs) + 1


def main():
    failures, count = compare_references()
    reader_failures, readers = reader_agreement()
    failures += reader_failures
    for failure in failures:
        print('FAIL: ' + failure)
    print('%s: %d Perl reference comparisons; %d reader/table comparisons' %
          ('FAIL' if failures else 'PASS', count, readers))
    return bool(failures)


if __name__ == '__main__':
    sys.exit(main())
