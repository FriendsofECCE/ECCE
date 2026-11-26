#!/usr/bin/env python3
"""Selecting an SSH folder lists it off the GUI thread (#1220).

A remembered SSH folder is reached through tests/apps/fakessh/ssh, which
runs each remote command here after a delay.  With every remote command
taking DELAY seconds, selecting a sub-folder must be answered by the
Organizer within about a second, the progress panel with Cancel must come
up, and the listing must be in the tree after the delay.  A second folder
is cancelled: nothing is applied.  Exit 77 (skip) without an install or Xvfb.
"""
import getpass
import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
import checks     # noqa: E402

DELAY = 5

#  Every check in run(); a run that records fewer skipped its body (#1309).
CHECKS = checks.Checks(12, "ssh listing async")
check = CHECKS
failures = CHECKS.failures


def run(display):
    work = tempfile.mkdtemp(prefix="ecce-sshlist-")
    try:
        folder = os.path.join(work, "remote")
        for sub in ("slow", "other"):
            for i in range(6):
                os.makedirs(os.path.join(folder, sub, "d%02d" % i))
        address = "sftp://%s@localhost:2299%s" % (getpass.getuser(), folder)
        prefs = os.path.join(display.env()["ECCE_REALUSERHOME"], ".ECCE")
        os.makedirs(prefs, exist_ok=True)
        added = os.path.join(prefs, "AddedServers")
        saved = open(added).read() if os.path.exists(added) else None
        with open(added, "w") as h:
            h.write("ssh\tSlow\t%s\t0\n" % address)
        delay = os.path.join(work, "delay")
        sshlog = os.path.join(work, "ssh.log")
        with open(delay, "w") as h:
            h.write("0")
        control = os.path.join(work, "cm")
        os.makedirs(control, mode=0o700)
        commands, answers = os.path.join(work, "commands"), os.path.join(work, "answers")
        open(commands, "w").close()
        seen = []

        def command(line, timeout=300):
            with open(commands, "a") as h:
                h.write(line + "\n")
            end = time.monotonic() + timeout
            while time.monotonic() < end:
                rows = open(answers).read().splitlines() if os.path.exists(answers) else []
                if len(rows) > len(seen):
                    seen.append(rows[len(seen)])
                    return seen[-1]
                time.sleep(0.05)
            return "no answer"

        def until(pred, timeout):
            end = time.monotonic() + timeout
            last = ""
            while time.monotonic() < end:
                last = command("listing")
                if last.startswith("listing: "):   # answers echo the command
                    last = last[len("listing: "):]
                if pred(last):
                    return last
                time.sleep(0.2)
            return "timeout: " + last

        def inspect(_display):
            o = {"connect": command("server connect Slow")}
            command("select %s/" % address)
            o["rootdone"] = until(lambda a: a.startswith("idle"), 60)
            with open(delay, "w") as h:
                h.write(str(DELAY))
            t0 = time.monotonic()
            o["select"] = command("select %s/slow/" % address)
            o["selwall"] = time.monotonic() - t0
            o["bar"] = until(lambda a: "bar=1" in a, 4)
            o["done"] = until(lambda a: a.startswith("idle"), 60)
            o["donewall"] = time.monotonic() - t0
            o["kids"] = command("treechildren %s/slow/" % address)
            # a second folder, cancelled while it lists
            o["select2"] = command("select %s/other/" % address)
            o["bar2"] = until(lambda a: "bar=1" in a, 4)
            o["cancel"] = command("listingcancel")
            o["after"] = command("listing")
            time.sleep(DELAY + 3)
            o["later"] = command("listing")
            o["kids2"] = command("treechildren %s/other/" % address)
            with open(delay, "w") as h:
                h.write("0")
            return o

        env = {"ECCE_TEST_ORGANIZER": commands, "ECCE_TEST_ORGANIZER_ANSWERS": answers,
               "PATH": os.path.join(HERE, "fakessh") + os.pathsep + display.env()["PATH"],
               "FAKE_SSH_DELAY_FILE": delay, "FAKE_SSH_LOG": sshlog,
               "ECCE_SSH_CONTROL_DIR": control, "ECCE_ASKPASS": "/bin/false",
               "ECCE_LOCAL_DATA": os.path.join(work, "local")}
        try:
            result = apps.run(display, "organizer", windowTimeout=60, settle=3,
                              env=env, inspect=inspect)
        finally:
            if saved is None:
                os.remove(added)
            else:
                open(added, "w").write(saved)
        if not check(not result.crashed and result.inspected, "the Organizer ran",
                     (result.log or "")[-1500:]):
            return
        o = result.inspected
        print("  %s" % o)
        check(o["connect"].endswith(": connected"), "the folder connects", o["connect"])
        check(o["rootdone"].startswith("idle"), "the root's listing finishes", o["rootdone"])
        check(o["select"].endswith(": ok"), "the sub-folder is selected", o["select"])
        check(o["selwall"] < 1.5,
              "selecting is answered within 1.5 s while each remote command takes %d s" % DELAY,
              "%.2f s" % o["selwall"])
        check("bar=1" in o["bar"], "the progress panel appears", o["bar"])
        check(o["donewall"] >= DELAY - 1 and o["done"].startswith("idle") and "applied=2" in o["done"],
              "the listing is applied after the delay", "%s after %.1f s" % (o["done"], o["donewall"]))
        check(all("d%02d" % i in o["kids"] for i in range(6)),
              "the listing is in the tree", o["kids"])
        check("bar=1" in o["bar2"], "the second listing shows the panel", o["bar2"])
        check(o["after"].startswith("listing: idle bar=0"), "Cancel ends the listing and hides the panel", o["after"])
        check("applied=2" in o["later"], "a cancelled listing is not applied", o["later"])
        check(not re.search(r"d00", o["kids2"]), "the cancelled folder stays unloaded", o["kids2"])
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    def check_app(display, name, results, verbose=False):
        run(display)
        CHECKS.recordOn(results)
    run_tests.checkApp = check_app
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
