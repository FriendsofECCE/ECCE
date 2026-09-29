#!/usr/bin/env python3
"""ecce-dataserver-adduser: batch mode and password-safety checks.

Runs the real packaging/dataserver/ecce-dataserver-adduser against a
throwaway $ECCE_REALUSERHOME, with a wrapper `htpasswd` placed first on
PATH that logs its own argv before exec-ing the real one -- so the suite
can assert that no invocation, in any mode, ever puts a password on a
command line (`ps` visible to every other user on a shared server; this
is the whole point of --from and of switching -b to `htpasswd -i`).

Needs nothing but Python 3 and the real htpasswd (apache2-utils).

    tests/dataserver/adduser.py
"""

import os
import shutil
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPT = os.path.join(REPO, "packaging", "dataserver", "ecce-dataserver-adduser")

REAL_HTPASSWD = shutil.which("htpasswd")

WRAPPER = """#!/bin/bash
echo "$@" >> "%(log)s"
exec "%(real)s" "$@"
"""

failures = []


def ok(msg):
    print("ok - %s" % msg)


def fail(msg):
    print("FAIL - %s" % msg)
    failures.append(msg)


class Harness:
    """One throwaway $ECCE_REALUSERHOME + PATH-wrapped htpasswd."""

    def __init__(self):
        self.home = tempfile.mkdtemp(prefix="ecce-adduser-test-")
        self.statedir = os.path.join(self.home, ".ECCE", "dataserver")
        os.makedirs(self.statedir)
        self.bindir = os.path.join(self.home, "bin")
        os.makedirs(self.bindir)
        self.log = os.path.join(self.home, "htpasswd.argv.log")
        wrapper_path = os.path.join(self.bindir, "htpasswd")
        with open(wrapper_path, "w") as f:
            f.write(WRAPPER % {"log": self.log, "real": REAL_HTPASSWD})
        os.chmod(wrapper_path, 0o755)

    def cleanup(self):
        shutil.rmtree(self.home, ignore_errors=True)

    def env(self):
        e = dict(os.environ)
        e["ECCE_REALUSERHOME"] = self.home
        e["PATH"] = self.bindir + os.pathsep + e["PATH"]
        return e

    def run(self, args, cwd=None, input=None):
        return subprocess.run(
            [SCRIPT] + args,
            cwd=cwd or self.home,
            env=self.env(),
            input=input,
            capture_output=True,
            text=True,
        )

    def users_file(self):
        return os.path.join(self.statedir, "users")

    def userdir(self, userid):
        return os.path.join(self.statedir, "htdocs", "Ecce", "users", userid)

    def log_text(self):
        if not os.path.exists(self.log):
            return ""
        with open(self.log) as f:
            return f.read()


def verify(h, userid, password):
    """htpasswd -v -i, password fed on stdin -- never argv."""
    r = subprocess.run(
        [REAL_HTPASSWD, "-v", "-i", h.users_file(), userid],
        input=password,
        capture_output=True,
        text=True,
    )
    return r.returncode == 0


def assert_no_password_in_log(h, *passwords):
    text = h.log_text()
    for p in passwords:
        if p and p in text:
            fail("password %r leaked into htpasswd argv log" % p)
            return
    ok("no password appeared in any htpasswd invocation's argv")


def test_batch_flag():
    h = Harness()
    try:
        r = h.run(["-b", "carol", "s3cret", "Carol", "Cee"])
        if r.returncode != 0:
            fail("-b carol: exit %d: %s" % (r.returncode, r.stderr))
            return
        if verify(h, "carol", "s3cret"):
            ok("-b: password verifies via htpasswd -v -i")
        else:
            fail("-b: password does not verify")
        assert_no_password_in_log(h, "s3cret")
    finally:
        h.cleanup()


def test_from_csv():
    h = Harness()
    try:
        csv = os.path.join(h.home, "class.csv")
        with open(csv, "w") as f:
            f.write(
                "username,password,first,last\n"
                "\n"
                "# a comment line\n"
                "dave,giveme1,Dave,Dee\n"
                "erin,,Erin,Emm\n"
                "bad user,x,Bad,User\n"
            )
        r = h.run(["--from", csv])
        out = r.stdout

        if "created dave" in out:
            ok("--from: header line skipped, comment/blank ignored, dave created")
        else:
            fail("--from: expected 'created dave' in output:\n%s" % out)

        if "created erin" in out:
            ok("--from: blank-password row created")
        else:
            fail("--from: expected 'created erin' in output:\n%s" % out)

        if "failed line 6" in out and "bad user" in out:
            ok("--from: invalid username reported as failed with the line number")
        else:
            fail("--from: expected a failed-line-6 report for 'bad user':\n%s" % out)

        if r.returncode == 1:
            ok("--from: exit status 1 when any line failed")
        else:
            fail("--from: expected exit 1, got %d" % r.returncode)

        if verify(h, "dave", "giveme1"):
            ok("--from: admin-supplied password stored correctly")
        else:
            fail("--from: dave's password does not verify")

        passfile = csv + ".passwords"
        if not os.path.exists(passfile):
            fail("--from: %s was not written" % passfile)
        else:
            mode = stat.S_IMODE(os.stat(passfile).st_mode)
            if mode == 0o600:
                ok("--from: .passwords file created with mode 0600")
            else:
                fail("--from: .passwords mode is %o, expected 0600" % mode)

            with open(passfile) as f:
                lines = f.read().splitlines()
            erin_line = [l for l in lines if l.startswith("erin,")]
            if len(erin_line) == 1:
                erin_pass = erin_line[0].split(",", 1)[1]
                ok("--from: generated password for erin recorded in .passwords")
                if verify(h, "erin", erin_pass):
                    ok("--from: erin's generated password verifies")
                else:
                    fail("--from: erin's generated password does not verify")
            else:
                fail("--from: no (or multiple) 'erin,...' lines in .passwords")

            dave_lines = [l for l in lines if l.startswith("dave,")]
            if not dave_lines:
                ok("--from: admin-supplied password (dave) never written to .passwords")
            else:
                fail("--from: dave's admin-supplied password leaked into .passwords")

        assert_no_password_in_log(h, "giveme1")

        # Running again while last run's .passwords file is still there
        # must refuse outright, before touching any account.
        r2 = h.run(["--from", csv])
        if r2.returncode != 0 and "already exists" in (r2.stdout + r2.stderr):
            ok("--from: refuses to run again while .passwords already exists")
        else:
            fail(
                "--from: expected refusal to overwrite .passwords, got rc=%d:\n%s%s"
                % (r2.returncode, r2.stdout, r2.stderr)
            )

        # Once that stale .passwords is out of the way, re-running must
        # skip the now-existing accounts and leave their passwords alone.
        os.remove(passfile)
        r3 = h.run(["--from", csv])
        if "skipped dave (already exists)" in r3.stdout:
            ok("--from: re-run skips an existing user")
        else:
            fail("--from: re-run did not report dave as skipped:\n%s" % r3.stdout)

        if verify(h, "dave", "giveme1"):
            ok("--from: existing user's password unchanged by re-run")
        else:
            fail("--from: dave's password changed on re-run")
    finally:
        h.cleanup()


def test_from_stdin():
    h = Harness()
    try:
        r = h.run(["--from", "-"], input="frank,pw12345,Frank,Ford\n")
        if r.returncode != 0:
            fail("--from -: exit %d: %s" % (r.returncode, r.stderr))
        elif verify(h, "frank", "pw12345"):
            ok("--from -: reads from stdin and creates the account")
        else:
            fail("--from -: frank's password does not verify")
        passfile = os.path.join(h.home, "ecce-accounts.passwords")
        if os.path.exists(passfile):
            ok("--from -: writes ./ecce-accounts.passwords")
        else:
            # frank supplied a password, so nothing generated -- the
            # file should still exist (empty) per spec's "created" file.
            fail("--from -: ecce-accounts.passwords was not created")
    finally:
        h.cleanup()


if __name__ == "__main__":
    if REAL_HTPASSWD is None:
        print("ecce-dataserver-adduser tests need htpasswd (apache2-utils) on PATH")
        sys.exit(1)

    test_batch_flag()
    test_from_csv()
    test_from_stdin()

    print()
    if failures:
        print("%d FAILURE(S)" % len(failures))
        sys.exit(1)
    print("all tests passed")
    sys.exit(0)
