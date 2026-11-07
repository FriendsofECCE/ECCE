#!/usr/bin/env python3
"""The MD Analysis panel and ecce-gmxanalysis on real GROMACS runs (#877).

Two short runs through the real Launch, gensub, job monitor and job store:
NaCl in SPC/E water (the General Chemistry example, 12079 atoms, 1000 steps
with pressure coupling and nstvout, 4 threads) and liquid argon at 85 K
(500 steps, 2 threads).  The job script is given no mdanalysis
(ECCE_MDANALYSIS), so the speed and MSD data must come from the job store.

  * traj.xtc, ener.edr, topol.tpr, ecce_pp.top and traj.trr are stored with
    the calculation; GEOMTRACE has a LATTICEVEC (the box) per frame;
  * MDMSD and MDSPEED are stored although the host skipped mdanalysis;
  * ecce-gmxanalysis rdf (Na+ around water O) equals gmx rdf run directly,
    energy equals gmx energy (series and averages); rms, gyrate, density,
    hbond and groups give data; a file left on the host is named as such;
  * the argon g(r) has its first peak at 0.37-0.38 nm;
  * the Builder's MD Analysis panel (script commands gmxa ...) draws the
    numbers the command prints, for g(r) and for an energy term.

    tests/launch/gromacs_md_panel_test.py [--build build-cmake] [--keep] [--png DIR]

Exit status 77 (CTest SKIP) without gmx, Xvfb or the built binaries.
"""
import argparse
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

EX = os.path.join(REPO, "data", "client", "examples", "genchem", "gases-liquids-solids")
NACL = os.path.join(EX, "gromacs-nacl-dissolving", "1-dynamics")
AR = os.path.join(EX, "gromacs-ar-liquid", "1-dynamics")
CLI = os.path.join(REPO, "scripts", "ecce_gmxanalysis.py")

NACL_MDP = """integrator = md
dt = 0.002
nsteps = 1000
cutoff-scheme = Verlet
coulombtype = PME
rcoulomb = 1.0
vdwtype = Cut-off
vdw-modifier = Potential-shift
rvdw = 1.0
DispCorr = EnerPres
pbc = xyz
constraint-algorithm = lincs
tcoupl = v-rescale
tc-grps = System
tau-t = 1.0
ref-t = 300
pcoupl = C-rescale
pcoupltype = isotropic
tau-p = 1.0
ref-p = 1.0
compressibility = 4.5e-5
gen-vel = no
continuation = yes
nstcalcenergy = 50
nstenergy = 50
nstlog = 500
nstxout-compressed = 50
nstvout = 100
"""
AR_MDP = """integrator = md
dt = 0.002
nsteps = 500
cutoff-scheme = Verlet
nstlist = 20
coulombtype = Cut-off
rcoulomb = 1.2
vdwtype = Cut-off
vdw-modifier = Potential-shift
rvdw = 1.2
DispCorr = EnerPres
pbc = xyz
tcoupl = v-rescale
tc-grps = System
tau-t = 1.0
ref-t = 85
pcoupl = no
gen-vel = yes
gen-temp = 85
gen-seed = 877
nstcalcenergy = 50
nstenergy = 50
nstlog = 100
nstxout-compressed = 10
"""
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def where(u):
    return u[len("file://"):].rstrip("/")


def steps(task, key):
    for path in glob.glob(os.path.join(task, "Props", key + "*")):
        if "/.DAV/" not in path:
            return len(re.findall(r"<step\b", open(path, errors="replace").read()))
    return 0


def first_step(task, key):
    for path in glob.glob(os.path.join(task, "Props", key + "*")):
        if "/.DAV/" not in path:
            m = re.search(r"<step[^>]*>([^<]*)</step>", open(path, errors="replace").read())
            return [float(v) for v in m.group(1).split()] if m else []
    return []


def records(task, key):
    for path in glob.glob(os.path.join(task, "Props", key + "*")):
        if "/.DAV/" in path:
            continue
        text = open(path, errors="replace").read()
        body = text.split(">", 2)[-1].rsplit("</vecstring>", 1)[0]
        return body.split()
    return None


def waitProps(task, keys, seconds=120):
    end = time.time() + seconds
    while time.time() < end:
        if all(records(task, k) is not None for k in keys):
            return True
        time.sleep(1)
    return False


def cli(env, *args):
    r = subprocess.run([sys.executable, CLI] + list(args) + ["--json"], env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=300)
    try:
        return r.returncode, json.loads(r.stdout.decode().strip().splitlines()[-1])
    except Exception:
        return r.returncode, {"error": r.stdout.decode() + r.stderr.decode()}


def xvg(path):
    cols = []
    for line in open(path):
        if line[:1] in "#@" or not line.strip():
            continue
        for i, v in enumerate(line.split()):
            if len(cols) <= i:
                cols.append([])
            cols[i].append(float(v))
    return cols


def same(a, b, tol=1e-9):
    return len(a) == len(b) and all(abs(x - y) <= tol * max(1.0, abs(x)) for x, y in zip(a, b))


def runTask(s, name, top, gro, itps, mdp, procs):
    rc, out = s.driver("gromacsstudy", s.userUrl(), name)
    urls = [l for l in out.splitlines() if l.startswith("file://")]
    if not check(rc == 0 and len(urls) == 4, "%s: a GROMACS study was created" % name):
        return None
    dyn = urls[3].rstrip("/") + "/"
    rundir = os.path.join(s.state, "run-" + name)
    check(s.driver("mdsetup", dyn, "localhost", rundir, s.user(), "queue=normal",
                   "procs=%d" % procs)[0] == 0, "%s: launch settings stored" % name)
    ok = s.driver("mdfile", dyn, top, "topol.gmxtop")[0] == 0 and \
        s.driver("mdfile", dyn, gro, "start.gro")[0] == 0
    for itp in itps:
        ok = ok and s.driver("mdfile", dyn, itp, os.path.basename(itp))[0] == 0
    check(ok, "%s: topology and structure attached" % name)
    check(s.driver("mdmdp", dyn)[0] == 0, "%s: .mdp written" % name)
    with open(os.path.join(where(dyn), "Inputs", "gromacs.mdp"), "w") as h:
        h.write(mdp)
    t0 = time.time()
    check(s.launch(dyn)[0] == 0, "%s: Dynamics submitted" % name)
    state = s.waitState(dyn, 400)
    check(state in ("completed", "loaded"), "%s: Dynamics ran to %s (%.0f s)"
          % (name, state, time.time() - t0))
    return dyn


def stop(proc):
    if proc is not None and proc.poll() is None:
        try:
            os.killpg(proc.pid, 15)
            proc.wait(timeout=20)
        except Exception:
            pass


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--png", help="copy the panel screenshots here")
    ap.add_argument("--procs", type=int, default=4, help="threads for the NaCl run (at most 4)")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in ("mdanalysis", "builder"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import builder_overlay_test as bo
    import xdisplay

    s = harness.Session(build, "gmxmdpanel", {"GROMACS": gmx}, local=True, keep=args.keep)
    for exe in ("mdanalysis",) + bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    harness.link(CLI, os.path.join(s.home, "bin", "ecce-gmxanalysis"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    env = s.env({"OMP_NUM_THREADS": "4"})
    display = None
    try:
        # The job script's mdanalysis fails (an installed ecce-mdanalysis on
        # PATH would otherwise stand in); the job store must make the data.
        broken = os.path.join(s.state, "broken-mda")
        os.makedirs(broken, exist_ok=True)
        with open(os.path.join(broken, "ecce-mdanalysis"), "w") as h:
            h.write("#!/bin/sh\necho 'mdanalysis: not usable on this host' >&2\nexit 1\n")
        os.chmod(os.path.join(broken, "ecce-mdanalysis"), 0o755)
        s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
        with open(os.path.join(s.state, ".ECCE", "CONFIG.localhost"), "a") as h:
            h.write("gromacs_setup {\nsetenv ECCE_MDANALYSIS /nonexistent/mdanalysis\n"
                    "setenv PATH %s:$PATH\n}\n" % broken)
        if not check(s.services(True), "services came up"):
            return 1
        nacl = runTask(s, "nacl", os.path.join(NACL, "topol.top"),
                       os.path.join(NACL, "confout.gro"),
                       [os.path.join(NACL, "salt.itp")], NACL_MDP, min(4, args.procs))
        ar = runTask(s, "argon", os.path.join(AR, "topol.top"),
                     os.path.join(AR, "confout.gro"), [], AR_MDP, min(2, args.procs))
        if not nacl or not ar:
            return 1
        d, a = where(nacl), where(ar)

        # --- step 2: files, box per frame, client-side mdanalysis ---------
        out = os.path.join(d, "Outputs")
        have = sorted(f for f in ("traj.xtc", "ener.edr", "topol.tpr", "ecce_pp.top",
                                  "traj.trr") if os.path.isfile(os.path.join(out, f)))
        check(len(have) == 5, "stored with the NaCl run: %s" % have)
        ng, nl = steps(d, "GEOMTRACE"), steps(d, "LATTICEVEC")
        check(ng > 1 and ng == nl, "a box per frame: GEOMTRACE %d, LATTICEVEC %d" % (ng, nl))
        cell = first_step(d, "LATTICEVEC")
        check(len(cell) == 9 and abs(cell[0] - 49.49) < 1.0 and abs(cell[4] - cell[0]) < 1e-3,
              "the first frame's box is the cubic 49.5 Angstrom cell: %s" % cell[:3])
        log = open(os.path.join(out, "gromacs.gmxlog"), errors="replace").read() \
            if os.path.exists(os.path.join(out, "gromacs.gmxlog")) else ""
        check("MD analysis (displacement) failed" in log and
              "ecce-parse-md.txt written" not in log,
              "the job script's mdanalysis failed, as on a host without ECCE")
        check(waitProps(d, ("MDMSD", "MDSPEED")), "MDMSD and MDSPEED stored by the job store")
        sp = records(d, "MDSPEED") or []
        check(sp and sp != ["none"], "speeds from the fetched traj.trr: %s"
              % [x[:20] for x in sp])

        # --- step 3: the command against gmx --------------------------------
        work = tempfile.mkdtemp(prefix="gmxpanel-", dir=s.state)
        tpr, xtc, edr = (os.path.join(out, f) for f in ("topol.tpr", "traj.xtc", "ener.edr"))
        subprocess.run([gmx, "rdf", "-s", tpr, "-f", xtc, "-ref", "resname NA",
                        "-sel", "name OW", "-bin", "0.002", "-rmax", "1.0", "-o", "rdf.xvg",
                        "-cn", "cn.xvg"], cwd=work, env=env, capture_output=True)
        g_ref, cn_ref = xvg(os.path.join(work, "rdf.xvg")), xvg(os.path.join(work, "cn.xvg"))
        rc, rdf = cli(env, d, "rdf", "--ref", "residue:NA", "--sel", "residue:SOL & element:O",
                      "--rmax", "1.0")
        ok = rc == 0 and "series" in rdf
        check(ok, "ecce-gmxanalysis rdf ran: %s" % rdf.get("error", ""))
        if ok:
            check(same(rdf["x"]["values"], g_ref[0]) and same(rdf["series"][0]["values"], g_ref[1])
                  and same(rdf["series"][1]["values"], cn_ref[1]),
                  "rdf: r, g(r) and cn(r) equal gmx rdf's (%d points)" % len(g_ref[0]))
            summ = dict((x["name"], x.get("value")) for x in rdf["summary"])
            peak = summ.get("first peak position") or 0
            cnum = summ.get("coordination number (to the first minimum)") or 0
            check(0.22 < peak < 0.26 and 4.5 < cnum < 7,
                  "Na-O first peak %.3f nm, coordination number %.2f" % (peak, cnum))
        terms = ["Potential", "Temperature", "Pressure", "Density"]
        r = subprocess.run([gmx, "energy", "-f", edr, "-o", "e.xvg"], cwd=work, env=env,
                           input=("\n".join(terms) + "\n\n").encode(), capture_output=True)
        e_ref = xvg(os.path.join(work, "e.xvg"))
        avg_ref = {}
        for line in r.stdout.decode().splitlines():
            f = line.split()
            if f and f[0] in terms:
                avg_ref[f[0]] = float(f[1])
        rc, en = cli(env, d, "energy")
        check(rc == 0 and all(t in en.get("terms", []) for t in terms),
              "energy lists the terms of ener.edr (%d)" % len(en.get("terms", [])))
        rc, en = cli(env, d, "energy", "--terms", ",".join(terms))
        ok = rc == 0 and len(en.get("series", [])) == 4
        check(ok, "ecce-gmxanalysis energy ran: %s" % en.get("error", ""))
        if ok:
            check(same(en["x"]["values"], e_ref[0]) and
                  all(same(en["series"][k]["values"], e_ref[k + 1]) for k in range(4)),
                  "energy: the four terms vs time equal gmx energy's (%d points)" % len(e_ref[0]))
            check(all(abs(x["average"] - avg_ref[x["name"]]) < 1e-9 * max(1, abs(avg_ref[x["name"]]))
                      for x in en["summary"]) and len(avg_ref) == 4,
                  "energy: averages equal gmx energy's %s" % avg_ref)
            check(en["series"][3]["unit"] == "kg m⁻³", "density unit %s" % en["series"][3]["unit"])
        for an, extra in (("rms", ["--group", "residue:CL"]), ("gyrate", ["--group", "residue:NA"]),
                          ("density", ["--group", "element:Na", "--axis", "z"]),
                          ("hbond", ["--ref", "residue:SOL"]), ("groups", [])):
            rc, res = cli(env, d, an, *extra)
            check(rc == 0 and ("series" in res or an == "groups"),
                  "%s ran: %s" % (an, res.get("error") or
                                  [x.get("name") for x in res.get("summary", [])][:2]))
        g = res.get("groups", {})
        check(sorted(e["name"] for e in g.get("elements", [])) == ["Cl", "H", "Na", "O"],
              "groups: the elements of the run %s" % g.get("elements"))
        fake = os.path.join(s.state, "fake-calc")
        os.makedirs(os.path.join(fake, "Outputs"), exist_ok=True)
        shutil.copy(edr, os.path.join(fake, "Outputs"))
        shutil.copy(tpr, os.path.join(fake, "Outputs"))
        with open(os.path.join(fake, "Outputs", "ecce.submit.log"), "w") as h:
            h.write("traj.xtc was left in the run directory: it is larger than the 500 MB "
                    "fetch limit. Use ecce-fetch to fetch it later.\n")
        rc, res = cli(env, fake, "rdf", "--ref", "all", "--sel", "all")
        check(rc == 2 and "500 MB fetch limit" in res.get("error", ""),
              "a trajectory left on the host is named as such: %s" % res.get("error"))

        rc, arr = cli(env, a, "rdf", "--ref", "element:Ar", "--sel", "element:Ar")
        summ = dict((x["name"], x.get("value")) for x in arr.get("summary", []))
        peak = summ.get("first peak position") or 0
        cnum = summ.get("coordination number (to the first minimum)") or 0
        check(rc == 0 and 0.365 <= peak <= 0.385 and 11 <= cnum <= 14,
              "liquid argon at 85 K: first g(r) peak at %.3f nm, %.1f neighbours" % (peak, cnum))

        # --- the panel ------------------------------------------------------
        display = xdisplay.Display(screen="1400x1050x24").__enter__()
        benv = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        png = os.path.join(s.state, "png")
        os.makedirs(png, exist_ok=True)
        dumps = dict((k, os.path.join(s.state, "panel-%s.txt" % k)) for k in ("rdf", "energy"))
        text, shots = bo.runScript(
            s, display, benv, wrappers, ["-context", nacl],
            ["wait 2000", "gmxa wait", "gmxa state",
             "gmxa analysis rdf", "gmxa set ref residue:NA",
             "gmxa set sel residue:SOL & element:O", "gmxa set rmax 1.0",
             "gmxa run", "gmxa dump %s" % dumps["rdf"], "gmxa state", "shot rdf",
             "gmxa analysis energy", "gmxa wait", "gmxa set terms Potential",
             "gmxa run", "gmxa dump %s" % dumps["energy"], "gmxa state", "shot energy",
             "quit"], png, "gmxa",
            {"ECCE_PANEL_MODE": "classic", "ECCE_OPEN_PANEL": "MD Analysis",
             "ECCE_OPEN_PANEL_SIZE": "760x860"})
        bad = [l for l in text.splitlines()
               if l.startswith("ECCE_BUILDER_SCRIPT: gmxa") and not l.endswith(": ok")]
        check(not bad, "the panel's script commands: %s" % (bad or "all ok"))
        states = [l for l in text.splitlines() if l.startswith("BUILDER: gmxa ")]
        check(states and "residue:NA" in states[0] and "element:O" in states[0],
              "the group pickers list the topology: %s" % (states[:1] or text[-400:]))

        def panelCurves(path):
            if not os.path.exists(path):
                return {}
            out = {}
            for line in open(path).read().splitlines():
                if line.startswith("#") or "|" not in line:
                    continue
                name, pts = line.split("|", 1)
                xy = [p.split() for p in pts.split(";") if p]
                out[name] = ([float(p[0]) for p in xy], [float(p[1]) for p in xy])
            return out
        pr = panelCurves(dumps["rdf"])
        check("g(r)" in pr and rdf.get("series") and same(pr["g(r)"][0], rdf["x"]["values"], 1e-8)
              and same(pr["g(r)"][1], rdf["series"][0]["values"], 1e-8),
              "the panel draws the g(r) ecce-gmxanalysis prints (%d points)"
              % len(pr.get("g(r)", ([], []))[0]))
        pe = panelCurves(dumps["energy"])
        check("Potential" in pe and en.get("series")
              and same(pe["Potential"][1], en["series"][0]["values"], 1e-8),
              "the panel draws the Potential ecce-gmxanalysis prints")
        check(len(shots) == 2, "panel screenshots: %s" % sorted(shots))
        if args.png:
            os.makedirs(args.png, exist_ok=True)
            for k, p in shots.items():
                shutil.copy(p, os.path.join(args.png, "md-analysis-%s.png" % k))
    finally:
        if display is not None:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: %s" % failures if failures else "OK")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
