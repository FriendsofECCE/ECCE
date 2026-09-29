#!/usr/bin/env python3
"""`ecce -l`: session-only, and separate per mode (#-, live two-machine test).

Bug found live: `ecce -l stud1 -remote` used to write "LOGIN: stud1" into
~/.ECCE/ServerLogin (packaging/ecce.in), which Ecce::serverUser()
(src/util/genutil/Ecce.C) reads back for EVERY later `ecce` -- so a
central-server login picked once became the default for a subsequent
plain LOCAL session, where that name has no account.

Fix: -l exports ECCE_SERVER_LOGIN for this session only, and the
remembered-login file is split by mode (ServerLogin vs
ServerLogin.remote) so the two can never default to each other.

This exercises packaging/ecce.in's option handling directly: it is
configured the same way CMake's configure_file(@ONLY) does, and run
against a stub `ecce-gateway` that just dumps its argv and environment.
See serveruser.py for the other half -- Ecce::serverUser()'s resolution
order, checked against the real Ecce.C/Preferences.C.

    tests/login/login.py
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
ECCE_IN = os.path.join(REPO, "packaging", "ecce.in")

GATEWAY_STUB = """#!/bin/bash
echo "ARGV: $@"
echo "ECCE_REMOTE_SERVER=${ECCE_REMOTE_SERVER:-<unset>}"
echo "ECCE_SERVER_LOGIN=${ECCE_SERVER_LOGIN:-<unset>}"
echo "ECCE_HOME=${ECCE_HOME:-<unset>}"
"""

failures = []


def ok(msg):
    print("ok - %s" % msg)


def fail(msg):
    print("FAIL - %s" % msg)
    failures.append(msg)


def configure_ecce(bindir):
    """Reproduce CMake's configure_file(ecce.in ecce @ONLY) well enough
    for this test: substitute the two @VAR@ placeholders it uses."""
    with open(ECCE_IN) as f:
        text = f.read()
    text = text.replace("@ECCE_HOME_DIR@", "/nonexistent-ecce-home")
    text = text.replace("@ECCE_VERSION_RAW@", "v0.0.0-test")
    path = os.path.join(bindir, "ecce")
    with open(path, "w") as f:
        f.write(text)
    os.chmod(path, 0o755)
    return path


class Harness:
    """One throwaway $ECCE_REALUSERHOME + PATH holding `ecce` and a stub
    `ecce-gateway`."""

    def __init__(self):
        self.home = tempfile.mkdtemp(prefix="ecce-login-test-")
        self.bindir = os.path.join(self.home, "bin")
        os.makedirs(self.bindir)
        configure_ecce(self.bindir)
        stub = os.path.join(self.bindir, "ecce-gateway")
        with open(stub, "w") as f:
            f.write(GATEWAY_STUB)
        os.chmod(stub, 0o755)

    def cleanup(self):
        shutil.rmtree(self.home, ignore_errors=True)

    def env(self, extra=None):
        e = dict(os.environ)
        e["ECCE_REALUSERHOME"] = self.home
        e["HOME"] = self.home
        e["PATH"] = self.bindir + os.pathsep + e["PATH"]
        # Never let a real ECCE_HOME/ECCE_SERVER_LOGIN from the caller's
        # shell leak into the test.
        e.pop("ECCE_HOME", None)
        e.pop("ECCE_SERVER_LOGIN", None)
        e.pop("ECCE_REMOTE_SERVER", None)
        if extra:
            e.update(extra)
        return e

    def run(self, args, extra_env=None, timeout=20):
        return subprocess.run(
            [os.path.join(self.bindir, "ecce")] + args,
            cwd=self.home,
            env=self.env(extra_env),
            capture_output=True,
            text=True,
            timeout=timeout,
        )

    def server_login_file(self, remote=False):
        name = "ServerLogin.remote" if remote else "ServerLogin"
        return os.path.join(self.home, ".ECCE", name)


def test_help_text():
    h = Harness()
    try:
        r = h.run(["--help"])
        if "for this session only" in r.stdout and "-l LOGIN" in r.stdout:
            ok("--help describes -l as session-only")
        else:
            fail("--help does not mention -l is session-only:\n%s" % r.stdout)
        if "writes ~/.ECCE/ServerLogin" in r.stdout:
            fail("--help still advertises the old write-to-disk behavior")
        else:
            ok("--help no longer claims -l writes ~/.ECCE/ServerLogin")
    finally:
        h.cleanup()


def test_l_is_session_only():
    h = Harness()
    try:
        r = h.run(["-l", "stud1"])
        if r.returncode != 0:
            fail("ecce -l stud1: exit %d: %s" % (r.returncode, r.stderr))
            return
        if "ECCE_SERVER_LOGIN=stud1" in r.stdout:
            ok("-l stud1: ECCE_SERVER_LOGIN=stud1 reaches ecce-gateway")
        else:
            fail("-l stud1: stub did not see ECCE_SERVER_LOGIN=stud1:\n%s" % r.stdout)

        if os.path.exists(h.server_login_file()):
            fail("-l stud1: ~/.ECCE/ServerLogin was written -- should be session-only")
        else:
            ok("-l stud1: ~/.ECCE/ServerLogin was not written")
    finally:
        h.cleanup()


def test_plain_ecce_unaffected():
    """A later plain `ecce` (fresh process, same account) must not see a
    login some earlier `-l` session picked -- the whole point of the
    fix. Modeled as two runs of the harness sharing one $ECCE_REALUSERHOME."""
    h = Harness()
    try:
        r1 = h.run(["-l", "stud1"])
        if r1.returncode != 0:
            fail("setup: ecce -l stud1 failed: %s" % r1.stderr)
            return
        r2 = h.run([])
        if "ECCE_SERVER_LOGIN=<unset>" in r2.stdout:
            ok("plain ecce after -l stud1: ECCE_SERVER_LOGIN unset (no leakage)")
        else:
            fail("plain ecce after -l stud1: login leaked into next session:\n%s"
                 % r2.stdout)
    finally:
        h.cleanup()


def test_remote_passthrough():
    h = Harness()
    try:
        r = h.run(["-l", "stud1", "-remote"])
        if r.returncode != 0:
            fail("ecce -l stud1 -remote: exit %d: %s" % (r.returncode, r.stderr))
            return
        if "ECCE_REMOTE_SERVER=1" in r.stdout and "ECCE_SERVER_LOGIN=stud1" in r.stdout:
            ok("-l stud1 -remote: both ECCE_REMOTE_SERVER and ECCE_SERVER_LOGIN reach ecce-gateway")
        else:
            fail("-l stud1 -remote: stub output missing expected vars:\n%s" % r.stdout)
        if "-remote" in r.stdout.splitlines()[0]:
            ok("-l stud1 -remote: -remote is passed through to ecce-gateway's argv")
        else:
            fail("-l stud1 -remote: -remote missing from ecce-gateway argv:\n%s"
                 % r.stdout)
        if os.path.exists(h.server_login_file()) or os.path.exists(h.server_login_file(True)):
            fail("-l stud1 -remote: a ServerLogin file was written -- should be session-only")
        else:
            ok("-l stud1 -remote: no ServerLogin file written")
    finally:
        h.cleanup()


def test_bug_mode_reports_login():
    h = Harness()
    try:
        r = h.run(["--bug", "-l", "stud1", "-remote"])
        if r.returncode != 0:
            fail("ecce --bug -l stud1 -remote: exit %d: %s" % (r.returncode, r.stderr))
            return
        bugdirs = [d for d in os.listdir(h.home) if d.startswith("ecce-bug-")
                   and os.path.isdir(os.path.join(h.home, d))]
        if len(bugdirs) != 1:
            fail("--bug: expected exactly one ecce-bug-* directory, found %r" % bugdirs)
            return
        bugmode = os.path.join(h.home, bugdirs[0], "bug-mode.txt")
        if not os.path.exists(bugmode):
            fail("--bug: bug-mode.txt was not written")
            return
        text = open(bugmode).read()
        if "ECCE_SERVER_LOGIN=stud1" in text:
            ok("--bug: bug-mode.txt reports the session's ECCE_SERVER_LOGIN")
        else:
            fail("--bug: bug-mode.txt does not report the login in use:\n%s" % text)
        if "ECCE_REMOTE_SERVER=1" in text:
            ok("--bug: bug-mode.txt reports ECCE_REMOTE_SERVER")
        else:
            fail("--bug: bug-mode.txt does not report ECCE_REMOTE_SERVER:\n%s" % text)
    finally:
        h.cleanup()


def test_no_l_no_file_needed():
    """Backward compatibility: with no -l ever used, nothing is written
    and nothing is required -- Ecce::serverUser() falls back to the Unix
    username on its own (checked in serveruser.py, not here, since that
    is a C++-library-level concern)."""
    h = Harness()
    try:
        r = h.run([])
        if r.returncode != 0:
            fail("plain ecce: exit %d: %s" % (r.returncode, r.stderr))
            return
        if "ECCE_SERVER_LOGIN=<unset>" in r.stdout:
            ok("plain ecce: ECCE_SERVER_LOGIN unset with no -l ever given")
        else:
            fail("plain ecce: unexpected ECCE_SERVER_LOGIN:\n%s" % r.stdout)
        if not os.path.exists(os.path.join(h.home, ".ECCE")):
            ok("plain ecce: no ~/.ECCE directory created at all")
        else:
            # -l no longer creates it, but other machinery might -- only
            # a ServerLogin* file inside it would indicate a regression.
            leaked = [f for f in os.listdir(os.path.join(h.home, ".ECCE"))
                      if f.startswith("ServerLogin")]
            if leaked:
                fail("plain ecce: unexpected %r in ~/.ECCE" % leaked)
            else:
                ok("plain ecce: no ServerLogin* file in ~/.ECCE")
    finally:
        h.cleanup()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.parse_args()

    test_help_text()
    test_l_is_session_only()
    test_plain_ecce_unaffected()
    test_remote_passthrough()
    test_no_l_no_file_needed()
    test_bug_mode_reports_login()

    print()
    if failures:
        print("%d FAILURE(S)" % len(failures))
        sys.exit(1)
    print("all tests passed")
    sys.exit(0)
