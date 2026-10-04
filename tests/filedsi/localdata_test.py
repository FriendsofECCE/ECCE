#!/usr/bin/env python3
"""LocalData (#216): which folder a session uses, and moving that folder.

Drives the ecce-localdata program the wrappers call.  The move is checked
on one file system (a rename) and across two: the scratch directory and
ECCE_TEST_OTHER_FS (default ~/.cache/ecce-216-otherfs), which on a machine
whose /tmp is a tmpfs is a different file system; the cross-device cases
are skipped, and said so, when both are on one.

    tests/filedsi/localdata_test.py [build-dir]
"""

import fcntl
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
BUILD = sys.argv[1] if len(sys.argv) > 1 else os.environ.get(
    "ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
PROG = os.path.join(BUILD, "ecce-localdata")

failures = 0


def check(ok, name, why=""):
    global failures
    print(("PASS " if ok else "FAIL ") + name + ("" if ok else ": " + why))
    if not ok:
        failures += 1


def run(env, *args):
    p = subprocess.run([PROG] + list(args), env=env, capture_output=True,
                       text=True, timeout=60)
    return p.returncode, p.stdout.strip()


def make_store(root):
    """A small store: nested calculation files and a property sidecar."""
    calc = os.path.join(root, "users", "tester", "proj", "water")
    os.makedirs(os.path.join(calc, "Inputs"))
    with open(os.path.join(calc, "Inputs", "water.nw"), "w") as f:
        f.write("geometry\n O 0 0 0\nend\n")
    with open(os.path.join(calc, ".ecce-meta"), "w") as f:
        f.write(".\tecce:state\tstring\tcomplete\n")
    os.symlink("Inputs/water.nw", os.path.join(calc, "link.nw"))


def listing(root):
    out = {}
    for d, dirs, files in os.walk(root):
        for n in dirs + files:
            if n == ".ecce-in-use":
                continue
            p = os.path.join(d, n)
            rel = os.path.relpath(p, root)
            out[rel] = -2 if os.path.islink(p) else (
                -1 if os.path.isdir(p) else os.path.getsize(p))
    return out


def same_device(a, b):
    return os.stat(a).st_dev == os.stat(b).st_dev


def move_cases(env, scratch, other, label):
    src = os.path.join(scratch, "src-" + label)
    make_store(src)
    want = listing(src)
    dst = os.path.join(other, "moved-" + label)

    # Into a folder that is in use: refused, nothing changes.
    fd = os.open(os.path.join(src, ".ecce-in-use"), os.O_RDWR | os.O_CREAT)
    fcntl.flock(fd, fcntl.LOCK_SH)
    rc, out = run(env, "in-use", src)
    check(rc == 0, label + ": a held folder reports in use")
    rc, out = run(env, "move", src, dst)
    check(rc == 3 and os.path.isdir(src) and not os.path.exists(dst),
          label + ": move refused while in use", "rc=%d %s" % (rc, out))
    os.close(fd)
    rc, _ = run(env, "in-use", src)
    check(rc == 1, label + ": released folder is not in use")

    # Never into a folder that already holds something.
    full = os.path.join(other, "full-" + label)
    os.makedirs(full)
    open(os.path.join(full, "keep"), "w").close()
    rc, out = run(env, "move", src, full)
    check(rc == 2 and listing(full) == {"keep": 0} and listing(src) == want,
          label + ": move into a non-empty folder refused", out)

    # A failed copy keeps the old folder and leaves no partial new one.
    if label == "cross-fs" and os.getuid() != 0:
        bad = os.path.join(src, "users", "tester", "proj", "water",
                           "Inputs", "water.nw")
        os.chmod(bad, 0)
        rc, out = run(env, "move", src, dst)
        os.chmod(bad, 0o644)
        check(rc == 1 and listing(src) == want and not os.path.exists(dst)
              and src in out,
              label + ": failed copy keeps the old folder and says so", out)

    # Into an empty existing directory, then the real move.
    os.makedirs(dst)
    rc, out = run(env, "move", src, dst)
    got = listing(dst)
    check(rc == 0 and not os.path.exists(src) and got == want,
          label + ": moved with every file, size and the sidecar",
          "rc=%d %s" % (rc, out))


def main():
    global failures
    if not os.path.exists(PROG):
        print("SKIP: %s not built" % PROG)
        return 0
    scratch = tempfile.mkdtemp(prefix="localdata.")
    other_base = os.environ.get("ECCE_TEST_OTHER_FS",
                                os.path.expanduser("~/.cache/ecce-216-otherfs"))
    os.makedirs(other_base, exist_ok=True)
    other = tempfile.mkdtemp(prefix="localdata.", dir=other_base)
    try:
        home = os.path.join(scratch, "home")
        os.makedirs(os.path.join(home, ".ECCE"))
        env = {"PATH": os.environ.get("PATH", ""), "ECCE_HOME": REPO,
               "ECCE_REALUSERHOME": home, "HOME": home,
               "ECCE_REALUSER": "tester"}

        # Which folder: preference off, on, overridden, and under -remote.
        pref = os.path.join(home, ".ECCE", "EcceGlobal")
        rc, out = run(env, )
        check(rc == 0 and out == "" and not os.path.exists(
              os.path.join(home, ".ECCE-local")),
              "no preference: a data server, and no folder made", out)
        with open(pref, "w") as f:
            f.write("LOCALDATA:\ttrue\n")
        rc, out = run(env)
        check(out == os.path.join(home, ".ECCE-local"),
              "preference on: ~/.ECCE-local", out)
        with open(pref, "a") as f:
            f.write("LOCALDATA.FOLDER:\t%s/elsewhere\n" % scratch)
        rc, out = run(env)
        check(out == scratch + "/elsewhere", "preference folder", out)
        rc, out = run(dict(env, ECCE_LOCAL_DATA=scratch + "/env/"))
        check(out == scratch + "/env", "ECCE_LOCAL_DATA wins", out)
        rc, out = run(dict(env, ECCE_LOCAL_DATA=""))
        check(out == "", "empty ECCE_LOCAL_DATA pins a data server", out)
        rc, out = run(dict(env, ECCE_REMOTE_SERVER="1"))
        check(out == "", "-remote ignores the preference", out)

        move_cases(env, scratch, scratch, "same-fs")
        if same_device(scratch, other):
            print("SKIP cross-fs: %s and %s are one file system"
                  % (scratch, other))
        else:
            move_cases(env, scratch, other, "cross-fs")
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
        shutil.rmtree(other, ignore_errors=True)
    print(("FAILED %d" % failures) if failures else "ALL OK")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
