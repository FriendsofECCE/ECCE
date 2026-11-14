#!/usr/bin/env python3
"""QSAR tools: the descriptor table of a folder of calculations (#961).

Wx-free; reads stored calculation folders and talks to no ECCE session.

  ecce-qsar table <folder|calc>... [--groups G,...] [--no-qm] [--csv FILE] [--json]
  ecce-qsar table --from-csv FILE [--smiles-col N] [--name-col N] ...
  ecce-qsar pca TABLE.csv [--y COL]... [--exclude COL]... [--scale uv|pareto|ctr|none]
                [--components N] [--cv G] [--outdir DIR] [--json]
  ecce-qsar pls TABLE.csv --y COL [--exclude COL]... [--scale uv|pareto|ctr|none]
                [--components N] [--cv G] [--test-names A,B | --test-fraction F --seed S]
                [--permutations N] [--save-model FILE] [--outdir DIR] [--json]
  ecce-qsar predict MODEL.json TABLE.csv [--csv FILE]
  ecce-qsar columns TABLE.csv
  ecce-qsar groups

One row per calculation of a folder made by Batch Add from SMILES (the SMILES is
read from ecce:qsar.smiles); columns: name, state, measured values, descriptors
from the SMILES, and the calculation's own results (HOMO, LUMO, dipole, charges).
Calculations whose optimisation did not converge are flagged in `opt_converged`
and `problems`.  Run `ecce-qsar --help`.
"""
import argparse
import csv
import importlib.machinery
import importlib.util
import json
import math
import os
import re
import sys
import xml.etree.ElementTree as ET

_here = os.path.dirname(os.path.abspath(__file__))


def _load(module, installed):
    """The sibling script: importable in the source tree, bin/<installed> after install."""
    sys.path.insert(0, _here)
    try:
        return __import__(module)
    except ImportError:
        l = importlib.machinery.SourceFileLoader(module, os.path.join(_here, installed))
        s = importlib.util.spec_from_loader(module, l)
        m = importlib.util.module_from_spec(s)
        l.exec_module(m)
        return m
    finally:
        sys.path.pop(0)


# the libraries are modules, installed in <home>/scripts beside the parsers
sys.path.insert(0, os.path.join(os.path.dirname(_here), "scripts"))
S = _load("ecce_si_export", "ecce-si-export")
C = _load("ecce_qsar_chem", "ecce_qsar_chem.py")

HARTREE_EV = C.EV
DEBYE_STEP = 1.0


def _fail(msg, code=1):
    sys.stderr.write("error: %s\n" % msg)
    sys.exit(code)


def _floats(path):
    try:
        t = ET.parse(path).getroot().text
    except (OSError, ET.ParseError):
        return None
    try:
        return [float(x) for x in (t or "").split()]
    except ValueError:
        return None


def qm_values(calc):
    """HOMO, LUMO, gap (eV), dipole (Debye), Mulliken extremes and the total
    energy (Hartree) from Props/, each None where the calculation has none."""
    P = lambda n: os.path.join(calc, "Props", n)
    out = dict.fromkeys(C.QM_NAMES)
    te = _floats(P("TE"))
    if te:
        out["qm_E"] = te[-1]
    eng, occ = _floats(P("ORBENG")), _floats(P("ORBOCC"))
    if eng and occ and len(eng) == len(occ):
        occupied = [e for e, o in zip(eng, occ) if o > 0.5]
        empty = [e for e, o in zip(eng, occ) if o <= 0.5]
        if occupied:
            out["qm_HOMO"] = max(occupied) * HARTREE_EV
        if empty:
            out["qm_LUMO"] = min(empty) * HARTREE_EV
        if occupied and empty:
            out["qm_gap"] = out["qm_LUMO"] - out["qm_HOMO"]
    dip = _floats(P("DIPOLE"))
    if dip and len(dip) == 3:
        out["qm_dipole"] = math.sqrt(sum(x * x for x in dip))
    q = _floats(P("MULLIKEN"))
    if q:
        out["qm_qmin"], out["qm_qmax"] = min(q), max(q)
    return out


def calc_record(calc):
    """name, SMILES, measured values (JSON), checks and QM values of a calculation."""
    calc = calc.rstrip("/")
    meta = os.path.join(calc, ".ecce-meta")
    rec = {"name": os.path.basename(calc), "path": calc}
    rec["smiles"] = S._meta(meta, "qsar.smiles")
    try:
        rec["measured"] = json.loads(S._meta(meta, "qsar.measured") or "{}")
    except ValueError:
        rec["measured"] = {}
    rec["source"] = S._meta(meta, "qsar.source")
    rec["checks"] = S.read_checks(calc)
    rec["qm"] = qm_values(calc)
    return rec


def selected_columns(groups):
    names = []
    for g in groups:
        if g not in C.GROUPS:
            _fail("unknown descriptor group %r (see `ecce-qsar groups`)" % g)
        names += C.GROUPS[g]
    return names


def build_table(records, groups=None, qm=True, progress=None):
    """-> (columns, rows).  Each row is a dict; a row whose SMILES cannot be read
    keeps its measured and QM values and carries the reason in `problems`."""
    groups = list(groups) if groups is not None else list(C.GROUPS)
    dcols = selected_columns(groups)
    measured_cols = []
    for r in records:
        for k in r.get("measured", {}):
            if k not in measured_cols:
                measured_cols.append(k)
    cols = ["name", "state", "opt_converged"] + measured_cols + dcols
    if qm:
        cols += C.QM_NAMES
    cols.append("problems")
    rows = []
    for n, r in enumerate(records):
        row = dict.fromkeys(cols)
        row["name"] = r["name"]
        chk = r.get("checks") or {}
        row["state"] = chk.get("state")
        row["opt_converged"] = chk.get("opt_converged")
        probs = [p for p in (chk.get("problems") or "").split("; ") if p and p != "-"]
        for k, v in r.get("measured", {}).items():
            row[k] = v
        if not r.get("smiles"):
            probs.append("no SMILES stored")
        else:
            try:
                desc = C.all_descriptors(r["smiles"])
                for c in dcols:
                    row[c] = desc[c]
            except (C.SmilesError, KeyError, ValueError, ZeroDivisionError) as e:
                probs.append("descriptors: %s" % e)
        if qm:
            for k, v in (r.get("qm") or {}).items():
                row[k] = v
        row["problems"] = "; ".join(probs)
        rows.append(row)
        if progress:
            progress(n + 1, len(records), r["name"])
    return cols, rows


def _fmt(v):
    if v is None:
        return ""
    if isinstance(v, float):
        return repr(float(round(v, 8))) if v == v else ""
    return str(v)


def write_csv(path, cols, rows):
    out = sys.stdout if path in (None, "-") else open(path, "w", newline="")
    try:
        w = csv.writer(out)
        w.writerow(cols)
        for r in rows:
            w.writerow([_fmt(r.get(c)) for c in cols])
    finally:
        if out is not sys.stdout:
            out.close()


def records_from_csv(path, name_col, smiles_col):
    recs = []
    with open(path, newline="", errors="replace") as f:
        rd = list(csv.reader(f, delimiter="\t" if path.endswith(".tsv") else ","))
    head, body = rd[0], rd[1:]

    def ix(c, default):
        if c is None:
            return default
        return int(c) - 1 if str(c).isdigit() else head.index(c)

    ni, si = ix(name_col, 0), ix(smiles_col, 1)
    for line in body:
        if len(line) <= max(ni, si) or not line[ni].strip():
            continue
        meas = {}
        for k, (h, v) in enumerate(zip(head, line)):
            if k in (ni, si) or not v.strip():
                continue
            try:
                meas[h] = float(v)
            except ValueError:
                pass
        recs.append({"name": line[ni].strip(), "smiles": line[si].strip(),
                     "measured": meas, "source": os.path.basename(path)})
    return recs


def cmd_table(a):
    if a.from_csv:
        recs = records_from_csv(a.from_csv, a.name_col, a.smiles_col)
    else:
        if not a.calcs and not a.describe:
            _fail("give calculations, a folder or --from-csv", 64)
        paths = S.expand_calcs(a.calcs)
        if not paths and a.calcs:
            _fail("no calculations found", 2)
        recs = [calc_record(p) for p in paths]
    prog = None
    if a.progress:
        prog = lambda i, n, name: sys.stderr.write("PROGRESS %d %d %s\n" % (i, n, name))
    groups = [g for g in a.groups.split(",") if g] if a.groups else None
    qm = (not a.no_qm) if groups is None else "qm" in groups
    if groups is not None:
        groups = [g for g in groups if g != "qm"]
    cols, rows = build_table(recs, groups, qm=qm and not a.from_csv, progress=prog)
    info = []
    for col in cols:
        sources = sorted({r.get("source") or "unknown source (not recorded)"
                          for r in recs if col in r.get("measured", {})})
        info.append(C.column_definition(col, sources))
    if a.key_csv:
        with open(a.key_csv, "w", newline="", encoding="utf-8") as f:
            csv.writer(f).writerows((d["name"], C.definition_text(d)) for d in info)
    if a.describe:
        if a.json:
            json.dump({"columns": info}, sys.stdout, ensure_ascii=False)
            sys.stdout.write("\n")
        elif a.csv:
            write_csv(a.csv, ["name", "description", "unit", "group", "reference"], info)
        else:
            for d in info:
                print(d["name"] + ": " + C.definition_text(d))
        return 0
    if a.json:
        json.dump({"columns": cols, "rows": [[r.get(c) for c in cols] for r in rows]},
                  sys.stdout)
        sys.stdout.write("\n")
    if a.csv:
        write_csv(a.csv, cols, rows)
    if not a.json and not a.csv:
        write_csv("-", cols, rows)
    bad = sum(1 for r in rows if r["problems"])
    if bad:
        sys.stderr.write("%d of %d rows have problems (column `problems`)\n" % (bad, len(rows)))
    return 0


NON_X = ("name", "state", "opt_converged", "problems")


def load_table(path):
    """-> (header, rows as lists of strings)."""
    with open(path, newline="", errors="replace") as f:
        rd = list(csv.reader(f))
    if not rd:
        _fail("%s is empty" % path)
    return rd[0], rd[1:]


def _num(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return float("nan")


def _chemo():
    global CH
    try:
        CH = _load("ecce_qsar_chemo", "ecce_qsar_chemo.py")
    except ImportError as e:
        _fail("PCA and PLS need numpy (%s)" % e)
    return CH


def select_xy(head, rows, ys, exclude, skip_unconverged=True):
    """-> (kept row indices, x column names, X, y column names, Y, notes)."""
    import numpy as np
    notes = []
    for c in list(ys) + list(exclude):
        if c not in head:
            _fail("no column %r in the table" % c)
    col = {h: i for i, h in enumerate(head)}
    keep = []
    for i, r in enumerate(rows):
        if skip_unconverged and "opt_converged" in col and r[col["opt_converged"]] == "no":
            notes.append("%s left out: optimisation not converged" % r[col["name"]])
        elif ys and any(not np.isfinite(_num(r[col[y]])) for y in ys):
            notes.append("%s left out: no measured value" % r[col["name"]])
        else:
            keep.append(i)
    xnames = []
    for h in head:
        if h in NON_X or h in ys or h in exclude:
            continue
        vals = [_num(rows[i][col[h]]) for i in keep]
        if any(np.isfinite(v) for v in vals):
            xnames.append(h)
    X = np.array([[_num(rows[i][col[h]]) for h in xnames] for i in keep], float)
    Y = np.array([[_num(rows[i][col[y]]) for y in ys] for i in keep], float) if ys else None
    use, why = CH.usable_columns(X, xnames)
    for h, w in why.items():
        notes.append("column %s left out: %s" % (h, w))
    return keep, [xnames[j] for j in use], X[:, use], list(ys), Y, notes


def _jsonable(o):
    import numpy as np
    if isinstance(o, np.ndarray):
        return [None if (isinstance(x, float) and x != x) else x for x in o.tolist()] \
            if o.ndim == 1 else [_jsonable(r) for r in o]
    if isinstance(o, dict):
        return {k: _jsonable(v) for k, v in o.items()}
    if isinstance(o, (list, tuple)):
        return [_jsonable(v) for v in o]
    if isinstance(o, (np.floating, float)):
        return None if o != o else float(o)
    if isinstance(o, np.integer):
        return int(o)
    return o


def _write_rows(path, head, rows):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(head)
        for r in rows:
            w.writerow([_fmt(v) for v in r])


def _display_number(v, places=3):
    return "—" if v is None or not math.isfinite(float(v)) else ("%.*f" % (places, v))


def _at(m, key, i):
    values = m.get(key)
    return None if values is None else values[i]


def _collapsed_notes(notes, n):
    groups, other = {}, []
    for note in notes:
        if note.startswith("column ") and " left out: " in note:
            name, reason = note[7:].split(" left out: ", 1)
            if reason == "constant":
                reason = "no variance in these %d molecules" % n
        elif " left out: " in note:
            name, reason = note.split(" left out: ", 1)
        else:
            other.append(note)
            continue
        groups.setdefault(reason, []).append(name)
    return ["Left out (%s): %s" % (reason, ", ".join(names))
            for reason, names in groups.items()] + other


def _diagnostic_text(m, names):
    lines = ["95 %% limits: T² %s (far from the centre within the model plane); "
             "DModX %s (outside the model plane / far from the model)."
             % (_display_number(m["t2_limit"]), _display_number(m["dmodx_limit"]))]
    for key, label in (("t2", "T²"), ("dmodx", "DModX")):
        outside = [name for name, value in zip(names, m[key]) if value > m[key + "_limit"]]
        if outside:
            lines.append("Outside %s limit: %s" % (label, ", ".join(outside)))
    return lines


def _component_warnings(m, fixed, limit):
    lines = []
    if m["n"] < 10 * m["ncomp"]:
        lines.append("Warning: few molecules (%d) for %d components; cross-validated estimates may be unstable."
                     % (m["n"], m["ncomp"]))
    if fixed and m["q2"] is not None:
        for i in range(m["ncomp"]):
            if m["q2"][i] < limit or (limit == 0.05 and m["q2"][i] == limit):
                lines.append("Warning: component %d does not improve the cross-validated prediction "
                             "enough to meet the significance limit (Q² %s; limit %s)."
                             % (i + 1, _display_number(m["q2"][i], 2), limit))
    return lines


def _plain_table(head, rows):
    text = [[str(x) for x in head]] + [[str(x) for x in row] for row in rows]
    widths = [max(len(row[j]) for row in text) for j in range(len(head))]
    return "\n".join("  ".join(value.ljust(widths[j]) for j, value in enumerate(row))
                     for row in text)


def _present(a, summary, head, rows, vip=None):
    summary = "\n\n".join(summary) + "\n"
    if a.outdir:
        with open(os.path.join(a.outdir, "analysis_summary.txt"), "w", encoding="utf-8") as f:
            f.write(summary)
        for filename, header, data in (("analysis_components.csv", head, rows),
                                       ("analysis_vip.csv", ["Descriptor", "VIP", "Description"], vip or [])):
            with open(os.path.join(a.outdir, filename), "w", newline="", encoding="utf-8") as f:
                csv.writer(f).writerows([header] + data)
    print(summary.rstrip())
    print("\n" + _plain_table(head, rows))
    if vip:
        print("\n" + _plain_table(["Descriptor", "VIP", "Description"], vip))


def cmd_pca(a):
    _chemo()
    head, rows = load_table(a.table)
    keep, xn, X, _, _, notes = select_xy(head, rows, a.y or [], a.exclude or [],
                                         not a.keep_unconverged)
    if X.shape[0] < 4 or X.shape[1] < 2:
        _fail("need at least 4 rows and 2 usable descriptor columns (have %d x %d)"
              % X.shape)
    ni = head.index("name") if "name" in head else 0
    names = [rows[i][ni] for i in keep]
    m = CH.pca_model(X, a.scale, a.components, a.cv)
    out = {"scaling": a.scale, "n": m["n"], "k": m["k"], "ncomp": m["ncomp"],
           "eigenvalues": m["eigenvalues"], "r2x": m["r2x"], "r2x_cum": m["r2x_cum"],
           "q2": m["q2"], "q2_cum": m["q2_cum"], "t2_limit": m["t2_limit"],
           "dmodx_limit": m["dmodx_limit"], "names": names, "variables": xn,
           "scores": m["scores"], "loadings": m["loadings"], "t2": m["t2"],
           "dmodx": m["dmodx"], "notes": notes}
    A = m["ncomp"]
    if a.outdir:
        os.makedirs(a.outdir, exist_ok=True)
        _write_rows(os.path.join(a.outdir, "pca_scores.csv"),
                    ["name"] + ["t%d" % (i + 1) for i in range(A)] + ["T2", "DModX"],
                    [[names[i]] + list(m["scores"][i]) + [m["t2"][i], m["dmodx"][i]]
                     for i in range(len(names))])
        _write_rows(os.path.join(a.outdir, "pca_loadings.csv"),
                    ["variable"] + ["p%d" % (i + 1) for i in range(A)],
                    [[xn[j]] + list(m["loadings"][j]) for j in range(len(xn))])
        _write_rows(os.path.join(a.outdir, "pca_components.csv"),
                    ["component", "eigenvalue", "R2X", "R2X_cum", "Q2", "Q2_cum"],
                    [[i + 1, m["eigenvalues"][i], m["r2x"][i], m["r2x_cum"][i],
                      (m["q2"][i] if m["q2"] else None),
                      (m["q2_cum"][i] if m["q2_cum"] else None)]
                     for i in range(len(m["eigenvalues"]))])
    if a.json:
        json.dump(_jsonable(out), sys.stdout)
        sys.stdout.write("\n")
    else:
        summary = ["%d components explain %.0f %% of the descriptor variation (R²X)."
                   % (A, 100 * m["r2x_cum"][A - 1]),
                   "PCA: %d rows, %d descriptors, %s scaling, %d components"
                   % (m["n"], m["k"], a.scale, A)]
        summary += _component_warnings(m, a.components, 0.05)
        if m["q2"] is None:
            summary.append("Cross-validation is unavailable; Q² is not computed.")
        summary += ["* = component kept in the model; it does not mean significant by cross-validation. "
                    "R²X is the fraction of X variance explained; Q² measures cross-validated prediction. "
                    "Cumulative values use all components through that row."]
        summary += _diagnostic_text(m, names)
        summary += _collapsed_notes(notes, len(keep))
        headers = ["Component", "Kept (*)", "Eigenvalue", "R²X", "Cumulative R²X",
                   "X variance / %", "Cumulative X / %", "Q²", "Cumulative Q²"]
        display = [[i + 1, "*" if i < A else "", _display_number(m["eigenvalues"][i], 4),
                    _display_number(m["r2x"][i]), _display_number(m["r2x_cum"][i]),
                    _display_number(100 * m["r2x"][i], 1), _display_number(100 * m["r2x_cum"][i], 1),
                    _display_number(_at(m, "q2", i)), _display_number(_at(m, "q2_cum", i))]
                   for i in range(len(m["eigenvalues"]))]
        _present(a, summary, headers, display)
    return 0


def _model_json(m, xn, yname):
    keys = ("scaling", "n", "k", "x_mean", "x_scale", "y_mean", "y_scale", "coef",
            "intercept", "ncomp", "score_var", "s0", "wstar", "t2_limit", "dmodx_limit",
            "rmsee", "r2y_final", "q2_final", "rmsecv", "rmsep")
    d = {k: m[k] for k in keys if k in m}
    d["loadings"] = m["loadings"][:, :m["ncomp"]]
    d["variables"] = xn
    d["y"] = yname
    return _jsonable(d)


def cmd_pls(a):
    import numpy as np
    _chemo()
    head, rows = load_table(a.table)
    if len(a.y or []) != 1:
        _fail("pls needs exactly one --y column", 64)
    keep, xn, X, yn, Y, notes = select_xy(head, rows, a.y, a.exclude or [],
                                          not a.keep_unconverged)
    if X.shape[0] < 8 or X.shape[1] < 2:
        _fail("need at least 8 rows and 2 usable descriptor columns (have %d x %d)" % X.shape)
    ni = head.index("name") if "name" in head else 0
    names = [rows[i][ni] for i in keep]
    y = Y[:, 0]
    test = np.zeros(len(y), bool)
    if a.test_names:
        want = set(a.test_names.split(","))
        test = np.array([n in want for n in names])
        for w in want - set(names):
            notes.append("test molecule %s not in the table" % w)
    elif a.test_fraction:
        rng = np.random.RandomState(a.seed)
        k = max(1, int(round(a.test_fraction * len(y))))
        test[rng.permutation(len(y))[:k]] = True
    m = CH.pls_model(X, y, a.scale, a.components, a.cv, test=test,
                     permutations=a.permutations, seed=a.seed)
    A = m["ncomp"]
    tri, tei = m["train_index"], m["test_index"]
    out = {"y": yn[0], "scaling": a.scale, "n": m["n"], "n_test": m["n_test"], "k": m["k"],
           "ncomp": A, "r2y_cum": m["r2y_cum"], "r2x_cum": m["r2x_cum"], "q2": m["q2"],
           "q2_cum": m["q2_cum"], "rmsee": m["rmsee"], "rmsecv": m["rmsecv"],
           "rmsep": m["rmsep"], "q2_ext": m["q2_ext"], "t2_limit": m["t2_limit"],
           "dmodx_limit": m["dmodx_limit"], "variables": xn, "vip": m["vip"],
           "coef": m["coef"], "intercept": m["intercept"], "notes": notes,
           "names": names}
    if "permutation" in m:
        out["permutation"] = {"r2_intercept": m["permutation"]["r2_intercept"],
                              "q2_intercept": m["permutation"]["q2_intercept"],
                              "n": a.permutations}
    cvp = m["cv_predicted"][:, A - 1] if m["cv_predicted"] is not None else None
    if a.outdir:
        os.makedirs(a.outdir, exist_ok=True)
        nc = len(m["r2y"])
        _write_rows(os.path.join(a.outdir, "pls_components.csv"),
                    ["component", "R2X", "R2X_cum", "R2Y", "R2Y_cum", "Q2", "Q2_cum", "RMSECV"],
                    [[i + 1, m["r2x"][i], m["r2x_cum"][i], m["r2y"][i], m["r2y_cum"][i],
                      m["q2"][i] if m["q2"] is not None else None,
                      m["q2_cum"][i] if m["q2_cum"] is not None else None,
                      m["rmsecv"][i] if m["rmsecv"] is not None else None] for i in range(nc)])
        _write_rows(os.path.join(a.outdir, "pls_scores.csv"),
                    ["name"] + ["t%d" % (i + 1) for i in range(A)] + ["T2", "DModX"],
                    [[names[j]] + list(m["scores"][i, :A]) + [m["t2"][i], m["dmodx"][i]]
                     for i, j in enumerate(tri)])
        _write_rows(os.path.join(a.outdir, "pls_vip.csv"),
                    ["variable", "VIP", "coefficient_scaled", "coefficient"],
                    [[xn[j], m["vip"][j], m["coef_scaled"][j], m["coef"][j]]
                     for j in range(len(xn))])
        pred = []
        for i, j in enumerate(tri):
            pred.append([names[j], "train", y[j], m["fitted"][i],
                         cvp[i] if cvp is not None else None])
        for i, j in enumerate(tei):
            pred.append([names[j], "test", y[j], m["test_predicted"][i], None])
        _write_rows(os.path.join(a.outdir, "pls_predictions.csv"),
                    ["name", "set", "observed", "predicted", "cv_predicted"], pred)
        if "permutation" in m:
            pm = m["permutation"]
            _write_rows(os.path.join(a.outdir, "pls_permutation.csv"),
                        ["correlation", "R2Y", "Q2Y"],
                        [[pm["correlation"][i], pm["r2"][i], pm["q2"][i]]
                         for i in range(len(pm["r2"]))])
    if a.save_model:
        with open(a.save_model, "w") as f:
            json.dump(_model_json(m, xn, yn[0]), f)
    if a.json:
        json.dump(_jsonable(out), sys.stdout)
        sys.stdout.write("\n")
        return 0
    measured = re.match(r"^(.*?)\s*[\[(]([^\[\]()]+)[\])]\s*$", yn[0])
    unit = measured.group(2) if measured else C.column_definition(yn[0])["unit"]
    error_unit = " " + unit if unit != "unspecified" else " (measured value's unit; unspecified)"
    response = measured.group(1).strip() if measured else yn[0]
    summary = ["%d components explain %.0f %% of the variation in %s (R²Y)"
               % (A, 100 * m["r2y_final"], response)]
    if m["q2_final"] is not None:
        if m["q2_final"] >= 0:
            summary[0] += " and predict %.0f %% of it for molecules left out (Q²)." % (100 * m["q2_final"])
        else:
            summary[0] += "; cross-validation gives no useful prediction (Q² %.3f)." % m["q2_final"]
            summary.append("Warning: negative Q² means predictions for molecules left out are worse than using the training mean.")
    else:
        summary[0] += ". Cross-validation is unavailable."
    summary[0] += " Typical error: %.1f%s on the molecules fitted (RMSEE)" % (m["rmsee"], error_unit)
    if m["rmsecv"] is not None:
        summary[0] += ", %.1f%s on molecules left out (RMSECV)." % (m["rmsecv"][A - 1], error_unit)
        ratio = m["rmsecv"][A - 1] / max(m["rmsee"], 1e-15)
        if ratio >= 2:
            summary.append("Warning: the model fits much better than it predicts (RMSECV / RMSEE %.1f×)%s."
                           % (ratio, ": few molecules for this many descriptors" if m["k"] >= m["n"] else ""))
    else:
        summary[0] += "."
    summary += ["PLS of %s: %d training rows%s, %d descriptors, %s scaling, %d components"
                % (yn[0], m["n"], " (+%d test)" % m["n_test"] if m["n_test"] else "",
                   m["k"], a.scale, A)]
    summary += _component_warnings(m, a.components, 0.0975)
    summary += ["* = component kept in the model; it does not mean significant by cross-validation. "
                "R²X and R²Y are the fractions of descriptor and measured variation explained; "
                "Q² measures cross-validated prediction. Cumulative values use all components through that row. "
                "RMSEE is the fitted error (N − A − 1 degrees of freedom); RMSECV is the error for molecules left out. "
                "Both errors use the measured value's unit.",
                "VIP > 1 = more important than average. The ten largest VIP values are listed, in descending order."]
    if m["n_test"]:
        summary.append("Test set: %d molecules; RMSEP %.3g%s; external Q² %.3f."
                       % (m["n_test"], m["rmsep"], error_unit, m["q2_ext"]))
    if "permutation" in m:
        summary.append("Permutation (%d): R²Y intercept %.3f, Q² intercept %.3f."
                       % (a.permutations, m["permutation"]["r2_intercept"], m["permutation"]["q2_intercept"]))
    summary += _diagnostic_text(m, [names[j] for j in tri])
    summary += _collapsed_notes(notes, len(keep))
    headers = ["Component", "Kept (*)", "R²X", "Cumulative R²X", "X variance / %", "Cumulative X / %",
               "R²Y", "Cumulative R²Y", "Q²", "Cumulative Q²", "RMSEE / " + unit, "RMSECV / " + unit]
    ssy = float(((y[tri] - m["y_mean"]) ** 2).sum())
    display = [[i + 1, "*" if i < A else "", _display_number(m["r2x"][i]),
                _display_number(m["r2x_cum"][i]), _display_number(100 * m["r2x"][i], 1),
                _display_number(100 * m["r2x_cum"][i], 1), _display_number(m["r2y"][i]),
                _display_number(m["r2y_cum"][i]), _display_number(_at(m, "q2", i)),
                _display_number(_at(m, "q2_cum", i)),
                _display_number(math.sqrt(max(0, ssy * (1 - m["r2y_cum"][i])) / max(m["n"] - i - 2, 1)), 2),
                _display_number(_at(m, "rmsecv", i), 2)] for i in range(len(m["r2y"]))]
    vip = [[xn[j], _display_number(m["vip"][j], 2), C.column_definition(xn[j])["description"]]
           for j in np.argsort(-m["vip"])[:10]]
    _present(a, summary, headers, display, vip)
    return 0


def cmd_predict(a):
    import numpy as np
    _chemo()
    model = json.load(open(a.model))
    m = {k: (np.array(v) if isinstance(v, list) else v) for k, v in model.items()
         if k != "variables"}
    head, rows = load_table(a.table)
    col = {h: i for i, h in enumerate(head)}
    miss = [v for v in model["variables"] if v not in col]
    if miss:
        _fail("the table lacks %d descriptor columns of the model, e.g. %s" % (len(miss), miss[0]))
    ni = col.get("name", 0)
    X = np.array([[_num(r[col[v]]) for v in model["variables"]] for r in rows], float)
    ok = np.all(np.isfinite(X), axis=1)
    pred = np.full(len(rows), np.nan)
    t2 = np.full(len(rows), np.nan)
    dm = np.full(len(rows), np.nan)
    if ok.any():
        pred[ok] = CH.pls_predict(m, X[ok])
        t2[ok], dm[ok] = CH.pls_diagnose(m, X[ok])
    head_out = ["name", "predicted_" + model["y"], "T2", "DModX", "in_domain"]
    out = []
    for i, r in enumerate(rows):
        dom = "" if not ok[i] else ("yes" if t2[i] <= model["t2_limit"] * 1.0 and
                                    dm[i] <= model["dmodx_limit"] else "no")
        out.append([r[ni], pred[i], t2[i], dm[i], dom or "missing descriptors"])
    _write_rows(a.csv or "/dev/stdout", head_out, out)
    return 0


def measured_columns(head):
    """Columns that are neither descriptors nor bookkeeping: the measured values."""
    known = set(NON_X) | set(C.QM_NAMES)
    for names in C.GROUPS.values():
        known.update(names)
    return [h for h in head if h not in known]


def cmd_columns(a):
    head, _ = load_table(a.table)
    for h in measured_columns(head):
        print(h)
    return 0


def cmd_groups(a):
    for g, names in C.GROUPS.items():
        print("%-9s %d: %s" % (g, len(names), ", ".join(names)))
    print("%-9s %d: %s" % ("qm", len(C.QM_NAMES), ", ".join(C.QM_NAMES)))
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(prog="ecce-qsar", description=__doc__.split("\n")[0])
    sub = p.add_subparsers(dest="cmd")
    t = sub.add_parser("table", help="descriptor table of calculations or of a CSV")
    t.add_argument("calcs", nargs="*")
    t.add_argument("--groups", help="comma list of %s,qm (default: all)" % ",".join(C.GROUPS))
    t.add_argument("--no-qm", action="store_true", help="leave out the results of the calculations")
    t.add_argument("--csv", help="write the table here (- for stdout)")
    t.add_argument("--json", action="store_true")
    t.add_argument("--describe", action="store_true", help="describe current columns, or all built-in columns without input; --json gives structured definitions")
    t.add_argument("--key-csv", help="also write column names and their key text as UTF-8 CSV")
    t.add_argument("--progress", action="store_true", help="PROGRESS lines on stderr")
    t.add_argument("--from-csv", help="a table of names and SMILES instead of calculations")
    t.add_argument("--name-col")
    t.add_argument("--smiles-col")
    t.set_defaults(fn=cmd_table)
    pc = sub.add_parser("pca", help="principal component analysis of a descriptor table")
    pc.add_argument("table")
    pc.add_argument("--y", action="append", help="a measured column, not a descriptor (repeatable)")
    pc.add_argument("--exclude", action="append", help="a column to leave out (repeatable)")
    pc.add_argument("--scale", default="uv", choices=("uv", "pareto", "ctr", "none"))
    pc.add_argument("--components", type=int, help="fixed number (default: by cross-validation)")
    pc.add_argument("--cv", type=int, default=7, help="cross-validation groups (0: none)")
    pc.add_argument("--keep-unconverged", action="store_true")
    pc.add_argument("--outdir", help="write pca_scores.csv, pca_loadings.csv, pca_components.csv")
    pc.add_argument("--json", action="store_true")
    pc.set_defaults(fn=cmd_pca)
    pl = sub.add_parser("pls", help="PLS regression of one measured value on the descriptors")
    pl.add_argument("table")
    pl.add_argument("--y", action="append", help="the measured column to model")
    pl.add_argument("--exclude", action="append")
    pl.add_argument("--scale", default="uv", choices=("uv", "pareto", "ctr", "none"))
    pl.add_argument("--components", type=int)
    pl.add_argument("--cv", type=int, default=7)
    pl.add_argument("--test-names", help="comma list of molecules kept out of the fit")
    pl.add_argument("--test-fraction", type=float, help="random share kept out of the fit")
    pl.add_argument("--seed", type=int, default=1)
    pl.add_argument("--permutations", type=int, default=0, help="Y-permutation runs (0: none)")
    pl.add_argument("--keep-unconverged", action="store_true")
    pl.add_argument("--save-model", help="write the model for `ecce-qsar predict`")
    pl.add_argument("--outdir")
    pl.add_argument("--json", action="store_true")
    pl.set_defaults(fn=cmd_pls)
    pr = sub.add_parser("predict", help="predict a saved PLS model on a descriptor table")
    pr.add_argument("model")
    pr.add_argument("table")
    pr.add_argument("--csv")
    pr.set_defaults(fn=cmd_predict)
    cc = sub.add_parser("columns", help="the measured-value columns of a table")
    cc.add_argument("table")
    cc.set_defaults(fn=cmd_columns)
    g = sub.add_parser("groups", help="list the descriptor groups")
    g.set_defaults(fn=cmd_groups)
    a = p.parse_args(argv)
    if not getattr(a, "fn", None):
        p.print_help()
        return 64
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
