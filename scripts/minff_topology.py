#!/usr/bin/env python3
"""MINFF topology builder for GROMACS (wx-free; issue #241).

Takes a mineral unit cell, replicates it, optionally substitutes atoms, lets
atomipy assign MINFF atom types and charges, and writes a GROMACS system:

    min.itp    the mineral molecule (types, charges, bonds, angles)
    conf.gro   coordinates and box
    topol.top  selects min.ff, the MINFF variant and the angle constant
    minff.json summary (atoms, per-type counts, total charge, untyped atoms)
    min.ff/    copy of the MINFF parameter directory, so the run directory
               is self-contained on any machine

atomipy is found through ECCE_ATOMIPY_PYTHON (a python that has it), else the
python running this script.  min.ff is fetched from github.com/mholmboe/minff
at a pinned commit into ~/.cache/ecce/minff/<commit> (ECCE_MINFF_DIR points
at an existing checkout instead).  Its files are never stored in this repo.

    minff_topology.py build CELL -o OUTDIR [--replicate NA NB NC]
        [--substitute FROM TO COUNT]... [--min-distance A] [--seed N]
        [--variant gminff|tminff] [--mineral NAME] [--angle-k 0|250|500|1500]
        [--water opc3]
    minff_topology.py minerals [--angle-k K]
    minff_topology.py fetch
"""
import argparse
import contextlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

MINFF_REPO = "mholmboe/minff"
MINFF_COMMIT = "d12b895b9495458a7d0f6f08b1648f5f317068a3"
ANGLE_KS = (0, 250, 500, 1500)
# water model -> (water .itp, -D name of its oxygen/hydrogen types, ion-set suffix)
WATER_MODELS = {
    "opc3":    ("opc3.itp",    "OPC3",    "OPC3"),
    "opc":     ("opc.itp",     "OPC",     "OPC"),
    "spce":    ("spce.itp",    "SPCE",    "SPCE"),
    "tip3p":   ("tip3p.itp",   "TIP3P",   "TIP3P"),
    "tip3p-fb": ("tip3p-fb.itp", "TIP3PFB", "TIP3PFB"),
    "tip4p-fb": ("tip4p-fb.itp", "TIP4PFB", "TIP4PFB"),
    "tip4pew": ("tip4pew.itp", "TIP4PEW", "TIP4PEW"),
}
# min.ff, plus the example systems the tests use (the rest of the repo is 80 MB)
FETCH_PATHS = ("min.ff/", "Systems/UC_conf/", "Systems/conf/", "Systems/conf.mdp",
               "Systems/topol.top", "Systems/itp/min27.itp")
ATOMIPY_HINT = ("atomipy is not available. Install it with:  pip install atomipy\n"
                "(or set ECCE_ATOMIPY_PYTHON to a python that has it).")


class MinffError(Exception):
    pass


# ---------------------------------------------------------------- min.ff

def cache_root():
    return os.path.join(os.environ.get("XDG_CACHE_HOME") or
                        os.path.expanduser("~/.cache"), "ecce", "minff")


def minff_dir(fetch=True):
    """Directory that contains min.ff/ (a minff checkout)."""
    d = os.environ.get("ECCE_MINFF_DIR")
    if d:
        if not os.path.isfile(os.path.join(d, "min.ff", "forcefield.itp")):
            raise MinffError("ECCE_MINFF_DIR=%s has no min.ff/forcefield.itp" % d)
        return d
    d = os.path.join(cache_root(), MINFF_COMMIT)
    if os.path.isfile(os.path.join(d, "min.ff", "forcefield.itp")):
        return d
    if not fetch:
        raise MinffError("min.ff not fetched yet; run: minff_topology.py fetch")
    url = "https://github.com/%s/archive/%s.tar.gz" % (MINFF_REPO, MINFF_COMMIT)
    os.makedirs(cache_root(), exist_ok=True)
    tmp = tempfile.mkdtemp(dir=cache_root(), prefix="fetch-")
    try:
        tgz = os.path.join(tmp, "minff.tar.gz")
        try:
            with urllib.request.urlopen(url, timeout=120) as r, open(tgz, "wb") as f:
                shutil.copyfileobj(r, f)
        except OSError as e:
            raise MinffError("could not download min.ff from %s (%s); download a "
                             "minff checkout by hand and set ECCE_MINFF_DIR" % (url, e))
        with tarfile.open(tgz) as t:
            for m in t.getmembers():
                rel = m.name.split("/", 1)[-1]
                if not m.isfile() or ".." in rel or not rel.startswith(FETCH_PATHS):
                    continue
                m.name = rel                          # drop minff-<commit>/
                t.extract(m, os.path.join(tmp, "x"), **(
                    {"filter": "data"} if sys.version_info >= (3, 12) else {}))
        os.rename(os.path.join(tmp, "x"), d)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return d


def tminff_file(ffdir, k):
    return os.path.join(ffdir, "min.ff", "ffnonbonded_tminff_k%d.itp" % k)


def list_minerals(k=500, ffdir=None):
    """Mineral blocks of the tailored set for angle constant k."""
    ffdir = ffdir or minff_dir()
    pat = re.compile(r"^#ifdef\s+(.+?)_k%d\s*$" % k)
    out = []
    with open(tminff_file(ffdir, k), encoding="utf-8", errors="replace") as f:
        for line in f:
            m = pat.match(line.rstrip())
            if m:
                out.append(m.group(1))
    return out


def ff_block_types(ffdir, variant, k, mineral):
    """Atom type names defined by the selected parameter block."""
    if variant == "tminff":
        fn, block = tminff_file(ffdir, k), "%s_k%d" % (mineral, k)
    else:
        fn, block = os.path.join(ffdir, "min.ff", "ffnonbonded.itp"), "GMINFF_k%d" % k
    types, inside = set(), False
    with open(fn, encoding="utf-8", errors="replace") as f:
        for line in f:
            s = line.strip()
            if s.startswith("#ifdef"):
                inside = s.split()[1] == block
            elif s.startswith("#endif"):
                inside = False
            elif inside and s and not s.startswith(";") and not s.startswith("["):
                types.add(s.split()[0])
    return types


# -------------------------------------------------------------- atomipy

def import_atomipy():
    try:
        import atomipy as ap
        return ap
    except ImportError:
        raise MinffError(ATOMIPY_HINT)


def find_atomipy_python():
    """Python able to import atomipy: ECCE_ATOMIPY_PYTHON, else python3."""
    cands = [os.environ.get("ECCE_ATOMIPY_PYTHON"), sys.executable, "python3"]
    for c in cands:
        if not c:
            continue
        try:
            if subprocess.run([c, "-c", "import atomipy"], capture_output=True).returncode == 0:
                return c
        except OSError:
            pass
    return None


# ----------------------------------------------------------------- build

def topol_text(variant, k, mineral, water, ion_set=True):
    wfile, wdef, ionsuf = WATER_MODELS[water]
    block = ("%s_k%d" % (mineral, k)) if variant == "tminff" else ("GMINFF_k%d" % k)
    L = ["; MINFF system written by ECCE minff_topology.py",
         "; variant: %s%s, angle force constant %d kJ/mol/rad2, water %s" % (
             variant.upper(), (" (" + mineral + ")") if mineral else "", k, water),
         "; the angle constant is also written into the angles of min.itp",
         "",
         "#define KANGLE %.2f" % k,
         "#define %s          ; selects the parameter block in min.ff" % block,
         "#define %s          ; water model types" % wdef,
         "#define %s_HFE_LM   ; ion parameters matching the water model" % ionsuf,
         ""]
    if variant == "gminff":
        L.append('#include "min.ff/forcefield.itp"')
    else:
        # forcefield.itp includes the general ffnonbonded.itp; the tailored
        # set replaces that one file.  ffbonded.itp names atom types a
        # tailored block does not define (grompp: "Unknown bond_atomtype"),
        # so min_bonded.itp is ffbonded.itp restricted to the defined types.
        L += ["#define _FF_MINFF", "[ defaults ]",
              "; nbfunc        comb-rule       gen-pairs       fudgeLJ fudgeQQ",
              "1               2               no              1.0     1.0", "",
              '#include "min.ff/ffnonbonded_tminff_k%d.itp"' % k,
              '#include "min_bonded.itp"']
    L += ['#include "min.ff/ions.itp"', '#include "min.ff/%s"' % wfile,
          '#include "min.itp"', "",
          "[ system ]", "MINFF mineral", "",
          "[ molecules ]", "; name  count", "MIN    1", ""]
    return "\n".join(L)


def restrict_bonded(src, types):
    """ffbonded.itp text keeping only [ bondtypes ]/[ angletypes ] rows whose
    atom types are all in `types`; comments and #if lines are kept."""
    out, n = [], 0
    with open(src, encoding="utf-8", errors="replace") as f:
        for line in f:
            s = line.split(";")[0].split()
            if line.lstrip().startswith("["):
                n = {"bondtypes": 2, "angletypes": 3}.get(line.strip("[] \n\t").strip(), 0)
            elif s and n and not s[0].startswith("#"):
                if not all(t in types for t in s[:n]):
                    continue
            out.append(line)
    return "".join(out)


def read_itp_atoms(path):
    """[(type, charge)] from the [ atoms ] section of an itp."""
    out, sec = [], None
    with open(path, encoding="utf-8") as f:
        for line in f:
            s = line.split(";")[0].strip()
            if not s:
                continue
            if s.startswith("["):
                sec = s.strip("[] ").lower()
            elif sec == "atoms":
                p = s.split()
                out.append((p[1], float(p[6])))
    return out


def build(cell, outdir, replicate=(1, 1, 1), substitutions=(), min_distance=5.5,
          seed=None, variant="gminff", mineral=None, angle_k=500, water="opc3",
          log=sys.stderr):
    if variant not in ("gminff", "tminff"):
        raise MinffError("variant must be gminff or tminff")
    if angle_k not in ANGLE_KS:
        raise MinffError("angle k must be one of %s" % (ANGLE_KS,))
    if water not in WATER_MODELS:
        raise MinffError("water model must be one of %s" % sorted(WATER_MODELS))
    if not os.path.isfile(cell):
        raise MinffError("unit cell file not found: %s" % cell)
    ap = import_atomipy()
    ffdir = minff_dir()
    if variant == "tminff":
        minerals = list_minerals(angle_k, ffdir)
        if mineral not in minerals:
            raise MinffError("TMINFF needs --mineral NAME, one of: %s" % ", ".join(minerals))
    else:
        mineral = None

    os.makedirs(outdir, exist_ok=True)
    # atomipy is chatty on stdout and drops side files next to its output
    with contextlib.redirect_stdout(log), tempfile.TemporaryDirectory() as tmp:
        atoms, box = ap.import_auto(cell)
        if not atoms:
            raise MinffError("no atoms read from %s" % cell)
        if tuple(replicate) != (1, 1, 1):
            atoms, box, _cell = ap.replicate_system(atoms, box, list(replicate))
        if substitutions:
            import numpy as np
            if seed is not None:
                np.random.seed(seed)
            for frm, to, count in substitutions:
                atoms, box, _ = ap.substitute(atoms, box, count, frm, to, min_distance)
        atoms = ap.minff(atoms, box)
        itp_tmp = os.path.join(tmp, "min.itp")
        ap.write_itp(atoms, box, itp_tmp, molecule_name="MIN", KANGLE=angle_k,
                     water_model=water)
        gro_tmp = os.path.join(tmp, "conf.gro")
        ap.write_gro(atoms, box, gro_tmp)
        shutil.copy(itp_tmp, os.path.join(outdir, "min.itp"))
        shutil.copy(gro_tmp, os.path.join(outdir, "conf.gro"))

    dst = os.path.join(outdir, "min.ff")
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(os.path.join(ffdir, "min.ff"), dst)
    known = ff_block_types(ffdir, variant, angle_k, mineral)
    if variant == "tminff":
        with open(os.path.join(outdir, "min_bonded.itp"), "w", encoding="utf-8") as f:
            f.write(restrict_bonded(os.path.join(ffdir, "min.ff", "ffbonded.itp"), known))
    with open(os.path.join(outdir, "topol.top"), "w", encoding="utf-8") as f:
        f.write(topol_text(variant, angle_k, mineral, water))

    itp = read_itp_atoms(os.path.join(outdir, "min.itp"))
    counts = {}
    for t, _ in itp:
        counts[t] = counts.get(t, 0) + 1
    # atomipy leaves an atom it cannot classify with its element as type
    untyped = [i + 1 for i, (t, _) in enumerate(itp) if t not in known]
    summary = {
        "variant": variant.upper() if variant == "gminff" else "TMINFF",
        "mineral": mineral,
        "angle_k": angle_k,
        "water": water,
        "cell": os.path.basename(cell),
        "replicate": list(replicate),
        "substitutions": [{"from": a, "to": b, "count": c} for a, b, c in substitutions],
        "seed": seed,
        "n_atoms": len(itp),
        "type_counts": dict(sorted(counts.items())),
        "total_charge": round(sum(q for _, q in itp), 6),
        "untyped_atoms": untyped,      # 1-based; no parameters in the chosen set
        "types_missing_from_ff": sorted(t for t in counts if t not in known),
        "minff_commit": MINFF_COMMIT if not os.environ.get("ECCE_MINFF_DIR") else "ECCE_MINFF_DIR",
        "files": {"topology": "topol.top", "molecule": "min.itp", "structure": "conf.gro"},
    }
    with open(os.path.join(outdir, "minff.json"), "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)
        f.write("\n")
    return summary


# ------------------------------------------------------------------- CLI

def main(argv=None):
    p = argparse.ArgumentParser(description="Build a MINFF GROMACS topology with atomipy.")
    sub = p.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build", help="build min.itp, conf.gro, topol.top, minff.json")
    b.add_argument("cell", help="unit cell: .pdb, .gro or .cif")
    b.add_argument("-o", "--outdir", required=True)
    b.add_argument("--replicate", nargs=3, type=int, default=[1, 1, 1],
                   metavar=("NA", "NB", "NC"))
    b.add_argument("--substitute", nargs=3, action="append", default=[],
                   metavar=("FROM", "TO", "COUNT"),
                   help="replace COUNT atoms of type/element FROM by type TO (e.g. Al Mgo 4)")
    b.add_argument("--min-distance", type=float, default=5.5,
                   help="minimum distance (A) between substituted atoms (default 5.5)")
    b.add_argument("--seed", type=int)
    b.add_argument("--variant", choices=("gminff", "tminff"), default="gminff")
    b.add_argument("--mineral", help="mineral block for --variant tminff")
    b.add_argument("--angle-k", type=int, default=500, choices=ANGLE_KS)
    b.add_argument("--water", default="opc3", choices=sorted(WATER_MODELS))
    m = sub.add_parser("minerals", help="list the TMINFF minerals")
    m.add_argument("--angle-k", type=int, default=500, choices=ANGLE_KS)
    sub.add_parser("fetch", help="download min.ff to the cache and print its directory")
    a = p.parse_args(argv)
    if a.cmd == "build" and not os.environ.get("ECCE_MINFF_REEXEC"):
        try:
            import atomipy  # noqa: F401
        except ImportError:
            py = find_atomipy_python()
            if not py:
                print("minff_topology: " + ATOMIPY_HINT, file=sys.stderr)
                return 1
            env = dict(os.environ, ECCE_MINFF_REEXEC="1")
            return subprocess.call([py, os.path.abspath(__file__)] + (
                sys.argv[1:] if argv is None else list(argv)), env=env)
    try:
        if a.cmd == "fetch":
            print(minff_dir())
        elif a.cmd == "minerals":
            print("\n".join(list_minerals(a.angle_k)))
        else:
            subs = []
            for frm, to, cnt in a.substitute:
                try:
                    subs.append((frm, to, int(cnt)))
                except ValueError:
                    raise MinffError("substitution count must be an integer: %s" % cnt)
            s = build(a.cell, a.outdir, a.replicate, subs, a.min_distance, a.seed,
                      a.variant, a.mineral, a.angle_k, a.water)
            print(json.dumps(s, indent=2))
    except MinffError as e:
        print("minff_topology: %s" % e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
