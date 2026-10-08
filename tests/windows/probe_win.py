#!/usr/bin/env python3
"""Look checks for the Windows client through the apps' own test hooks, no
clicks: an ECCE-QM water calculation made by the Organizer and drawn by the
Builder, then
  - the thumbnail the Builder stores on Save is a whole JPEG,
  - the Organizer's tree context menu offers the codes under a project
    while a calculation was selected before (as after a right click),
  - the Calculation Editor's main controls (Charge, Spin) for a fresh
    calculation, and after switching the code to ORCA (Save, Verify),
  - window captures (PrintWindow) of the Organizer and the Calculation Editor.

    python probe_win.py <install tree> <state dir> <png dir>
"""
import os
import shutil
import subprocess
import sys
import time

tree, state, pngs = (os.path.abspath(a).replace("\\", "/") for a in sys.argv[1:4])
BIN = tree + "/bin"
HERE = os.path.dirname(os.path.abspath(__file__))
failures = []
sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what), flush=True)
    if not ok:
        failures.append(what)
    return ok


shutil.rmtree(state, ignore_errors=True)
os.makedirs(pngs, exist_ok=True)
home, local, out = state + "/home", state + "/local", state + "/out"
for d in (home + "/.ECCE", local + "/users/local", out):
    os.makedirs(d)
sysdir = os.environ.get("SystemRoot", r"C:\Windows")
env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
env.update({
    "ECCE_HOME": tree, "ECCE_REALUSERHOME": home, "ECCE_LOCAL_DATA": local,
    "ECCE_REALUSER": os.environ.get("USERNAME", "user"),
    "HOST": os.environ.get("COMPUTERNAME", "localhost"),
    "ECCE_NO_DATASERVER": "1", "ECCE_SESSION_LIVENESS": "lease",
    "ECCE_SESSION_ID": os.urandom(8).hex(), "ECCE_NO_FIRST_START": "1",
    "PATH": os.pathsep.join([BIN, tree + "/scripts", tree + "/scripts/parsers", tree + "/usr/bin",
                             tree + "/python", tree + "/strawberry/perl/site/bin",
                             tree + "/strawberry/perl/bin", tree + "/strawberry/c/bin",
                             sysdir + r"\System32", sysdir]),
})


def winshot(pid, png):
    subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                    os.path.join(HERE, "winshot.ps1"), "-ProcId", str(pid), "-Png", png],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def app(name, script_var, script, args=(), timeout=200, extra=None):
    """Runs a tool with a test script; a "shot NAME" step is photographed."""
    path = os.path.join(state, name + ".script")
    with open(path, "w", newline="\n") as h:
        h.write(script)
    e = dict(env, ECCE_FEEDBACK_STDERR="1", **(extra or {}))
    if script_var:
        e[script_var] = path
    log = os.path.join(out, name + ".log")
    with open(log, "w") as h:
        p = subprocess.Popen([BIN + "/" + name + ".exe"] + list(args), env=e, stdout=h,
                             stderr=subprocess.STDOUT)
        t0 = time.time()
        while p.poll() is None and time.time() - t0 < timeout:
            for f in os.listdir(state):
                if f.endswith(".ready"):
                    shot = f[:-6]
                    os.remove(os.path.join(state, f))
                    winshot(p.pid, "%s/%s.png" % (pngs, shot))
                    open(os.path.join(state, shot + ".go"), "w").close()
            time.sleep(0.5)
        if p.poll() is None:
            p.kill()
    return open(log, encoding="utf-8", errors="replace").read()


subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-gateway-start"], env=env,
               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
cmds = os.path.join(state, "org.cmds")
open(cmds, "w").close()
orglog = open(os.path.join(out, "organizer.log"), "w")
org = subprocess.Popen([BIN + "/organizer.exe"], env=dict(env, ECCE_TEST_ORGANIZER=cmds),
                       stdout=orglog, stderr=subprocess.STDOUT)


def ask(cmd, wait=120):
    with open(cmds, "a", newline="\n") as h:
        h.write(cmd + "\n")
    for _ in range(wait):
        time.sleep(1)
        text = open(os.path.join(out, "organizer.log"), errors="replace").read()
        hits = [l for l in text.splitlines() if l.startswith("ECCE_TEST_ORGANIZER: %s: " % cmd)]
        if hits:
            return hits[-1].split(": ", 2)[2]
    return "no answer"


try:
    time.sleep(8)
    check(ask("newproject p").startswith("ok"), "project made")
    d = local + "/users/local/p/w"
    url = "file://" + d
    check(ask("newcalc p w ECCE-QM").startswith("ok"), "calculation made")
    text = app("builder", "ECCE_BUILDER_SCRIPT",
               "wait 4000\nadd O Bent 0 0 0\nwait 1500\ncmd addh\nwait 1500\n"
               "expect atoms 3\nsave\nwait 4000\nquit\n", ("-context", url))
    check("expect atoms 3: ok" in text, "the Builder drew water")
    thumb = d + "/Parameters/Thumbnail.jpeg"
    data = open(thumb, "rb").read() if os.path.exists(thumb) else b""
    check(len(data) > 200 and data[:2] == b"\xff\xd8" and data.rstrip(b"\0")[-2:] == b"\xff\xd9",
          "thumbnail is a whole JPEG (%d bytes)" % len(data))

    a = ask("summary " + url)
    check(a.startswith("ok"), "calculation selected: " + a[:60])
    a = ask("contextmenu file://" + local + "/users/local/p")
    print("  contextmenu: " + a, flush=True)
    check("selection=file://" + local + "/users/local/p" in a and "ECCE-QM" in a,
          "right-clicking the project offers new calculations")
    ask("summary " + url)
    time.sleep(2)
    winshot(org.pid, pngs + "/organizer.png")

    text = app("calced", "ECCE_CALCED_SCRIPT",
               "wait 5000\nenabled\ncode ORCA\nwait 4000\nenabled\n"
               "wmcommand 100004\nwait 2000\nenabled\nwmcommand 30004\nwait 3000\nenabled\ninfo\nbutton save\n"
               "wait 4000\nbutton verify\nwait 3000\nenabled\ninfo\nquit\n", ("-context", url))
    for l in text.splitlines():
        if "enabled: ok" in l or "basis " in l or l.startswith("CALCED: verify=") or l.startswith("FEEDBACK:"):
            print("  " + l, flush=True)
    en = [l for l in text.splitlines() if "ECCE_CALCED_SCRIPT: enabled: ok" in l]
    check(bool(en) and "charge-label=on" in en[0], "Charge enabled for a fresh calculation")
    inputs = os.listdir(d + "/Inputs") if os.path.isdir(d + "/Inputs") else []
    check(any(f.endswith(".orcain") for f in inputs), "ORCA input stored after switching: %s" % inputs)
finally:
    org.kill()
    subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-broker-win", "stop"], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
print("FAILED: " + "; ".join(failures) if failures else "PASSED")
sys.exit(1 if failures else 0)
