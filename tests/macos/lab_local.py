#!/usr/bin/env python3
"""The first-year diatomics lab as a local client, through the apps' test hooks.

    lab_local.py win <install tree> <work dir> [cases]
    lab_local.py mac <ECCE.app>     <work dir> [cases]

cases (default all): C2 O2 He2 HF CO (Energy), CO-geo HF-geo, CO-geovib HF-geovib.
Library geometry from Teaching/Diatomics, ECCE-QM, DFT B3LYP, 6-31G*; O2 triplet.
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import time

plat, root, work = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3]).replace("\\", "/")
ALL = ["C2", "O2", "He2", "HF", "CO", "CO-geo", "HF-geo", "CO-geovib", "HF-geovib"]
CASES = sys.argv[4].split(",") if len(sys.argv) > 4 else ALL
WIN = plat == "win"
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
T0 = time.time()
HERE = os.path.dirname(os.path.abspath(__file__))
P = work + "/prof"
PNG = work + "/png"
OUT = work + "/out"
SC = work + "/scripts"
RUNS = work + "/runs"
for d in (PNG, OUT, SC, RUNS):
    os.makedirs(d, exist_ok=True)


def say(t):
    print("[%6.1f] %s" % (time.time() - T0, t), flush=True)


if WIN:
    tree = os.path.abspath(root).replace("\\", "/")
    BIN = tree + "/bin"
    HOME = tree
    TW = work + "/tw"
    LOCAL = P + "/ecce-local"
    sysdir = os.environ.get("SystemRoot", r"C:\Windows")
    base = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
    base["USERPROFILE"] = P.replace("/", "\\")
    env = dict(base)
    env.update({
        "ECCE_HOME": tree, "ECCE_ROOT": tree.replace("/", "\\"), "ECCE_REALUSERHOME": P,
        "ECCE_LOCAL_DATA": LOCAL, "ECCE_REALUSER": os.environ.get("USERNAME", "user"),
        "HOST": os.environ.get("COMPUTERNAME", "localhost"),
        "ECCE_NO_DATASERVER": "1", "ECCE_LOCAL_BROKER": "1", "ECCE_SESSION_LIVENESS": "lease",
        "ECCE_TMPDIR": os.environ.get("TEMP", P).replace("\\", "/"),
        "ECCE_SESSION_ID": os.urandom(8).hex(), "ECCE_NO_FIRST_START": "1",
        "PATH": os.pathsep.join([BIN, tree + "/scripts", tree + "/scripts/parsers", tree + "/usr/bin",
                                 tree + "/python", tree + "/strawberry/perl/site/bin",
                                 tree + "/strawberry/perl/bin", tree + "/strawberry/c/bin",
                                 sysdir + r"\System32", sysdir, os.environ.get("PATH", "")]),
    })

    def exe(name):
        return BIN + "/" + name + ".exe"
else:
    APP = os.path.abspath(root)
    A = APP + "/Contents/Resources"
    HOME = A + "/ecce"
    BIN = HOME + "/bin"
    LOCAL = work + "/data"
    env = dict(os.environ)
    env.update({
        "ECCE_HOME": HOME, "ECCE_REALUSERHOME": P, "ECCE_LOCAL_DATA": LOCAL,
        "ECCE_SESSION_LIVENESS": "lease", "ECCE_NO_REAP": "1", "ECCE_TRANSPARENCY_FALLBACK_MS": "0",
        "PATH": ":".join([A + "/python/bin", A + "/bin", HOME + "/libexec", "/usr/bin", "/bin", "/usr/sbin", "/sbin"]),
    })
    env.pop("ECCE_NO_FIRST_START", None)

    def exe(name):
        return A + "/bin/ecce-" + name

os.makedirs(P, exist_ok=True)


def ps(script, *args, timeout=120):
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                        TW + "/" + script] + [str(a) for a in args],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    return r.stdout.decode(errors="replace").strip()


def shotpid(pid, png):
    if WIN:
        ps("winshot.ps1", "-ProcId", pid, "-Png", PNG + "/" + png)
    else:
        subprocess.run(["screencapture", "-x", PNG + "/" + png], stderr=subprocess.DEVNULL)
    say("  shot %s%s" % (png, "" if os.path.exists(PNG + "/" + png) else " FAILED"))


def kill_app(names):
    for n in names:
        if WIN:
            subprocess.run(["taskkill", "/F", "/IM", n + ".exe"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            subprocess.run(["pkill", "-f", HOME + "/bin/" + n], stderr=subprocess.DEVNULL)
            subprocess.run(["pkill", "-f", r"^\./%s( |$)" % n], stderr=subprocess.DEVNULL)


def kill_all():
    if WIN:
        subprocess.run(["powershell", "-NoProfile", "-Command",
                        "Get-Process | ? { $_.Path -like '%s\\*' -and $_.Id -ne %d } | Stop-Process -Force"
                        % (tree.replace("/", "\\"), os.getpid())],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        subprocess.run(["pkill", "-f", HOME + "/bin/"], stderr=subprocess.DEVNULL)
        subprocess.run(["pkill", "-f", r"^\./(organizer|builder|calced|launcher|machregister|gateway|viewer)( |$)"],
                       stderr=subprocess.DEVNULL)
        subprocess.run(["pkill", "-f", P], stderr=subprocess.DEVNULL)


# ---- first start through the real launcher, fresh profile -------------------
def first_start():
    say("=== first start (fresh profile %s)" % P)
    e = dict(env, ECCE_FIRST_START_ANSWER="local", ECCE_FIRST_START_SHOT=PNG + "/01-first-start.png")
    e.pop("ECCE_LOCAL_DATA", None)
    e.pop("ECCE_NO_FIRST_START", None)
    e["ECCE_SESSION_ID"] = os.urandom(8).hex()
    if WIN:
        e = dict(base, ECCE_FIRST_START_ANSWER="local", ECCE_FIRST_START_SHOT=PNG + "/01-first-start.png")
        cmd = ["cmd", "/c", tree.replace("/", "\\") + "\\ecce.cmd"]
    else:
        e["ECCE_REALUSERHOME"] = P
        cmd = [APP + "/Contents/MacOS/ecce"]
    t = time.time()
    log = open(OUT + "/ecce-start.log", "w")
    p = subprocess.Popen(cmd, env=e, stdout=log, stderr=subprocess.STDOUT)
    up = 0
    for _ in range(120):
        r = subprocess.run((["tasklist"] if WIN else ["pgrep", "-fl", "organizer"]),
                           stdout=subprocess.PIPE, stderr=subprocess.DEVNULL).stdout.decode(errors="replace")
        if "organizer" in r.lower():
            up = time.time() - t
            break
        time.sleep(1)
    say("  organizer up %.1f s after ecce started (0 = never)" % up)
    time.sleep(12)
    if WIN:
        pid = subprocess.run(["powershell", "-NoProfile", "-Command",
                              "(Get-Process organizer -ErrorAction SilentlyContinue | Select -Last 1).Id"],
                             stdout=subprocess.PIPE).stdout.decode().strip()
        if pid:
            shotpid(int(pid), "01-organizer-first.png")
    else:
        shotpid(0, "01-organizer-first.png")
    ans = P + "/.ECCE/first-start-answer"
    say("  first-start-answer: " + (open(ans).read().strip() if os.path.exists(ans) else "(none)"))
    say("  ecce output: " + open(OUT + "/ecce-start.log", errors="replace").read().strip()[-300:])
    say("  profile: " + " ".join(sorted(os.listdir(P))))
    pref = P + "/.ECCE/EcceGlobal"
    say("  prefs: " + (" | ".join(l for l in open(pref).read().splitlines() if "ocal" in l) if os.path.exists(pref) else "(none)"))
    p.terminate() if not WIN else None
    kill_all()
    time.sleep(3)


first_start()

# ---- the session for the remaining steps ------------------------------------
if WIN:
    r = subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-gateway-start"], env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    say("session broker: rc=%d %s" % (r.returncode, r.stdout.decode(errors="replace").strip()[-200:]))
cmds = OUT + "/org.cmds"
open(cmds, "w").close()
ORGLOG = OUT + "/organizer.log"
orgenv = dict(env, ECCE_TEST_ORGANIZER=cmds, ECCE_FEEDBACK_STDERR="1")
org = subprocess.Popen([exe("organizer")], env=orgenv, stdout=open(ORGLOG, "a"), stderr=subprocess.STDOUT)
if not WIN:
    for _ in range(90):
        if re.search("starting mosquitto|broker already", open(ORGLOG, errors="replace").read()):
            break
        time.sleep(1)
    time.sleep(20)
else:
    time.sleep(12)


def ask(cmd, wait=120):
    with open(cmds, "a", newline="\n") as h:
        h.write(cmd + "\n")
    for _ in range(wait * 4):
        time.sleep(0.25)
        text = open(ORGLOG, errors="replace").read()
        hits = [l for l in text.splitlines() if l.startswith("ECCE_TEST_ORGANIZER: %s: " % cmd)]
        if hits:
            return hits[-1].split(": ", 2)[2]
    return "no answer"


def app(name, var, script, args=(), tag="", timeout=400, extra=None):
    tag = tag or name
    path = SC + "/" + tag + ".script"
    with open(path, "w", newline="\n") as h:
        h.write(script)
    e = dict(env, ECCE_FEEDBACK_STDERR="1", **(extra or {}))
    if var:
        e[var] = path
    log = OUT + "/" + tag + ".log"
    p = subprocess.Popen([exe(name)] + list(args), env=e, stdout=open(log, "w"), stderr=subprocess.STDOUT)
    t = time.time()
    while p.poll() is None and time.time() - t < timeout:
        time.sleep(0.3)
    if p.poll() is None:
        say("  %s did not finish in %d s: killed" % (tag, timeout))
        p.kill()
        kill_app([name])
    text = open(log, encoding="utf-8", errors="replace").read()
    for l in text.splitlines():
        if ("FAIL" in l or "not in the" in l or "rror" in l or l.startswith(("CALCED:", "BUILDER:"))) and \
                "CALCED: ES." not in l:
            say("  | " + l[:260])
    return text


def value(calc, key):
    f = calc + "/Props/" + key
    if not os.path.exists(f):
        return None
    m = re.findall(r">\s*([-+]?\d+\.\d+(?:[eE][-+]?\d+)?)\s*<", open(f, errors="replace").read())
    return [float(x) for x in m]


def spin_meta(calc, word):
    f = calc + "/.ecce-meta"
    key = "http://www.emsl.pnl.gov/ecce:spinmultiplicity"
    lines = open(f).read().splitlines() if os.path.exists(f) else []
    out, hit = [], False
    for l in lines:
        if key in l:
            l = ".\t%s\t\t%s" % (key, word)
            hit = True
        out.append(l)
    if not hit:
        out.append(".\t%s\t\t%s" % (key, word))
    open(f, "w", newline="\n").write("\n".join(out) + "\n")


results = []
refs = {}


def lab_case(case):
    mol, _, kind = case.partition("-")
    runtype = {"": "Energy", "geo": "Geometry", "geovib": "GeoVib"}[kind]
    row = {"case": case, "runtype": runtype, "result": "?", "energy": None, "secs": None}
    results.append(row)
    say("--- %s: %s" % (case, runtype))
    a = ask("newcalc lab %s ECCE-QM" % case)
    say("  newcalc: " + a)
    m = re.match(r"ok (\S+)", a)
    if not m:
        row["result"] = "newcalc failed: " + a
        return
    url = m.group(1)
    calc = url[len("file://"):]
    mvm = "%s/data/client/StructureLibrary/Teaching/Diatomics/%s.mvm" % (HOME, mol)
    app("builder", "ECCE_BUILDER_SCRIPT",
        "wait 5000\nlibrary Teaching/Diatomics %s\nwait 1500\ncmd open fileName %s streamType MVM\nwait 1500\n"
        "info\nexpect atoms %d\nsave\nwait 4000\nquit\n" % (mol, mvm, 1 if False else 2),
        ("-context", url), tag=case + ".builder")
    if mol == "O2":
        spin_meta(calc, "Triplet")
    steps = ["wait 3000", "wait 6000", "theory DFT", "runtype " + runtype, "basis 6-31G*", "wait 2000",
             "button save", "wait 4000"] + \
        (["button verify", "wait 3000"] if WIN else []) + ["info", "quit"]   # verify's modal result dialog blocks the hook's timer on macOS
    app("calced", "ECCE_CALCED_SCRIPT", "\n".join(steps) + "\n", ("-context", url), tag=case + ".calced", timeout=150)
    deck = glob.glob(calc + "/Inputs/*.qmin")
    if not deck:
        row["result"] = "no input deck stored"
        return
    dtext = open(deck[0]).read()
    say("  deck: " + " ".join(l for l in dtext.splitlines() if l.split(" ")[0] in
                              ("method", "basis", "multiplicity", "charge", "runtype", "orbitals")))
    row["deck"] = dtext
    rundir = RUNS + "/" + case
    os.makedirs(rundir, exist_ok=True)
    t0 = time.time()
    lenv = dict(env, ECCE_FEEDBACK_STDERR="1")
    lp = SC + "/" + case + ".launcher.script"
    open(lp, "w", newline="\n").write("wait 8000\nmachine localhost\nrundir %s\nwait 4000\nlaunch\nwait 5000\nquit\n" % rundir)
    lenv["ECCE_LAUNCHER_SCRIPT"] = lp
    lp_proc = subprocess.Popen([exe("launcher"), "-context", url], env=lenv,
                               stdout=open(OUT + "/" + case + ".launcher.log", "w"), stderr=subprocess.STDOUT)
    st, box, snapped = "", [], False
    clicked = False
    while time.time() - t0 < 900:
        st = ask("state " + url, wait=30)
        if st.lower().startswith(("complete", "failed", "killed", "unsuccess", "system")):
            break
        if st.lower().startswith(("running", "submitted")):
            if not clicked:
                ask("click " + url, wait=20)
                clicked = True
            b = ask("progressbox", wait=20)
            box.append(b)
            if not snapped and "shown=1" in b:
                ask("snap %s/%s-progress.png" % (PNG, case), wait=30)
                snapped = True
        time.sleep(0.5)
    row["secs"] = time.time() - t0
    lp_proc.poll() is None and lp_proc.kill()
    row["result"] = st
    say("  state %s after %.0f s (launch click to Complete, incl. Launcher start ~13 s)" % (st, row["secs"]))
    shown = [b for b in box if "shown=1" in b]
    row["progress"] = (shown[-1][:200] if shown else ("polled %d, never shown: %s" % (len(box), box[-1][:120] if box else "no running state seen")))
    say("  progress box: " + row["progress"])
    if not st.lower().startswith("complete"):
        say("  reason: " + ask("summary " + url, wait=30))
        return
    time.sleep(3)
    ask("click " + url, wait=20)
    row["summary"] = ask("summary " + url, wait=30)
    say("  summary: " + row["summary"][:300])
    ask("snap %s/%s-organizer-done.png" % (PNG, case), wait=30)
    props = sorted(os.listdir(calc + "/Props")) if os.path.isdir(calc + "/Props") else []
    say("  props: " + " ".join(props))
    te = value(calc, "TE")
    row["energy"] = te[0] if te else None
    say("  TE: %s" % (te,))
    row["props"] = props
    # orbital list
    r = subprocess.run([HOME + "/bin/mocomp" + (".exe" if WIN else ""), calc, "--list"], env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
    otext = r.stdout.decode(errors="replace")
    open(OUT + "/" + case + ".mocomp.txt", "w").write(otext)
    rows = [l.split("\t") for l in otext.splitlines() if l and l[0].isdigit()]
    row["norb"] = len(rows)
    if rows:
        say("  orbitals: %d; first %s ... occupied=%d; idx %s..%s" % (
            len(rows), rows[0][:3], sum(1 for x in rows if float(x[2]) > 0), rows[0][0], rows[-1][0]))
        row["orbrows"] = rows
    else:
        say("  mocomp --list: " + otext[:300])
    # Viewer: MOs pane list, no MO Diagram, HOMO picture (+ the other panes)
    metrics = OUT + "/" + case + ".panes.txt"
    if os.path.exists(metrics):
        os.remove(metrics)
    sc = SC + "/scene-" + case
    shutil.rmtree(sc, ignore_errors=True)
    os.makedirs(sc)
    open(sc + "/scene", "w", newline="\n").write("mopanel compute\nmogrid grid\nmotable tab\nsnap mo-homo\n")
    ve = dict(env, ECCE_VIEWER_SCENE=sc + "/scene", ECCE_VIEWER_SCENE_OUT=sc,
              ECCE_INVOKE_VIEWER="1", ECCE_INVOKE_FROMECCE="1")
    p = subprocess.Popen([exe("builder"), "-context", url], env=ve, stdout=open(OUT + "/" + case + ".viewer.log", "w"),
                         stderr=subprocess.STDOUT)
    try:
        p.wait(timeout=240)
    except subprocess.TimeoutExpired:
        p.kill()
        kill_app(["builder"])
    pe = dict(env, ECCE_OPEN_PANEL="MOs", ECCE_PANEL_METRICS=metrics, ECCE_INVOKE_VIEWER="1", ECCE_INVOKE_FROMECCE="1")
    p = subprocess.Popen([exe("builder"), "-context", url], env=pe, stdout=open(OUT + "/" + case + ".panes.log", "w"),
                         stderr=subprocess.STDOUT)
    for _ in range(120):
        if os.path.exists(metrics) and os.path.getsize(metrics) > 0 and time.time() > 0:
            time.sleep(3)
            break
        time.sleep(1)
    p.kill()
    kill_app(["builder"])
    grid = open(sc + "/grid.txt").read().split() if os.path.exists(sc + "/grid.txt") else []
    tab = open(sc + "/tab.txt").read() if os.path.exists(sc + "/tab.txt") else ""
    failed = open(sc + "/FAILED").read().strip() if os.path.exists(sc + "/FAILED") else ""
    panes = open(metrics).read() if os.path.exists(metrics) else ""
    names = re.findall(r'pane "([^"]+)"', panes)
    row["homo"] = "grid " + " ".join(grid) + (" FAILED " + failed if failed else "")
    row["panes"] = names
    row["modiagram"] = "MO Diagram" in names
    say("  viewer: grid=%s failed=%s modiagram_pane=%s panes=%s" % (" ".join(grid), failed or "no", row["modiagram"], names))
    say("  motable: " + " | ".join(tab.splitlines()[:6]))
    pics = [f for f in os.listdir(sc) if f.endswith((".ppm", ".png"))]
    row["pics"] = pics
    for f in pics:
        shutil.copy(sc + "/" + f, PNG + "/%s-%s" % (case, f))
    # LUMO: no hook selects it; its coefficients from mocomp instead
    if rows:
        homo = max(int(x[0]) for x in rows if float(x[2]) > 0)
        r = subprocess.run([HOME + "/bin/mocomp" + (".exe" if WIN else ""), calc, "--mo", str(homo + 1), "--by", "atom"],
                           env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
        row["lumo"] = r.stdout.decode(errors="replace").strip()[:400]
    if runtype in ("Geometry", "GeoVib"):
        outs = glob.glob(calc + "/Outputs/*.qmout") + glob.glob(rundir + "/**/*.qmout", recursive=True)
        otxt = open(outs[0], errors="replace").read() if outs else ""
        row["converged"] = "converged=yes" in otxt or "optimization converged" in otxt.lower() or "converged" in otxt
        conv = [l for l in otxt.splitlines() if "converged" in l.lower()][-2:]
        say("  convergence lines: " + " | ".join(conv))
        row["convlines"] = conv
        geo = value(calc, "GEOMTRACE")
        say("  GEOMTRACE: %s" % (geo[:8] if geo else None))
        secs = re.findall(r"begin atoms[^\n]*\n(.*?)\nend atoms", otxt, re.S)
        if secs:
            import math
            xyz = [[float(x) for x in l.split()[3:6]] for l in secs[-1].splitlines()]
            row["bond"] = math.dist(xyz[0], xyz[1])
            say("  optimised bond length %.4f A" % row["bond"])
    if runtype == "GeoVib":
        def vec(k):
            fn = calc + "/Props/" + k
            m = re.search(r">\s*([^<]+)</vector", open(fn).read()) if os.path.exists(fn) else None
            return [float(x) for x in m.group(1).split()] if m else []
        f = vec("VIBFREQ")
        row["freq"] = f
        say("  VIBFREQ: %s ; VIBIR: %s ; real modes above 1 cm-1: %d, imaginary: %d" % (
            f, vec("VIBIR"), sum(1 for x in f if x > 1), sum(1 for x in f if x < -1)))
        vs = SC + "/vib-" + case + ".script"
        vt = app("builder", "ECCE_BUILDER_SCRIPT",
                 "wait 9000\nlist Vibrational Frequencies\nwait 3000\noverlay\nquit\n",
                 ("-context", url), tag=case + ".vib",
                 extra={"ECCE_INVOKE_VIEWER": "1", "ECCE_INVOKE_FROMECCE": "1"})
        row["vibview"] = [l for l in vt.splitlines() if l.startswith("BUILDER: overlay") or "not in the" in l]
        say("  vib viewer: " + " || ".join(row["vibview"]))


try:
    say("probe: " + ask("probe"))
    ask("snap " + PNG + "/02-organizer-session.png")
    say("newproject: " + ask("newproject lab"))
    for c in CASES:
        try:
            lab_case(c)
        except Exception as ex:
            import traceback
            traceback.print_exc()
            results[-1]["result"] = "driver error: %s" % ex
finally:
    org.kill()
    if WIN:
        subprocess.run([tree + "/usr/bin/bash.exe", tree + "/bin/ecce-broker-win", "stop"], env=env,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    kill_all()

import json
json.dump(results, open(OUT + "/results.json", "w"), indent=1, default=str)
say("=== table")
for r in results:
    say("%-10s %-9s %-10s %s %s" % (r["case"], r["runtype"], r["result"],
                                    ("%.8f" % r["energy"]) if r.get("energy") is not None else "-",
                                    ("%.0f s" % r["secs"]) if r.get("secs") else ""))
say("done")
