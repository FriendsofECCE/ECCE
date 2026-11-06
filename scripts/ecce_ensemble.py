#!/usr/bin/env python3
"""Ensemble picks, lowest-G minimum per species and benchmark tables (#836).

Wx-free; reads stored calculation folders and talks to no ECCE session.

  ecce-ensemble pick <crest calc> [n=N] [--rmsd X] [--skip-failed] [--json]
                     [--create TEMPLATE FOLDER]
  ecce-ensemble minimum <calc|folder|pattern>... [--treatment SPEC] [--json]
  ecce-ensemble benchmark <spec file> [--treatment SPEC] [--T K]
                     [--save NAME] [--dir DIR] [--json]

Run `ecce-ensemble --help` or `ecce-ensemble <subcommand> --help`.
"""
import argparse
import datetime
import glob
import importlib.machinery
import importlib.util
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
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


D = _load("ecce_displacement", "ecce-displacement")
C = _load("ecce_contacts", "ecce-contacts")
S = _load("ecce_si_export", "ecce-si-export")

DEFAULT_RMSD = 0.5          # heavy-atom RMSD (Angstrom) below which two members are one
HALIDES = {"F", "Cl", "Br", "I"}
HALIDE_H_MAX = 3.0          # halide counts as bound to an H within this distance (A)
KJ_PER_HARTREE = 4.3597447222071e-18 * 6.02214076e23 / 1000.0
R_KJ = 8.314462618e-3       # as src/tdat/reaction/EnergySums.C


def _fail(msg, code=1):
    sys.stderr.write("error: %s\n" % msg)
    sys.exit(code)


# --- pick --------------------------------------------------------------

def _confrel(calc):
    path = os.path.join(calc, "Props", "CONFREL")
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError):
        return None
    return [float(c.text) for c in root if c.tag == "step" and c.text]


def _label(els, i):
    return "%s%d" % (els[i], i + 1)


def _bond_cut(a, b):
    return C.RCOV.get(a, 1.5) + C.RCOV.get(b, 1.5) + C.BOND_TOL


def _contact_cut(a, b):
    return C.RCOV.get(a, 1.5) + C.RCOV.get(b, 1.5) + C.SHELL_TOL


def proton_counts(els, xyz):
    """H count per heavy atom: each H goes to its nearest non-H atom within
    bonding distance, so an H-bond does not move a proton."""
    heavy = [i for i, e in enumerate(els) if e != "H"]
    counts = {i: 0 for i in heavy}
    for h, e in enumerate(els):
        if e != "H":
            continue
        best, bd = None, None
        for i in heavy:
            d = D._dist(xyz[h], xyz[i])
            if d <= _bond_cut("H", els[i]) and (bd is None or d < bd):
                best, bd = i, d
        if best is not None:
            counts[best] += 1
    return counts


def ion_contacts(els, xyz, i):
    """Atoms binding ion i: donors within r_cov sum + 0.5 A for a metal;
    H within HALIDE_H_MAX (or a metal within the same cutoff) for a halide."""
    out = []
    for j, e in enumerate(els):
        if j == i:
            continue
        d = D._dist(xyz[i], xyz[j])
        if els[i] in C.METALS:
            if e in C.DONORS and e not in C.METALS and d <= _contact_cut(els[i], e):
                out.append((j, d))
        elif (e == "H" and d <= HALIDE_H_MAX) or (
                e in C.METALS and d <= _contact_cut(els[i], e)):
            out.append((j, d))
    return out


def ions_of(els, xyz):
    """Counter-ion candidates: metal atoms, and halides with no covalent partner."""
    out = []
    for i, e in enumerate(els):
        if e in C.METALS:
            out.append(i)
        elif e in HALIDES and not any(
                j != i and D._dist(xyz[i], xyz[j]) <= _bond_cut(e, els[j])
                for j in range(len(els))):
            out.append(i)
    return out


def metal_donors(els, xyz):
    """Metal - N/O/S contacts as {(metal index, donor index): distance}."""
    out = {}
    for m, e in enumerate(els):
        if e not in C.METALS:
            continue
        for j, d in ion_contacts(els, xyz, m):
            if els[j] in ("N", "O", "S"):
                out[(m, j)] = d
    return out


def check_member(els, ref_xyz, xyz, ref_prot, ref_ions, ref_md):
    prot = proton_counts(els, xyz)
    changed = [{"atom": _label(els, i), "input": ref_prot[i], "member": prot[i]}
               for i in sorted(ref_prot) if prot[i] != ref_prot[i]]
    ions, lost = [], []
    for i, n0 in ref_ions.items():
        n = len(ion_contacts(els, xyz, i))
        bound = n > 0
        ions.append({"ion": _label(els, i), "contacts_input": n0,
                     "contacts": n, "bound": bound})
        if not bound:
            lost.append(_label(els, i))
    md = metal_donors(els, xyz)
    flags = []
    if changed:
        flags.append("protonation changed: " + ", ".join(c["atom"] for c in changed))
    if lost:
        flags.append("counter-ion no longer bound: " + ", ".join(lost))
    return {
        "protonation_ok": not changed, "changed_protons": changed, "ions": ions,
        "metal_contacts": [{"metal": _label(els, m), "donor": _label(els, j),
                            "distance": round(d, 3)} for (m, j), d in sorted(md.items())],
        "metal_contacts_lost": [_label(els, m) + "-" + _label(els, j)
                                for (m, j) in sorted(ref_md) if (m, j) not in md],
        "metal_contacts_gained": [_label(els, m) + "-" + _label(els, j)
                                  for (m, j) in sorted(md) if (m, j) not in ref_md],
        "ok": not flags, "flags": flags}


def pick(calc, n, thr, skip_failed):
    calc = calc.rstrip("/")
    try:
        els, frames = D.read_geomtrace(calc)
    except (ValueError, OSError, ET.ParseError) as e:
        _fail(str(e))
    rel = _confrel(calc)
    if not rel or len(rel) != len(frames):
        _fail("%s is not a conformer ensemble (Props/CONFREL must have one value "
              "per GEOMTRACE frame)" % calc)
    if any(e is None for e in els):
        _fail("cannot read the elements of %s" % calc)
    try:
        cels, cxyz, charge = S._chemsys(calc)
    except (OSError, ValueError):
        _fail("%s has no Parameters/chemsys.mvm to check the members against" % calc)
    if cels != els:
        _fail("the input structure and the ensemble have different atoms")
    ref_prot = proton_counts(els, cxyz)
    ref_ions = {i: len(ion_contacts(els, cxyz, i)) for i in ions_of(els, cxyz)}
    ref_ions = {i: k for i, k in ref_ions.items() if k > 0}   # bound in the input
    ref_md = metal_donors(els, cxyz)
    heavy = [i for i, e in enumerate(els) if e != "H"]
    sub = lambda f: [f[i] for i in heavy]
    order = sorted(range(len(frames)), key=lambda k: (rel[k], k))
    prots = [proton_counts(els, f) for f in frames]
    picks, skipped = [], []
    for k in order:
        if len(picks) >= n:
            break
        dup = None
        for p in picks:
            if prots[k] != prots[p["_k"]]:   # a proton moved: never the same conformer
                continue
            r, _ = D.kabsch(sub(frames[p["_k"]]), sub(frames[k]))
            if r < thr:
                dup = p
                break
        if dup:
            dup["merged"].append(k + 1)
            continue
        chk = check_member(els, cxyz, frames[k], ref_prot, ref_ions, ref_md)
        if skip_failed and not chk["ok"]:
            skipped.append({"member": k + 1, "rel_kcal_mol": rel[k], "flags": chk["flags"]})
            continue
        rec = {"_k": k, "rank": len(picks) + 1, "member": k + 1,
               "rel_kcal_mol": rel[k], "merged": []}
        rec.update(chk)
        picks.append(rec)
    for p in picks:
        r, _ = D.kabsch(sub(frames[picks[0]["_k"]]), sub(frames[p["_k"]]))
        p["heavy_rmsd_to_lowest"] = round(r, 4)
    return {"calculation": calc, "members": len(frames), "rmsd_threshold": thr,
            "charge": charge, "elements": els,
            "frames": [frames[p["_k"]] for p in picks],
            "picks": [{k: v for k, v in p.items() if k != "_k"} for p in picks],
            "skipped": skipped}


def create_calcs(res, template, folder):
    """One calculation per pick, from the setup of TEMPLATE, via ecce-series."""
    exe = None
    for cand in (os.path.join(_here, "ecce-series"), shutil.which("ecce-series")):
        if cand and os.path.isfile(cand):
            exe = cand
            break
    if not exe:
        return [{"error": "ecce-series is not installed"}]
    base = re.sub(r"[^\w.-]+", "_", os.path.basename(res["calculation"]))
    mult = S.MULT.get(S._meta(os.path.join(res["calculation"], ".ecce-meta"),
                              "spinmultiplicity").lower(), 1)
    out = []
    with tempfile.TemporaryDirectory(prefix="ensemble-") as d:
        files = []
        for p, fr in zip(res["picks"], res["frames"]):
            f = os.path.join(d, "%s_conf%d.xyz" % (base, p["rank"]))
            with open(f, "w") as fh:
                fh.write("%d\ncharge=%d mult=%d rel=%.3f kcal/mol member %d\n" % (
                    len(fr), res["charge"], mult, p["rel_kcal_mol"], p["member"]))
                for e, c in zip(res["elements"], fr):
                    fh.write("%s %.8f %.8f %.8f\n" % ((e,) + tuple(c)))
            files.append(f)
        r = subprocess.run([exe, template, folder] + files, capture_output=True, text=True)
        out = [{"output": ln} for ln in r.stdout.splitlines()]
        if r.returncode:
            out.append({"error": (r.stderr.strip() or "ecce-series failed (%d)" % r.returncode)})
    return out


def cmd_pick(a):
    n = a.n if a.n else 5
    res = pick(a.calc, n, a.rmsd, a.skip_failed)
    made = create_calcs(res, a.create[0], a.create[1]) if a.create else None
    if a.json:
        out = {k: v for k, v in res.items() if k not in ("frames", "elements")}
        if made is not None:
            out["create"] = made
        print(json.dumps(out, indent=2))
    else:
        print("%s: %d members, %d distinct picks (heavy-atom RMSD >= %.2f A)" % (
            res["calculation"], res["members"], len(res["picks"]), res["rmsd_threshold"]))
        for p in res["picks"]:
            print("  #%d member %d  %.3f kcal/mol  RMSD to #1 %.3f  %s%s" % (
                p["rank"], p["member"], p["rel_kcal_mol"], p["heavy_rmsd_to_lowest"],
                "ok" if p["ok"] else "CHECK: " + "; ".join(p["flags"]),
                ("  (merged: %s)" % ",".join(map(str, p["merged"]))) if p["merged"] else ""))
            for i in p["ions"]:
                print("      ion %s: %d contacts (input %d)%s" % (
                    i["ion"], i["contacts"], i["contacts_input"],
                    "" if i["bound"] else "  NOT BOUND"))
            if p["metal_contacts"]:
                print("      metal contacts: " + ", ".join(
                    "%s-%s %.2f" % (m["metal"], m["donor"], m["distance"])
                    for m in p["metal_contacts"]))
            for k, w in (("metal_contacts_lost", "lost"), ("metal_contacts_gained", "gained")):
                if p[k]:
                    print("      metal contacts %s vs input: %s" % (w, ", ".join(p[k])))
        for s in res["skipped"]:
            print("  skipped member %d (%.3f kcal/mol): %s" % (
                s["member"], s["rel_kcal_mol"], "; ".join(s["flags"])))
        for m in made or []:
            print("  " + (m.get("output") or "error: " + m["error"]))
    if made and any("error" in m for m in made):
        return 1
    return 0 if res["picks"] else 1


# --- minimum -----------------------------------------------------------

def _thermo_exe():
    for cand in (os.environ.get("ECCE_THERMO"),
                 os.path.join(os.environ.get("ECCE_HOME", ""), "bin", "ecce-thermo"),
                 os.path.join(_here, "ecce-thermo"), shutil.which("ecce-thermo")):
        if cand and os.path.isfile(cand):
            return cand
    return "ecce-thermo"


def _gibbs(calc, treatment):
    if treatment:
        os.environ.setdefault("ECCE_THERMO", _thermo_exe())
        env_home = os.environ.get("ECCE_HOME")
        g = S.thermo_G(calc, treatment)
        if env_home is None:
            os.environ.pop("ECCE_HOME", None)
        return g
    t = S._value_text(os.path.join(calc, "Props", "EGIBBS"))
    return float(t) if t else None


def expand(args):
    paths = []
    for a in args:
        paths += sorted(glob.glob(a)) if re.search(r"[*?\[]", a) else [a]
    out = []
    for p in S.expand_calcs(paths):
        if S.is_calc(p) and p not in out:
            out.append(p)
    return out


def method_key(calc):
    meta = os.path.join(calc, ".ecce-meta")
    basis = S._meta(os.path.join(calc, "Parameters", ".ecce-meta"), "name",
                    "BasisSet.ecce_basisset")
    solv = S._value_text(os.path.join(calc, "Props", "SOLVMODEL")) or "-"
    return " / ".join([S._meta(meta, "application") or "-",
                       S._meta(meta, "theory") or "-", basis or "-", solv])


def minimum(paths, treatment):
    """-> dict: chosen record or None, rows for every calculation."""
    rows = []
    for c in expand(paths):
        c = c.rstrip("/")
        r = {"name": os.path.basename(c), "path": os.path.abspath(c),
             "method": method_key(c), "imaginary_modes": None, "G_Eh": None}
        vf = S._value_text(os.path.join(c, "Props", "VIBFREQ"))
        if vf is not None:
            r["imaginary_modes"] = S.mode_counts(vf.split())[1]
        r["G_Eh"] = _gibbs(c, treatment)
        if r["G_Eh"] is None:
            r["status"] = "excluded: no G"
        elif r["imaginary_modes"] is None:
            r["status"] = "excluded: no frequencies"
        elif r["imaginary_modes"] > 0:
            r["status"] = "excluded: %d imaginary mode(s)" % r["imaginary_modes"]
        else:
            r["status"] = "minimum"
        rows.append(r)
    ok = [r for r in rows if r["status"] == "minimum"]
    chosen = min(ok, key=lambda r: r["G_Eh"]) if ok else None
    for r in rows:
        r["dG_kJ_mol"] = None if r["G_Eh"] is None or not chosen else \
            (r["G_Eh"] - chosen["G_Eh"]) * KJ_PER_HARTREE
    rows.sort(key=lambda r: (r["G_Eh"] is None, r["G_Eh"] or 0))
    methods = sorted({r["method"] for r in ok})
    return {"chosen": chosen, "rows": rows, "treatment": treatment or "stored EGIBBS",
            "mixed_methods": methods if len(methods) > 1 else []}


def cmd_minimum(a):
    res = minimum(a.paths, a.treatment)
    if not res["rows"]:
        _fail("no calculation found")
    if res["mixed_methods"]:
        res["chosen"] = None
    if a.json:
        print(json.dumps(res, indent=2))
    else:
        print("G from: %s" % res["treatment"])
        for r in res["rows"]:
            print("  %-30s %s  %s%s" % (
                r["name"], "%.6f Eh" % r["G_Eh"] if r["G_Eh"] is not None else "no G       ",
                r["status"], "" if r["dG_kJ_mol"] is None else
                "  (+%.2f kJ/mol)" % r["dG_kJ_mol"]))
        if res["mixed_methods"]:
            print("refused: the calculations differ in method (never compared): "
                  + "; ".join(res["mixed_methods"]))
        elif res["chosen"]:
            print("lowest-G minimum: %s" % res["chosen"]["name"])
        else:
            print("no minimum with a Gibbs energy among these calculations")
    return 0 if res["chosen"] else 1


# --- benchmark ---------------------------------------------------------

def parse_spec(path):
    """-> (set name, {species: [paths]}, [reaction dicts])."""
    base = os.path.dirname(os.path.abspath(path))
    name = re.sub(r"\.[^.]*$", "", os.path.basename(path))
    species, rxns = {}, []
    for n, ln in enumerate(open(path), 1):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        key, _, rest = ln.partition(":")
        key = key.strip().lower()
        if key == "set":
            name = rest.strip()
        elif key == "species":
            lab, eq, val = rest.partition("=")
            if not eq or not lab.strip() or not val.split():
                _fail("%s:%d: species needs 'species: LABEL = PATH [PATH...]'" % (path, n))
            species[lab.strip()] = [p if os.path.isabs(p) else os.path.join(base, p)
                                    for p in val.split()]
        elif key == "reaction":
            f = [x.strip() for x in rest.split("|")]
            if len(f) < 2 or "->" not in f[1]:
                _fail("%s:%d: reaction needs 'reaction: LABEL | LHS -> RHS | exp log K | source'"
                      % (path, n))
            exp = None
            if len(f) > 2 and f[2]:
                try:
                    exp = float(f[2])
                except ValueError:
                    _fail("%s:%d: experimental log K is not a number: %s" % (path, n, f[2]))
            rxns.append({"label": f[0], "text": f[1], "exp_logK": exp,
                         "source": f[3] if len(f) > 3 else ""})
        else:
            _fail("%s:%d: unknown line (set:, species:, reaction:, # comment)" % (path, n))
    return name, species, rxns


def parse_side(side, species):
    terms = []
    for item in side.split(","):
        item = item.strip()
        if not item:
            continue
        m = re.match(r"^(?:(\d+)\s*\*\s*)?(.+)$", item)
        lab = m.group(2).strip()
        if lab not in species:
            _fail("reaction names species '%s' with no species: line" % lab)
        terms.append((lab, int(m.group(1) or 1)))
    return terms


def reaction_dG(chosen, lhs, rhs, treatment, T):
    """Delta G (Eh) with the 1 M standard state from ecce-thermo --reaction."""
    names = [chosen[l]["name"] for l, _ in lhs + rhs]
    if len({chosen[l]["path"] for l, _ in lhs + rhs}) != len(set(names)):
        _fail("two species resolve to calculations with the same name")
    side = lambda ts: ", ".join("%d*%s" % (c, chosen[l]["name"]) for l, c in ts)
    paths = sorted({chosen[l]["path"] for l, _ in lhs + rhs})
    cmd = [_thermo_exe()] + ["file://" + p for p in paths] + [
        "--reaction", "%s -> %s" % (side(lhs), side(rhs)), "--standard-state", "1M",
        "--treatment", treatment, "--T", str(T), "--json"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    i = r.stdout.find("{")
    if r.returncode or i < 0:
        return None, (r.stdout.strip() or r.stderr.strip() or "ecce-thermo failed")
    return json.loads(r.stdout[i:])["deltaG"][treatment], ""


def benchmark(spec, treatment, T):
    name, species, rxns = parse_spec(spec)
    chosen, species_out = {}, {}
    for lab, paths in species.items():
        m = minimum(paths, treatment)
        species_out[lab] = {"candidates": len(m["rows"]), "chosen": m["chosen"] and m["chosen"]["name"],
                            "excluded": [{"name": r["name"], "status": r["status"]}
                                         for r in m["rows"] if r["status"] != "minimum"],
                            "mixed_methods": m["mixed_methods"]}
        if m["chosen"] and not m["mixed_methods"]:
            chosen[lab] = m["chosen"]
    out = []
    for r in rxns:
        lhs_t, _, rhs_t = r["text"].partition("->")
        rec = {"label": r["label"], "reaction": r["text"], "exp_logK": r["exp_logK"],
               "source": r["source"]}
        lhs, rhs = parse_side(lhs_t, species), parse_side(rhs_t, species)
        miss = [l for l, _ in lhs + rhs if l not in chosen]
        if miss:
            rec["status"] = "not computed: no usable minimum for " + ", ".join(sorted(set(miss)))
        else:
            dg, why = reaction_dG(chosen, lhs, rhs, treatment, T)
            if dg is None:
                rec["status"] = "not computed: " + why
            else:
                dgk = dg * KJ_PER_HARTREE
                rec.update(status="ok", species={l: chosen[l]["name"] for l, _ in lhs + rhs},
                           deltaG_kJ_mol=round(dgk, 3),
                           logK_calc=round(-dgk / (R_KJ * T * math.log(10)), 3))
                if r["exp_logK"] is not None:
                    rec["calc_minus_exp"] = round(rec["logK_calc"] - r["exp_logK"], 3)
        out.append(rec)
    errs = [r["calc_minus_exp"] for r in out if "calc_minus_exp" in r]
    summary = {"reactions": len(out), "with_experiment": len(errs)}
    if errs:
        summary.update(mean_signed_error=round(sum(errs) / len(errs), 3),
                       mean_absolute_error=round(sum(map(abs, errs)) / len(errs), 3),
                       rmse=round(math.sqrt(sum(e * e for e in errs) / len(errs)), 3))
    return {"format": "ecce-benchmark-1", "name": name, "spec": os.path.abspath(spec),
            "created": datetime.datetime.now().isoformat(timespec="seconds"),
            "T": T, "treatment": treatment, "standard_state": "1M",
            "species": species_out, "reactions": out, "summary": summary}


def cmd_benchmark(a):
    res = benchmark(a.spec, a.treatment, a.T)
    saved = None
    if a.save:
        d = a.dir or os.path.dirname(os.path.abspath(a.spec))
        saved = os.path.join(d, re.sub(r"[^\w.-]+", "_", a.save) + ".benchmark.json")
        res["name"] = a.save
        with open(saved, "w") as f:
            json.dump(res, f, indent=2)
            f.write("\n")
    if a.json:
        print(json.dumps(res, indent=2))
    else:
        print("benchmark %s: %s, 1 M standard state, T = %.2f K" % (
            res["name"], res["treatment"], res["T"]))
        for lab, s in res["species"].items():
            print("  species %-12s %s%s" % (lab, s["chosen"] or "NO MINIMUM",
                  "".join("; excluded %s (%s)" % (e["name"], e["status"]) for e in s["excluded"])))
        print("  %-18s %10s %9s %9s %9s  %s" % ("reaction", "dG kJ/mol", "logK calc",
                                                "logK exp", "calc-exp", "source"))
        for r in res["reactions"]:
            f = lambda k, w: ("%*.2f" % (w, r[k])) if r.get(k) is not None else "%*s" % (w, "-")
            print("  %-18s %s %s %s %s  %s" % (r["label"][:18], f("deltaG_kJ_mol", 10),
                  f("logK_calc", 9), f("exp_logK", 9), f("calc_minus_exp", 9),
                  r["source"] if r["status"] == "ok" else r["status"]))
        s = res["summary"]
        if "rmse" in s:
            print("  n=%d  mean signed %.2f  MAE %.2f  RMSE %.2f" % (
                s["with_experiment"], s["mean_signed_error"], s["mean_absolute_error"], s["rmse"]))
    if saved and not a.json:
        print("saved: " + saved)
    return 0 if all(r["status"] == "ok" for r in res["reactions"]) else 1


# --- command line ------------------------------------------------------

EPILOG = """\
pick: the N (default 5) lowest members of a CREST/xTB ensemble calculation
(Props/CONFREL and Props/GEOMTRACE), lowest first. A member is merged into a
better one with the same protonation pattern when the heavy-atom RMSD after a
proper-rotation fit is below --rmsd (default %.2f A); atoms are compared in the stored order, so symmetry-
equivalent atoms are not permuted. Each pick is checked against the input
structure (Parameters/chemsys.mvm): protonation (H count per heavy atom, each H
on its nearest atom within r_cov sum + 0.4 A) must be unchanged; every metal
and every halide that is bound in the input (metal: a donor atom within r_cov
sum + 0.5 A; halide: an H within %.1f A or a metal within r_cov sum + 0.5 A)
must still be; metal-N/O/S contacts (same cutoff as the metal) are listed
with distances and compared with the input. A failed check is reported in
"flags"; --skip-failed takes the next distinct member instead.
--create TEMPLATE FOLDER makes one calculation per pick with ecce-series
(setup of the template calculation: code, method, basis, solvent, machine,
cores, wall), named <ensemble>_conf<rank>; nothing is launched.
minimum: the lowest-G calculation without imaginary modes (|f| < 1 cm-1 counts
as zero, as in the Checks table) among the given calculations, folders (all
calculations below) or patterns. G is the stored EGIBBS, or with --treatment
SPEC (as ecce-thermo, e.g. qrrho-grimme:100) recomputed by ecce-thermo.
Calculations with imaginary modes, without frequencies or without G are
listed and excluded. Calculations that differ in code, theory, basis or
solvent are never compared: the command refuses.
benchmark: SPEC FILE lines (# comment, blank lines ignored):
  set: NAME
  species: LABEL = PATH [PATH ...]    calculation, folder or pattern; relative
                                      to the spec file; the lowest-G minimum is used
  reaction: LABEL | LHS -> RHS | EXP_LOGK | SOURCE      e.g.
  reaction: ML2 | M, 2*L -> ML2 | 5.40 | Smith 1999, Table 2
Sides are comma-separated species with optional N* coefficients, as Compare
Energies. dG is ecce-thermo's with the 1 M standard state (dn*RT*ln(24.465)
at 298.15 K) and --treatment (default qrrho-grimme:100); log K = -dG/(RT ln 10);
calc - exp is log K calc minus EXP_LOGK. EXP_LOGK and SOURCE may be empty.
--save NAME writes NAME.benchmark.json next to the spec file (or in --dir):
JSON {format "ecce-benchmark-1", name, spec, created, T, treatment,
standard_state, species{label: chosen, excluded}, reactions[{label, reaction,
status, species, deltaG_kJ_mol, logK_calc, exp_logK, source, calc_minus_exp}],
summary{n, mean_signed_error, mean_absolute_error, rmse}}.
Exit codes: 0 done; 1 something was not done (no pick, no minimum, a reaction
not computed, a failed ecce-series); 2 usage error. --json prints JSON only.
""" % (DEFAULT_RMSD, HALIDE_H_MAX)


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="ecce-ensemble", formatter_class=argparse.RawDescriptionHelpFormatter,
        description="Ensemble picks, lowest-G minimum per species, benchmark tables.",
        epilog=EPILOG)
    sub = ap.add_subparsers(dest="cmd")
    p = sub.add_parser("pick", help="N lowest distinct members of an ensemble, checked")
    p.add_argument("calc")
    p.add_argument("opts", nargs="*", help="n=N")
    p.add_argument("--n", type=int)
    p.add_argument("--rmsd", type=float, default=DEFAULT_RMSD)
    p.add_argument("--skip-failed", action="store_true")
    p.add_argument("--create", nargs=2, metavar=("TEMPLATE", "FOLDER"))
    p.add_argument("--json", action="store_true")
    m = sub.add_parser("minimum", help="lowest-G calculation without imaginary modes")
    m.add_argument("paths", nargs="+")
    m.add_argument("--treatment")
    m.add_argument("--json", action="store_true")
    b = sub.add_parser("benchmark", help="log K against experiment")
    b.add_argument("spec")
    b.add_argument("--treatment", default="qrrho-grimme:100")
    b.add_argument("--T", type=float, default=298.15)
    b.add_argument("--save")
    b.add_argument("--dir")
    b.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    if not a.cmd:
        ap.print_help()
        return 2
    if a.cmd == "pick":
        for o in a.opts:
            mm = re.match(r"^n=(\d+)$", o)
            if not mm:
                ap.error("unknown argument %s (n=N)" % o)
            a.n = int(mm.group(1))
        if a.n is not None and a.n < 1:
            ap.error("n must be at least 1")
        return cmd_pick(a)
    return cmd_minimum(a) if a.cmd == "minimum" else cmd_benchmark(a)


if __name__ == "__main__":
    sys.exit(main())
