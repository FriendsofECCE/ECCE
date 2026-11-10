#!/usr/bin/env python3
"""Preferences > Data and the Add Server dialog (#767), headless.

The ECCE_TEST_ORGANIZER "prefs data ..." hook drives the real Preferences
window and dialog as clicks do: an empty server list, Add Server for an ECCE
server and an SSH folder (through the dialog's OK path), the stored
~/.ECCE/AddedServers, Remove, and the row of a server set by the installation
(ECCE_TEST_PREFS_ADMIN_SERVER, shown in every picture).  PNGs go to ECCE_SHOTS_DIR
(default ~/ecce-767/shots) as 767-*.png.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402

failures = []
SHOTS = os.environ.get("ECCE_SHOTS_DIR") or os.path.expanduser("~/ecce-767/shots")
os.makedirs(SHOTS, exist_ok=True)


def shot(name):
    return os.path.join(SHOTS, "767-" + name + ".png")


ECC = "ecce|Course server|https://ecce.example.edu"
SSH = "ssh|Cluster storage|sftp://me@hpc.example.edu/home/me/ecce"

PLAIN = [
    ("prefs data show", lambda a: a.startswith("rows=1;This installation|ecce|Set by your administrator;")),
    ("prefs data snap " + shot("data-empty"), lambda a: a == "saved"),
    # #690: the switch is off until ticked, and the tick reaches EcceGlobal
    ("prefs data discard", lambda a: a == "box=off;file=unset"),
    ("prefs data discard on", lambda a: a == "box=on;file=on"),
    ("prefs data discard off", lambda a: a == "box=off;file=off"),
    ("prefs data form ssh " + shot("add-ssh"), lambda a: a == "saved"),
    ("prefs data form ecce " + shot("add-ecce"), lambda a: a == "saved"),
    ("prefs data add ecce Course_server https://ecce.example.edu " + shot("add-ecce-filled"),
     lambda a: a == "saved"),
    ("prefs data file", lambda a: a == ECC + ";"),
    ("prefs data add ssh Cluster_storage hpc.example.edu me /home/me/ecce " + shot("add-ssh-filled"),
     lambda a: a == "saved"),
    ("prefs data file", lambda a: a == ECC + ";" + SSH + ";"),
    ("prefs data show", lambda a: a.startswith("rows=3;This installation|ecce|Set by your administrator;Course server|ecce|https://ecce.example.edu;Cluster storage|ssh|sftp://me@hpc.example.edu/home/me/ecce;")),
    ("prefs data snap " + shot("data-two"), lambda a: a == "saved"),
    # a duplicate name is refused and stores nothing
    ("prefs data add ecce Course_server https://other.example.edu " + shot("add-duplicate"),
     lambda a: a == "saved"),
    ("prefs data file", lambda a: a == ECC + ";" + SSH + ";"),
    ("prefs data remove Course_server", lambda a: a == "ok"),
    ("prefs data file", lambda a: a == SSH + ";"),
    ("prefs data snap " + shot("data-after-remove"), lambda a: a == "saved"),
    # the installation's row cannot be removed
    ("prefs data remove This_installation", lambda a: a == "ok"),
    ("prefs data show", lambda a: a.startswith("rows=2;This installation|ecce|Set by your administrator;Cluster storage|")),
]
def run(display, steps, extra, label):
    work = tempfile.mkdtemp(prefix="ecce-prefsdata-")
    data = tempfile.mkdtemp(prefix="ecce-prefsdata-data-")
    try:
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("".join(c + "\n" for c, _ in steps))
        env = {"ECCE_TEST_ORGANIZER": cmds, "ECCE_LOCAL_DATA": data,
               "ECCE_NATIVE_DIALOGS": "explicit"}
        env.update(extra)
        result = apps.run(display, "organizer", windowTimeout=60, settle=30, env=env)
        log = result.log or ""
        answers = [l.split("ECCE_TEST_ORGANIZER: ", 1)[1]
                   for l in log.splitlines() if "ECCE_TEST_ORGANIZER: " in l]
        for number, (command, ok) in enumerate(steps):
            got = None
            if number < len(answers) and answers[number].startswith(command + ": "):
                got = answers[number][len(command) + 2:].strip()
            if got is not None and ok(got):
                print("ok    [%s] %s -> %s" % (label, command.split("/")[-1], got))
            else:
                failures.append(label + ": " + command)
                print("FAIL  [%s] %s -> %r" % (label, command, got))
        if failures:
            print(log[-1500:])
    finally:
        shutil.rmtree(work, ignore_errors=True)
        shutil.rmtree(data, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        # One organizer start: a second one in the same isolated state hit an
        # unrelated bookmark-file parse abort.
        run(display, PLAIN, {"ECCE_TEST_PREFS_ADMIN_SERVER": "1"}, "prefs")
        for png in ("data-empty", "add-ssh", "add-ecce", "data-two"):
            if not os.path.exists(shot(png)):
                failures.append("no picture " + png)
                print("FAIL  missing " + shot(png))
        if failures:
            results.fail("prefs-data", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
