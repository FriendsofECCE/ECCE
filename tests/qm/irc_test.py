#!/usr/bin/env python3
"""qm_irc: ecce-qm IRC (runtype irc) of HCN <-> HNC from the ORCA 6.1.1
OptTS geometry, against ORCA's IRC (irc_oracle.json, #452 stage 2).

    irc_test.py ECCE_QM_EXE [--quick] [--table]

Compared with ORCA: the energy and structure at both ends of the path
(HCN and HNC; 3e-4 Eh, 0.02 angstrom, 2 degrees) and the energy profile
against the path length s (3e-3 Eh at ORCA's points, limited by the chord-sum s of its frames).  Independent of the
oracle: the energy falls monotonically away from the TS, and ecceqm.parse
turns the output into the properties the IRC panel reads.
"""
import json, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ts_cases import CASES, internals

PARSE = os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts", "parsers", "ecceqm.parse")
TOL_E, TOL_R, TOL_A, TOL_PROFILE = 3e-4, 0.02, 2.0, 3e-3


def qm_input(case, oracle):
    return ("runtype irc\nmethod hf\nbasis %s\nconv_energy 1e-10\nconv_grad 1e-7\nirc_step 0.1\nirc_max_points 90\n"
            % case["basis"] + "geometry\n" + "".join("%s %.8f %.8f %.8f\n" % tuple(a) for a in oracle["ts_xyz"]) + "end\n")


def blocks(out, name):
    return re.findall(r"^(begin %s\b.*?^end %s)\n" % (name, name), out, re.S | re.M)


def parse_block(text):
    return subprocess.run(["perl", PARSE], input=text, capture_output=True, text=True).stdout


def interp(xs, ys, x):
    for i in range(1, len(xs)):
        if xs[i - 1] <= x <= xs[i]:
            t = (x - xs[i - 1]) / (xs[i] - xs[i - 1])
            return ys[i - 1] + t * (ys[i] - ys[i - 1])
    return None


def check(exe, name, table):
    case, oracle = CASES[name], json.load(open(os.path.join(HERE, "irc_oracle.json")))[name]
    with tempfile.TemporaryDirectory() as work:
        inp = os.path.join(work, "irc.qm")
        open(inp, "w").write(qm_input(case, oracle))
        p = subprocess.run([exe, inp], capture_output=True, text=True)
    out, fails = p.stdout, []
    if p.returncode != 0:
        return ["exit status %d: %s" % (p.returncode, p.stderr[-300:])]
    rows = [[float(v) for v in l.split()[1:3]] for l in blocks(out, "irc_table")[-1].splitlines()[1:-1]]
    geoms = [[[float(v) for v in l.split()[2:5]] for l in b.splitlines()[1:-1]] for b in blocks(out, "irc_geometry")]
    if len(geoms) != len(rows):
        return ["%d structures for %d table rows" % (len(geoms), len(rows))]
    s, e = [r[0] for r in rows], [r[1] for r in rows]
    t = s.index(0.0)
    if abs(e[t] - oracle["ts_energy"]) > 1e-6:
        fails.append("TS energy %.8f against ORCA %.8f" % (e[t], oracle["ts_energy"]))
    # direction 1 = the end with the lower energy (HCN), whichever sign s has
    ends = {"HCN": (0, oracle["backward"]), "HNC": (len(rows) - 1, oracle["forward"])}
    if e[0] > e[-1]:
        ends = {"HNC": (0, oracle["forward"]), "HCN": (len(rows) - 1, oracle["backward"])}
    for lab, (i, ref) in ends.items():
        x = internals(geoms[i])
        # CN, CH, NH distances and the HCN angle; the H-C-N angle is not
        # defined (about 180 or 0) at the ends, so it is compared as is.
        if abs(e[i] - ref["end_energy"]) > TOL_E:
            fails.append("%s energy %.6f against ORCA %.6f" % (lab, e[i], ref["end_energy"]))
        for l2, a, b, tol in zip(["C-N", "C-H", "N-H", "H-C-N"], x, ref["end_internals"], [TOL_R] * 3 + [TOL_A]):
            if abs(a - b) > tol:
                fails.append("%s %s %.4f against ORCA %.4f" % (lab, l2, a, b))
        if table:
            print("  %s ecce-qm E %.6f s %.3f %s" % (lab, e[i], s[i], [round(v, 4) for v in x]))
            print("  %s ORCA    E %.6f s %.3f %s" % (lab, ref["end_energy"], ref["s"][-1], [round(v, 4) for v in ref["end_internals"]]))
    for lab, ref in (("forward", oracle["forward"]), ("backward", oracle["backward"])):
        sign = 1 if (ref is oracle["forward"]) == (e[0] < e[-1]) else -1
        worst = 0.0
        for si, ei in zip(ref["s"], ref["energy"]):
            v = interp(s, e, sign * si)
            if v is not None:
                worst = max(worst, abs(v - ei))
        if table:
            print("  %-8s profile: largest difference to ORCA %.2e Eh" % (lab, worst))
        if worst > TOL_PROFILE:
            fails.append("%s profile differs from ORCA by %.2e Eh" % (lab, worst))
    for part in (e[:t + 1][::-1], e[t:]):
        if any(b > a + 1e-9 for a, b in zip(part, part[1:])):
            fails.append("energy rises along the path")
    if not re.search(r"^progress irc point=\d+/\d+", out, re.M):
        fails.append("no progress line")
    pt = parse_block(blocks(out, "irc_table")[-1])
    if not re.search(r"key: IRCCOORD\nsize:\n%d\n" % len(rows), pt) or "key: IRCENERGY" not in pt or "sqrt(amu)*bohr" not in pt:
        fails.append("ecceqm.parse gives no IRCCOORD/IRCENERGY for the table")
    pg = parse_block(blocks(out, "irc_geometry")[0])
    if not re.search(r"key: IRCGEOM\nsize:\n1 3 3\n", pg):
        fails.append("ecceqm.parse gives no IRCGEOM for a structure")
    return fails


def main():
    args = sys.argv[1:]
    exe, quick, table = args[0], "--quick" in args, "--table" in args
    bad = 0
    oracle = json.load(open(os.path.join(HERE, "irc_oracle.json")))
    for name, case in CASES.items():
        if name not in oracle or (quick and not case["quick"]):
            continue
        fails = check(exe, name, table)
        print("%-22s %s" % (name, "ok" if not fails else "FAIL"))
        for f in fails:
            print("   ", f)
        bad += bool(fails)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
