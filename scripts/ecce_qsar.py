#!/usr/bin/env python3
"""QSAR tools: the descriptor table of a folder of calculations (#961).

Wx-free; reads stored calculation folders and talks to no ECCE session.

  ecce-qsar table <folder|calc>... [--groups G,...] [--no-qm] [--csv FILE] [--json]
  ecce-qsar table --from-csv FILE [--smiles-col N] [--name-col N] ...
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
    groups = list(groups) if groups else list(C.GROUPS)
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
        return repr(round(v, 8)) if v == v else ""
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
                     "measured": meas})
    return recs


def cmd_table(a):
    if a.from_csv:
        recs = records_from_csv(a.from_csv, a.name_col, a.smiles_col)
    else:
        if not a.calcs:
            _fail("give calculations, a folder or --from-csv", 64)
        paths = S.expand_calcs(a.calcs)
        if not paths:
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
    t.add_argument("--progress", action="store_true", help="PROGRESS lines on stderr")
    t.add_argument("--from-csv", help="a table of names and SMILES instead of calculations")
    t.add_argument("--name-col")
    t.add_argument("--smiles-col")
    t.set_defaults(fn=cmd_table)
    g = sub.add_parser("groups", help="list the descriptor groups")
    g.set_defaults(fn=cmd_groups)
    a = p.parse_args(argv)
    if not getattr(a, "fn", None):
        p.print_help()
        return 64
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
