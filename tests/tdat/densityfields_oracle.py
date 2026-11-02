#!/usr/bin/env python3
"""Compare ecce-density-field with Multiwfn at the same points (#799, #800).

  densityfields_oracle.py ECCE_DENSITY_FIELD MULTIWFN WORKDIR NAME:CALC:MOLDEN ...

WORKDIR must hold a settings.ini with RDG_maxrho= 0 and ELF_addminimal= 0
(Multiwfn's cut-off for s and its +1e-5 on D are not part of the definitions).
Points are 60 seeded random ones in the molecule's box plus margin 2.5 A.
Multiwfn's menu 1 ("properties at a point") is scripted through stdin.  Prints,
per molecule, the largest differences in the tolerances of the issues:
ELF, LOL absolute; rho and |grad rho| relative; s where rho > 1e-4; lambda2 and
sign(lambda2)rho.
"""
import math
import os
import random
import re
import subprocess
import sys

exe, mwfn, work = sys.argv[1:4]
env = dict(os.environ, KMP_STACKSIZE="200M")


def ours(calc, pts):
    cmd = [exe, calc]
    for p in pts:
        cmd += ["--at", "%.6f,%.6f,%.6f" % p]
    rows = [l.split() for l in subprocess.check_output(cmd, universal_newlines=True).splitlines()
            if not l.startswith("#")]
    return [[float(v) for v in r] for r in rows]


def theirs(molden, pts):
    stdin = "1\n" + "".join("%.6f,%.6f,%.6f\n2\n" % p for p in pts) + "q\n0\nq\n"
    text = subprocess.run([mwfn, molden], input=stdin, cwd=work, env=env, universal_newlines=True,
                          stdout=subprocess.PIPE, stderr=subprocess.DEVNULL).stdout
    f = lambda pat: [float(m) for m in re.findall(pat + r":\s+(\S+)", text)]
    g = [[float(v) for v in m.split()] for m in
         re.findall(r"Components of gradient in x/y/z are:\n(.*)\n", text)]
    e = [[float(v) for v in m.split()] for m in re.findall(r"Eigenvalues of Hessian:(.*)\n", text)]
    return (f(r"Density of all electrons"), g, e, f(r"\(ELF\)"), f(r"\(LOL\)"),
            f(r"Reduced density gradient \(RDG\)"), f(r"Sign\(lambda2\)\*rho"))


for spec in sys.argv[4:]:
    name, calc, molden = spec.split(":")
    xyz = [l for l in open(molden).read().split("[Atoms] Angs\n")[1].splitlines() if len(l.split()) == 6]
    at = [[float(v) for v in l.split()[3:6]] for l in xyz]
    lo = [min(a[k] for a in at) - 2.5 for k in range(3)]
    hi = [max(a[k] for a in at) + 2.5 for k in range(3)]
    rnd = random.Random(7)
    pts = [tuple(lo[k] + rnd.random()*(hi[k] - lo[k]) for k in range(3)) for _ in range(60)]
    # a few on the bond side of each atom, where the density is large
    pts += [(a[0] + 0.3, a[1] + 0.2, a[2] + 0.4) for a in at]
    o = ours(calc, pts)
    rho, grad, eig, elf, lol, rdg, sl2 = theirs(molden, pts)
    n = len(pts)
    assert all(len(v) == n for v in (rho, grad, eig, elf, lol, rdg, sl2)), "Multiwfn output incomplete"
    d = dict(elf=0, lol=0, rho=0, grad=0, rdg=0, sl2=0, l2=0)
    for i in range(n):
        r = o[i]
        d["elf"] = max(d["elf"], abs(r[14] - elf[i]))
        d["lol"] = max(d["lol"], abs(r[15] - lol[i]))
        d["rho"] = max(d["rho"], abs(r[3] - rho[i])/rho[i])
        gm = math.sqrt(sum(v*v for v in grad[i]))
        go = math.sqrt(sum(v*v for v in r[4:7]))
        d["grad"] = max(d["grad"], max(abs(r[4 + k] - grad[i][k]) for k in range(3))/max(gm, 1e-8))
        if rho[i] > 1e-4:
            d["rdg"] = max(d["rdg"], abs(r[16] - rdg[i]))
        d["sl2"] = max(d["sl2"], abs(r[17] - sl2[i]))
        d["l2"] = max(d["l2"], abs(r[12] - eig[i][1]))
    print("%-6s n=%d  ELF %.1e  LOL %.1e  rho(rel) %.1e  grad(rel) %.1e  s %.1e  sign(l2)rho %.1e  lambda2 %.1e"
          % (name, n, d["elf"], d["lol"], d["rho"], d["grad"], d["rdg"], d["sl2"], d["l2"]))
