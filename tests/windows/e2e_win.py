#!/usr/bin/env python3
"""Water single points end to end on Windows through the apps' own test
hooks, no clicks (the Windows twin of tests/macos/e2e.sh, #133):
the Organizer makes the calculation, the Builder draws water, Register
Machines finds the codes, the Calculation Editor sets up (its Theory and
Runtype dialogs must start) and Verifies, the Launcher runs it on localhost,
the job monitor stores the results, the Builder opens the MOs; then the
same input is run directly with the code and the energies compared.
Run on a desktop session from an installed tree:

    python e2e_win.py <install tree> <state dir> [mopac] [ecceqm] [orca]
"""
import os
import re
import shutil
import subprocess
import sys
import time

tree, state = (os.path.abspath(a).replace("\\", "/") for a in sys.argv[1:3])
cases = sys.argv[3:] or ["mopac", "ecceqm"]
BIN = tree + "/bin"
failures = []
summary = []


def say(t):
    print(t, flush=True)
    summary.append(t)


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


shutil.rmtree(state, ignore_errors=True)
home, local, out = state + "/home", state + "/local", state + "/out"
for d in (home + "/.ECCE", local + "/users/local", out, state + "/runs", state + "/direct"):
    os.makedirs(d)
sysdir = os.environ.get("SystemRoot", r"C:\Windows")
userpath = os.environ.get("PATH", "")
env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
env.update({
    "ECCE_HOME": tree, "ECCE_REALUSERHOME": home, "ECCE_LOCAL_DATA": local,
    "ECCE_REALUSER": os.environ.get("USERNAME", "user"),
    "HOST": os.environ.get("COMPUTERNAME", "localhost"),
    "ECCE_NO_DATASERVER": "1", "ECCE_SESSION_LIVENESS": "lease",
    "ECCE_SESSION_ID": os.urandom(8).hex(),
    # as ecce.cmd: the package first, then the user's own PATH
    "PATH": os.pathsep.join([BIN, tree + "/usr/bin", tree + "/python",
                             tree + "/strawberry/perl/site/bin", tree + "/strawberry/perl/bin",
                             tree + "/strawberry/c/bin", sysdir + r"\System32", sysdir, userpath]),
})


def app(name, script_var, script, args=(), log=None, timeout=200, extra=None):
    path = os.path.join(state, name + ".script")
    with open(path, "w", newline="\n") as h:
        h.write(script)
    e = dict(env, ECCE_FEEDBACK_STDERR="1", **(extra or {}))
    if script_var:
        e[script_var] = path
    log = log or os.path.join(out, name + ".log")
    with open(log, "w") as h:
        try:
            rc = subprocess.run([BIN + "/" + name + ".exe"] + list(args), env=e, stdout=h,
                                stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            rc = "timeout"
    return rc, open(log, encoding="utf-8", errors="replace").read()


rc = subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-gateway-start"], env=env,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
say("broker: " + rc.stdout.decode(errors="replace").strip())
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


time.sleep(10)
check(ask("newproject e2e").startswith("ok"), "project e2e made")

# Register Machines: Find for each code, as a user fills the Codes tab.
finds = "".join("code %s\nclick code:find\n" % c for c in ("MOPAC", "ORCA") if c.lower() in cases)
rc, text = app("machregister", "ECCE_MACHREG_SCRIPT",
               "wait 3000\nselect localhost\ntab codes\n%sdump\nsave\nwait 2000\nquit\n" % finds)
cfg = home + "/.ECCE/CONFIG.localhost"
cfgtext = open(cfg).read() if os.path.exists(cfg) else ""
say("CONFIG.localhost: " + " | ".join(l for l in cfgtext.splitlines() if ":" in l))
for c, key in (("mopac", "MOPAC:"), ("orca", "ORCA:")):
    if c in cases:
        check(key in cfgtext, "Register Machines' Find filled %s" % key)

TYPES = {"mopac": ("MOPAC", None, None), "ecceqm": ("ECCE-QM", None, None),
         "orca": ("ORCA", "RHF", "def2-svp")}
for c in cases:
    ctype, theory, basis = TYPES[c]
    name = "water-" + c
    say("===== %s (%s)" % (c, ctype))
    a = ask("newcalc e2e %s %s" % (name, ctype))
    if not check(a.startswith("ok"), "calculation made (%s)" % a):
        continue
    d = local + "/users/local/e2e/" + name
    url = "file://" + d
    rc, text = app("builder", "ECCE_BUILDER_SCRIPT",
                   "wait 4000\nadd O Bent 0 0 0\nwait 1500\ncmd addh\nwait 1500\ninfo\n"
                   "expect atoms 3\nsave\nwait 4000\nquit\n", ("-context", url),
                   log=os.path.join(out, c + ".builder.log"))
    check("expect atoms 3: ok" in text, "the Builder drew water")
    steps = ["wait 3000", "wait 6000" if c == "ecceqm" else "ready"]
    if theory:
        steps.append("theory " + theory)
    if basis:
        steps.append("basis " + basis)
    steps += ["wait 1500", "button save", "wait 5000", "button verify", "wait 3000",
              "info", "quit"]
    rc, text = app("calced", "ECCE_CALCED_SCRIPT", "\n".join(steps) + "\n", ("-context", url),
                   log=os.path.join(out, c + ".calced.log"))
    if c != "ecceqm":
        check("ready: ok" in text, "the Theory and Runtype dialogs started (ready)")
    lamp = [l for l in text.splitlines() if l.startswith("CALCED: verify=")]
    say("  verify lamp: " + (lamp[-1] if lamp else "none"))
    for l in text.splitlines():
        if l.startswith("FEEDBACK: ") and l.strip() != "FEEDBACK:":
            say("  " + l)
    check(bool(lamp) and "unchecked" not in lamp[-1] and "\u2715" not in lamp[-1],
          "Verify ran and found no error")
    inputs = os.listdir(d + "/Inputs") if os.path.isdir(d + "/Inputs") else []
    check(bool(inputs), "input stored: %s" % " ".join(inputs))
    run = state + "/runs/" + name
    os.makedirs(run)
    rc, text = app("launcher", "ECCE_LAUNCHER_SCRIPT",
                   "wait 3000\nmachine localhost\nrundir %s\nwait 1500\nlaunch\nwait 5000\nquit\n" % run,
                   ("-context", url), log=os.path.join(out, c + ".launcher.log"))
    check("FAIL" not in text, "the Launcher launched")
    st = ""
    for _ in range(200):
        st = ask("state " + url, wait=30)
        if st.lower().startswith(("complete", "failed", "killed", "unsuccess", "system")):
            break
        time.sleep(3)
    check(st.lower().startswith("complete"), "the run completed (%s)" % st)
    props = os.listdir(d + "/Props") if os.path.isdir(d + "/Props") else []
    say("  Props: " + " ".join(sorted(props)))
    te = ""
    if "TE" in props:
        m = re.search(r">\s*([-+0-9.eE]+)\s*</value", open(d + "/Props/TE", errors="replace").read())
        te = m.group(1) if m else ""
    say("  TE stored: " + te)
    # the same input, run directly with the code
    dd = state + "/direct/" + c
    os.makedirs(dd)
    for f in inputs:
        shutil.copy(d + "/Inputs/" + f, dd)
    direct = ""
    if c == "mopac":
        mop = [f for f in inputs if f.endswith(".mop")]
        exe = shutil.which("mopac", path=env["PATH"])
        if mop and exe:
            subprocess.run([exe, mop[0]], cwd=dd, env=env, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
            o = open(os.path.join(dd, mop[0][:-4] + ".out"), errors="replace").read()
            m = re.search(r"FINAL HEAT OF FORMATION =\s*([-0-9.]+)", o)
            direct = m.group(1) if m else ""
            so = open(d + "/Outputs/" + [f for f in os.listdir(d + "/Outputs") if f.endswith(("out", "mopout"))][0], errors="replace").read() if os.path.isdir(d + "/Outputs") else ""
            m2 = re.search(r"FINAL HEAT OF FORMATION =\s*([-0-9.]+)", so)
            check(m2 is not None and direct and abs(float(m2.group(1)) - float(direct)) < 1e-4,
                  "heat of formation %s (ECCE run) = %s (direct)" % (m2.group(1) if m2 else "?", direct))
    elif c == "ecceqm":
        qm = [f for f in inputs if f.endswith(".qmin")]
        if qm:
            r = subprocess.run([BIN + "/ecce-qm.exe", qm[0]], cwd=dd, env=dict(env,
                               ECCE_BASIS_DIR=tree + "/data/admin/basissets", OMP_NUM_THREADS="1"),
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            o = r.stdout.decode(errors="replace")
            m = re.findall(r"energy_total\s+([-0-9.eE+]+)", o)
            direct = m[-1] if m else ""
            check(te and direct and abs(float(te) - float(direct)) < 1e-6,
                  "total energy %s (ECCE run) = %s (direct)" % (te, direct))
    elif c == "orca":
        oi = [f for f in inputs if f.endswith(".orcain")]
        exe = shutil.which("orca", path=env["PATH"])
        if oi and exe:
            r = subprocess.run([exe, oi[0]], cwd=dd, env=env, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
            m = re.findall(r"FINAL SINGLE POINT ENERGY\s+([-0-9.]+)", r.stdout.decode(errors="replace"))
            direct = m[-1] if m else ""
            check(te and direct and abs(float(te) - float(direct)) < 1e-6,
                  "total energy %s (ECCE run) = %s (direct)" % (te, direct))
    # the Builder opens the MOs
    metrics = os.path.join(out, c + ".panes.txt")
    p = subprocess.Popen([BIN + "/builder.exe", "-context", url],
                         env=dict(env, ECCE_OPEN_PANEL="MOs", ECCE_PANEL_METRICS=metrics),
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(60):
        time.sleep(1)
        if os.path.exists(metrics) and '"MOs"' in open(metrics, errors="replace").read():
            break
    p.kill()
    pane = [l for l in open(metrics, errors="replace").read().splitlines() if '"MOs"' in l] \
        if os.path.exists(metrics) else []
    if c != "mopac" or "MO" in props:
        check(bool(pane), "the Builder lists the MOs pane: %s" % (pane[0] if pane else "no"))

org.kill()
subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-broker-win", "stop"], env=env)
say("")
say("FAILED: " + "; ".join(failures) if failures else "PASSED")
sys.exit(1 if failures else 0)
