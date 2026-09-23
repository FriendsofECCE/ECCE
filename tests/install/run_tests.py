#!/usr/bin/env python3
"""Is the ECCE you are running the ECCE you are reading?

    tests/install/run_tests.py            check the default install
    tests/install/run_tests.py -v         list every file, not just drift
    ECCE_TEST_HOME=/some/where ...        check a different install

ECCE runs a mix of compiled binaries and interpreted files -- Perl
parsers, wxPython dialogs, XML config, basis data.  The interpreted ones
are read from ``$ECCE_HOME`` at run time, so they can drift from the
repository without any build failing and without the application saying
anything.  What you are running stops being what you are reading, and
nothing tells you.

This is not a correctness test and does not belong in CI, where the
build and the install happen in one go and cannot drift.  It is the
first thing to run on a developer box when a fix "didn't take" -- which
is worth automating because guessing wrong at it has cost real time
twice:

  * A Gaussian dispersion dropdown was reported as reverted.  Nothing
    was reverted; /opt/ecce/scripts/codereg/ged16theory.py predated the
    commit and had no Dispersion field at all.
  * A job-monitor fix was reported as not working.  The installed
    eccejobstore was eighteen minutes older than the build.

Exit status is 0 when the install matches, 1 when it has drifted, and 2
when there is no install to compare against.
"""

import argparse
import filecmp
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

INSTALL = os.environ.get("ECCE_TEST_HOME", "/opt/ecce")
BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))

#  Trees installed verbatim from the source tree, so a byte difference is
#  real drift.  Deliberately NOT siteconfig: CMake rewrites several of
#  those files at configure time (paths, ports), so the installed copy is
#  expected to differ and comparing them would cry wolf every run.
VERBATIM = [
    "scripts/parsers",
    "scripts/codereg",
    "data/client/cap",
    "data/client/config",
]

#  Noise: caches, editor droppings, and files a running ECCE writes back.
IGNORE_NAMES = {"__pycache__", ".gitignore"}
IGNORE_SUFFIX = (".pyc", "~", ".orig", ".rej", ".bak")


def interesting(name):
    return (name not in IGNORE_NAMES
            and not name.endswith(IGNORE_SUFFIX))


def walk(root):
    """Relative paths of the interesting files under root."""
    found = set()
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if interesting(d)]
        for name in filenames:
            if interesting(name):
                full = os.path.join(dirpath, name)
                found.add(os.path.relpath(full, root))
    return found


class Result(object):
    def __init__(self):
        self.missing = []    # in the repo, absent from the install
        self.differs = []    # present in both, different content
        self.extra = []      # in the install, absent from the repo
        self.stale = []      # binaries older than the build


def compareTrees(res, verbose):
    for rel in VERBATIM:
        src = os.path.join(ROOT, rel)
        dst = os.path.join(INSTALL, rel)
        if not os.path.isdir(src):
            continue
        if not os.path.isdir(dst):
            res.missing.append("%s/  (whole directory)" % rel)
            continue

        srcFiles = walk(src)
        dstFiles = walk(dst)

        for name in sorted(srcFiles - dstFiles):
            #  The trap CLAUDE.md records: a file under scripts/ with no
            #  install() rule is simply absent from the package, and
            #  presents as "command not found" only once installed.
            res.missing.append(os.path.join(rel, name))
        for name in sorted(dstFiles - srcFiles):
            res.extra.append(os.path.join(rel, name))
        for name in sorted(srcFiles & dstFiles):
            a = os.path.join(src, name)
            b = os.path.join(dst, name)
            if not filecmp.cmp(a, b, shallow=False):
                res.differs.append(os.path.join(rel, name))
            elif verbose:
                print("  same     %s" % os.path.join(rel, name))


def compareBinaries(res, verbose):
    """Installed binaries against the build tree.

    Compared by MTIME rather than content: the packaged binary is
    stripped, so it legitimately differs byte for byte from the one in
    build-cmake.  What matters is whether it predates the last build.
    """
    binDir = os.path.join(INSTALL, "bin")
    if not os.path.isdir(binDir) or not os.path.isdir(BUILD):
        return
    for name in sorted(os.listdir(binDir)):
        built = os.path.join(BUILD, name)
        installed = os.path.join(binDir, name)
        if not os.path.isfile(built) or not os.path.isfile(installed):
            continue
        builtTime = os.path.getmtime(built)
        instTime = os.path.getmtime(installed)
        #  A second of slack: the packaging step copies these, so exact
        #  equality is not expected even when they correspond.
        if builtTime > instTime + 1:
            res.stale.append((name, instTime, builtTime))
        elif verbose:
            print("  current  bin/%s" % name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    print("Install parity: %s\n      against: %s\n" % (INSTALL, ROOT))
    if not os.path.isdir(INSTALL):
        print("No install found at %s." % INSTALL)
        print("Set ECCE_TEST_HOME if it lives elsewhere.")
        return 2

    res = Result()
    compareTrees(res, args.verbose)
    compareBinaries(res, args.verbose)

    for path in res.missing:
        print("  MISSING   %s" % path)
        print("            in the repo, not in the install -- check for an")
        print("            install() rule in CMakeLists.txt")
    for path in res.differs:
        print("  DIFFERS   %s" % path)
    for path in res.extra:
        print("  LEFTOVER  %s" % path)
        print("            in the install, not in the repo -- residue from")
        print("            an earlier version")
    for name, instTime, builtTime in res.stale:
        import time
        print("  STALE     bin/%s" % name)
        print("            installed %s, built %s"
              % (time.strftime("%Y-%m-%d %H:%M", time.localtime(instTime)),
                 time.strftime("%Y-%m-%d %H:%M", time.localtime(builtTime))))

    total = (len(res.missing) + len(res.differs)
             + len(res.extra) + len(res.stale))
    print("")
    if total == 0:
        print("PASS  the installed ECCE matches this working tree")
        return 0
    print("DRIFT  %d file(s): %d missing, %d differing, %d leftover, "
          "%d stale binaries" % (total, len(res.missing), len(res.differs),
                                 len(res.extra), len(res.stale)))
    print("")
    print("Anything tested against this install is testing the older copy.")
    print("Rebuild and reinstall before drawing conclusions from a GUI test.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
