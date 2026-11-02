#!/usr/bin/env python3
"""qm_irc_sn2: ecce-qm IRC of Cl- + CH3Br (HF/3-21G) from a start 0.1-0.2
angstrom off the saddle point, as a TS from another program or level is
(#764).  The branches must end in different minima: Cl-...CH3Br (C-Br bonded,
C-Cl far) and ClCH3...Br- (the reverse).  Before the fix both ended in the
Cl-...CH3Br valley.

    irc_sn2_test.py ECCE_QM_EXE
"""
import math, re, subprocess, sys, tempfile, os

# C-Cl 2.42, C-Br 2.47 (the HF/3-21G saddle has 2.31 and 2.61)
START = """C 0 0 -0.028
Cl 0 0 2.392
Br 0 0 -2.501
H 1.07 0 -0.028
H -0.535 0.927 -0.028
H -0.535 -0.927 -0.028
"""
BONDED, APART = 2.25, 2.9   # angstrom


def main():
    inp = ("runtype irc\nmethod hf\nbasis 3-21G\ncharge -1\nirc_step 0.2\nirc_max_points 40\ngeometry\n" + START + "end\n")
    with tempfile.TemporaryDirectory() as work:
        path = os.path.join(work, "irc.qm")
        open(path, "w").write(inp)
        p = subprocess.run([sys.argv[1], path], capture_output=True, text=True)
    if p.returncode != 0:
        print("exit status %d: %s" % (p.returncode, p.stderr[-300:]))
        return 1
    geoms = [[[float(v) for v in l.split()[2:5]] for l in b.splitlines()[1:-1]]
             for b in re.findall(r"^begin irc_geometry.*?^end irc_geometry", p.stdout, re.S | re.M)]
    ends = []
    for lab, g in (("first", geoms[0]), ("last", geoms[-1])):
        ccl, cbr = math.dist(g[0], g[1]), math.dist(g[0], g[2])
        print("  %-5s end: C-Cl %.3f C-Br %.3f" % (lab, ccl, cbr))
        ends.append((ccl, cbr))
    (a, b), (c, d) = ends
    reactant = lambda ccl, cbr: cbr < BONDED and ccl > APART   # Cl- ... CH3Br
    product = lambda ccl, cbr: ccl < BONDED and cbr > APART    # ClCH3 ... Br-
    if not ((reactant(a, b) and product(c, d)) or (product(a, b) and reactant(c, d))):
        print("FAIL: the two ends are not Cl-...CH3Br and ClCH3...Br-")
        return 1
    print("PASS")
    return 0


sys.exit(main())
