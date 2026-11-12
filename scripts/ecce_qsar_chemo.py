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


# ---- PLS (one response) -------------------------------------------------------------
#
#  NIPALS PLS1 on preprocessed X and y.  Cross-validation leaves out every G-th
#  row in turn and scales the training rows alone, so Q2 does not borrow the
#  left-out rows' means and spreads.  A component is kept while
#  PRESS(a)/PRESS(a-1) <= 0.95^2 (SIMCA's rule, Q2 of the component >= 0.0975).
#  RMSEE = sqrt(SSE/(N-A-1)) on the training rows, RMSECV = sqrt(PRESS/N),
#  RMSEP = sqrt(mean squared error) on rows kept out of the fit.

def pls1_nipals(Xs, ys, ncomp):
    """-> W, T, P, q (loadings weights, scores, X loadings, y loadings)."""
    E, f = Xs.copy(), ys.copy()
    W, T, P, q = [], [], [], []
    for _ in range(ncomp):
        w = E.T @ f
        nw = np.linalg.norm(w)
        if nw < 1e-12:
            break
        w = w / nw
        t = E @ w
        tt = float(t @ t)
        if tt < 1e-12:
            break
        p = E.T @ t / tt
        qa = float(f @ t) / tt
        E = E - np.outer(t, p)
        f = f - qa * t
        W.append(w)
        T.append(t)
        P.append(p)
        q.append(qa)
    return np.array(W).T, np.array(T).T, np.array(P).T, np.array(q)


def pls_coef(W, P, q, a):
    """Coefficients (scaled space) of the model with the first a components."""
    return W[:, :a] @ np.linalg.solve(P[:, :a].T @ W[:, :a], q[:a])


def _pls_predict_all(Xtr, ytr, Xte, scaling, amax):
    """Predictions of Xte for 1..amax components, model built on the training rows."""
    Xs, mean, scale = prepare(Xtr, scaling)
    ym, ysd = float(ytr.mean()), float(ytr.std(ddof=1))
    ysd = ysd if ysd > 0 else 1.0
    W, T, P, q = pls1_nipals(Xs, (ytr - ym) / ysd, amax)
    Xt = (Xte - mean) / scale
    out = np.zeros((Xte.shape[0], amax))
    for a in range(1, amax + 1):
        aa = min(a, W.shape[1])
        out[:, a - 1] = (Xt @ pls_coef(W, P, q, aa)) * ysd + ym
    return out


def pls_cv(X, y, scaling, amax, groups=7):
    """Cross-validated predictions (N x amax): row i is left out with i mod G."""
    n = len(y)
    pred = np.zeros((n, amax))
    for g in range(groups):
        te = np.arange(n) % groups == g
        if not te.any() or te.all():
            continue
        pred[te] = _pls_predict_all(X[~te], y[~te], X[te], scaling, amax)
    return pred


def pls_model(X, y, scaling="uv", ncomp=None, cv_groups=7, max_comp=10,
              test=None, permutations=0, seed=1):
    """PLS1 of y on X.  `test` is a boolean mask of rows kept out of the fit.
    ncomp None: chosen by cross-validation.  Returns a dict."""
    X = np.asarray(X, float)
    y = np.asarray(y, float)
    n_all = len(y)
    test = np.zeros(n_all, bool) if test is None else np.asarray(test, bool)
    tr = ~test
    Xt, yt = X[tr], y[tr]
    n, k = Xt.shape
    amax = min(max_comp if ncomp is None else ncomp, n - 2, k)
    Xs, mean, scale = prepare(Xt, scaling)
    ym, ysd = float(yt.mean()), float(yt.std(ddof=1))
    ysd = ysd if ysd > 0 else 1.0
    ys = (yt - ym) / ysd
    W, T, P, q = pls1_nipals(Xs, ys, amax)
    amax = W.shape[1]
    res = {"scaling": scaling, "n": n, "k": k, "x_mean": mean, "x_scale": scale,
           "y_mean": ym, "y_scale": ysd, "weights": W, "scores": T, "loadings": P,
           "yloadings": q, "n_test": int(test.sum())}
    sst = float(ys @ ys)
    ssy_a = q ** 2 * (T ** 2).sum(0)
    res["r2y"] = ssy_a / sst
    res["r2y_cum"] = np.cumsum(ssy_a) / sst
    ssx = float((Xs ** 2).sum())
    ssx_a = (T ** 2).sum(0) * (P ** 2).sum(0)
    res["r2x"] = ssx_a / ssx
    res["r2x_cum"] = np.cumsum(ssx_a) / ssx
    if cv_groups and cv_groups > 1 and n >= cv_groups:
        cvp = pls_cv(Xt, yt, scaling, amax, cv_groups)
        press = np.array([float(((yt - cvp[:, a]) ** 2).sum()) for a in range(amax)])
        pr0 = float(((yt - ym) ** 2).sum())
        prev = np.concatenate([[pr0], press[:-1]])
        res["press"] = press
        res["q2_cum"] = 1.0 - press / pr0
        res["q2"] = 1.0 - press / prev
        res["cv_predicted"] = cvp
        res["rmsecv"] = np.sqrt(press / n)
    else:
        res["press"] = res["q2"] = res["q2_cum"] = res["cv_predicted"] = res["rmsecv"] = None
    if ncomp is None:
        a = 0
        if res["press"] is not None:
            while a < amax and res["q2"][a] >= 0.0975:
                a += 1
        a = max(a, 1)
    else:
        a = amax
    res["ncomp"] = a
    b = pls_coef(W, P, q, a)
    res["coef_scaled"] = b
    res["coef"] = b * ysd / scale
    res["intercept"] = ym - float(mean @ res["coef"])
    fit = Xs @ b * ysd + ym
    res["fitted"] = fit
    sse = float(((yt - fit) ** 2).sum())
    res["rmsee"] = math.sqrt(sse / max(n - a - 1, 1))
    res["r2y_final"] = 1.0 - sse / float(((yt - ym) ** 2).sum())
    res["q2_final"] = None if res["q2_cum"] is None else float(res["q2_cum"][a - 1])
    # VIP: weights scaled by the share of y explained per component
    w2 = (W[:, :a] ** 2) / (W[:, :a] ** 2).sum(0)
    res["vip"] = np.sqrt(k * (w2 @ ssy_a[:a]) / ssy_a[:a].sum())
    # X-space diagnostics as for PCA
    Ta, Pa = T[:, :a], P[:, :a]
    E = Xs - Ta @ Pa.T
    s2 = (Ta ** 2).sum(0) / (n - 1)
    res["t2"] = ((Ta ** 2) / s2).sum(1)
    res["t2_limit"] = (a * (n * n - 1.0) / (n * (n - a)) * f_ppf(0.95, a, n - a)
                       if n > a else float("nan"))
    sx = float((E ** 2).sum())
    res["score_var"] = s2
    res["s0"] = math.sqrt(sx / ((n - a - 1) * (k - a))) if (k > a and n > a + 1) else float("nan")
    res["wstar"] = W[:, :a] @ np.linalg.inv(Pa.T @ W[:, :a])
    if k > a and n > a + 1 and sx > 0:
        res["dmodx"] = np.sqrt((E ** 2).sum(1) / (k - a)) / math.sqrt(sx / ((n - a - 1) * (k - a)))
        res["dmodx_limit"] = math.sqrt(f_ppf(0.95, k - a, (n - a - 1) * (k - a)))
    else:
        res["dmodx"], res["dmodx_limit"] = np.full(n, np.nan), float("nan")
    # rows kept out of the fit
    res["train_index"] = np.where(tr)[0]
    res["test_index"] = np.where(test)[0]
    if test.any():
        pt = pls_predict(res, X[test])
        res["test_predicted"] = pt
        err = y[test] - pt
        res["rmsep"] = math.sqrt(float((err ** 2).mean()))
        res["q2_ext"] = 1.0 - float((err ** 2).sum()) / float(((y[test] - ym) ** 2).sum())
    else:
        res["test_predicted"] = None
        res["rmsep"] = res["q2_ext"] = None
    if permutations:
        res["permutation"] = pls_permutation(Xt, yt, scaling, a, permutations, cv_groups, seed)
    return res


def pls_predict(model, Xnew):
    """y for new rows (columns as in the fit)."""
    return np.asarray(Xnew, float) @ model["coef"] + model["intercept"]


def pls_diagnose(model, Xnew):
    """Hotelling T2 and DModX of new rows in a PLS model, with the model's own
    scales; rows far outside the limits are outside its domain."""
    Xs = (np.asarray(Xnew, float) - model["x_mean"]) / model["x_scale"]
    a = model["ncomp"]
    t = Xs @ model["wstar"]
    E = Xs - t @ model["loadings"][:, :a].T
    k = Xs.shape[1]
    t2 = ((t ** 2) / model["score_var"]).sum(1)
    dm = np.sqrt((E ** 2).sum(1) / (k - a)) / model["s0"]
    return t2, dm


def pls_permutation(X, y, scaling, a, n_perm, cv_groups, seed):
    """Y-permutation test: R2Y and Q2Y of models of permuted y against the
    correlation of the permuted with the true y, and the intercepts of the
    regression lines at zero correlation (Eriksson et al.: R2 intercept below
    0.3-0.4 and Q2 intercept below 0.05 mean the model is not by chance)."""
    rng = np.random.RandomState(seed)
    n = len(y)
    corr = [1.0]
    r2, q2 = [], []

    def one(yy):
        Xs, mean, scale = prepare(X, scaling)
        ym, ysd = float(yy.mean()), float(yy.std(ddof=1))
        W, T, P, q = pls1_nipals(Xs, (yy - ym) / ysd, a)
        aa = W.shape[1]
        fit = Xs @ pls_coef(W, P, q, aa) * ysd + ym
        r = 1.0 - float(((yy - fit) ** 2).sum()) / float(((yy - ym) ** 2).sum())
        cvp = pls_cv(X, yy, scaling, a, cv_groups)[:, -1]
        qq = 1.0 - float(((yy - cvp) ** 2).sum()) / float(((yy - ym) ** 2).sum())
        return r, qq

    r, qq = one(y)
    r2.append(r)
    q2.append(qq)
    for _ in range(n_perm):
        yp = y[rng.permutation(n)]
        r, qq = one(yp)
        corr.append(abs(float(np.corrcoef(y, yp)[0, 1])))
        r2.append(r)
        q2.append(qq)
    c = np.array(corr)
    slope_r, icpt_r = np.polyfit(c, r2, 1)
    slope_q, icpt_q = np.polyfit(c, q2, 1)
    return {"correlation": c, "r2": np.array(r2), "q2": np.array(q2),
            "r2_intercept": float(icpt_r), "q2_intercept": float(icpt_q)}
