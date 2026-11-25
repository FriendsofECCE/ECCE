#!/usr/bin/env python3
"""The command-line tools decide "inside the data folder" in one place (#1285).

Each tool in src/apps/launch had its own copy of a test that compared
realpath() strings with a "/" separator, which refuses every Windows path
("C:\\..."). They now call LocalData::toUrl (paths compared with
lexically_relative); this fails if a tool compares against "<base>/" again.

    one_data_folder_check_test.py [SOURCE_ROOT]
"""
import os
import re
import sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..")
PATTERN = re.compile(r'base\s*\+\s*"/"')


def main():
    folder = os.path.join(ROOT, "src", "apps", "launch")
    bad = []
    for name in sorted(os.listdir(folder)):
        if not name.endswith(".C"):
            continue
        for n, line in enumerate(open(os.path.join(folder, name)), 1):
            if PATTERN.search(line):
                bad.append("%s:%d: %s" % (name, n, line.strip()))
    for b in bad:
        print("FAIL " + b)
    print("FAILED: %d string-prefix data-folder checks" % len(bad) if bad
          else "PASSED")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
