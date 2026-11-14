#!/usr/bin/env python3
"""PCA of scripts/ecce_qsar_chemo.py and `ecce-qsar pca` (#961).

Oracles: an eigen-decomposition of the covariance matrix and a NIPALS loop
written here (the library uses an SVD), tabulated F quantiles, scipy and
scikit-learn where installed, and data of known rank for the cross-validation.
"""
import csv
import json
import math
import os
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import ecce_qsar_chemo as CH  # noqa: E402

fails = []


def check(c, msg):
    if not c:
        fails.append(msg)
        print("FAIL:", msg)


def sign_align(A, B):
    """B with each column's sign chosen to match A."""
    s = np.sign((A * B).sum(0))
    s[s == 0] = 1
    return B * s


rng = np.random.RandomState(7)
n, k, rank = 60, 20, 3
scores = rng.randn(n, rank) * np.array([4.0, 3.0, 2.0])
load = np.linalg.qr(rng.randn(k, rank))[0]
X = scores @ load.T + 0.3 * rng.randn(n, k) + rng.randn(k) * 5

# F quantiles: published table values and scipy
check(abs(CH.f_ppf(0.95, 5, 10) - 3.3258) < 1e-3, "F(0.95;5,10)")
check(abs(CH.f_ppf(0.95, 3, 20) - 3.0984) < 1e-3, "F(0.95;3,20)")
try:
    from scipy import stats
    for d1, d2 in ((2, 30), (7, 11), (15, 400)):
        check(abs(CH.f_ppf(0.95, d1, d2) - stats.f.ppf(0.95, d1, d2)) < 1e-6,
              "F quantile vs scipy %d,%d" % (d1, d2))
except ImportError:
    print("note: scipy missing")

# against eigh of the covariance matrix (UV scaling)
m = CH.pca_model(X, "uv", ncomp=4, cv_groups=0)
Xs = (X - X.mean(0)) / X.std(0, ddof=1)
w, V = np.linalg.eigh(Xs.T @ Xs / (n - 1))
order = np.argsort(w)[::-1]
w, V = w[order], V[:, order]
check(np.allclose(m["eigenvalues"], w[:4], rtol=1e-9), "eigenvalues vs eigh")
check(np.allclose(sign_align(V[:, :4], m["loadings"]), V[:, :4], atol=1e-8), "loadings vs eigh")
check(np.allclose(sign_align(Xs @ V[:, :4], m["scores"]), Xs @ V[:, :4], atol=1e-7),
      "scores vs eigh")
check(abs(m["r2x_cum"][-1] - w[:4].sum() / w.sum()) < 1e-10, "R2X vs eigenvalues")


def nipals(Xs, a):
    E, T, P = Xs.copy(), [], []
    for _ in range(a):
        t = E[:, [np.argmax(E.var(0))]]
        for _ in range(500):
            p = E.T @ t / (t.T @ t)
            p /= np.linalg.norm(p)
            tn = E @ p
            if np.linalg.norm(tn - t) < 1e-13:
                t = tn
                break
            t = tn
        T.append(t[:, 0])
        P.append(p[:, 0])
        E = E - t @ p.T
    return np.array(T).T, np.array(P).T


Tn, Pn = nipals(Xs, 3)
check(np.allclose(sign_align(Tn, m["scores"][:, :3]), Tn, atol=1e-6), "scores vs NIPALS")
check(np.allclose(sign_align(Pn, m["loadings"][:, :3]), Pn, atol=1e-6), "loadings vs NIPALS")

try:
    from sklearn.decomposition import PCA
    sk = PCA(4).fit(Xs)
    check(np.allclose(sk.explained_variance_, m["eigenvalues"][:4], rtol=1e-9), "sklearn eigenvalues")
    check(np.allclose(np.abs(sk.transform(Xs)), np.abs(m["scores"]), atol=1e-7), "sklearn scores")
except ImportError:
    print("note: scikit-learn not installed, comparison skipped")

# cross-validation finds the rank of the data
ma = CH.pca_model(X, "uv", None, 7)
check(ma["ncomp"] == rank, "components chosen by Q2: %d" % ma["ncomp"])
check(all(q > 0.3 for q in ma["q2"][:rank]) and ma["q2"][rank] < 0.05,
      "Q2 per component %s" % np.round(ma["q2"][:5], 3))
noise = rng.randn(50, 15)
mn = CH.pca_model(noise, "uv", None, 7)
check(mn["q2"][0] < 0.05, "no significant component in noise (Q2 %.3f)" % mn["q2"][0])

# diagnostics: closed-form relations
a = m["ncomp"]
check(abs(m["t2"].mean() - a * (n - 1) / n) < 1e-9, "mean T2")
check(abs((m["dmodx"] ** 2).mean() - (n - a - 1) / n) < 1e-9, "mean DModX^2")
Xo = X.copy()
v = rng.randn(k)
v -= load @ (load.T @ v)
Xo[5] += 6 * v / np.linalg.norm(v)
mo = CH.pca_model(Xo, "uv", 3, 0)
check(mo["dmodx"][5] > mo["dmodx_limit"] and mo["dmodx"].argmax() == 5, "outlier row has the largest DModX")
check(np.allclose(CH.pca_project(m, X), m["scores"], atol=1e-8), "projection reproduces scores")
for sc in ("pareto", "ctr", "none"):
    CH.pca_model(X, sc, 2, 0)

# the command
tmp = tempfile.mkdtemp(prefix="qsar-pca-")
tab = os.path.join(tmp, "t.csv")
with open(tab, "w", newline="") as f:
    w_ = csv.writer(f)
    w_.writerow(["name", "opt_converged", "act"] + ["d%d" % j for j in range(k)] +
                ["const", "const2", "missing1", "missing2", "problems"])
    for i in range(n):
        w_.writerow(["m%d" % i, "no" if i == 3 else "yes", "%.3f" % scores[i, 0]] +
                    ["%.10f" % v for v in X[i]] + ["1", "2", "" if i == 0 else i,
                                                     "" if i == 1 else i, ""])
r = subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "ecce_qsar.py"), "pca", tab,
                    "--y", "act", "--components", "3", "--outdir", tmp, "--json"],
                   capture_output=True, text=True)
check(r.returncode == 0, "pca exit: " + r.stderr)
d = json.loads(r.stdout)
keepi = [i for i in range(n) if i != 3]
ref = CH.pca_model(X[keepi], "uv", 3, 7)
check(d["n"] == n - 1 and d["k"] == k and d["ncomp"] == 3, "shape: row 3 (not converged) and the constant column dropped")
check(any("m3 left out" in x for x in d["notes"]) and any("const" in x for x in d["notes"]), "notes: %s" % d["notes"])
check(np.allclose(d["eigenvalues"][:3], ref["eigenvalues"][:3], rtol=1e-6), "CLI eigenvalues")
sc = list(csv.reader(open(os.path.join(tmp, "pca_scores.csv"))))
check(len(sc) == n and sc[0][:4] == ["name", "t1", "t2", "t3"], "scores file")
check(os.path.exists(os.path.join(tmp, "pca_loadings.csv")) and
      os.path.exists(os.path.join(tmp, "pca_components.csv")), "loadings and components files")

command = [sys.executable, os.path.join(ROOT, "scripts", "ecce_qsar.py"), "pca", tab,
           "--y", "act", "--components", "4", "--outdir", tmp]
r = subprocess.run(command, capture_output=True, text=True)
check(r.returncode == 0, "readable PCA report exit: " + r.stderr)
check("component 4 does not improve the cross-validated prediction" in r.stdout,
      "fixed component below significance limit is explained")
check("Left out (no variance in these 59 molecules): const, const2" in r.stdout and
      "Left out (missing values): missing1, missing2" in r.stdout and
      "Left out (optimisation not converged): m3" in r.stdout, "left-out notes grouped by reason")
with open(os.path.join(tmp, "analysis_components.csv"), encoding="utf-8") as f:
    display = list(csv.DictReader(f))
check(len(display) == 4 and "Cumulative Q²" in display[0] and
      abs(float(display[0]["X variance / %"]) - 100 * ref["r2x"][0]) <= 0.051,
      "display table has symbols and correctly converted percentages")
with open(os.path.join(tmp, "analysis_summary.txt"), encoding="utf-8") as f:
    summary = f.read()
check("* = component kept" in summary and "far from the model" in summary and
      "Eigenvalue" not in summary, "summary keeps legend and diagnostics apart from table")
r = subprocess.run(command + ["--cv", "0"], capture_output=True, text=True)
check(r.returncode == 0 and "Cross-validation is unavailable" in r.stdout,
      "PCA without CV explains unavailable Q²")

if fails:
    print("%d FAILED" % len(fails))
    sys.exit(1)
print("qsar pca: ok")
