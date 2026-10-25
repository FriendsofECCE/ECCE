#!/usr/bin/env python3
"""qm_ts_scanpoint: runtype ts from a relaxed-scan point (#509).

    ts_scanpoint_test.py ECCE_QM_EXE

Cl- + CH3F, HF/3-21G, started at the C-Cl = 2.0 angstrom point of a relaxed
scan, from where a search that follows the lowest mode of an updated model
Hessian wanders off.  Compared with ORCA (ts_scanpoint_oracle.json): energy
1e-5 Eh, every interatomic distance 0.01 angstrom, the imaginary frequency
2 cm-1; and exactly one imaginary frequency, converged.
"""
import itertools, json, math, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
START = """Cl 0.0000000703 0.0000000000 0.7681040758
C 0.0000000013 0.0000000000 2.7681040758
F -0.0000000651 0.0000000000 4.6650977183
H 1.0541049937 0.0000000000 2.8662314136
H -0.5270525001 0.9128817047 2.8662313583
H -0.5270525001 -0.9128817047 2.8662313583
"""


def main():
    exe = sys.argv[1]
    oracle = json.load(open(os.path.join(HERE, "ts_scanpoint_oracle.json")))
    with tempfile.TemporaryDirectory() as work:
        inp = os.path.join(work, "ts.qm")
        open(inp, "w").write("runtype ts\nmethod hf\nbasis 3-21G\ncharge -1\nmaxsteps 60\n"
                             "geometry\n" + START + "end\n")
        p = subprocess.run([exe, inp], capture_output=True, text=True)
    fails = []
    if p.returncode != 0:
        fails.append("exit status %d: %s" % (p.returncode, p.stderr[-300:]))
    else:
        out = p.stdout
        if not re.search(r"^progress opt step=\d+ converged=yes", out, re.M):
            fails.append("not converged")
        e = float(re.search(r"^energy_total\s+(\S+)", out, re.M).group(1))
        if abs(e - oracle["energy"]) > 1e-5:
            fails.append("energy %.8f against ORCA %.8f" % (e, oracle["energy"]))
        geo = re.findall(r"^begin geometry[^\n]*\n(.*?)^end geometry", out, re.S | re.M)[-1].splitlines()
        xyz = [[float(c) for c in l.split()[2:5]] for l in geo]
        d = sorted(math.dist(a, b) for a, b in itertools.combinations(xyz, 2))
        bad = [(round(a, 3), b) for a, b in zip(d, oracle["distances"]) if abs(a - b) > 0.01]
        if bad:
            fails.append("distances differ from ORCA (ours, ORCA): %s" % bad)
        n = int(re.search(r"^vib_imaginary\s+(\d+)", out, re.M).group(1))
        if n != 1:
            fails.append("%d imaginary frequencies" % n)
        nrot = int(re.search(r"^vib_transrot\s+(\d+)", out, re.M).group(1))
        vib = re.findall(r"^begin vibrations[^\n]*\n(.*?)^end vibrations", out, re.S | re.M)[-1].splitlines()
        f0 = float(vib[nrot].split()[1])
        if abs(f0 - oracle["imaginary"]) > 2.0:
            fails.append("imaginary frequency %.1f against ORCA %.1f" % (f0, oracle["imaginary"]))
    print("%-22s %s" % ("cl_ch3f_scanpoint", "ok" if not fails else "FAIL"))
    for f in fails:
        print("   ", f)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
