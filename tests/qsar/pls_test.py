#!/usr/bin/env python3
"""PLS of scripts/ecce_qsar_chemo.py and `ecce-qsar pls|predict` (#961).

Oracle: the PLS1 model with A components is the least-squares solution
restricted to the Krylov space span{X'y, (X'X)X'y, ...}; the test solves that
with lstsq, which shares no code with the NIPALS loop.  Cross-validation is
re-done with it fold by fold.  scikit-learn is compared when installed.
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


def krylov_coef(Xs, ys, a):
    K = [Xs.T @ ys]
    for _ in range(a - 1):
        K.append(Xs.T @ (Xs @ K[-1]))
    # Orthonormal basis of the same space: the raw powers become nearly
    # parallel by the fifth, and lstsq on them misses 1e-8.
    K = np.linalg.qr(np.array(K).T)[0]
    c = np.linalg.lstsq(Xs @ K, ys, rcond=None)[0]
    return K @ c


rng = np.random.RandomState(11)
n, k = 70, 25
Z = rng.randn(n, 4)
Lm = np.linalg.qr(rng.randn(k, 4))[0]
X = Z @ Lm.T * 3 + 0.4 * rng.randn(n, k) + rng.randn(k) * 4
y = 2.0 * Z[:, 0] - 1.0 * Z[:, 1] + 0.3 * rng.randn(n) + 7

Xs = (X - X.mean(0)) / X.std(0, ddof=1)
ys = (y - y.mean()) / y.std(ddof=1)
W, T, P, q = CH.pls1_nipals(Xs, ys, 5)
for a in (1, 2, 3, 5):
    b = CH.pls_coef(W, P, q, a)
    check(np.allclose(b, krylov_coef(Xs, ys, a), atol=1e-8), "coefficients, %d components, vs Krylov" % a)
Wf, Tf, Pf, qf = CH.pls1_nipals(Xs, ys, k)
bols = np.linalg.lstsq(Xs, ys, rcond=None)[0]
check(np.allclose(CH.pls_coef(Wf, Pf, qf, Wf.shape[1]), bols, atol=1e-6), "all components = OLS")
check(np.allclose(T.T @ T, np.diag((T ** 2).sum(0)), atol=1e-8), "scores orthogonal")

try:
    from sklearn.cross_decomposition import PLSRegression
    sk = PLSRegression(3, scale=False).fit(Xs, ys)
    check(np.allclose(sk.predict(Xs).ravel(), Xs @ CH.pls_coef(W, P, q, 3), atol=1e-6), "sklearn predictions")
except ImportError:
    print("note: scikit-learn not installed, comparison skipped")

m = CH.pls_model(X, y, "uv", ncomp=4, cv_groups=7)
check(np.allclose(m["coef_scaled"], krylov_coef(Xs, ys, 4), atol=1e-8), "model coefficients vs Krylov")
check(np.allclose(m["fitted"], m["coef_scaled"] @ Xs.T * y.std(ddof=1) + y.mean(), atol=1e-8), "fitted values")
check(np.allclose(CH.pls_predict(m, X), m["fitted"], atol=1e-8), "original-unit coefficients and intercept")

# cross-validation redone with the Krylov solution, fold by fold
press = np.zeros(4)
for g in range(7):
    te = np.arange(n) % 7 == g
    Xa, ya = X[~te], y[~te]
    mu, sd = Xa.mean(0), Xa.std(0, ddof=1)
    ym, ysd = ya.mean(), ya.std(ddof=1)
    xs = (Xa - mu) / sd
    for a in range(1, 5):
        b = krylov_coef(xs, (ya - ym) / ysd, a)
        pr = ((X[te] - mu) / sd) @ b * ysd + ym
        press[a - 1] += ((y[te] - pr) ** 2).sum()
check(np.allclose(m["press"], press, rtol=1e-8), "PRESS vs Krylov cross-validation")
check(abs(m["q2_cum"][3] - (1 - press[3] / ((y - y.mean()) ** 2).sum())) < 1e-9, "Q2")
check(abs(m["rmsecv"][3] - math.sqrt(press[3] / n)) < 1e-9, "RMSECV")

# component choice: two latent factors drive y
mc = CH.pls_model(X, y, "uv", None, 7)
check(2 <= mc["ncomp"] <= 4 and mc["q2_final"] > 0.8, "components %d, Q2 %.3f" % (mc["ncomp"], mc["q2_final"]))
check(mc["r2y_final"] >= mc["q2_final"], "R2Y >= Q2")

# VIP: sum of squares is K; the two descriptors that carry y rank highest
check(abs((mc["vip"] ** 2).sum() - k) < 1e-8, "sum VIP^2 = K")
a = mc["ncomp"]
w2 = mc["weights"][:, :a] ** 2 / (mc["weights"][:, :a] ** 2).sum(0)
ssy = mc["yloadings"][:a] ** 2 * (mc["scores"][:, :a] ** 2).sum(0)
check(np.allclose(mc["vip"], np.sqrt(k * (w2 @ ssy) / ssy.sum())), "VIP formula")

# a held-out set
test = np.zeros(n, bool)
test[::5] = True
mt = CH.pls_model(X, y, "uv", 3, 7, test=test)
ref = CH.pls_model(X[~test], y[~test], "uv", 3, 7)
check(np.allclose(mt["coef"], ref["coef"]) and mt["n"] == (~test).sum(), "test rows are not in the fit")
err = y[test] - (X[test] @ ref["coef"] + ref["intercept"])
check(abs(mt["rmsep"] - math.sqrt((err ** 2).mean())) < 1e-9, "RMSEP")
check(mt["q2_ext"] > 0.7, "external Q2 %.3f" % mt["q2_ext"])

# permutation: the real model stands out, the permuted ones do not
mp = CH.pls_model(X, y, "uv", 3, 7, permutations=40, seed=3)["permutation"]
check(mp["q2"][0] > 0.8 and mp["q2_intercept"] < 0.05 and mp["r2_intercept"] < 0.5,
      "permutation intercepts R2 %.3f Q2 %.3f" % (mp["r2_intercept"], mp["q2_intercept"]))
noise = rng.randn(n)
mn = CH.pls_model(X, noise, "uv", None, 7)
check(mn["q2_final"] is not None and mn["q2_final"] < 0.1, "noise response: Q2 %.3f" % mn["q2_final"])

# new rows: prediction and domain
t2, dm = CH.pls_diagnose(m, X)
check(np.allclose(t2, m["t2"], atol=1e-8) and np.allclose(dm, m["dmodx"], atol=1e-8),
      "diagnostics of the training rows agree")

# commands
tmp = tempfile.mkdtemp(prefix="qsar-pls-")
tab = os.path.join(tmp, "t.csv")
with open(tab, "w", newline="") as f:
    w_ = csv.writer(f)
    w_.writerow(["name", "opt_converged", "pIC50"] + ["d%d" % j for j in range(k)] + ["problems"])
    for i in range(n):
        w_.writerow(["m%d" % i, "yes", "%.12f" % y[i]] + ["%.10f" % v for v in X[i]] + [""])
cli = os.path.join(ROOT, "scripts", "ecce_qsar.py")
r = subprocess.run([sys.executable, cli, "pls", tab, "--y", "pIC50", "--components", "3",
                    "--test-names", ",".join("m%d" % i for i in range(0, n, 5)),
                    "--permutations", "10", "--outdir", tmp, "--save-model", tmp + "/m.json",
                    "--json"], capture_output=True, text=True)
check(r.returncode == 0, "pls exit: " + r.stderr)
d = json.loads(r.stdout)
check(d["n"] == (~test).sum() and d["n_test"] == test.sum() and d["ncomp"] == 3, "split and components")
check(abs(d["rmsep"] - mt["rmsep"]) < 1e-6 and np.allclose(d["coef"], mt["coef"], rtol=1e-6), "CLI = library")
for fn in ("pls_components.csv", "pls_scores.csv", "pls_vip.csv", "pls_predictions.csv", "pls_permutation.csv"):
    check(os.path.exists(os.path.join(tmp, fn)), fn)
pr = list(csv.DictReader(open(os.path.join(tmp, "pls_predictions.csv"))))
check({x["set"] for x in pr} == {"train", "test"} and len(pr) == n, "observed/predicted file")
out = os.path.join(tmp, "pred.csv")
r = subprocess.run([sys.executable, cli, "predict", tmp + "/m.json", tab, "--csv", out],
                   capture_output=True, text=True)
check(r.returncode == 0, "predict exit: " + r.stderr)
pp = list(csv.DictReader(open(out)))
check(len(pp) == n and np.allclose([float(x["predicted_pIC50"]) for x in pp],
                                   X @ mt["coef"] + mt["intercept"], atol=1e-5), "saved model predicts the same")
check(pp[1]["in_domain"] in ("yes", "no"), "domain column")

# A small, wide table exercises the warning that a good fit can predict poorly.
wide = np.random.RandomState(83)
Xt = wide.randn(16, 80)
yt = 40 + 15 * Xt[:, 0] + 10 * wide.randn(16)
report_tab = os.path.join(tmp, "report.csv")
with open(report_tab, "w", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)
    writer.writerow(["name", "bp (°C)", "a_nC"] + ["v%d" % j for j in range(1, 80)])
    writer.writerows([["m%d" % i, yt[i]] + list(Xt[i]) for i in range(16)])
command = [sys.executable, cli, "pls", report_tab, "--y", "bp (°C)", "--components", "2",
           "--outdir", tmp]
num = subprocess.run(command + ["--json"], capture_output=True, text=True)
check(num.returncode == 0, "report numerical fixture: " + num.stderr)
numbers = json.loads(num.stdout)
r = subprocess.run(command, capture_output=True, text=True)
check(r.returncode == 0, "readable PLS report exit: " + r.stderr)
check("explain %.0f %% of the variation in bp" % (100 * numbers["r2y_cum"][1]) in r.stdout and
      "Typical error: %.1f °C" % numbers["rmsee"] in r.stdout and
      "%.1f °C on molecules left out" % numbers["rmsecv"][1] in r.stdout,
      "plain-language summary percentages and errors come from model numbers with units")
ratio = numbers["rmsecv"][1] / numbers["rmsee"]
check(ratio > 2 and "fits much better than it predicts" in r.stdout and
      "%.1f×" % ratio in r.stdout and "few molecules for this many descriptors" in r.stdout,
      "large cross-validation error gets ratio and small-sample warning")
check("few molecules (16) for 2 components" in r.stdout and
      "component 2 does not improve" in r.stdout, "fixed weak component and small N are explained")
with open(os.path.join(tmp, "analysis_components.csv"), encoding="utf-8") as f:
    display = list(csv.DictReader(f))
check(len(display) == 2 and "Cumulative R²Y" in display[0] and "RMSECV / °C" in display[0] and
      abs(float(display[1]["RMSEE / °C"]) - numbers["rmsee"]) <= 0.0051,
      "PLS display table uses proper symbols and measured error units")
with open(os.path.join(tmp, "analysis_vip.csv"), encoding="utf-8") as f:
    vip = list(csv.DictReader(f))
check(len(vip) == 10 and all(float(vip[j]["VIP"]) >= float(vip[j + 1]["VIP"]) for j in range(9)) and
      any(row["Descriptor"] == "a_nC" and row["Description"] == "Number of carbon atoms" for row in vip),
      "short sorted VIP table uses shared descriptor descriptions")
r = subprocess.run(command + ["--cv", "0"], capture_output=True, text=True)
check(r.returncode == 0 and "Cross-validation is unavailable" in r.stdout,
      "PLS without CV explains unavailable Q²")

if fails:
    print("%d FAILED" % len(fails))
    sys.exit(1)
print("qsar pls: ok")
