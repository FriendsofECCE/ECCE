#!/usr/bin/env python3
"""Does the descriptor ranking put the real ligand site first more often? (#1320)

Downloads each complex, keeps one chain and its largest drug-like ligand,
runs ecce-pockets on the protein alone, and scores both rankings against the
ligand.  Weights are tuned on the even-indexed complexes (sorted by PDB code)
and reported on the odd-indexed ones; the untuned all-ones weights are
reported on both halves.

  pocket_rank_bench.py --pockets BUILD/ecce-pockets --work DIR
"""
import argparse, itertools, math, os, re, subprocess, sys, urllib.request

CODES = """1a4g 1a9u 1aq1 1b9v 1bma 1c5x 1cx2 1d3h 1dwc 1e66 1eve 1f0r 1fkg 1gpk 1hpv
1hsg 1hvr 1hwi 1iep 1j4h 1jd0 1ke5 1kv1 1l2s 1m17 1mq6 1n2v 1nav 1oq5 1pmn 1qhi
1r9o 1sqn 1stp 1t46 1uml 1xm6 1yet 2bsm 2brb 3ert 3ptb 4dfr 4phv 1err 1abe 1ai5
1ake 1apt 1atl 1azm 1b58 1ba8 1bcd 1byb 1cbs 1cil 1com 1coy 1cps 1dg5 1dhf 1eed
1etr 1fax 1fki 1glq 1hdy 1hfc 1hri 1lst 1mrg 1pha 1rne 1tmn 1ydr 2ctc 2gbp 2yhx 3cpa 3tpi""".split()
ADDITIVE = set("""HOH WAT DOD SO4 PO4 GOL EDO PEG PGE PG4 MPD DMS ACT ACE FMT TRS MES EPE CL NA K CA MG ZN MN FE FE2 CU NI CO CD
HG IOD BR NO3 SCN BME DTT FLC CIT TAR MLI IPA EOH NH4 AZI PEP PLP 1PE P6G OXY""".split())
HEAVY_MIN = 10

def fetch(code, work):
    p = os.path.join(work, code + ".pdb")
    if not os.path.exists(p):
        try:
            data = urllib.request.urlopen("https://files.rcsb.org/download/%s.pdb" % code.upper(), timeout=60).read()
        except Exception as e:
            return None
        open(p, "wb").write(data)
    return p

def split(path, code, work):
    """-> (protein_file, ligand_xyz) or None."""
    prot, het = {}, {}
    for l in open(path):
        if l.startswith("ENDMDL"): break
        if l.startswith(("ATOM", "HETATM")) and l[16] in " A":
            if l.startswith("ATOM"):
                prot.setdefault(l[21], []).append(l)
            elif l[17:20].strip() not in ADDITIVE and l[76:78].strip() != "H":
                het.setdefault((l[21], l[17:20], l[22:27]), []).append(l)
    best = None
    for (ch, rn, rs), ls in het.items():
        if ch in prot and len(ls) >= HEAVY_MIN and len(prot[ch]) > 800 and (best is None or len(ls) > len(het[best])):
            best = (ch, rn, rs)
    if not best: return None
    out = os.path.join(work, code + "_prot.pdb")
    open(out, "w").write("".join(prot[best[0]]) + "END\n")
    xyz = [(float(l[30:38]), float(l[38:46]), float(l[46:54])) for l in het[best]]
    return out, xyz

def run(exe, pdb):
    txt = subprocess.run([exe, pdb, "--top", "60", "--threads", "2"], capture_output=True, text=True).stdout
    pk = []
    for m in re.finditer(r"#\d+ score (\S+) volume (\S+) A3 enclosure (\S+) apolar (\S+) mouth (\S+) rank \S+ centre (\S+) (\S+) (\S+)", txt):
        v = [float(x) for x in m.groups()]
        pk.append(dict(score=v[0], enc=v[2], apolar=v[3], mouth=v[4], c=tuple(v[5:8])))
    return pk

def hit(p, xyz, d=8.0):
    return min(math.dist(p["c"], a) for a in xyz) <= d

def key(p, w):
    return (p["score"] * max(p["apolar"], 1e-3) ** w[0] * max(p["enc"] / 7, 1e-3) ** w[1]
            * max(1 - p["mouth"], 1e-3) ** w[2])

def ranks(items, w):
    """first rank at which a pocket hits, per complex (inf = never)."""
    r = []
    for pk, xyz in items:
        order = sorted(pk, key=(lambda p: -p["score"]) if w is None else (lambda p: -key(p, w)))
        r.append(next((i + 1 for i, p in enumerate(order) if hit(p, xyz)), math.inf))
    return r

def rate(r, k): return 100.0 * sum(x <= k for x in r) / len(r)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pockets", required=True); ap.add_argument("--work", required=True)
    a = ap.parse_args(); os.makedirs(a.work, exist_ok=True)
    data = []
    for code in sorted(set(CODES)):
        f = fetch(code, a.work)
        s = split(f, code, a.work) if f else None
        if not s: print("skip", code); continue
        pk = run(a.pockets, s[0])
        if not pk: print("no pockets", code); continue
        data.append((code, pk, s[1]))
    items = [(pk, xyz) for _, pk, xyz in data]
    print("complexes used:", len(data))
    # a complex whose site is never found by any pocket cannot be ranked: report it
    print("site found in some pocket:", sum(1 for pk, x in items if any(hit(p, x) for p in pk)))
    tune, test = items[0::2], items[1::2]
    grid = list(itertools.product([0, .5, 1, 2, 3], repeat=3))
    # tune on top-1, ties broken by top-5
    best = max(grid, key=lambda w: (rate(ranks(tune, w), 1), rate(ranks(tune, w), 5)))
    print("tuned weights (apolar, enclosure, mouth) on tune half:", best)
    for name, half in (("tune half (%d)" % len(tune), tune), ("test half (%d)" % len(test), test), ("all (%d)" % len(items), items)):
        for label, w in (("current", None), ("descriptors w=1,1,1", (1, 1, 1)), ("descriptors tuned", best)):
            r = ranks(half, w)
            print("%-16s %-20s top-1 %5.1f%%  top-3 %5.1f%%  top-5 %5.1f%%" % (name, label, rate(r, 1), rate(r, 3), rate(r, 5)))
    r0, r1 = ranks(items, None), ranks(items, best)
    print("first-hit rank current/tuned:", " ".join("%s:%s/%s" % (c, x, y) for (c, _, _), x, y in zip(data, r0, r1)))

main()
