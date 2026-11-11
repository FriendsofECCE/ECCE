"""Principal component analysis for QSAR tables (#961).

Wx-free; numpy only.  Scaling follows SIMCA's names (uv: unit variance,
pareto, ctr: centre only).  Cross-validation of the components is Wold's
element-wise scheme (elements left out in a diagonal pattern in 7 groups, the
model fitted to the rest by iterated SVD imputation), which is what makes the
PRESS fall and rise with the number of components for PCA; leaving out whole
rows would only ever favour more components.

  R2X(a)   share of the sum of squares in component a
  Q2(a)    1 - PRESS(a)/PRESS(a-1), PRESS(0) the sum of squares
  T2       Hotelling's T2 = sum_a t_a^2 / s_a^2, limit from the F distribution
  DModX    residual distance to the model, normalised (Eriksson et al.,
           Multi- and Megavariate Data Analysis): s_i = sqrt(e_i.e_i/(K-A)),
           S0 = sqrt(E.E/((N-A-1)(K-A))), DModX = s_i/S0, limit sqrt(F)
"""
import math

import numpy as np

SCALINGS = ("uv", "pareto", "ctr", "none")


# ---- F distribution without scipy --------------------------------------------

def _betacf(a, b, x):
    tiny = 1e-300
    qab, qap, qam = a + b, a + 1.0, a - 1.0
    c, d = 1.0, 1.0 - qab * x / qap
    d = 1.0 / (d if abs(d) > tiny else tiny)
    h = d
    for m in range(1, 400):
        m2 = 2 * m
        aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        delta = d * c
        h *= delta
        if abs(delta - 1.0) < 1e-14:
            break
    return h


def betainc(a, b, x):
    """Regularised incomplete beta function I_x(a, b)."""
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    lbeta = math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b)
    front = math.exp(lbeta + a * math.log(x) + b * math.log1p(-x))
    if x < (a + 1.0) / (a + b + 2.0):
        return front * _betacf(a, b, x) / a
    return 1.0 - front * _betacf(b, a, 1.0 - x) / b


def f_cdf(x, d1, d2):
    if x <= 0:
        return 0.0
    return betainc(d1 / 2.0, d2 / 2.0, d1 * x / (d1 * x + d2))


def f_ppf(p, d1, d2):
    """Quantile of the F distribution by bisection."""
    lo, hi = 0.0, 1.0
    while f_cdf(hi, d1, d2) < p:
        hi *= 2.0
        if hi > 1e12:
            return float("inf")
    for _ in range(200):
        mid = 0.5 * (lo + hi)
        if f_cdf(mid, d1, d2) < p:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


# ---- preprocessing --------------------------------------------------------------

def prepare(X, scaling="uv"):
    """-> (Xs, mean, scale).  Columns with no variance get scale 1 (and are
    expected to have been dropped by `usable_columns`)."""
    if scaling not in SCALINGS:
        raise ValueError("scaling must be one of %s" % ", ".join(SCALINGS))
    X = np.asarray(X, float)
    mean = X.mean(0) if scaling != "none" else np.zeros(X.shape[1])
    sd = X.std(0, ddof=1)
    if scaling == "uv":
        scale = sd
    elif scaling == "pareto":
        scale = np.sqrt(sd)
    else:
        scale = np.ones(X.shape[1])
    scale = np.where(scale > 0, scale, 1.0)
    return (X - mean) / scale, mean, scale


def usable_columns(X, names=None, eps=1e-12):
    """Indices of the columns that are finite in every row and not constant,
    and the reasons the others were left out."""
    keep, why = [], {}
    for j in range(X.shape[1]):
        c = X[:, j]
        n = names[j] if names else j
        if not np.all(np.isfinite(c)):
            why[n] = "missing values"
        elif np.ptp(c) <= eps * max(1.0, np.abs(c).max()):
            why[n] = "constant"
        else:
            keep.append(j)
    return keep, why


# ---- PCA ----------------------------------------------------------------------------

def pca_fit(Xs, ncomp):
    """SVD PCA of preprocessed data: scores T (N x A), loadings P (K x A) and the
    sums of squares of the A components."""
    U, s, Vt = np.linalg.svd(Xs, full_matrices=False)
    a = min(ncomp, len(s))
    P = Vt[:a].T
    T = U[:, :a] * s[:a]
    return T, P, s[:a] ** 2, float((s ** 2).sum())


def _impute_fit(Xs, mask, a, iters=500, tol=1e-9):
    """Rank-a model of Xs where mask (True = known) is observed: the unknown
    elements are replaced by the model's values until they settle."""
    Z = np.where(mask, Xs, 0.0)
    for _ in range(iters):
        U, s, Vt = np.linalg.svd(Z, full_matrices=False)
        R = (U[:, :a] * s[:a]) @ Vt[:a]
        Zn = np.where(mask, Xs, R)
        if np.abs(Zn - Z).max() < tol:
            Z = Zn
            break
        Z = Zn
    return R


def pca_cv(Xs, ncomp, groups=7):
    """PRESS(0..A) by Wold's element-wise cross-validation."""
    n, k = Xs.shape
    idx = (np.arange(n)[:, None] * k + np.arange(k)[None, :])
    # a diagonal pattern: element (i, j) goes to group (i*K + j) mod G; with K
    # divisible by G that would put whole columns together, so shift per row
    grp = (idx + np.arange(n)[:, None] * (1 if k % groups == 0 else 0)) % groups
    press = np.zeros(ncomp + 1)
    press[0] = float((Xs ** 2).sum())
    for g in range(groups):
        held = grp == g
        mask = ~held
        for a in range(1, ncomp + 1):
            R = _impute_fit(Xs, mask, a)
            press[a] += float(((Xs - R)[held] ** 2).sum())
    return press


def pca_model(X, scaling="uv", ncomp=None, cv_groups=7, max_comp=10, limit=0.05,
              names=None, ids=None):
    """Fit PCA to the table X (N x K).  `ncomp` None: add components while the
    cross-validated Q2 of the component exceeds `limit` (SIMCA's significance
    rule), up to max_comp.  Returns a dict of arrays and numbers."""
    X = np.asarray(X, float)
    n, k = X.shape
    Xs, mean, scale = prepare(X, scaling)
    amax = min(max_comp if ncomp is None else ncomp, n - 1, k)
    T, P, ss, sst = pca_fit(Xs, amax)
    press = pca_cv(Xs, amax, cv_groups) if cv_groups and cv_groups > 1 else None
    q2 = None
    if press is not None:
        q2 = [1.0 - press[a] / press[a - 1] for a in range(1, amax + 1)]
        q2cum = [1.0 - press[a] / press[0] for a in range(1, amax + 1)]
    if ncomp is None:
        a = 0
        while a < amax and (q2 is None or q2[a] > limit):
            a += 1
        a = max(a, 1)
    else:
        a = amax
    T, P = T[:, :a], P[:, :a]
    E = Xs - T @ P.T
    sse = float((E ** 2).sum())
    s2 = (T ** 2).sum(0) / (n - 1)
    t2 = ((T ** 2) / s2).sum(1)
    res = {
        "scaling": scaling, "n": n, "k": k, "ncomp": a, "mean": mean, "scale": scale,
        "scores": T, "loadings": P, "eigenvalues": ss[:amax] / (n - 1),
        "r2x": ss[:amax] / sst, "r2x_cum": np.cumsum(ss[:amax]) / sst,
        "q2": q2, "q2_cum": q2cum if q2 is not None else None,
        "press": press, "t2": t2, "score_var": s2,
        "ss_total": sst, "sse": sse,
    }
    res["t2_limit"] = (a * (n * n - 1.0) / (n * (n - a)) * f_ppf(0.95, a, n - a)
                       if n > a else float("nan"))
    if k > a and n > a + 1:
        si = np.sqrt((E ** 2).sum(1) / (k - a))
        s0 = math.sqrt(sse / ((n - a - 1) * (k - a)))
        res["dmodx"] = si / s0 if s0 > 0 else np.zeros(n)
        res["dmodx_limit"] = math.sqrt(f_ppf(0.95, k - a, (n - a - 1) * (k - a)))
    else:
        res["dmodx"], res["dmodx_limit"] = np.full(n, np.nan), float("nan")
    return res


def pca_project(model, Xnew):
    """Scores of new rows (same columns) in the model."""
    Xs = (np.asarray(Xnew, float) - model["mean"]) / model["scale"]
    return Xs @ model["loadings"]
