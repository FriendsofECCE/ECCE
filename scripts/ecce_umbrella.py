#!/usr/bin/env python3
"""ecce-umbrella (#762): umbrella sampling from a GROMACS pull run, and WHAM.

Wx-free; works on stored GROMACS calculations (or plain run directories) and
talks to no ECCE session.  The Builder's Umbrella Sampling panel draws what
this prints with --json.

  ecce-umbrella windows <pull calc> [--spacing 0.2] [--from D] [--to D]
                     frames of the pull trajectory at the chosen spacing of
                     the pull coordinate (frame, time, distance)
  ecce-umbrella extract <pull calc> --out DIR [same choices]
                     writes window_NN.gro (the whole system, box included)
  ecce-umbrella wham <window calc>... [--bins 200] [--temp 300]
                     [--zero end|min|none] [--jacobian]
                     gmx wham on the windows' topol.tpr and pullf.xvg: the PMF
                     (kJ/mol) and the histograms, with overlap warnings

A calculation's files are looked for in its Outputs folder, then in the folder
itself.  The pull coordinate comes from pullx.xvg/pullf.xvg, else from the
stored properties PULLTIME, PULLDIST and PULLFORCE.

gmx is --gmx, else $ECCE_GROMACS, else gmx or gmx_mpi on PATH.
"""
import argparse
import csv
import importlib.machinery
import importlib.util
import io
import json
import math
import os
import re
import shutil
import sys
import tempfile
import xml.etree.ElementTree as ET

_here = os.path.dirname(os.path.abspath(__file__))


def _load(module, installed):
    sys.path.insert(0, _here)
    try:
        return __import__(module)
    except ImportError:
        l = importlib.machinery.SourceFileLoader(module, os.path.join(_here, installed))
        s = importlib.util.spec_from_loader(module, l)
        m = importlib.util.module_from_spec(s)
        l.exec_module(m)
        return m
    finally:
        sys.path.pop(0)


GA = _load("ecce_gmxanalysis", "ecce-gmxanalysis")
Failure = GA.Failure

R_KJ = 8.314462618e-3
# Neighbouring windows whose normalised histograms share less than this are
# reported: the WHAM profile across such a gap is not trustworthy.
MIN_OVERLAP = 0.05


# --- pull data ---------------------------------------------------------------

def _prop_vector(calc, name):
    try:
        t = ET.parse(os.path.join(calc, "Props", name)).getroot().text
    except (OSError, ET.ParseError):
        return None
    return [float(v) for v in t.split()] if t else None


def pull_series(calc):
    """-> (times ps, coordinate nm, force kJ/mol/nm or None)."""
    px = GA.find_file(calc, "pullx.xvg")
    if px:
        c = GA.read_xvg(px)[0]
        pf = GA.find_file(calc, "pullf.xvg")
        f = GA.read_xvg(pf)[0][1] if pf else None
        return c[0], c[1], f
    t, x = _prop_vector(calc, "PULLTIME"), _prop_vector(calc, "PULLDIST")
    if not t or not x:
        raise Failure("%s has no pull data (pullx.xvg or the PULLDIST property): "
                      "is it a pulling run?" % calc)
    return t, x, _prop_vector(calc, "PULLFORCE")


def interp(ts, xs, t):
    if t <= ts[0]:
        return xs[0]
    if t >= ts[-1]:
        return xs[-1]
    lo, hi = 0, len(ts) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if ts[mid] <= t:
            lo = mid
        else:
            hi = mid
    w = (t - ts[lo]) / (ts[hi] - ts[lo]) if ts[hi] > ts[lo] else 0.0
    return xs[lo] + w * (xs[hi] - xs[lo])


def frame_times(gmx, calc):
    """The times of the frames in traj.xtc, from gmx check."""
    xtc = GA.find_file(calc, "traj.xtc")
    if not xtc:
        raise Failure("%s has no trajectory (traj.xtc): tick Write trajectory "
                      "in the pull run's settings" % calc)
    work = tempfile.mkdtemp(prefix="ecce-umbrella-")
    try:
        out, err = GA.Gmx(gmx, work, 1).run(["check", "-f", xtc])
    finally:
        shutil.rmtree(work, ignore_errors=True)
    text = out + err
    m = re.search(r"Coords\s+(\d+)\s+(\S+)", text)
    n = int(m.group(1)) if m else 0
    dt = float(m.group(2)) if m else 0.0
    m0 = re.search(r"Reading frame\s+0\s+time\s+(\S+)", text)
    t0 = float(m0.group(1)) if m0 else 0.0
    if n < 1:
        raise Failure("gmx check found no frames in %s" % xtc)
    return xtc, [t0 + i * dt for i in range(n)]


def pick_windows(times, dists, spacing, lo=None, hi=None):
    """One frame per target lo, lo+spacing, ...: the frame nearest each target.

    Targets the pull never reached are left out; a frame serves one target.
    """
    if spacing <= 0:
        raise Failure("the spacing must be positive")
    dmin, dmax = min(dists), max(dists)
    lo = dmin if lo is None else max(lo, dmin)
    hi = dmax if hi is None else min(hi, dmax)
    picks, used = [], set()
    target = lo
    while target <= hi + 1e-9:
        best = min(range(len(dists)), key=lambda i: abs(dists[i] - target))
        if best not in used and abs(dists[best] - target) <= spacing / 2:
            used.add(best)
            picks.append({"frame": best, "time": times[best],
                          "distance": dists[best], "target": target})
        target += spacing
    return picks


def do_windows(a, gmx):
    calc = GA.calc_dir(a.calc)
    t, x, _ = pull_series(calc)
    xtc, ft = frame_times(gmx, calc)
    fd = [interp(t, x, v) for v in ft]
    picks = pick_windows(ft, fd, a.spacing, a.lo, a.hi)
    return {"analysis": "windows", "calc": calc, "trajectory": xtc,
            "frames": len(ft), "spacing": a.spacing, "windows": picks,
            "pull": {"time": t, "distance": x}}


def do_extract(a, gmx):
    res = do_windows(a, gmx)
    calc = res["calc"]
    tpr = GA.find_file(calc, "topol.tpr")
    if not tpr:
        raise Failure("%s has no topol.tpr" % calc)
    os.makedirs(a.out, exist_ok=True)
    g = GA.Gmx(gmx, a.out, 1)
    for i, w in enumerate(res["windows"]):
        name = "window_%02d.gro" % (i + 1)
        # 0 = System: the window starts from the whole box, solvent included
        g.run(["trjconv", "-s", tpr, "-f", res["trajectory"], "-dump",
               "%g" % w["time"], "-o", name], stdin="0\n")
        w["file"] = os.path.join(os.path.abspath(a.out), name)
    res["analysis"] = "extract"
    return res


# --- WHAM ----------------------------------------------------------------------

def _write_pullf(calc, path):
    """pullf.xvg from the stored PULLTIME/PULLFORCE, when the file was not kept."""
    t, f = _prop_vector(calc, "PULLTIME"), _prop_vector(calc, "PULLFORCE")
    if not t or not f or len(t) != len(f):
        return False
    with open(path, "w") as h:
        h.write('@    title "Pull force"\n@    xaxis  label "Time (ps)"\n'
                '@    yaxis  label "Force (kJ/mol/nm)"\n@TYPE xy\n@ s0 legend "1"\n')
        for a, b in zip(t, f):
            h.write("%.4f\t%.6g\n" % (a, b))
    return True


def overlaps(hx, hists):
    """Normalised overlap of each window's histogram with the next one's (by mean)."""
    def mean(h):
        s = sum(h)
        return sum(x * v for x, v in zip(hx, h)) / s if s > 0 else 0.0
    order = sorted(range(len(hists)), key=lambda i: mean(hists[i]))
    out = []
    for i, j in zip(order, order[1:]):
        si, sj = sum(hists[i]) or 1.0, sum(hists[j]) or 1.0
        ov = sum(min(a / si, b / sj) for a, b in zip(hists[i], hists[j]))
        out.append({"from": i + 1, "to": j + 1, "at": 0.5 * (mean(hists[i]) + mean(hists[j])),
                    "overlap": ov})
    return out


def minima_and_barriers(x, y):
    """Local minima and maxima of the PMF, smoothed over 5 bins against noise."""
    n = len(y)
    s = [sum(y[max(0, i - 2):i + 3]) / len(y[max(0, i - 2):i + 3]) for i in range(n)]
    mins, maxs = [], []
    for i in range(3, n - 3):
        if s[i] < min(s[i - 3:i]) and s[i] <= min(s[i + 1:i + 4]):
            mins.append({"distance": x[i], "pmf": y[i]})
        if s[i] > max(s[i - 3:i]) and s[i] >= max(s[i + 1:i + 4]):
            maxs.append({"distance": x[i], "pmf": y[i]})
    return mins, maxs


def do_wham(a, gmx):
    calcs = [GA.calc_dir(c) for c in a.calcs]
    if len(calcs) < 2:
        raise Failure("WHAM needs at least two windows")
    work = tempfile.mkdtemp(prefix="ecce-wham-")
    try:
        tprs, pfs = [], []
        for i, c in enumerate(calcs):
            tpr = GA.find_file(c, "topol.tpr")
            if not tpr:
                raise Failure("%s has no topol.tpr: has the window run?" % c)
            pf = GA.find_file(c, "pullf.xvg")
            if not pf:
                pf = os.path.join(work, "pullf_%03d.xvg" % i)
                if not _write_pullf(c, pf):
                    raise Failure("%s has no pull force (pullf.xvg or PULLFORCE)" % c)
            tprs.append(tpr)
            pfs.append(pf)
        with open(os.path.join(work, "tpr.dat"), "w") as h:
            h.write("\n".join(tprs) + "\n")
        with open(os.path.join(work, "pullf.dat"), "w") as h:
            h.write("\n".join(pfs) + "\n")
        args = ["wham", "-it", "tpr.dat", "-if", "pullf.dat", "-o", "profile.xvg",
                "-hist", "histo.xvg", "-unit", "kJ", "-bins", str(a.bins),
                "-temp", "%g" % a.temp]
        if a.begin:
            args += ["-b", "%g" % a.begin]
        g = GA.Gmx(gmx, work, a.threads)
        g.run(args)
        prof = GA.read_xvg(os.path.join(work, "profile.xvg"))[0]
        hist = GA.read_xvg(os.path.join(work, "histo.xvg"))[0]
    finally:
        shutil.rmtree(work, ignore_errors=True)
    x, y = prof[0], prof[1]
    if a.jacobian:
        # distance geometry: remove the 4 pi r^2 volume term, W(r) = A(r) + 2RT ln r
        y = [v + 2 * R_KJ * a.temp * math.log(r) if r > 0 else v for r, v in zip(x, y)]
    finite = [v for v in y if math.isfinite(v)]
    if a.zero == "end" and finite:
        ref = [v for v in y[-max(1, len(y) // 20):] if math.isfinite(v)]
        z = sum(ref) / len(ref) if ref else 0.0
        y = [v - z for v in y]
    elif a.zero == "min" and finite:
        z = min(finite)
        y = [v - z for v in y]
    mins, maxs = minima_and_barriers(x, y)
    ov = overlaps(hist[0], hist[1:])
    warnings = ["windows %d and %d barely overlap near %.3f nm (%.0f %%): add a window there"
                % (o["from"], o["to"], o["at"], 100 * o["overlap"])
                for o in ov if o["overlap"] < MIN_OVERLAP]
    fy = [v for v in y if math.isfinite(v)]
    return {"analysis": "wham", "windows": calcs, "temperature": a.temp,
            "jacobian": a.jacobian, "zero": a.zero,
            "pmf": {"distance": x, "pmf": y, "unit": "kJ/mol"},
            "histograms": {"distance": hist[0], "counts": hist[1:]},
            "overlap": ov, "minima": mins, "maxima": maxs,
            "depth": (max(fy) - min(fy)) if fy else None,
            "warnings": warnings, "commands": [" ".join(c) for c in g.commands]}


# --- reports -------------------------------------------------------------------

def text_report(res):
    out = io.StringIO()
    if res["analysis"] in ("windows", "extract"):
        out.write("%d windows from %d frames, spacing %g nm\n"
                  % (len(res["windows"]), res["frames"], res["spacing"]))
        out.write("%6s %8s %10s %10s\n" % ("window", "frame", "time/ps", "dist/nm"))
        for i, w in enumerate(res["windows"]):
            out.write("%6d %8d %10.2f %10.4f%s\n" % (i + 1, w["frame"], w["time"],
                      w["distance"], ("  " + w["file"]) if "file" in w else ""))
    else:
        out.write("PMF from %d windows at %g K%s\n" % (len(res["windows"]), res["temperature"],
                  ", with the 2RT ln r term" if res["jacobian"] else ""))
        for m in res["minima"]:
            out.write("minimum  %.3f nm  %8.2f kJ/mol\n" % (m["distance"], m["pmf"]))
        for m in res["maxima"]:
            out.write("maximum  %.3f nm  %8.2f kJ/mol\n" % (m["distance"], m["pmf"]))
        if res["depth"] is not None:
            out.write("highest - lowest: %.2f kJ/mol\n" % res["depth"])
        for w in res["warnings"]:
            out.write("warning: %s\n" % w)
    return out.getvalue()


def csv_report(res):
    out = io.StringIO()
    w = csv.writer(out)
    if res["analysis"] == "wham":
        n = len(res["histograms"]["counts"])
        w.writerow(["distance_nm", "pmf_kJ_per_mol"] + ["hist_%d" % (i + 1) for i in range(n)])
        hx = res["histograms"]["distance"]
        for i, (x, y) in enumerate(zip(res["pmf"]["distance"], res["pmf"]["pmf"])):
            row = [x, y]
            if i < len(hx):
                row += [c[i] for c in res["histograms"]["counts"]]
            w.writerow(row)
    else:
        w.writerow(["window", "frame", "time_ps", "distance_nm"])
        for i, p in enumerate(res["windows"]):
            w.writerow([i + 1, p["frame"], p["time"], p["distance"]])
    return out.getvalue()


def main(argv=None):
    p = argparse.ArgumentParser(prog="ecce-umbrella", description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--gmx")
    p.add_argument("--json", action="store_true")
    p.add_argument("--csv", action="store_true")
    sub = p.add_subparsers(dest="cmd", required=True)
    for name in ("windows", "extract"):
        s = sub.add_parser(name)
        s.add_argument("calc")
        s.add_argument("--spacing", type=float, default=0.2, help="nm (0.2)")
        s.add_argument("--from", dest="lo", type=float, help="first distance, nm")
        s.add_argument("--to", dest="hi", type=float, help="last distance, nm")
        if name == "extract":
            s.add_argument("--out", required=True)
    s = sub.add_parser("wham")
    s.add_argument("calcs", nargs="+")
    s.add_argument("--bins", type=int, default=200)
    s.add_argument("--temp", type=float, default=300.0)
    s.add_argument("--begin", type=float, default=0.0, help="ps of each window to skip")
    s.add_argument("--zero", choices=("end", "min", "none"), default="end")
    s.add_argument("--jacobian", action="store_true")
    s.add_argument("--threads", type=int, default=1)
    a = p.parse_args(argv)
    try:
        gmx = GA.find_gmx(a.gmx)
        res = {"windows": do_windows, "extract": do_extract, "wham": do_wham}[a.cmd](a, gmx)
    except Failure as e:
        if a.json:
            print(json.dumps({"error": str(e)}))
        else:
            sys.stderr.write("error: %s\n" % e)
        return 1
    if a.json:
        print(json.dumps(res, allow_nan=False, default=lambda v: None)
              if all(math.isfinite(v) for v in res.get("pmf", {}).get("pmf", [0]))
              else json.dumps(res).replace("Infinity", "null").replace("NaN", "null"))
    elif a.csv:
        sys.stdout.write(csv_report(res))
    else:
        sys.stdout.write(text_report(res))
    return 0


if __name__ == "__main__":
    sys.exit(main())
