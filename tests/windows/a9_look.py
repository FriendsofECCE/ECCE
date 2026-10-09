#!/usr/bin/env python3
"""Windows look and redraw checks through the apps' own test hooks, no clicks:
  - the Organizer's summary of a CH4 calculation (formula order, the formula
    column against "Charge:") and a capture of the Organizer;
  - how often the Organizer rebuilds its tree for the messages a Save in the
    Calculation Editor sends (test command "counts");
  - the states the Calculation Editor shows while it opens from the
    Organizer (framelog.ps1: one line per state seen on screen);
  - captures of the Calculation Editor (NWChem) and the Launcher;
  - New Calculation with the name prompt: whether the Organizer stays the
    foreground window (test command "newprompt", framelog's fg/bg);
  - with "qm": an ECCE-QM water run, then the Builder opened on it: which
    top-level windows and panes it opens by itself.
Needs the desktop (a scheduled task with /it).

    python a9_look.py <install tree> <state dir> <png dir> [qm]
"""
import os
import shutil
import subprocess
import sys
import time

tree, state, pngs = (os.path.abspath(a).replace("\\", "/") for a in sys.argv[1:4])
QM = "qm" in sys.argv[4:]
BIN = tree + "/bin"
HERE = os.path.dirname(os.path.abspath(__file__))
sys.stdout.reconfigure(encoding="utf-8", errors="replace")

shutil.rmtree(state, ignore_errors=True)
os.makedirs(pngs, exist_ok=True)
home, local, out = state + "/home", state + "/local", state + "/out"
for d in (home + "/.ECCE", local + "/users/local", out, state + "/runs", state + "/tmp"):
    os.makedirs(d)
sysdir = os.environ.get("SystemRoot", r"C:\Windows")
env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
env.update({
    "ECCE_HOME": tree, "ECCE_REALUSERHOME": home, "ECCE_LOCAL_DATA": local,
    "ECCE_REALUSER": os.environ.get("USERNAME", "user"),
    "HOST": os.environ.get("COMPUTERNAME", "localhost"),
    "ECCE_NO_DATASERVER": "1", "ECCE_SESSION_LIVENESS": "lease",
    "ECCE_TMPDIR": state + "/tmp",
    "ECCE_SESSION_ID": os.urandom(8).hex(), "ECCE_NO_FIRST_START": "1",
    "PATH": os.pathsep.join([BIN, tree + "/scripts", tree + "/scripts/parsers", tree + "/usr/bin",
                             tree + "/python", tree + "/strawberry/perl/site/bin",
                             tree + "/strawberry/perl/bin", tree + "/strawberry/c/bin",
                             sysdir + r"\System32", sysdir]),
})


def ps(script, *args, timeout=120):
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                        os.path.join(HERE, script)] + [str(a) for a in args],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    return r.stdout.decode(errors="replace")


def framelog(seconds, name="", pid=0):
    """Starts framelog.ps1 in the background; .result() gives its lines."""
    path = os.path.join(out, "frames-%d.txt" % time.time_ns())
    args = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
            os.path.join(HERE, "framelog.ps1"), "-Seconds", str(seconds), "-Out", path]
    args += ["-Name", name] if name else ["-ProcId", str(pid)]
    p = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    class R:
        def result(self):
            p.wait(timeout=seconds + 60)
            return open(path, errors="replace").read().splitlines() if os.path.exists(path) else []
    return R()


def app(name, script_var, script, args=(), timeout=200, extra=None, wait=True):
    path = os.path.join(state, name + ".script")
    with open(path, "w", newline="\n") as h:
        h.write(script)
    e = dict(env, ECCE_FEEDBACK_STDERR="1", **(extra or {}))
    if script_var:
        e[script_var] = path
    log = os.path.join(out, name + ".log")
    h = open(log, "w")
    p = subprocess.Popen([BIN + "/" + name + ".exe"] + list(args), env=e, stdout=h,
                         stderr=subprocess.STDOUT)
    if not wait:
        return p
    try:
        p.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        p.kill()
    h.close()
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
    for _ in range(wait * 2):
        time.sleep(0.5)
        text = open(os.path.join(out, "organizer.log"), errors="replace").read()
        hits = [l for l in text.splitlines() if l.startswith("ECCE_TEST_ORGANIZER: %s: " % cmd)]
        if hits:
            return hits[-1].split(": ", 2)[2]
    return "no answer"


def say(t):
    print(t, flush=True)


try:
    time.sleep(8)
    proj = "file://" + local + "/users/local/p"
    say("newproject: " + ask("newproject p"))
    say("newcalc: " + ask("newcalc p m NWChem"))
    url = proj + "/m"
    text = app("builder", "ECCE_BUILDER_SCRIPT",
               "wait 4000\nadd C Tetrahedral 0 0 0\nwait 1500\ncmd addh\nwait 1500\n"
               "expect atoms 5\nsave\nwait 4000\nquit\n", ("-context", url))
    say("builder: " + ("CH4 drawn" if "expect atoms 5: ok" in text else "FAILED"))
    say("summary: " + ask("summary " + url))
    time.sleep(2)
    ps("winshot.ps1", "-ProcId", org.pid, "-Png", pngs + "/organizer.png")

    # 1. the tree rebuilds for one Save in the Calculation Editor
    say("counts before save: " + ask("counts"))
    text = app("calced", "ECCE_CALCED_SCRIPT",
               "wait 5000\nbasis 6-31G*\nwait 1500\nbutton save\nwait 4000\nquit\n",
               ("-context", url))
    time.sleep(3)
    say("counts after save: " + ask("counts"))
    # 2. the Calculation Editor opening from the Organizer
    fl = framelog(10, name="calced")
    time.sleep(0.5)
    say("start calced: " + ask("start CalculationEditor " + url))
    lines = fl.result()
    say("calced open: %d states seen" % len(lines))
    for l in lines:
        say("   " + l)
    r = subprocess.run(["powershell", "-NoProfile", "-Command",
                        "(Get-Process calced | Sort StartTime | Select -Last 1).Id"],
                       stdout=subprocess.PIPE)
    cpid = r.stdout.decode().strip()
    if cpid:
        ps("winshot.ps1", "-ProcId", cpid, "-Png", pngs + "/calced.png")
        subprocess.run(["taskkill", "/F", "/PID", cpid], stdout=subprocess.DEVNULL)

    # 5. the Launcher
    p = app("launcher", "ECCE_LAUNCHER_SCRIPT", "wait 7000\nquit\n", ("-context", url), wait=False)
    time.sleep(5)
    ps("winshot.ps1", "-ProcId", p.pid, "-Png", pngs + "/launcher.png")
    p.wait(timeout=60)

    # 4. New Calculation with the name prompt
    fl = framelog(12, pid=org.pid)
    time.sleep(1)
    say("newprompt: " + ask("newprompt %s NWChem" % proj))
    lines = fl.result()
    say("organizer during New: %d states, %s" % (len(lines), "background seen" if any(
        l.endswith(" bg") for l in lines[1:]) else "stayed foreground"))
    for l in lines:
        say("   " + l)

    if QM:
        say("newcalc qm: " + ask("newcalc p w ECCE-QM"))
        qurl = proj + "/w"
        app("builder", "ECCE_BUILDER_SCRIPT",
            "wait 4000\nadd O Bent 0 0 0\nwait 1500\ncmd addh\nwait 1500\nsave\nwait 4000\nquit\n",
            ("-context", qurl))
        app("calced", "ECCE_CALCED_SCRIPT", "wait 6000\nbutton save\nwait 5000\nquit\n",
            ("-context", qurl))
        run = state + "/runs/w"
        os.makedirs(run)
        app("launcher", "ECCE_LAUNCHER_SCRIPT",
            "wait 3000\nmachine localhost\nrundir \"%s\"\nwait 1500\nlaunch\nwait 5000\nquit\n" % run,
            ("-context", qurl))
        st = ""
        for _ in range(100):
            st = ask("state " + qurl, wait=30)
            if st.lower().startswith(("complete", "failed", "killed", "unsuccess", "system")):
                break
            time.sleep(3)
        say("qm run: " + st)
        metrics = os.path.join(out, "panes.txt")
        p = subprocess.Popen([BIN + "/builder.exe", "-context", qurl],
                             env=dict(env, ECCE_PANEL_METRICS=metrics),
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(60):
            time.sleep(1)
            if os.path.exists(metrics) and "pane " in open(metrics, errors="replace").read():
                break
        time.sleep(3)
        say(ps("titles.ps1", "-ProcId", p.pid).strip())
        for l in open(metrics, errors="replace").read().splitlines() if os.path.exists(metrics) else []:
            say("   " + l)
        ps("winshot.ps1", "-ProcId", p.pid, "-Png", pngs + "/builder.png")
        p.kill()
finally:
    org.kill()
    subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-broker-win", "stop"], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
say("done")
