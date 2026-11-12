#!/usr/bin/env python3
"""Umbrella sampling end to end (#762): pull, windows, WHAM, on a Na+ Cl- pair.

A Na+ Cl- pair in TIP3P water (AMBER99SB-ILDN ions) is minimised, pulled
apart through the real md.gmxtask / launchpp / gensub job script, windows are
picked and cut from the pull trajectory by ecce-umbrella, each window is
equilibrated and sampled with the pull rate set to 0 and the reference fixed
at the window's target distance (GmxPullInit, as the window tasks made from
a pull run have), and ecce-umbrella wham makes the PMF.  The reference is the known shape of the NaCl PMF in
water (e.g. Smith & Dang, J. Chem. Phys. 100, 3757 (1994); Fennell et al.,
J. Phys. Chem. B 113, 6782 (2009)): a contact minimum near 0.27-0.29 nm, a
barrier near 0.35-0.38 nm, a solvent-separated minimum near 0.49-0.53 nm.

    tests/gromacs_md/umbrella_test.py [-v] [--quick]

--quick runs short windows (a few minutes; positions only checked loosely).
NTHREADS (default 4) threads per run.  KEEP=1 keeps the run folders and the
WHAM JSON (printed).  Exit 77 (CTest SKIP) without gmx.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402
from run_tests import check, run  # noqa: E402

UMB = os.path.join(rt.REPO, "scripts", "ecce_umbrella.py")
NT = int(os.environ.get("NTHREADS", "4"))
QUICK = "--quick" in sys.argv
BOX = 3.2          # nm: the pull reaches 1.3 nm, under half the box
SPACING = 0.04     # nm: an ion pair's minima are 0.1 nm wide
K = 6000           # kJ/mol/nm^2 for the windows (sigma about 0.02 nm)
EQ_STEPS = 2500 if not QUICK else 1000       # 5 ps / 2 ps
PROD_STEPS = 50000 if not QUICK else 5000    # 100 ps / 10 ps


def tag(name, value):
    return "    <%s>%s</%s>\n" % (name, str(value).replace("\n", "&#10;"), name)


def pull_inter(rate, k, init=None):
    return (tag("GmxGroups", "Na = NA\nCl = CL") + tag("GmxPull", 1)
            + tag("GmxPullGroup1", "Na") + tag("GmxPullGroup2", "Cl")
            + tag("GmxPullRate", rate) + tag("GmxPullK", k)
            + (tag("GmxPullInit", "%.4f" % init) if init is not None else ""))


def files_traj(every):
    return ("    <WriteTrajectory> 1</WriteTrajectory>\n"
            "    <CoordinatesSoluteFreq> %d</CoordinatesSoluteFreq>\n"
            "    <CoordinatesSolutePrint> 1</CoordinatesSolutePrint>\n" % every)


def build(gmx, tmp):
    top = os.path.join(tmp, "topol.top")
    with open(top, "w") as h:
        h.write('#include "amber99sb-ildn.ff/forcefield.itp"\n'
                '#include "amber99sb-ildn.ff/tip3p.itp"\n'
                '#include "amber99sb-ildn.ff/ions.itp"\n\n'
                "[ system ]\nNaCl in water\n\n[ molecules ]\nNA 1\nCL 1\n")
    gro = os.path.join(tmp, "pair.gro")
    c = BOX / 2
    with open(gro, "w") as h:
        h.write("NaCl\n2\n    1NA      NA    1%8.3f%8.3f%8.3f\n"
                "    2CL      CL    2%8.3f%8.3f%8.3f\n%10.5f%10.5f%10.5f\n"
                % (c - 0.14, c, c, c + 0.14, c, c, BOX, BOX, BOX))
    rc, out = run([gmx, "solvate", "-cp", gro, "-cs", "spc216.gro", "-p", top,
                   "-o", os.path.join(tmp, "solv.gro")], cwd=tmp)
    check(rc == 0, "gmx solvate filled the box")
    return os.path.join(tmp, "solv.gro"), top


def umbrella(*args):
    rc, out = run([sys.executable, UMB, "--json"] + list(args))
    try:
        return json.loads(out)
    except ValueError:
        print(out)
        return {"error": out}


def main():
    rt.verbose = "-v" in sys.argv
    gmx = rt.find_gmx()
    if gmx is None:
        print("SKIP: gmx is not installed")
        return 77
    base = os.environ.get("UMBRELLA_TMP") or os.path.expanduser("~/.cache")
    os.makedirs(base, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="gmx-umbrella-", dir=base)
    try:
        home = rt.Home(tmp, gmx)
        start, top = build(gmx, tmp)
        em = rt.stage(gmx, home, tmp, "em", "ecceMdOptimize", dict(rt.STAGES[0][2]),
                      start, nprocs=NT, topology=top)
        if em is None:
            return 1
        # held at contact (fixed reference 0.28 nm): left free, the pair can
        # drift apart while the water settles and the pull then starts at
        # 0.5 nm or more and leaves no windows at the contact minimum
        eq = rt.stage(gmx, home, tmp, "npt", "ecceMdEquilibrate",
                      dict(interaction=pull_inter(0, K, 0.28),
                           dynamics=rt.dyn(5000, False, npt=True)), em["gro"],
                      nprocs=NT, topology=top)
        if eq is None:
            return 1
        # 0.01 nm/ps for 100 ps: from contact to about 1.3 nm
        pull = rt.stage(gmx, home, tmp, "pull", "ecceMdDynamics",
                        dict(interaction=pull_inter(0.01, K),
                             dynamics=rt.dyn(50000, True, npt=True), files=files_traj(250)),
                        eq["gro"], nprocs=NT, topology=top)
        if pull is None:
            return 1

        win = umbrella("extract", pull["rundir"], "--spacing", str(SPACING),
                       "--from", "0.24", "--to", "1.2", "--out", os.path.join(tmp, "windows"))
        ws = win.get("windows", [])
        print("  %d windows: %s" % (len(ws), " ".join("%.3f" % w["distance"] for w in ws)))
        check(len(ws) >= 20, "at least 20 windows between 0.24 and 1.2 nm (%d)" % len(ws))
        gaps = [b["distance"] - a["distance"] for a, b in zip(ws, ws[1:])]
        check(gaps and max(gaps) < 2 * SPACING, "no gap of twice the spacing (largest %.3f)"
              % (max(gaps) if gaps else 0))

        prods = []
        for i, w in enumerate(ws):
            inter = pull_inter(0, K, w["target"])
            e = rt.stage(gmx, home, tmp, "w%02d-eq" % (i + 1), "ecceMdEquilibrate",
                         dict(interaction=inter, dynamics=rt.dyn(EQ_STEPS, False, npt=True)),
                         w["file"], nprocs=NT, topology=top)
            if e is None:
                return 1
            p = rt.stage(gmx, home, tmp, "w%02d" % (i + 1), "ecceMdDynamics",
                         dict(interaction=inter, dynamics=rt.dyn(PROD_STEPS, True, npt=True)),
                         e["gro"], nprocs=NT, topology=top)
            if p is None:
                return 1
            prods.append(p["rundir"])

        res = {}
        for jac in (False, True):
            args = ["wham"] + prods + ["--temp", "300", "--bins", "100"]
            res[jac] = umbrella(*(args + (["--jacobian"] if jac else [])))
        r = res[True]
        check("error" not in r, "ecce-umbrella wham ran (%s)" % r.get("error", "ok"))
        if "error" in r:
            return 1
        x, y = r["pmf"]["distance"], r["pmf"]["pmf"]
        mins = r["minima"]
        print("  minima (with 2RT ln r): " + ", ".join("%.3f nm %.2f" % (m["distance"], m["pmf"])
                                                    for m in mins))
        print("  maxima: " + ", ".join("%.3f nm %.2f" % (m["distance"], m["pmf"])
                                      for m in r["maxima"]))
        print("  minima (gmx wham as is): " + ", ".join(
            "%.3f nm %.2f" % (m["distance"], m["pmf"]) for m in res[False]["minima"]))
        for wmsg in r["warnings"]:
            print("  " + wmsg)
        if not QUICK:
            check(not r["warnings"], "every pair of neighbouring windows overlaps by 5 %% or more"
                  " (%d warnings)" % len(r["warnings"]))
        contact = [m for m in mins if 0.24 <= m["distance"] <= 0.33]
        ssm = [m for m in mins if 0.44 <= m["distance"] <= 0.60]
        check(contact, "a contact minimum between 0.24 and 0.33 nm")
        if not QUICK:
            check(ssm, "a solvent-separated minimum between 0.44 and 0.60 nm")
            bar = [m for m in r["maxima"] if 0.32 <= m["distance"] <= 0.45]
            check(bar, "a barrier between them (0.32-0.45 nm)")
        check(len(r["histograms"]["counts"]) == len(prods), "one histogram per window")
        if os.environ.get("KEEP"):
            out = os.path.join(tmp, "wham.json")
            with open(out, "w") as h:
                json.dump({"jacobian": res[True], "plain": res[False],
                           "windows": ws}, h)
            print("  WHAM result in", out)
    finally:
        if os.environ.get("KEEP"):
            print("kept", tmp)
        else:
            shutil.rmtree(tmp, ignore_errors=True)
    print("FAILED" if rt.failures else "OK")
    return 1 if rt.failures else 0


if __name__ == "__main__":
    sys.exit(main())
