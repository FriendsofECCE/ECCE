#!/usr/bin/env python3
"""ecce-pockets against fpocket on the #1320 pocket benchmark complexes.

Same complexes, same cleaned files and same hit test (pocket centre within
8 A of any ligand atom) as pocket_rank_bench.py.  fpocket pocket centre = mean
of the pocket's alpha-sphere centres (pocketN_vert.pqr).  Needs ECCE_HOME.

  ECCE_HOME=<src> fpocket_oracle.py --pockets BUILD/ecce-pockets --fpocket BIN \\
      --fpocket-noexp BIN2 --keep DIR --csv out.csv
"""
import argparse, csv, glob, math, os, re, shutil, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pocket_rank_bench as B

def run_fpocket(exe, prot, tmp):
    os.makedirs(tmp, exist_ok=True)
    cp = os.path.join(tmp, "p.pdb")
    shutil.copy(prot, cp)
    subprocess.run([exe, "-f", cp], capture_output=True, text=True, check=True)
    files = glob.glob(os.path.join(tmp, "p_out", "pockets", "pocket*_vert.pqr"))
    cs = []
    for f in sorted(files, key=lambda x: int(re.search(r"pocket(\d+)_vert", x).group(1))):
        v = [(float(l[30:38]), float(l[38:46]), float(l[46:54])) for l in open(f) if l.startswith("ATOM")]
        cs.append(tuple(sum(c[i] for c in v) / len(v) for i in range(3)))
    shutil.rmtree(tmp)
    return cs

def first_hit(centres, xyz):
    return next((i + 1 for i, c in enumerate(centres) if min(math.dist(c, a) for a in xyz) <= 8.0), None)

def pct(n, d): return "%.1f%% (%d/%d)" % (100.0 * n / d, n, d)

def summarise(rows):
    tools = (("ecce-pockets", "r_ecce"), ("fpocket default (expensive on)", "r_fp"),
             ("fpocket expensive off", "r_fpnoexp"))
    for hn, h in (("test half", rows[1::2]), ("all", rows)):
        print("\n%s, n=%d" % (hn, len(h)))
        for tn, k in tools:
            r = [x[k] for x in h]
            print("  %-32s top-1 %s  top-5 %s  detection %s" % (tn, pct(sum(1 for v in r if v == 1), len(h)),
                  pct(sum(1 for v in r if v and v <= 5), len(h)), pct(sum(1 for v in r if v), len(h))))
        for tn, k in tools[1:]:
            print(" ", tn)
            print("    fpocket detects, ecce does not:", " ".join(x["code"] for x in h if x[k] and not x["r_ecce"]))
            print("    ecce detects, fpocket does not:", " ".join(x["code"] for x in h if x["r_ecce"] and not x[k]))
            print("    both detect, fpocket ranks higher:", " ".join("%s(%s<%s)" % (x["code"], x[k], x["r_ecce"])
                  for x in h if x[k] and x["r_ecce"] and x[k] < x["r_ecce"]))
            print("    both detect, ecce ranks higher:", " ".join("%s(%s<%s)" % (x["code"], x["r_ecce"], x[k])
                  for x in h if x[k] and x["r_ecce"] and x["r_ecce"] < x[k]))

def main():
    ap = argparse.ArgumentParser()
    for n in ("pockets", "fpocket", "fpocket-noexp", "keep", "csv"):
        ap.add_argument("--" + n, required=True)
    a = ap.parse_args()
    os.makedirs(a.keep, exist_ok=True)
    rows = []
    for code in sorted(set(B.CODES)):
        f = B.fetch(code, a.keep)
        s = B.split(f, code, a.keep) if f else None
        if not s: continue
        prot, xyz, lig = B.keep(a.keep, code, s)
        pk = B.run(a.pockets, prot)
        if not pk: sys.exit("ERROR: no ecce-pockets pockets for %s (ECCE_HOME set?)" % code)
        ecce = [p["c"] for p in sorted(pk, key=lambda p: -p["score"])]
        fp = run_fpocket(a.fpocket, prot, os.path.join(a.keep, "fp_" + code))
        fn = run_fpocket(a.fpocket_noexp, prot, os.path.join(a.keep, "fn_" + code))
        rows.append(dict(code=code, ligand=lig, n_ecce=len(ecce), n_fp=len(fp), n_fpnoexp=len(fn),
                         r_ecce=first_hit(ecce, xyz), r_fp=first_hit(fp, xyz), r_fpnoexp=first_hit(fn, xyz)))
        print(rows[-1], flush=True)
    with open(a.csv, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0]))
        w.writeheader()
        for r in rows: w.writerow({k: ("none" if v is None else v) for k, v in r.items()})
    summarise(rows)

if __name__ == "__main__":
    main()
