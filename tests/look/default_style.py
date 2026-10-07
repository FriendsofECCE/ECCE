#!/usr/bin/env python3
"""The saved DefaultStyle migrates from the old default once (#227).

    tests/look/default_style.py

Compiles default_style.C against wxBase (include/wxviz/DefaultStyle.H) and
runs it on scratch config files.  Exit 77 without g++ or wx-config.
"""
import os
import shlex
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def main():
    if not shutil.which("g++") or not shutil.which("wx-config"):
        print("SKIP: needs g++ and wx-config")
        return 77
    work = tempfile.mkdtemp(prefix="defstyle")
    exe = os.path.join(work, "t")
    flags = subprocess.check_output(["wx-config", "--cxxflags", "--libs", "base"]).decode()
    r = subprocess.run(["g++", "-std=c++17", "-w", "-I", os.path.join(ROOT, "include"),
                        os.path.join(HERE, "default_style.C"), "-o", exe]
                       + shlex.split(flags), capture_output=True, text=True)
    if r.returncode:
        print("FAIL: did not compile\n" + r.stderr[-1500:])
        return 1
    r = subprocess.run([exe, work], capture_output=True, text=True)
    print(r.stdout, end="")
    print("PASS" if r.returncode == 0 else "FAIL")
    return r.returncode


sys.exit(main())
