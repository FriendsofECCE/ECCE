#!/usr/bin/env python3
"""Launching a calculation stored on an added ECCE server (#767, a2b).

A local-mode session (data folder, per-user broker) with this run's own data
server added as "Course".  An xtb water calculation made on that server is
launched on localhost by the Organizer's Launch Selected; the job store and
monitor must store to the calculation's own URL with the server's login, so the
calculation reaches completed with its output and properties in the server's
htdocs and nothing in Local data.

    tests/launch/server_launch_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without xtb, Xvfb or the built binaries.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
import xtb_test  # noqa: E402

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    xtb = shutil.which("xtb")
    if not xtb:
        harness.skip("xtb is not installed")
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "srvlaunch", {"xTB": xtb}, local=True,
                        keep=args.keep)
    harness.link(os.path.join(build, "organizer"),
                 os.path.join(s.home, "bin", "organizer"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    s.registerMachine("localhost", "Shell", {"xTB": xtb})
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        #  The data server of this run, started beside the local session.
        rc, out = s.run([os.path.join(s.home, "bin", "ecce-dataserver-start")],
                        timeout=240)
        say("  ecce-dataserver-start: rc=%d %s" % (rc, out.strip().replace("\n", " | ")[:200]))
        if not check(rc == 0, "the data server started"):
            return 1
        os.environ.setdefault("ECCE_REALUSER", s.user())
        s.fixture.ensureRealUserAccount()
        user = s.user()
        password = s.fixture.passwordFor(user)
        port = s.fixture.dataserverPort()
        server = "http://localhost:%d" % port
        with open(os.path.join(s.state, ".ECCE", "AddedServers"), "w") as h:
            h.write("ecce\tCourse\t%s\n" % server)
        htdocs = os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", user)
        userUrl = "%s/Ecce/users/%s" % (server, user)

        pipeCount = [0]

        def pipe():
            pipeCount[0] += 1
            path = os.path.join(s.state, "auth.%d.pipe" % pipeCount[0])
            root = os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", user)
            realm = s.fixture.realm(root)
            with open(path, "w") as h:
                h.write("2\n%s/%s|%s|%s\n%s/|%s|%s\n"
                        % (server, realm, user, password, server, user, password))
            os.chmod(path, 0o600)
            return path

        def drive(*argv):
            return s.run([os.path.join(s.home, "bin", "launchjob"), "-pipe", pipe()]
                         + list(argv), cwd=os.path.join(s.home, "bin"))

        text, rc = xtb_test.deck(xtb_test.WATER, "Energy")
        deck = os.path.join(work, "w.xtbin")
        open(deck, "w").write(text)
        xyz = os.path.join(work, "w.xyz")
        with open(xyz, "w") as h:
            h.write("3\nwater\n")
            for a in xtb_test.WATER:
                h.write("%s %.6f %.6f %.6f\n" % a)
        name = "w-%d" % int(time.time())
        rc, out = drive("create", userUrl, name, "xtb_es", deck, "xtb.xtbin",
                        "localhost", rundir, user, "procs=1", "queue=normal", "nodes=1", "code=xTB",
                        "xyz=" + xyz)
        url = out.strip().splitlines()[-1] if rc == 0 else None
        if not check(url and url.startswith("http"), "calculation made on the server: %s"
                     % (url or out[-200:])):
            return 1
        rc, out = drive("state", url)
        check(out.strip().splitlines()[-1] == "ready", "it is ready: %s" % out.strip()[-40:])

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"],
                      "ECCE_NATIVE_DIALOGS": "explicit",
                      "ECCE_TEST_ALLOW_HTTP_SERVER": "1",
                      "ECCE_TEST_AUTH_ANSWER": "login:%s:%s" % (user, password)})
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=300):
            seen = len(open(olog, errors="replace").read().splitlines())
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if org.poll() is not None:
                    return "the Organizer ended (%s)" % org.returncode
                for t in open(olog, errors="replace").read().splitlines()[seen:]:
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        ans = send("server connect Course")
        check(ans == "connected", "the server tree connected: %s" % ans)
        ans = send("server launchcheck " + url)
        check(ans == "allowed", "launch from the server tree is allowed: %s" % ans)
        ans = send("launchselected " + url, timeout=240)
        say("  answer: %s" % ans)
        check(ans.startswith("ok ") and "submitted" in ans, "Launch Selected submitted it")

        state = ""
        end = time.time() + 300
        while time.time() < end:
            rc, out = drive("state", url)
            state = out.strip().splitlines()[-1] if out.strip() else ""
            if state in ("completed", "failed", "unsuccessful", "system_failure"):
                break
            time.sleep(2)
        check(state == "completed", "it reached completed (last: %s)" % state)
        time.sleep(2)

        calc = os.path.join(htdocs, url.rstrip("/").split("/Ecce/users/%s/" % user)[1])
        check(os.path.exists(os.path.join(calc, "Outputs", "xtb.xtbout")),
              "the output is stored on the server")
        props = sorted(os.listdir(os.path.join(calc, "Props"))) if os.path.isdir(
            os.path.join(calc, "Props")) else []
        say("  properties: " + " ".join(props))
        for key in ("TE", "HLGAP", "DIPOLE"):
            check(key in props, "%s is stored on the server" % key)
        stray = [r for r, d, f in os.walk(s.localData()) if name in os.path.basename(r)]
        check(not stray, "nothing of it in Local data: %s" % stray)
    finally:
        if org is not None and org.poll() is None:
            org.terminate()
            try:
                org.wait(10)
            except Exception:
                org.kill()
        if display is not None:
            display.__exit__(None, None, None)
        s.stop()
    if failures:
        say("FAILED: " + "; ".join(failures))
        return 1
    say("ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
