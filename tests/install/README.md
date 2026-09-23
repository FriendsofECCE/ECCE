# tests/install — is the ECCE you are running the ECCE you are reading?

```
tests/install/run_tests.py          check the default install (/opt/ecce)
tests/install/run_tests.py -v       list matching files too
ECCE_TEST_HOME=... run_tests.py     check an install elsewhere
ECCE_TEST_BUILD=... run_tests.py    compare binaries against another build
```

Exit 0 when the install matches, 1 on drift, 2 when there is no install.

## Why

ECCE runs a mix of compiled binaries and interpreted files — Perl
parsers, wxPython dialogs, XML config, basis data. The interpreted ones
are read from `$ECCE_HOME` at run time, so they can drift from the
repository without any build failing and without the application saying
anything.

Guessing wrong about that has cost real time twice:

- A Gaussian dispersion dropdown was reported as reverted. Nothing was
  reverted — `/opt/ecce/scripts/codereg/ged16theory.py` predated the
  commit and had no Dispersion field at all.
- A job-monitor fix was reported as not working. The installed
  `eccejobstore` was eighteen minutes older than the build.

Both were one `diff` away from being obvious. This runs that diff.

## What it checks

| | |
|---|---|
| **MISSING** | in the repo, absent from the install — usually a file under `scripts/` with no `install()` rule in CMakeLists.txt, the trap CLAUDE.md records hitting independently five times |
| **DIFFERS** | present in both, different bytes |
| **LEFTOVER** | in the install, absent from the repo — residue from an earlier version, like the bundled-NWChem files |
| **STALE** | an installed binary older than the one in `build-cmake` |

Binaries are compared by **mtime, not content**: the packaged binary is
stripped, so it legitimately differs byte for byte from the build-tree
one. What matters is whether it predates the last build.

`siteconfig/` is deliberately **not** compared. CMake rewrites several of
those files at configure time (paths, ports), so the installed copies are
expected to differ and comparing them would cry wolf on every run.

## Where it belongs

Not CI. There the build and the install happen in one go and cannot
drift, so the check would always pass and prove nothing.

This is a developer-box tool, and the right moment to reach for it is
when a fix "didn't take" — before re-reading the code, and certainly
before concluding that something was reverted.
