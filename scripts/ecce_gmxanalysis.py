#!/usr/bin/env python3
"""ecce-gmxanalysis (#877): GROMACS analysis tools on a stored GROMACS calculation.

Runs gmx on the calculation's traj.xtc, topol.tpr and ener.edr (in its Outputs
folder) in a scratch directory and reports the .xvg data, as text, CSV or JSON.
Wx-free; the Builder's MD Analysis panel draws what this prints with --json.

  ecce-gmxanalysis <calc> groups                       molecule, residue, element and atom names
  ecce-gmxanalysis <calc> rdf --ref SEL --sel SEL      g(r) and coordination number cn(r)
  ecce-gmxanalysis <calc> energy [--terms A,B]         the terms of ener.edr; chosen ones vs time
  ecce-gmxanalysis <calc> rms --group SEL              RMSD from the starting structure
  ecce-gmxanalysis <calc> gyrate --group SEL           radius of gyration
  ecce-gmxanalysis <calc> density --group SEL --axis z number or mass density along an axis
  ecce-gmxanalysis <calc> hbond --ref SEL [--sel SEL]  hydrogen bonds vs time

A selection SEL is one or more terms joined by "&" (all must hold), each
kind:name[,name...] with kind molecule, residue, element or atom (atom name),
or "all".  Examples: "residue:NA", "residue:SOL & element:O", "element:Na,K".
The index groups are written from the run's topology (gmx dump of topol.tpr):
gmx's own selection language has no element or molecule-type keyword.

gmx is --gmx, else $ECCE_GROMACS, else gmx or gmx_mpi on PATH.
"""
import argparse
import csv
import io
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile

ANALYSES = ("groups", "rdf", "energy", "rms", "gyrate", "density", "hbond")

# Standard atomic masses, for atoms whose force field leaves the atomic number out.
MASSES = [
    ("H", 1.008), ("He", 4.0026), ("Li", 6.94), ("Be", 9.0122), ("B", 10.81),
    ("C", 12.011), ("N", 14.007), ("O", 15.999), ("F", 18.998), ("Ne", 20.180),
    ("Na", 22.990), ("Mg", 24.305), ("Al", 26.982), ("Si", 28.085), ("P", 30.974),
    ("S", 32.06), ("Cl", 35.45), ("Ar", 39.948), ("K", 39.098), ("Ca", 40.078),
    ("Ti", 47.867), ("Cr", 51.996), ("Mn", 54.938), ("Fe", 55.845), ("Co", 58.933),
    ("Ni", 58.693), ("Cu", 63.546), ("Zn", 65.38), ("Br", 79.904), ("Kr", 83.798),
    ("Rb", 85.468), ("Sr", 87.62), ("Ag", 107.87), ("Cd", 112.41), ("I", 126.90),
    ("Xe", 131.29), ("Cs", 132.91), ("Ba", 137.33), ("Au", 196.97), ("Hg", 200.59),
    ("Pb", 207.2),
]
SYMBOLS = ["X", "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", "Na", "Mg",
           "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca", "Sc", "Ti", "V", "Cr", "Mn",
           "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb",
           "Sr", "Y", "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In",
           "Sn", "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm",
           "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta",
           "W", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi"]
SUPERSCRIPT = str.maketrans("0123456789-+", "⁰¹²³⁴⁵⁶⁷⁸⁹⁻⁺")


class Failure(Exception):
    """A message for the user; the command exits 2 with it."""


# --- units --------------------------------------------------------------------

def pretty_unit(u):
    """gmx's "kg/m^3", "nm^-3", "bar nm" ... with real superscripts:
    kg m⁻³, nm⁻³.  A "/x^n" becomes " x⁻ⁿ"; kJ/mol stays as it is."""
    u = (u or "").strip().strip("()").strip()
    if u in ("", "-"):
        return ""
    u = re.sub(r"/([A-Za-z]+)\^(\d+)", lambda m: " %s^-%s" % (m.group(1), m.group(2)), u)
    return re.sub(r"\^(-?\d+)", lambda m: m.group(1).translate(SUPERSCRIPT), u)


# --- the calculation's files --------------------------------------------------

def calc_dir(arg):
    p = arg[len("file://"):] if arg.startswith("file://") else arg
    p = os.path.abspath(p.rstrip("/") or "/")
    if not os.path.isdir(p):
        raise Failure("%s is not a calculation folder" % arg)
    return p


def find_file(calc, name):
    for d in (os.path.join(calc, "Outputs"), calc):
        p = os.path.join(d, name)
        if os.path.isfile(p) and os.path.getsize(p) > 0:
            return p
    return None


def _small_texts(calc):
    """The calculation's small text files (job outputs, model XML) to look in."""
    for d in (os.path.join(calc, "Outputs"), calc):
        if not os.path.isdir(d):
            continue
        for n in sorted(os.listdir(d)):
            p = os.path.join(d, n)
            if os.path.isfile(p) and os.path.getsize(p) < 20 * 1024 * 1024:
                try:
                    yield n, open(p, errors="replace").read()
                except OSError:
                    pass


def why_missing(calc, name):
    """A precise reason why <name> is not stored with the calculation."""
    left = re.compile(r"%s was left in the run directory: it is larger than the "
                      r"(\d+) MB fetch limit" % re.escape(name))
    for _, text in _small_texts(calc):
        m = left.search(text)
        if m:
            return ("%s is not stored with this calculation: it was left in the "
                    "run directory on the compute host because it is larger than "
                    "the %s MB fetch limit (ECCE_GROMACS_FETCH_LIMIT). Fetch it "
                    "with ecce-fetch, or raise the limit and run again." % (name, m.group(1)))
    for _, text in _small_texts(calc):
        if re.search(r"<GmxLeaveTraj>\s*1\s*</GmxLeaveTraj>", text):
            return ("%s is not stored with this calculation: the task was set to "
                    "leave trajectory files on the machine (GROMACS Run settings, "
                    "Leave trajectory files on the machine)." % name)
    if name == "traj.xtc" and find_file(calc, "ener.edr"):
        return ("traj.xtc is not stored with this calculation: the run wrote no "
                "compressed trajectory (nstxout-compressed = 0) or it was not "
                "brought back from the compute host.")
    return ("%s is not stored with this calculation: it was not brought back from "
            "the compute host (runs made before ECCE fetched GROMACS's binary "
            "outputs keep it in the run directory only)." % name)


def need(calc, *names):
    paths = {}
    for n in names:
        p = find_file(calc, n)
        if not p:
            raise Failure(why_missing(calc, n))
        paths[n] = p
    return paths


# --- gmx ----------------------------------------------------------------------

def find_gmx(given):
    for cand in (given, os.environ.get("ECCE_GROMACS")):
        if cand:
            p = shutil.which(cand) if os.sep not in cand else cand
            if p and os.access(p, os.X_OK):
                return p
            raise Failure("gmx not found at %s" % cand)
    for n in ("gmx", "gmx_mpi"):
        p = shutil.which(n)
        if p:
            return p
    raise Failure("GROMACS (gmx) is not installed on this computer, or not on "
                  "PATH; set ECCE_GROMACS to its path")


class Gmx(object):
    def __init__(self, exe, work, threads):
        self.exe = exe
        self.work = work
        self.commands = []
        self.env = dict(os.environ, GMX_MAXBACKUP="-1", GMX_NO_QUOTES="1")
        self.env.setdefault("OMP_NUM_THREADS", str(threads))

    def run(self, args, stdin="", check=True):
        argv = [self.exe] + args
        self.commands.append(argv + (["<<< %r" % stdin] if stdin else []))
        r = subprocess.run(argv, input=stdin.encode(), cwd=self.work, env=self.env,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out = r.stdout.decode("utf-8", "replace")
        err = r.stderr.decode("utf-8", "replace")
        if r.returncode != 0 and check:
            raise Failure("gmx %s failed: %s" % (args[0], gmx_error(err + out)))
        return out, err


def gmx_error(text):
    """The message between gmx's dashed "Fatal error" lines, or its last line."""
    m = re.search(r"(?:Fatal error|Error in user input|Inconsistency in user input):\n(.*?)\n\n",
                  text, re.S)
    if m:
        return " ".join(m.group(1).split())
    lines = [l for l in text.splitlines() if l.strip() and not l.startswith("---")]
    return lines[-1].strip() if lines else "no message"


def read_xvg(path):
    """-> (columns, legends, xlabel, ylabel): columns[0] is x."""
    cols, legends, xl, yl = [], {}, "", ""
    for line in open(path, errors="replace"):
        if line.startswith("@"):
            m = re.match(r'@\s+s(\d+)\s+legend\s+"(.*)"', line)
            if m:
                legends[int(m.group(1))] = m.group(2)
            m = re.match(r'@\s+([xy])axis\s+label\s+"(.*)"', line)
            if m:
                if m.group(1) == "x":
                    xl = m.group(2)
                else:
                    yl = m.group(2)
            continue
        if line.startswith("#") or not line.strip():
            continue
        vals = [float(v) for v in line.split()]
        while len(cols) < len(vals):
            cols.append([])
        for i, v in enumerate(vals):
            cols[i].append(v)
    return cols, legends, xl, yl


# --- topology and index groups ------------------------------------------------

def element_of(atomnumber, mass):
    if 0 < atomnumber < len(SYMBOLS):
        return SYMBOLS[atomnumber]
    if mass <= 0.5:
        return "X"       # virtual site or shell
    best = min(MASSES, key=lambda e: abs(e[1] - mass))
    return best[0] if abs(best[1] - mass) < 1.0 else "X"


def read_topology(gmx, tpr):
    """Per atom in run order: (molecule type, residue name, atom name, element)."""
    out, _ = gmx.run(["dump", "-s", tpr])
    blocks, types = [], {}
    cur = None
    section = None
    for line in out.splitlines():
        s = line.strip()
        m = re.match(r"moltype\s+=\s+(\d+)\s+\"(.*)\"", s)
        if m:
            blocks.append([int(m.group(1)), 0])
            continue
        m = re.match(r"#molecules\s+=\s+(\d+)", s)
        if m and blocks:
            blocks[-1][1] = int(m.group(1))
            continue
        m = re.match(r"moltype \((\d+)\):", s)
        if m:
            cur = {"name": "", "atoms": [], "names": [], "res": []}
            types[int(m.group(1))] = cur
            section = None
            continue
        if cur is None:
            continue
        m = re.match(r'name="(.*)"$', s)
        if m and not cur["name"]:
            cur["name"] = m.group(1)
            continue
        m = re.match(r"atom\[\s*\d+\]=\{type=.*?m=\s*([-\d.eE+]+).*resind=\s*(\d+),\s*atomnumber=\s*(-?\d+)", s)
        if m:
            cur["atoms"].append((float(m.group(1)), int(m.group(2)), int(m.group(3))))
            continue
        m = re.match(r'atom\[\s*\d+\]=\{name="(.*)"\}', s)
        if m:
            cur["names"].append(m.group(1))
            continue
        m = re.match(r'residue\[\s*\d+\]=\{name="(.*?)"', s)
        if m:
            cur["res"].append(m.group(1))
            continue
        if s.startswith("grp[") or re.match(r"^x \(\d+x3\):", s):
            cur = None
    atoms = []
    for t, n in blocks:
        mt = types.get(t)
        if mt is None:
            raise Failure("cannot read molecule type %d from topol.tpr" % t)
        one = []
        for i, (mass, resind, z) in enumerate(mt["atoms"]):
            res = mt["res"][resind] if resind < len(mt["res"]) else ""
            name = mt["names"][i] if i < len(mt["names"]) else ""
            one.append((mt["name"], res, name, element_of(z, mass)))
        atoms.extend(one * n)
    if not atoms:
        raise Failure("topol.tpr lists no atoms (gmx dump gave no molecule types)")
    return atoms


def group_names(atoms):
    def count(k):
        seen = {}
        for a in atoms:
            seen[a[k]] = seen.get(a[k], 0) + 1
        return [{"name": n, "atoms": c} for n, c in seen.items()]
    return {"atoms": len(atoms), "molecules": count(0), "residues": count(1),
            "atomnames": count(2), "elements": count(3)}


KINDS = {"molecule": 0, "mol": 0, "residue": 1, "res": 1, "resname": 1,
         "atom": 2, "name": 2, "element": 3, "el": 3}


def select(atoms, spec):
    """1-based atom numbers of the selection; Failure for a bad or empty one."""
    spec = (spec or "").strip()
    if not spec:
        raise Failure("an empty selection")
    terms = [t.strip() for t in spec.split("&")]
    keep = None
    for term in terms:
        if term.lower() in ("all", "system"):
            hit = set(range(len(atoms)))
        else:
            if ":" not in term:
                raise Failure("selection term '%s' is not kind:name (kinds: molecule, "
                              "residue, element, atom)" % term)
            kind, names = term.split(":", 1)
            k = KINDS.get(kind.strip().lower())
            if k is None:
                raise Failure("unknown selection kind '%s' (molecule, residue, element, "
                              "atom)" % kind.strip())
            want = set(n.strip() for n in names.split(",") if n.strip())
            if k == 3:
                want = set(n.capitalize() for n in want)
            hit = set(i for i, a in enumerate(atoms) if a[k] in want)
        keep = hit if keep is None else keep & hit
    if not keep:
        raise Failure("the selection '%s' has no atoms" % spec)
    return [i + 1 for i in sorted(keep)]


def write_index(path, groups):
    with open(path, "w") as h:
        for name, idx in groups:
            h.write("[ %s ]\n" % name)
            for k in range(0, len(idx), 15):
                h.write(" ".join(str(i) for i in idx[k:k + 15]) + "\n")


def make_groups(atoms, work, specs):
    """Index file with one group per selection: [(name, spec, natoms)]."""
    groups, made = [], []
    for k, spec in enumerate(specs):
        idx = select(atoms, spec)
        name = "ecce_sel%d" % (k + 1)
        groups.append((name, idx))
        made.append({"name": name, "selection": spec, "atoms": len(idx)})
    ndx = os.path.join(work, "ecce_groups.ndx")
    write_index(ndx, groups)
    return ndx, made


# --- analyses -----------------------------------------------------------------

def time_args(a):
    t = []
    if a.begin is not None:
        t += ["-b", "%g" % a.begin]
    if a.end is not None:
        t += ["-e", "%g" % a.end]
    return t


def smooth(g, half):
    out = []
    for i in range(len(g)):
        w = g[max(0, i - half):i + half + 1]
        out.append(sum(w) / len(w))
    return out


def first_peak_and_minimum(r, g, cn):
    """First peak of g(r) and the first minimum after it, found on g smoothed
    over 0.01 nm (a short run's g(r) is noisy bin to bin): the peak is the
    maximum of the first stretch above 1, the minimum the lowest point before
    g, having fallen below 1, rises above 1 again.  cn is read at the minimum."""
    n = len(g)
    if n < 3:
        return []
    half = max(1, int(round(0.005 / max(r[1] - r[0], 1e-6))))
    s = smooth(g, half)
    i = next((k for k in range(n) if s[k] > 1.0), None)
    if i is None:
        return []
    j = i
    while j < n and s[j] > 1.0:
        j += 1
    peak = max(range(i, j), key=lambda k: s[k])
    out = [{"name": "first peak position", "value": r[peak], "unit": "nm"},
           {"name": "first peak height g(r)", "value": s[peak], "unit": ""}]
    if j >= n:
        return out
    k = j
    while k < n and s[k] <= 1.0:
        k += 1
    if k >= n:
        return out
    lo = min(range(peak, k), key=lambda m: s[m])
    out += [{"name": "first minimum position", "value": r[lo], "unit": "nm"},
            {"name": "coordination number (to the first minimum)",
             "value": cn[lo] if lo < len(cn) else float("nan"), "unit": ""}]
    return out


def do_rdf(a, calc, gmx, res):
    f = need(calc, "traj.xtc", "topol.tpr")
    if not a.ref or not a.sel:
        raise Failure("rdf needs --ref and --sel")
    atoms = read_topology(gmx, f["topol.tpr"])
    ndx, made = make_groups(atoms, gmx.work, [a.ref, a.sel])
    res["groups"] = made
    args = ["rdf", "-s", f["topol.tpr"], "-f", f["traj.xtc"], "-n", ndx,
            "-ref", made[0]["name"], "-sel", made[1]["name"],
            "-bin", "%g" % a.bin, "-o", "rdf.xvg", "-cn", "cn.xvg"]
    if a.rmax > 0:
        args += ["-rmax", "%g" % a.rmax]
    if a.surf:
        args += ["-surf", a.surf]
    args += time_args(a)
    gmx.run(args)
    rdf, _, _, _ = read_xvg(os.path.join(gmx.work, "rdf.xvg"))
    cn, _, _, _ = read_xvg(os.path.join(gmx.work, "cn.xvg"))
    r, g = rdf[0], rdf[1]
    c = cn[1] if len(cn) > 1 else []
    res["x"] = {"name": "r", "unit": "nm", "values": r}
    res["series"] = [{"name": "g(r)", "unit": "", "values": g},
                     {"name": "cn(r)", "unit": "", "values": c}]
    res["summary"] = first_peak_and_minimum(r, g, c)
    res["notes"].append("g(r) of %s (%d atoms) around %s (%d atoms); cn(r) is the "
                        "number of %s atoms within r of one %s atom (gmx rdf -cn). The peak and "
                        "minimum are read from g(r) smoothed over 0.01 nm."
                        % (a.sel, made[1]["atoms"], a.ref, made[0]["atoms"], a.sel, a.ref))


def energy_terms(gmx, edr):
    """[(number, name)] of the energy file, from gmx energy's menu."""
    # Ending the menu with no term at all makes gmx energy exit 1 after listing.
    out, err = gmx.run(["energy", "-f", edr, "-o", "terms.xvg"], "0\n", check=False)
    text = err + out
    menu = text.split("-" * 20, 1)[-1]
    terms = re.findall(r"(?:^|\s)(\d+)\s+(\S+)", menu.split("\n\n")[0])
    if not terms:
        raise Failure("gmx energy listed no terms in ener.edr")
    return [(int(n), name) for n, name in terms]


def do_energy(a, calc, gmx, res):
    f = need(calc, "ener.edr")
    terms = energy_terms(gmx, f["ener.edr"])
    res["terms"] = [name for _, name in terms]
    if not a.terms:
        res["notes"].append("%d terms; choose some with --terms" % len(terms))
        return
    want = [t.strip() for t in a.terms.split(",") if t.strip()]
    by_name = dict((name.lower(), n) for n, name in terms)
    picked = []
    for w in want:
        n = by_name.get(w.lower())
        if n is None:
            raise Failure("ener.edr has no term '%s' (it has: %s)"
                          % (w, ", ".join(name for _, name in terms)))
        picked.append((n, [name for k, name in terms if k == n][0]))
    stdin = "".join("%d\n" % n for n, _ in picked) + "0\n"
    out, _ = gmx.run(["energy", "-f", f["ener.edr"], "-o", "energy.xvg"] + time_args(a),
                     stdin)
    cols, legends, xl, _ = read_xvg(os.path.join(gmx.work, "energy.xvg"))
    stats = {}
    for line in out.splitlines():
        m = re.match(r"^(\S+)\s+(-?[\d.eE+-]+|nan)\s+(\S+)\s+(\S+)\s+(\S+)\s+\((.*)\)\s*$", line)
        if m:
            stats[m.group(1)] = m.groups()[1:]
    res["x"] = {"name": "time", "unit": "ps", "values": cols[0] if cols else []}
    res["series"] = []
    res["summary"] = []
    for k, (_, name) in enumerate(picked):
        st = stats.get(name)
        unit = pretty_unit(st[4]) if st else ""
        res["series"].append({"name": legends.get(k, name), "unit": unit,
                              "values": cols[k + 1] if k + 1 < len(cols) else []})
        if st:
            def num(s):
                try:
                    return float(s)
                except ValueError:
                    return None
            res["summary"].append({"name": name, "unit": unit, "average": num(st[0]),
                                   "error estimate": num(st[1]), "rmsd": num(st[2]),
                                   "drift": num(st[3])})
    res["notes"].append("Averages, error estimates, RMSD and total drift over the "
                        "run (the drift is a least-squares line's change from "
                        "start to end), as gmx energy prints them.")


def selection_tool(a, calc, gmx, res, need_sel):
    f = need(calc, "traj.xtc", "topol.tpr")
    if not a.group and not a.ref:
        raise Failure("%s needs --group" % a.analysis)
    atoms = read_topology(gmx, f["topol.tpr"])
    return f, atoms


def do_rms(a, calc, gmx, res):
    f, atoms = selection_tool(a, calc, gmx, res, True)
    fit = a.fit or a.group
    ndx, made = make_groups(atoms, gmx.work, [fit, a.group])
    res["groups"] = made
    gmx.run(["rms", "-s", f["topol.tpr"], "-f", f["traj.xtc"], "-n", ndx, "-tu", "ps",
             "-o", "rmsd.xvg"] + time_args(a), "%s\n%s\n" % (made[0]["name"], made[1]["name"]))
    cols, _, _, _ = read_xvg(os.path.join(gmx.work, "rmsd.xvg"))
    res["x"] = {"name": "time", "unit": "ps", "values": cols[0]}
    res["series"] = [{"name": "RMSD", "unit": "nm", "values": cols[1]}]
    v = cols[1]
    res["summary"] = [{"name": "mean RMSD", "value": sum(v) / len(v), "unit": "nm"},
                      {"name": "final RMSD", "value": v[-1], "unit": "nm"},
                      {"name": "maximum RMSD", "value": max(v), "unit": "nm"}]
    res["notes"].append("RMSD of %s from the starting structure (topol.tpr, the frame "
                        "at the start of the run) after a least-squares fit on %s."
                        % (a.group, fit))


def do_gyrate(a, calc, gmx, res):
    f, atoms = selection_tool(a, calc, gmx, res, True)
    ndx, made = make_groups(atoms, gmx.work, [a.group])
    res["groups"] = made
    gmx.run(["gyrate", "-s", f["topol.tpr"], "-f", f["traj.xtc"], "-n", ndx,
             "-sel", made[0]["name"], "-o", "gyrate.xvg"] + time_args(a))
    cols, _, _, _ = read_xvg(os.path.join(gmx.work, "gyrate.xvg"))
    res["x"] = {"name": "time", "unit": "ps", "values": cols[0]}
    names = ["Rg", "Rg about x", "Rg about y", "Rg about z"]
    res["series"] = [{"name": names[k - 1] if k - 1 < len(names) else "column %d" % k,
                      "unit": "nm", "values": cols[k]} for k in range(1, len(cols))]
    v = cols[1]
    res["summary"] = [{"name": "mean Rg", "value": sum(v) / len(v), "unit": "nm"},
                      {"name": "final Rg", "value": v[-1], "unit": "nm"}]
    res["notes"].append("Mass-weighted radius of gyration of %s; molecules are made "
                        "whole across the box first. A group of separate molecules "
                        "(ions, solvent) gives the spread of the whole group." % a.group)


def do_density(a, calc, gmx, res):
    f, atoms = selection_tool(a, calc, gmx, res, True)
    ndx, made = make_groups(atoms, gmx.work, [a.group])
    res["groups"] = made
    axis = a.axis.upper()
    gmx.run(["density", "-s", f["topol.tpr"], "-f", f["traj.xtc"], "-n", ndx,
             "-d", axis, "-sl", str(a.slices), "-dens", a.dens, "-o", "density.xvg"]
            + time_args(a), "%s\n" % made[0]["name"])
    cols, _, _, _ = read_xvg(os.path.join(gmx.work, "density.xvg"))
    unit = {"mass": "kg m⁻³", "number": "nm⁻³", "charge": "e nm⁻³",
            "electron": "e nm⁻³"}[a.dens]
    res["x"] = {"name": axis.lower(), "unit": "nm", "values": cols[0]}
    res["series"] = [{"name": "%s density" % a.dens, "unit": unit, "values": cols[1]}]
    v = cols[1]
    res["summary"] = [{"name": "mean density", "value": sum(v) / len(v), "unit": unit},
                      {"name": "maximum density", "value": max(v), "unit": unit},
                      {"name": "position of the maximum", "value": cols[0][v.index(max(v))],
                       "unit": "nm"}]
    res["notes"].append("%s density of %s in %d slices along %s, averaged over the "
                        "frames (gmx density; positions from the box origin)."
                        % (a.dens.capitalize(), a.group, a.slices, axis.lower()))


def do_hbond(a, calc, gmx, res):
    f = need(calc, "traj.xtc", "topol.tpr")
    ref = a.ref or a.group
    if not ref:
        raise Failure("hbond needs --ref")
    atoms = read_topology(gmx, f["topol.tpr"])
    specs = [ref] + ([a.sel] if a.sel else [])
    ndx, made = make_groups(atoms, gmx.work, specs)
    res["groups"] = made
    args = ["hbond", "-s", f["topol.tpr"], "-f", f["traj.xtc"], "-n", ndx,
            "-r", made[0]["name"], "-num", "hbnum.xvg", "-o", "hbond.ndx"]
    # gmx hbond reads a missing -t from the terminal: within one group is -t = -r.
    args += ["-t", made[1 if a.sel else 0]["name"], "-de", a.donors, "-ae", a.acceptors]
    gmx.run(args + time_args(a))
    cols, _, _, _ = read_xvg(os.path.join(gmx.work, "hbnum.xvg"))
    res["x"] = {"name": "time", "unit": "ps", "values": cols[0]}
    res["series"] = [{"name": "hydrogen bonds", "unit": "", "values": cols[1]}]
    v = cols[1]
    res["summary"] = [{"name": "mean number of hydrogen bonds", "value": sum(v) / len(v),
                       "unit": ""}]
    res["notes"].append("Hydrogen bonds %s (gmx hbond: donor-acceptor distance "
                        "at most 0.35 nm, hydrogen-donor-acceptor angle at most 30°; "
                        "donor elements %s, acceptor elements %s)."
                        % ("within %s" % ref if not a.sel else "between %s and %s" % (ref, a.sel),
                           a.donors, a.acceptors))


def do_groups(a, calc, gmx, res):
    f = need(calc, "topol.tpr")
    res["groups"] = group_names(read_topology(gmx, f["topol.tpr"]))
    for n in ("traj.xtc", "ener.edr"):
        if not find_file(calc, n):
            res["notes"].append(why_missing(calc, n))


# --- output -------------------------------------------------------------------

def fmt(v):
    if v is None:
        return "-"
    if isinstance(v, float):
        return "%.6g" % v
    return str(v)


def text_report(res):
    out = io.StringIO()
    out.write("%s of %s\n" % (res["analysis"], res["calc"]))
    if res["analysis"] == "groups":
        g = res["groups"]
        out.write("%d atoms\n" % g["atoms"])
        for k in ("molecules", "residues", "elements", "atomnames"):
            out.write("%s: %s\n" % (k, ", ".join("%s (%d)" % (e["name"], e["atoms"])
                                                  for e in g[k])))
    if res["analysis"] == "energy" and "series" not in res:
        out.write("terms: %s\n" % ", ".join(res["terms"]))
    for s in res.get("summary", []):
        if "value" in s:
            out.write("%s: %s %s\n" % (s["name"], fmt(s["value"]), s["unit"]))
        else:
            out.write("%s: average %s, error estimate %s, RMSD %s, drift %s %s\n"
                      % (s["name"], fmt(s["average"]), fmt(s["error estimate"]),
                         fmt(s["rmsd"]), fmt(s["drift"]), s["unit"]))
    for n in res.get("notes", []):
        out.write("note: %s\n" % n)
    return out.getvalue()


def csv_report(res):
    out = io.StringIO()
    w = csv.writer(out, lineterminator="\n")
    if "x" not in res:
        return text_report(res)
    head = ["%s (%s)" % (res["x"]["name"], res["x"]["unit"]) if res["x"]["unit"]
            else res["x"]["name"]]
    head += ["%s (%s)" % (s["name"], s["unit"]) if s["unit"] else s["name"]
             for s in res["series"]]
    w.writerow(head)
    for i, x in enumerate(res["x"]["values"]):
        w.writerow([fmt(x)] + [fmt(s["values"][i]) if i < len(s["values"]) else ""
                               for s in res["series"]])
    return out.getvalue()


def _finite(v):
    """NaN and infinity become null: they are not JSON."""
    if isinstance(v, float) and not math.isfinite(v):
        return None
    if isinstance(v, dict):
        return dict((k, _finite(x)) for k, x in v.items())
    if isinstance(v, list):
        return [_finite(x) for x in v]
    return v


def main(argv=None):
    ap = argparse.ArgumentParser(prog="ecce-gmxanalysis", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("calc", help="the calculation folder (or file:// URL)")
    ap.add_argument("analysis", choices=ANALYSES)
    ap.add_argument("--ref", help="rdf: reference group; hbond: first group")
    ap.add_argument("--sel", help="rdf: the group counted around --ref; hbond: second group")
    ap.add_argument("--group", help="rms, gyrate, density: the group")
    ap.add_argument("--fit", help="rms: the group to fit on (default --group)")
    ap.add_argument("--terms", help="energy: comma-separated term names")
    ap.add_argument("--rmax", type=float, default=0.0,
                    help="rdf: largest r in nm (default: half the box)")
    ap.add_argument("--bin", type=float, default=0.002, help="rdf: bin width in nm")
    ap.add_argument("--surf", choices=("mol", "res"),
                    help="rdf: distances from the surface of each molecule/residue of --ref")
    ap.add_argument("--axis", default="z", choices=("x", "y", "z"), help="density: the axis")
    ap.add_argument("--slices", type=int, default=50, help="density: number of slices")
    ap.add_argument("--dens", default="number", choices=("number", "mass", "charge"),
                    help="density: what is counted")
    ap.add_argument("--donors", default="N O", help="hbond: donor elements (default \"N O\")")
    ap.add_argument("--acceptors", default="N O",
                    help="hbond: acceptor elements (default \"N O\")")
    ap.add_argument("--begin", type=float, help="first time to use (ps)")
    ap.add_argument("--end", type=float, help="last time to use (ps)")
    ap.add_argument("--gmx", help="the gmx program")
    ap.add_argument("--threads", type=int, default=4, help="OpenMP threads for gmx (default 4)")
    ap.add_argument("--keep", action="store_true", help="keep the scratch directory")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--csv", action="store_true")
    a = ap.parse_args(argv)

    res = {"analysis": a.analysis, "calc": a.calc, "notes": []}
    work = None
    try:
        calc = calc_dir(a.calc)
        res["calc"] = calc
        exe = find_gmx(a.gmx)
        res["gmx"] = exe
        work = tempfile.mkdtemp(prefix="ecce-gmxanalysis-")
        gmx = Gmx(exe, work, a.threads)
        globals()["do_" + a.analysis](a, calc, gmx, res)
        res["commands"] = [" ".join(c) for c in gmx.commands]
    except Failure as e:
        res["error"] = str(e)
    finally:
        if work and not a.keep:
            shutil.rmtree(work, ignore_errors=True)
        elif work:
            res["notes"].append("scratch directory kept: %s" % work)
    if a.json:
        sys.stdout.write(json.dumps(_finite(res), ensure_ascii=False) + "\n")
    elif "error" in res:
        pass
    elif a.csv:
        sys.stdout.write(csv_report(res))
    else:
        sys.stdout.write(text_report(res))
    if "error" in res:
        sys.stderr.write("ecce-gmxanalysis: %s\n" % res["error"])
        return 2
    return 0


if __name__ == "__main__":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass
    sys.exit(main())
