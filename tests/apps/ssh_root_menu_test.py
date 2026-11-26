#!/usr/bin/env python3
"""The right-click menu of an SSH folder's root opens at once on a slow link (#1273).

A remembered SSH folder (sftp://<user>@localhost:2299/<tmp dir>) is reached
through tests/apps/fakessh/ssh, which runs each remote command on this
machine after a delay, so no host is contacted.  The folder is connected
with no delay; then every remote command takes DELAY seconds and the
root is selected and its desk shown ("select", then "desk"): that must be
one remote script, under 1.5 x DELAY.  Then the "menulabels" hook builds
the root's menu as a right-click does: within 1 s, still offering the
server and folder items.  Exit 77 (skip)
without an install or Xvfb.
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
import fixture    # noqa: E402
import run_tests  # noqa: E402

DELAY = 3
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + str(detail)) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    work = tempfile.mkdtemp(prefix="ecce-sshmenu-")
    try:
        folder = os.path.join(work, "remote")
        os.makedirs(os.path.join(folder, "proj"))
        shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-opt"),
                        os.path.join(folder, "proj", "water"), symlinks=True)
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

        def inspect(_display):
            out = {"connect": command("server connect Slow")}
            out["roots"] = command("server roots")
            with open(delay, "w") as h:
                h.write(str(DELAY))
            mark = os.path.getsize(sshlog) if os.path.exists(sshlog) else 0
            t0 = time.monotonic()
            out["select"] = command("select %s/" % address)
            end = time.monotonic() + 60   # the listing runs off the GUI thread (#1220)
            while time.monotonic() < end and not command("listing").startswith("listing: idle"):
                time.sleep(0.2)
            out["desk"] = command("desk")
            out["selwall"] = time.monotonic() - t0
            out["selssh"] = open(sshlog).read()[mark:] if os.path.exists(sshlog) else ""
            mark = os.path.getsize(sshlog) if os.path.exists(sshlog) else 0
            t0 = time.monotonic()
            out["menu"] = command("menulabels %s/" % address)
            out["wall"] = time.monotonic() - t0
            out["ssh"] = open(sshlog).read()[mark:] if os.path.exists(sshlog) else ""
            with open(delay, "w") as h:
                h.write("0")
            return out

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
        out = result.inspected
        print("  connect=%s roots=%s" % (out["connect"], out["roots"]))
        print("  select + desk (%.1f s wall): %s / %s" % (out["selwall"], out["select"], out["desk"][:120]))
        print("  remote commands during select + desk (each %d s):\n    %s" %
              (DELAY, out["selssh"].strip().replace("\n", "\n    ") or "(none)"))
        print("  menulabels (%.1f s wall): %s" % (out["wall"], out["menu"][:400]))
        print("  remote commands during the menu (each %d s):\n    %s" %
              (DELAY, out["ssh"].strip().replace("\n", "\n    ") or "(none)"))
        print("  remote scripts and hooks, in order (Organizer log):\n    " + "\n    ".join(
            l[:200] for l in (result.log or "").splitlines()
            if "SftpEDSI-script" in l or l.startswith("ECCE_TEST_ORGANIZER:")))
        check(out["connect"].endswith(": connected"), "the slow SSH folder connects", out["connect"])
        check(out["select"].endswith(": ok") and " desk " in " " + out["desk"],
              "the root is selected and its desk shown", "%s / %s" % (out["select"], out["desk"]))
        nssh = len(out["selssh"].strip().splitlines())
        check(nssh == 1, "selecting the root makes one remote script", "%d scripts" % nssh)
        check(out["selwall"] < 1.5 * DELAY,
              "selecting the root and showing its desk takes under %.1f s (one round trip)" % (1.5 * DELAY),
              "%.1f s" % out["selwall"])
        m = re.search(r"menu select=(\d+) menu=(\d+)", out["menu"])
        if not check(m is not None, "the menu hook answered", out["menu"]):
            return
        print("  select took %s ms, the menu %s ms" % (m.group(1), m.group(2)))
        check(int(m.group(2)) < 1000,
              "the root's menu is built within 1 s while each remote command takes %d s" % DELAY,
              "%s ms" % m.group(2))
        for want in ("Disconnect", "Remove from list", "Paste", "Delete"):
            check(re.search(r"\| %s( \(disabled\))?( \||$)" % re.escape(want), out["menu"]) is not None,
                  "the root's menu offers " + want, out["menu"][:400])
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    def check_app(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("ssh root menu", ", ".join(failures))
    run_tests.checkApp = check_app
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
