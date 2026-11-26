---
type: rule
title: "A GUI test script that ran no check must fail, not pass"
area: services
section: "Pitfalls"
paths: ["tests/apps/checks.py", "tests/apps/checks_test.py", "tests/apps/run_tests.py", "tests/apps"]
issues: [1309, 1307, 1301, 1279]
---
**Exit 0 means "the checks ran and passed", never "no check ran".**
Each script used to end `return 1 if failures else 0`, so a body that was
skipped -- an early `return`, a `continue` over every case, a discovery
that found nothing -- reported PASS. `run_tests.report()` did the same with
an empty `Results`: "PASS  0 apps checked".

- **Declare the count**: `CHECKS = checks.Checks(N, "name")`, `check =
  CHECKS`, `return CHECKS.report()`. Fewer recorded checks than declared
  fails with "only N of M checks ran". `Checks(0)` is refused.
- **Dynamic work**: `CHECKS.expect(per_case * len(cases))` once the cases
  are known; an empty case list still leaves one expected check, so a
  selector that matches nothing (`ECCE_AUFBAU_ONLY=typo`) fails.
- **A skip is stated**: `CHECKS.skip(reason)` reports 77. A skip inferred
  from an empty run is the bug, not the escape hatch.
- **Own `checkApp`**: end with `CHECKS.recordOn(results)`; it adds the
  failures and the count to the harness's `Results`.
- `run_tests.py` fails a run that swept no app and ran no check module, and
  a `--check MODULE` whose `check()` recorded neither a check nor a failure.
