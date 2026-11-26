#!/usr/bin/env python3
"""The counted-check helper itself, and the harness's empty-run guard (#1309).

A GUI script that skips its body must not report PASS.  This needs no
install, no display and no services: it drives checks.Checks and
run_tests.report() directly.

    tests/apps/checks_test.py
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import checks  # noqa: E402


def case(what, ok, failures):
    print("%-5s %s" % ("ok" if ok else "FAIL", what), flush=True)
    if not ok:
        failures.append(what)


def main(argv=()):
    failures = []
    out = []

    #  All the declared checks ran and passed.
    c = checks.Checks(2, "full", out=out.append)
    c(True, "one")
    c(True, "two")
    case("a complete run passes", c.report() == 0, failures)

    #  A failing check fails.
    c = checks.Checks(2, "failing", out=out.append)
    c(True, "one")
    c(False, "two", "detail")
    case("a failed check fails the run", c.report() == 1, failures)

    #  The body was skipped: nothing was checked.
    c = checks.Checks(3, "skipped body", out=out.append)
    case("a run that checked nothing fails", c.report() == 1, failures)
    case("and says how many checks ran",
         "only 0 of 3 checks ran" in c.summary(), failures)

    #  Fewer than declared: some scenario was skipped.
    c = checks.Checks(3, "partial", out=out.append)
    c(True, "one")
    case("a partial run fails", c.report() == 1, failures)

    #  A stated skip is not a shortfall.
    c = checks.Checks(3, "stated skip", out=out.append)
    c.skip("no ORCA installed")
    case("a stated skip reports 77", c.report() == 77, failures)

    #  A test cannot declare that it checks nothing.
    try:
        checks.Checks(0)
        case("expected=0 is refused", False, failures)
    except ValueError:
        case("expected=0 is refused", True, failures)

    #  The app sweep's own report: no app checked is not a PASS.
    import run_tests
    results = run_tests.Results()
    case("the harness fails a run that checked nothing",
         run_tests.report(results, False) == 1, failures)
    done = run_tests.Results()
    done.checks = 1
    case("and passes one that did", run_tests.report(done, False) == 0,
         failures)

    print("%s  %d of 9 checks ran"
          % ("FAILED: " + "; ".join(failures) if failures else "PASSED", 9))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
