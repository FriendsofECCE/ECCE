#!/usr/bin/env python3
"""qm_irc_offset: ecce-qm IRC of HCN <-> HNC (HF/STO-3G) from a start
structure a little off the saddle point, as a TS taken from another program
or level is (#764).  The H atom is moved 0.05 angstrom along C-N either way;
the two branches must still end on opposite sides: one with H bonded to C
(HCN), the other with H bonded to N (HNC).  Before the fix both branches ran
into the same valley.

    irc_offset_test.py ECCE_QM_EXE
"""
import json, math, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
BONDED, APART = 1.25, 1.45   # angstrom: an X-H bond, and clearly no bond


def run(exe, dx):
    xyz = [list(a) for a in json.load(open(os.path.join(HERE, "irc_oracle.json")))["sto3g"]["ts_xyz"]]
    c, n = xyz[0][1:], xyz[1][1:]
    d = [n[i] - c[i] for i in range(3)]
    L = math.sqrt(sum(v * v for v in d))
    for i in range(3):
        xyz[2][1 + i] += dx * d[i] / L
    inp = ("runtype irc\nmethod hf\nbasis STO-3G\nirc_step 0.1\nirc_max_points 12\ngeometry\n" +
           "".join("%s %.8f %.8f %.8f\n" % tuple(a) for a in xyz) + "end\n")
    with tempfile.TemporaryDirectory() as work:
        path = os.path.join(work, "irc.qm")
        open(path, "w").write(inp)
        p = subprocess.run([exe, path], capture_output=True, text=True)
    if p.returncode != 0:
        return ["dx %+.2f: exit status %d: %s" % (dx, p.returncode, p.stderr[-300:])]
    out = p.stdout
    geoms = [[[float(v) for v in l.split()[2:5]] for l in b.splitlines()[1:-1]]
             for b in re.findall(r"^begin irc_geometry.*?^end irc_geometry", out, re.S | re.M)]
    fails = []
    for lab, g in (("backward", geoms[0]), ("forward", geoms[-1])):
        ch, nh = math.dist(g[0], g[2]), math.dist(g[1], g[2])
        print("  dx %+.2f %-8s C-H %.3f N-H %.3f" % (dx, lab, ch, nh))
        # forward = H moves to N (the mode's sign convention, as in irc_test.py)
        want_n = (lab == "forward") == (math.dist(geoms[-1][1], geoms[-1][2]) < math.dist(geoms[0][1], geoms[0][2]))
        x_h, other = (nh, ch) if want_n else (ch, nh)
        if not (x_h < BONDED and other > APART):
            fails.append("dx %+.2f %s end: C-H %.3f N-H %.3f is not %s" % (dx, lab, ch, nh, "HNC" if want_n else "HCN"))
    ch0, nh0 = math.dist(geoms[0][0], geoms[0][2]), math.dist(geoms[0][1], geoms[0][2])
    ch1, nh1 = math.dist(geoms[-1][0], geoms[-1][2]), math.dist(geoms[-1][1], geoms[-1][2])
    if (ch0 < nh0) == (ch1 < nh1):
        fails.append("dx %+.2f: both ends have H on the same atom" % dx)
    return fails


def main():
    exe = sys.argv[1]
    fails = run(exe, 0.05) + run(exe, -0.05)
    for f in fails:
        print("FAIL", f)
    print("qm_irc_offset:", "FAILED" if fails else "passed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
