---
type: pitfall
title: "The app suite tests the INSTALLED ECCE (/opt/ecce), not build-cmake"
area: services
section: "Pitfalls"
paths: ["tests/apps/apps.py", "tests/apps/fixture.py", "tests/apps/examples_test.py", "tests/apps/panel_layouts_test.py"]
issues: [385, 375]
---
**`ctest -R apps_` runs the binaries in `ECCE_TEST_HOME` (default
`/opt/ecce`) and the wrappers in `ECCE_TEST_WRAPPERS` (default `/usr/bin`),
whatever `build-cmake` holds.** On a machine whose `/opt/ecce` is an older
deb, a test of a new feature fails as if the feature were broken: the
panel-layout test reported `-1` table rows because the installed Builder did
not write the `rows` metric yet, and `apps_examples` reported a missing
example and no example-set version.

To test the tree without root, configure a separate build with
`-DECCE_HOME_DIR=<writable dir> -DECCE_WRAPPER_DESTINATION=<writable dir>`,
`ninja install` it, and run the test with `ECCE_TEST_HOME` and
`ECCE_TEST_WRAPPERS` pointing there (on niobium: `../build-fortify`,
`../fortify-install`, `../fortify-wrappers`). Before believing an app-suite
failure, check which builder ran (`ls -l $ECCE_TEST_HOME/bin/builder`).
