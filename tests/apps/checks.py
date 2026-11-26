"""Counted checks for the GUI test scripts (#1309).

A script states how many checks it runs.  A run that records fewer than
that fails, so a body that was skipped -- an early `return`, a `continue`
over every case, a discovery that found nothing -- cannot report PASS.
Nothing checked is a failure here, never a log line.

    import checks

    CHECKS = checks.Checks(3, "builder dirty scope")
    check = CHECKS                      # check(ok, what, detail="") -> bool
    failures = CHECKS.failures          # for scripts that append their own
    ...
    return CHECKS.report()              # 0 pass, 1 fail, 77 skip

A script that legitimately has nothing to test calls CHECKS.skip(reason)
and reports 77: the skip is stated, not inferred from an empty run.
"""

from __future__ import print_function

import sys

SKIP = 77


class Checks(object):

    def __init__(self, expected, name="", out=None):
        if int(expected) < 1:
            raise ValueError("a test that declares no checks cannot pass")
        self.expected = int(expected)
        self.name = name
        self.failures = []
        self.ran = []
        self.skipped = None
        self._out = out or (lambda line: print(line, flush=True))

    def __call__(self, ok, what, detail=""):
        ok = bool(ok)
        self.ran.append(what)
        self._out("%-5s %s" % ("ok" if ok else "FAIL", what))
        if not ok:
            self.failures.append(what)
            if detail:
                self._out("      " + str(detail)[-1500:])
        return ok

    #  Named forms, for scripts whose own check() is passed around.
    check = __call__

    def expect(self, n):
        """Raise the declared count once the work is known (per case)."""
        self.expected = max(1, int(n))
        return self.expected

    def skip(self, reason):
        self.skipped = reason
        return SKIP

    def shortfall(self):
        if self.skipped or len(self.ran) >= self.expected:
            return None
        return "only %d of %d checks ran" % (len(self.ran), self.expected)

    def problems(self):
        """Everything that must fail the run.

        A shortfall after a failure is usually that failure's own early
        return, so it is not listed twice; the run already fails.
        """
        if self.failures:
            return list(self.failures)
        short = self.shortfall()
        return [short] if short else []

    def summary(self):
        if self.skipped:
            return "SKIP: %s" % self.skipped
        head = "%s%d of %d checks ran" % (self.name + ": " if self.name else "",
                                          len(self.ran), self.expected)
        bad = self.problems()
        return "%s, %s" % (head, "FAILED -- " + "; ".join(bad) if bad
                           else "PASSED")

    def report(self):
        self._out(self.summary())
        if self.skipped:
            return SKIP
        return 1 if self.problems() else 0

    def recordOn(self, results, where=None):
        """Hand the outcome to a run_tests.Results (the --app scripts)."""
        for item in self.problems():
            results.fail(where or self.name or "checks", item)
        results.checks += len(self.ran)
        return not self.problems()


def main(argv=None):
    """`checks.py --selftest` -- see checks_test.py."""
    import checks_test
    return checks_test.main(argv if argv is not None else sys.argv[1:])


if __name__ == "__main__":
    sys.exit(main())
