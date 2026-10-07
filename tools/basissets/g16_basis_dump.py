#!/usr/bin/env python3
"""Dump a Gaussian 16 basis keyword as ECCE .BAS (and .POT) text.

    ./g16_basis_dump.py SDD --out /path/to/dir [-j 3] [--elements H-Rn]

Gaussian expands its own keywords in the log when asked for `gfinput`.  This
runs one single-atom job per element, stops each at the end of link 301 (the
basis is printed there, no SCF is done), and converts the printed general
basis input to ECCE's format.  It is the source for sets BSE does not
carry under Gaussian's spelling (SDD, CBSB7, def2-SV/TZV/QZV, ...); whether
the result is the same basis as the keyword is then judged by
tests/basisload/named_basis_check.py, not assumed.

Gaussian prints contractions "overlap normalised", which differs from a
published coefficient table by a constant per shell; the codes renormalise
contractions on input, so the energy is unaffected.  Generally contracted
shells come out as one shell per contraction.
"""
import argparse
import concurrent.futures
import os
import re
import signal
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diffreport import SYMBOLS  # noqa: E402

G16 = "/opt/gaussian/g16/g16"


def run301(keyword, symbol, mult, workdir):
    """The log up to the end of link 301, or None."""
    deck = ("%%nprocshared=1\n%%mem=1GB\n#p HF/%s gfinput 5D 7F nosymm\n\n"
            "t\n\n0 %d\n%s 0.0 0.0 0.0\n\n" % (keyword, mult, symbol))
    env = dict(os.environ, g16root="/opt/gaussian", GAUSS_SCRDIR=workdir)
    proc = subprocess.Popen([G16], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, errors="replace", cwd=workdir, env=env,
                            start_new_session=True)
    proc.stdin.write(deck)
    proc.stdin.close()
    lines = []
    for line in proc.stdout:
        lines.append(line)
        if "Leave Link  301" in line or "Error termination" in line:
            break
    os.killpg(proc.pid, signal.SIGKILL)
    proc.wait()
    time.sleep(0.2)
    return "".join(lines)


def parse(log):
    """(basis, ecp): basis = [(shell, [(exp, coef...)])], ecp = dict|None."""
    m = re.search(r"AO basis set in the form of general basis input.*?\n"
                  r"(.*?)\n\s*(?:Effective core potential|\d+ basis functions)",
                  log, re.S)
    if not m:
        return None, None
    shells = []
    for line in m.group(1).splitlines()[1:]:
        f = line.split()
        if not f or f[0] == "****":
            continue
        if re.match(r"^[A-Z]+$", f[0]) and len(f) == 4:
            shells.append([f[0], []])
        else:
            shells[-1][1].append([float(x.replace("D", "E")) for x in f])
    ecp = None
    m = re.search(r"general ECP input:\n(.*?)\n\s*\n\s*\d+ basis functions",
                  log, re.S)
    if m:
        ecp = parse_ecp(m.group(1))
    return shells, ecp


def parse_ecp(text):
    rows = [l for l in text.splitlines()[1:] if l.strip()]
    head = rows[0].split()
    lmax, ncore = int(head[1]), int(head[2])
    comps, i = [], 1
    while i < len(rows):
        label = rows[i].split()[0]
        n = int(rows[i + 1].split()[0])
        pots = [rows[i + 2 + k].split() for k in range(n)]
        comps.append((label, [(int(p[0]), p[1], p[2]) for p in pots]))
        i += 2 + n
    return {"lmax": lmax, "ncore": ncore, "components": comps}


def one(args):
    keyword, symbol = args
    with tempfile.TemporaryDirectory(dir=os.environ.get("G16DUMP_SCRATCH"),
                                     ignore_cleanup_errors=True) as d:
        for mult in (1, 2):
            log = run301(keyword, symbol, mult, d)
            shells, ecp = parse(log)
            #  an odd electron count under multiplicity 1 stops short
            if shells and " basis functions," in log:
                return symbol, shells, ecp
    return symbol, None, None


def bas_text(results):
    out = []
    for sym, shells, _ in results:
        if not shells:
            continue
        out.append("atom=%s" % sym)
        for label, prims in shells:
            ncoef = len(prims[0]) - 1
            out.append("contraction shell=%s num_primitives=%d "
                       "num_coefficients=%d" % (label, len(prims), ncoef))
            for p in prims:
                out.append(" ".join("%.10f" % v if i else repr(v)
                                    for i, v in enumerate(p)))
    return "\n".join(out) + "\n"


def pot_text(results):
    out = []
    for sym, _, ecp in results:
        if not ecp:
            continue
        out.append("atom=%s ncore=%d lmax=%d" % (sym, ecp["ncore"], ecp["lmax"]))
        for k, (label, pots) in enumerate(ecp["components"]):
            #  Gaussian's "G component" (local) and "S-G projection" are
            #  ECCE's "g" and "s-g"; l is the first letter's index.
            shell = label.lower() if k == 0 else label.lower()
            l = "spdfghi".index(shell[0]) if k else ecp["lmax"]
            out.append("ecp_potential%%l=%d%%shell=%s potential%%num_exponents=%d"
                       % (l, shell, len(pots)))
            for n, e, c in pots:
                out.append("%d %s %s" % (n, e, c))
    return "\n".join(out) + "\n" if out else ""


def elements(spec):
    if spec == "all":
        return SYMBOLS[:86]
    a, _, b = spec.partition("-")
    return SYMBOLS[SYMBOLS.index(a):SYMBOLS.index(b or a) + 1]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("keyword")
    ap.add_argument("--out", required=True)
    ap.add_argument("--elements", default="all", help="all, or e.g. H-Kr")
    ap.add_argument("--name", help="file stem (default: the keyword)")
    ap.add_argument("-j", type=int, default=3)
    a = ap.parse_args()
    with concurrent.futures.ThreadPoolExecutor(min(a.j, 3)) as pool:
        results = list(pool.map(one, [(a.keyword, s) for s in elements(a.elements)]))
    have = [r[0] for r in results if r[1]]
    stem = os.path.join(a.out, a.name or a.keyword)
    open(stem + ".BAS", "w").write(bas_text(results))
    pot = pot_text(results)
    if pot:
        open(stem + ".POT", "w").write(pot)
    print("%s: %d elements: %s" % (a.keyword, len(have), " ".join(have)))
    print("ECP: %s" % " ".join(r[0] for r in results if r[2]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
