#!/usr/bin/env python3
"""End-to-end suite: real code -> real eccejobmonitor -> real parsers.

    tests/e2e/run_tests.py              run everything whose code is present
    tests/e2e/run_tests.py -v           show every check, not just failures
    tests/e2e/run_tests.py --case NAME  run one case
    tests/e2e/run_tests.py --keep       keep the work directories

Absent codes are SKIPPED with their name printed, never silently passed.
Exit status is 0 only if every case that ran passed; a suite where every
code was missing exits 2, so "nothing was installed" cannot be mistaken
for "everything is fine".
"""

import argparse
import os
import shutil
import sys
import tempfile
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
#  Order matters: both directories contain a cases.py, and this suite's
#  must win.  tests/parsers is appended rather than inserted so its
#  module never shadows the local one.
sys.path.append(os.path.join(ROOT, 'tests', 'parsers'))
sys.path.insert(0, HERE)

import cases as CASEDEFS                                   # noqa: E402
import pipeline                                            # noqa: E402
from eccejobmonitor_sim import read_desc                   # noqa: E402

PARSERS = os.path.join(ROOT, 'scripts', 'parsers')


class Report(object):
    def __init__(self, name, verbose):
        self.name = name
        self.verbose = verbose
        self.failures = []
        self.checks = 0

    def check(self, ok, what):
        self.checks += 1
        if not ok:
            self.failures.append(what)
            print('    FAIL  %s' % what)
        elif self.verbose:
            print('    ok    %s' % what)


def run_case(case, verbose, keep):
    name = case['name']
    codedef = CASEDEFS.CODES[case['code']]
    exe = pipeline.which(codedef['binary'], codedef.get('search', ()))
    if exe is None:
        print('  %-22s SKIP (%s not installed; Debian package: %s)'
              % (name, codedef['binary'], codedef.get('packaged', '?')))
        return None

    workdir = tempfile.mkdtemp(prefix='ecce-e2e-%s-' % name)
    report = Report(name, verbose)
    try:
        deck_src = os.path.join(HERE, 'fixtures', case['deck'])
        deck = os.path.join(workdir, os.path.basename(deck_src))
        shutil.copy(deck_src, deck)

        # --- stage 1: the real code -----------------------------------
        rc, _out, err = pipeline.run_code(codedef['argv'](exe, deck), workdir)
        job_out = os.path.join(workdir, case['output'])
        if not os.path.exists(job_out):
            report.check(False, 'code produced %s (exit %d) %s'
                         % (case['output'], rc, err.strip()[:300]))
            return report
        report.check(True, 'code ran and produced output')

        # --- stage 2: the real monitor --------------------------------
        desc_path = os.path.join(PARSERS, case['desc'])
        results = pipeline.run_monitor(job_out, desc_path, workdir,
                                       calc_name=name)
        blocks = pipeline.unpack(results)
        report.check(bool(blocks),
                     'monitor emitted property blocks (got %d)' % len(blocks))
        if not blocks:
            return report

        # --- stage 3: the real parser scripts -------------------------
        desc = read_desc(desc_path)
        props = pipeline.run_parsers(blocks, desc, case['parse_args'])
        report.check(bool(props),
                     'parsers produced properties (got %d keys)' % len(props))

        # --- stage 4: what the calculation should actually contain ----
        case['expect'](props, report)
        return report
    except Exception:
        report.check(False, 'harness error:\n%s' % traceback.format_exc())
        return report
    finally:
        if keep:
            print('    workdir kept: %s' % workdir)
        else:
            shutil.rmtree(workdir, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-v', '--verbose', action='store_true')
    ap.add_argument('--case')
    ap.add_argument('--keep', action='store_true')
    args = ap.parse_args()

    selected = [c for c in CASEDEFS.CASES
                if not args.case or c['name'] == args.case]
    if not selected:
        print('no such case: %s' % args.case)
        return 2

    ran, failed, skipped = 0, 0, 0
    print('End-to-end: real code -> real eccejobmonitor -> real parsers\n')
    for case in selected:
        rep = run_case(case, args.verbose, args.keep)
        if rep is None:
            skipped += 1
            continue
        ran += 1
        if rep.failures:
            failed += 1
            print('  %-22s FAIL  (%d of %d checks)'
                  % (case['name'], len(rep.failures), rep.checks))
        else:
            print('  %-22s pass  (%d checks)' % (case['name'], rep.checks))

    print('\n%d case(s) run, %d failed, %d skipped' % (ran, failed, skipped))
    if ran == 0:
        print('NOTHING RAN -- no computational codes were found.')
        return 2
    print('FAILED' if failed else 'PASSED')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
