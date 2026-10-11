#!/usr/bin/env python3
"""What a user sees while the Organizer and the Viewer redraw (#254, #255,
#256), through the apps' own test hooks, no clicks.  Every state the screen
shows is logged by framelog.ps1 and saved as a PNG under <png dir>:
  - organizer-select/: the Organizer while the tree selection steps through
    four calculations (test command "click", the tree's own selection);
  - viewer-open/: the Viewer opened from the Organizer on an ECCE-QM water
    run, then switched by its Properties list (Builder script "list") to
    several property panels;
  - duplicate/: right-click > Duplicate for Rerun and Duplicate Setup with
    Last Geometry on a calculation, with another project expanded;
  - prompts: Run Mgmt > Reset to Rerun (after the viewer phase's run) and
    New Calculation with the name prompt, and whether the Organizer stays
    the foreground window.
Needs the desktop (a scheduled task with /it, see desktask.ps1).

    python redraw_win.py <install tree> <state dir> <png dir> [phases]

phases: any of select,viewer,duplicate,prompts (default all but duplicate).
"""
import os
import shutil
import subprocess
import sys
import time

tree, state, pngs = (os.path.abspath(a).replace("\\", "/") for a in sys.argv[1:4])
PHASES = (sys.argv[4] if len(sys.argv) > 4 else "select,viewer,prompts").split(",")
BIN = tree + "/bin"
HERE = os.path.dirname(os.path.abspath(__file__))
sys.stdout.reconfigure(encoding="utf-8", errors="replace")

# A running screen saver (an idle test PC) makes every screen capture fail.
subprocess.run(["taskkill", "/F", "/IM", "scrnsave.scr"], stdout=subprocess.DEVNULL,
               stderr=subprocess.DEVNULL)
shutil.rmtree(state, ignore_errors=True)
shutil.rmtree(pngs, ignore_errors=True)
os.makedirs(pngs)
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


def say(t):
    print(t, flush=True)


def framelog(seconds, dump, name="", pid=0):
    """Starts framelog.ps1 in the background; .result() gives its lines."""
    path = os.path.join(out, "frames-%d.txt" % time.time_ns())
    args = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
            os.path.join(HERE, "framelog.ps1"), "-Seconds", str(seconds), "-Out", path,
            "-Dump", os.path.join(pngs, dump)]
    args += ["-Name", name] if name else ["-ProcId", str(pid)]
    p = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    class R:
        def result(self):
            p.wait(timeout=seconds + 300)
            return open(path, errors="replace").read().splitlines() if os.path.exists(path) else []
    return R()


def report(title, lines):
    say("%s: %d states seen" % (title, len(lines)))
    for l in lines:
        say("   " + l)


def app(name, script_var, script, args=(), timeout=200):
    path = os.path.join(state, name + ".script")
    with open(path, "w", newline="\n") as h:
        h.write(script)
    e = dict(env, ECCE_FEEDBACK_STDERR="1", **{script_var: path})
    log = os.path.join(out, name + ".log")
    with open(log, "w") as h:
        p = subprocess.Popen([BIN + "/" + name + ".exe"] + list(args), env=e, stdout=h,
                             stderr=subprocess.STDOUT)
        try:
            p.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            p.kill()
    return open(log, encoding="utf-8", errors="replace").read()


subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-gateway-start"], env=env,
               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
cmds = os.path.join(state, "org.cmds")
open(cmds, "w").close()
# The Viewer the Organizer starts inherits this script (CreateProcess, same
# environment): it steps through the property panels once it is up.
vscript = os.path.join(state, "viewer.script")
with open(vscript, "w", newline="\n") as h:
    h.write("wait 6000\n" + "".join("list %s\nwait 2500\n" % n for n in
            ("Mulliken Charges", "MOs", "MO Diagram", "Energies", "Dipole")) + "wait 1000\nquit\n")
orglog = open(os.path.join(out, "organizer.log"), "w")
org = subprocess.Popen([BIN + "/organizer.exe"],
                       env=dict(env, ECCE_TEST_ORGANIZER=cmds, ECCE_BUILDER_SCRIPT=vscript),
                       stdout=orglog, stderr=subprocess.STDOUT)


def ask(cmd, wait=120):
    with open(cmds, "a", newline="\n") as h:
        h.write(cmd + "\n")
    for _ in range(wait * 4):
        time.sleep(0.25)
        text = open(os.path.join(out, "organizer.log"), errors="replace").read()
        hits = [l for l in text.splitlines() if l.startswith("ECCE_TEST_ORGANIZER: %s: " % cmd)]
        if hits:
            return hits[-1].split(": ", 2)[2]
    return "no answer"


def build(url, script):
    return app("builder", "ECCE_BUILDER_SCRIPT", script + "save\nwait 4000\nquit\n",
               ("-context", url))


try:
    time.sleep(8)
    proj = "file://" + local + "/users/local/p"
    say("newproject: " + ask("newproject p"))
    calcs = []
    for i, mol in enumerate(("C Tetrahedral", "O Bent", "N Pyramidal", "C Tetrahedral")):
        name = "c%d" % i
        say("newcalc %s: %s" % (name, ask("newcalc p %s NWChem" % name)))
        url = proj + "/" + name
        build(url, "wait 4000\nadd %s 0 0 0\nwait 1500\ncmd addh\nwait 1500\n" % mol)
        calcs.append(url)

    if "select" in PHASES:
        ask("summary " + calcs[-1])
        time.sleep(2)
        fl = framelog(14, "organizer-select", pid=org.pid)
        time.sleep(1)
        for url in calcs:
            t = time.time()
            # "click" selects as a user's click does; older builds lack it.
            if ask("click " + url, wait=10) == "unknown command":
                ask("summary " + url)
            say("select %s: %.2f s" % (url.rsplit("/", 1)[1], time.time() - t))
            time.sleep(2)
        report("organizer select", fl.result())

    if "duplicate" in PHASES:
        # A second project stays expanded: it used to collapse.
        say("newproject q: " + ask("newproject q"))
        say("newcalc x: " + ask("newcalc q x NWChem"))
        ask("summary file://%s/users/local/q/x" % local)
        ask("summary " + calcs[0])
        time.sleep(2)
        for cmd in ("duplicate " + calcs[0], "duplicate " + calcs[0] + " last"):
            fl = framelog(8, "duplicate-" + str(len(cmd)), pid=org.pid)
            time.sleep(1)
            say("%s: %s" % (cmd.split()[0], ask(cmd)))
            report("organizer during " + cmd, fl.result())

    if "viewer" in PHASES:
        say("newcalc qm: " + ask("newcalc p w ECCE-QM"))
        qurl = proj + "/w"
        build(qurl, "wait 4000\nadd O Bent 0 0 0\nwait 1500\ncmd addh\nwait 1500\n")
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
        ask("summary " + qurl)
        # Out of the way of the Viewer, whose screen area is photographed.
        subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                        os.path.join(HERE, "movewin.ps1"), "-ProcId", str(org.pid),
                        "-X", "1180", "-Y", "560"])
        fl = framelog(26, "viewer-open", name="builder")
        time.sleep(1)
        say("start viewer: " + ask("start CalculationViewer " + qurl))
        report("viewer open and property switches", fl.result())

    if "prompts" in PHASES:
        # Rerun needs a finished calculation: the viewer phase's run.
        for cmd in ("rmtask %s/w Reset to Rerun" % proj, "newprompt %s NWChem" % proj):
            fl = framelog(8, "prompt-" + cmd.split()[0], pid=org.pid)
            time.sleep(1)
            say("%s: %s" % (cmd.split()[0], ask(cmd)))
            lines = fl.result()
            report("organizer during %s (%s)" % (cmd.split()[0], "background seen" if any(
                l.endswith(" bg") for l in lines[1:]) else "stayed foreground"), lines)
finally:
    org.kill()
    subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-broker-win", "stop"], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
say("done")
