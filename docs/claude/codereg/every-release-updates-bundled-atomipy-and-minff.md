---
type: checklist
title: "Every release updates the bundled atomipy and min.ff to their newest versions, if tests/minff passes"
area: codereg
paths: [scripts/minff_topology.py, tests/minff/run_tests.py, docs/GROMACS.md]
issues: [241]
---
ECCE ships atomipy (MIT) and min.ff (bundled with its authors' permission,
October 2026) so that the MINFF route of GROMACS Prepare needs nothing
installed and nothing downloaded. Bundled copies go stale, so before every
release tag:

1. Fetch the newest atomipy release from PyPI and the newest min.ff commit,
   and record both versions (MINFF_COMMIT in the script, the atomipy version
   in `third_party/README.md`). The bundle is `third_party/python/atomipy`
   (the unused `structures/` libraries are left out) and
   `third_party/minff/min.ff`, with their licences.
2. Run `tests/minff/run_tests.py` against them (types, charges, grompp, a
   steepest-descent minimisation).
3. Passed: ship them and name both versions in the release notes. Failed:
   keep the previous versions and open an issue with what broke.

Downloading on first use was rejected: student laptops and cluster nodes are
often offline or behind proxies, a project should keep the versions it was
tested with, and pip is where Windows and macOS installs fail.
