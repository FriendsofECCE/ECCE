#!/usr/bin/env python3
"""Ecce::serverUser()'s resolution order (src/util/genutil/Ecce.C).

Builds tests/login/serveruser_driver.C -- a tiny program that just prints
Ecce::serverUser() -- against the real Ecce.C/Preferences.C already
compiled into build-cmake's libecceutil.a, and runs it against a
throwaway $ECCE_REALUSERHOME to check, against the real code:

  1. ECCE_SERVER_LOGIN (what `ecce -l` now exports, see login.py) wins
     over anything on disk, in both local and -remote mode.
  2. With no session override, the remembered login is read from a file
     that is DIFFERENT for local vs. -remote mode (ServerLogin vs.
     ServerLogin.remote) -- the fix for a central-server login becoming
     a later local session's default.
  3. With nothing set anywhere, it falls back to $ECCE_REALUSER, exactly
     as before this fix.

Needs a already-built build-cmake (for libecceutil.a) and a C++17
compiler; skips (not fails) if the library isn't there.

    tests/login/serveruser.py [--build build-cmake]
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
DRIVER_SRC = os.path.join(HERE, "serveruser_driver.C")

failures = []


def ok(msg):
    print("ok - %s" % msg)


def fail(msg):
    print("FAIL - %s" % msg)
    failures.append(msg)


def build_driver(build_dir, out_path):
    lib = os.path.join(build_dir, "libecceutil.a")
    if not os.path.exists(lib):
        return None
    cmd = [
        "g++", "-std=gnu++17", "-DECCE_VERSION=\\\"test\\\"",
        "-I", os.path.join(REPO, "include"),
        "-o", out_path, DRIVER_SRC,
        "-L", build_dir, "-lecceutil", "-pthread",
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr, file=sys.stderr)
        return None
    return out_path


class Harness:
    def __init__(self, driver):
        self.driver = driver
        self.home = tempfile.mkdtemp(prefix="ecce-serveruser-test-")
        # ecceHome() is reached indirectly (error-message file lookup);
        # give it somewhere harmless rather than leaving it unset, which
        # would abort the whole process -- real `ecce` always sets this.
        self.ecce_home = self.home

    def cleanup(self):
        shutil.rmtree(self.home, ignore_errors=True)

    def run(self, remote=False, session_login=None):
        e = dict(os.environ)
        e["ECCE_REALUSER"] = "unixuser"
        e["ECCE_REALUSERHOME"] = self.home
        e["ECCE_HOME"] = self.ecce_home
        e.pop("ECCE_SERVER_LOGIN", None)
        e.pop("ECCE_REMOTE_SERVER", None)
        if remote:
            e["ECCE_REMOTE_SERVER"] = "1"
        if session_login:
            e["ECCE_SERVER_LOGIN"] = session_login
        r = subprocess.run([self.driver], env=e, capture_output=True, text=True)
        # Last non-empty line is the printed login (earlier lines can be
        # the unrelated "Unable to open error message file" notice).
        lines = [l for l in r.stdout.splitlines() if l.strip()]
        return lines[-1] if lines else "<no output: %s>" % r.stderr

    def write_login_file(self, remote, name):
        d = os.path.join(self.home, ".ECCE")
        os.makedirs(d, exist_ok=True)
        fname = "ServerLogin.remote" if remote else "ServerLogin"
        with open(os.path.join(d, fname), "w") as f:
            f.write("LOGIN: %s\n" % name)


def test_fallback_to_unix_user(h):
    got = h.run()
    if got == "unixuser":
        ok("no session var, no file: falls back to $ECCE_REALUSER")
    else:
        fail("no session var, no file: expected 'unixuser', got %r" % got)


def test_session_login_wins(h):
    got = h.run(session_login="stud1")
    if got == "stud1":
        ok("ECCE_SERVER_LOGIN overrides everything (local mode)")
    else:
        fail("ECCE_SERVER_LOGIN (local): expected 'stud1', got %r" % got)

    got = h.run(remote=True, session_login="stud1")
    if got == "stud1":
        ok("ECCE_SERVER_LOGIN overrides everything (-remote mode)")
    else:
        fail("ECCE_SERVER_LOGIN (-remote): expected 'stud1', got %r" % got)


def test_files_kept_separate_by_mode(h):
    h.write_login_file(remote=False, name="localfile")
    got = h.run()
    if got == "localfile":
        ok("local mode reads ~/.ECCE/ServerLogin")
    else:
        fail("local mode: expected 'localfile', got %r" % got)

    got = h.run(remote=True)
    if got == "unixuser":
        ok("-remote mode ignores the local ServerLogin file (falls back to Unix user)")
    else:
        fail("-remote mode with only a local file: expected fallback 'unixuser', got %r"
             % got)

    h.write_login_file(remote=True, name="remotefile")
    got = h.run(remote=True)
    if got == "remotefile":
        ok("-remote mode reads ~/.ECCE/ServerLogin.remote once it exists")
    else:
        fail("-remote mode: expected 'remotefile', got %r" % got)

    got = h.run()
    if got == "localfile":
        ok("local mode still reads its own file, unaffected by the remote one")
    else:
        fail("local mode after remote file written: expected 'localfile', got %r" % got)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    args = parser.parse_args()

    tmp_out = tempfile.mktemp(prefix="serveruser-driver-")
    driver = build_driver(args.build, tmp_out)
    if driver is None:
        print("serveruser.py: %s not built (or g++ failed) -- skipping" % args.build)
        sys.exit(0)

    try:
        h = Harness(driver)
        try:
            test_fallback_to_unix_user(h)
            test_session_login_wins(h)
            test_files_kept_separate_by_mode(h)
        finally:
            h.cleanup()
    finally:
        if os.path.exists(tmp_out):
            os.remove(tmp_out)

    print()
    if failures:
        print("%d FAILURE(S)" % len(failures))
        sys.exit(1)
    print("all tests passed")
    sys.exit(0)
