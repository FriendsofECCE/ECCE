#!/usr/bin/env python3
"""An ECCE server added in the Organizer is its own tree root (#767, a2a).

A local-mode Organizer (ECCE_LOCAL_DATA) with this run's data server in
~/.ECCE/AddedServers: the root "<name> (ECCE server)" appears "(not connected)",
the login window (ECCE_TEST_AUTH_ANSWER=login:USER:PASS) connects it, a
project and a calculation made there land in that server's htdocs, a copy
between trees is refused, and a wrong password leaves the root unconnected
with Local data still the session's tree.  Exit 77 (skip) without an
install or Xvfb.
"""
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

failures = []
NAME = "Course"
ROOT = NAME + " (ECCE server)"


def serverFile():
    return os.path.join(os.environ.get("ECCE_REALUSERHOME", os.path.expanduser("~")),
                        ".ECCE", "AddedServers")


def run(display, steps, extra, label, after=None):
    work = tempfile.mkdtemp(prefix="ecce-srvroots-")
    data = tempfile.mkdtemp(prefix="ecce-srvroots-data-")
    try:
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("".join(c + "\n" for c, _ in steps))
        env = {"ECCE_TEST_ORGANIZER": cmds, "ECCE_LOCAL_DATA": data,
               "ECCE_NATIVE_DIALOGS": "explicit",
               "ECCE_TEST_ALLOW_HTTP_SERVER": "1"}
        env.update(extra)
        result = apps.run(display, "organizer", windowTimeout=60, settle=40, env=env)
        log = result.log or ""
        answers = [l.split("ECCE_TEST_ORGANIZER: ", 1)[1]
                   for l in log.splitlines() if "ECCE_TEST_ORGANIZER: " in l]
        bad = False
        for number, (command, ok) in enumerate(steps):
            got = None
            if number < len(answers) and answers[number].startswith(command + ": "):
                got = answers[number][len(command) + 2:].strip()
            if got is not None and ok(got):
                print("ok    [%s] %s -> %s" % (label, command, got))
            else:
                bad = True
                failures.append(label + ": " + command)
                print("FAIL  [%s] %s -> %r" % (label, command, got))
        if after:
            after(log)
        if bad:
            print(log[-2000:])
    finally:
        shutil.rmtree(work, ignore_errors=True)
        shutil.rmtree(data, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        user = fixture.realUser()
        fixture.ensureRealUserAccount()
        os.makedirs(os.path.dirname(serverFile()), exist_ok=True)
        with open(serverFile(), "w") as handle:
            handle.write("ecce\t%s\thttp://localhost:%d\n" % (NAME, fixture.dataserverPort()))
        home = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users", user)
        proj = "p767"
        notConn = lambda a: ROOT + " (not connected)" in a and "Local data" in a
        steps = [
            ("server roots", notConn),
            ("server connect " + NAME, lambda a: a == "connected"),
            ("server roots", lambda a: ROOT in a and "not connected" not in a
                             and a.startswith("Local data")),
            ("server newproject %s %s" % (NAME, proj), lambda a: a.startswith("ok http")),
            ("server newcalc %s %s c1" % (NAME, proj), lambda a: a.startswith("ok http")),
            ("server copycheck file:///tmp/x http://localhost:%d/Ecce/users/%s/%s"
             % (fixture.dataserverPort(), user, proj),
             lambda a: a == "refused: Copying between Local data and a server is not supported yet."),
            ("server copycheck http://localhost:%d/Ecce/users/%s/%s file:///tmp"
             % (fixture.dataserverPort(), user, proj),
             lambda a: a.startswith("refused: Copying between Local data")),
            ("server launchcheck http://localhost:%d/Ecce/users/%s/%s/c1"
             % (fixture.dataserverPort(), user, proj), lambda a: a == "refused"),
        ]
        def stored(log):
            if not os.path.isdir(os.path.join(home, proj, "c1")):
                failures.append("calculation not stored on the server")
                print("FAIL  c1 not under " + home)
            else:
                print("ok    calculation is in the server's htdocs")
        run(display, steps, {"ECCE_TEST_AUTH_ANSWER": "login:%s:%s" % (user, fixture.passwordFor(user))},
            "good", stored)
        bad = [
            ("server connect " + NAME, lambda a: a == "not connected"),
            ("server roots", notConn),
        ]
        def stayed(log):
            if "switched" in log.lower() and "local" in log.lower().split("switched")[1][:40]:
                failures.append("session mode changed")
        run(display, bad, {"ECCE_TEST_AUTH_ANSWER": "login:%s:wrongpass" % user}, "wrong", stayed)
        try:
            os.unlink(serverFile())
        except OSError:
            pass
        if failures:
            results.fail("server-roots", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
