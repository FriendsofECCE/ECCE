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

ECCE ships atomipy (<ECCE_HOME>/lib/python/atomipy) and min.ff
(<ECCE_HOME>/data/minff/min.ff); in a source tree they are in third_party/.
They win over everything below.  Otherwise atomipy is found through
ECCE_ATOMIPY_PYTHON (a python that has it), else the python running this
script, and ECCE_MINFF_DIR points at a minff checkout.  Only `fetch` (used
by the tests, which need minff's example systems) downloads anything: the
minff commit MINFF_COMMIT into ~/.cache/ecce/minff/<commit>.

    minff_topology.py build CELL -o OUTDIR [--replicate NA NB NC]
        [--substitute FROM TO COUNT]... [--min-distance A] [--seed N]
        [--variant gminff|tminff] [--mineral NAME] [--angle-k 0|250|500|1500]
        [--water opc3] [--add-gap [NM]] [--ion-set HFE|IOD] [--gui]
        [--forcefield minff|clayff] [--clayff-bend none|moh|moh2004]
        [--clayff-rigid-water]
    minff_topology.py minerals [--angle-k K]
    minff_topology.py fetch
"""
import argparse
import contextlib
import json
import math
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
ATOMIPY_HINT = ("atomipy is not available: this ECCE installation is incomplete "
                "(its lib/python/atomipy is missing).\n"
                "Reinstall ECCE, or set ECCE_ATOMIPY_PYTHON to a python that has atomipy.")


class MinffError(Exception):
    pass


# ---------------------------------------------------------------- min.ff

def cache_root():
    return os.path.join(os.environ.get("XDG_CACHE_HOME") or
                        os.path.expanduser("~/.cache"), "ecce", "minff")


def bundle_roots():
    """Install roots to look in for bundled copies: $ECCE_HOME, and the tree
    or install this script sits in (scripts/ is next to lib/ and data/); the
    realpath covers a test $ECCE_HOME whose scripts/ links to the source tree."""
    roots = [os.environ.get("ECCE_HOME"),
             os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
             os.path.dirname(os.path.dirname(os.path.realpath(__file__)))]
    return [r for r in roots if r]


# install layout, then the source tree's third_party/
_PY_SUBDIRS = (("lib", "python"), ("third_party", "python"))
_FF_SUBDIRS = (("data", "minff"), ("third_party", "minff"))
_CLAYFF_SUBDIRS = (("data", "clayff"), ("third_party", "clayff"))


def bundled_atomipy_dir():
    """Directory holding the bundled atomipy package, or None.
    Bundled layout: <root>/lib/python/atomipy/."""
    for r in bundle_roots():
        for sub in _PY_SUBDIRS:
            d = os.path.join(r, *sub)
            if os.path.isfile(os.path.join(d, "atomipy", "__init__.py")):
                return d
    return None


def bundled_minff_dir():
    """Bundled directory that contains min.ff/: <root>/data/minff."""
    for r in bundle_roots():
        for sub in _FF_SUBDIRS:
            d = os.path.join(r, *sub)
            if os.path.isfile(os.path.join(d, "min.ff", "forcefield.itp")):
                return d
    return None


def clayff_dir():
    """Directory that contains clayff.ff/ (bundled: <root>/data/clayff)."""
    for r in bundle_roots():
        for sub in _CLAYFF_SUBDIRS:
            d = os.path.join(r, *sub)
            if os.path.isfile(os.path.join(d, "clayff.ff", "forcefield.itp")):
                return d
    raise MinffError("clayff.ff is missing: this ECCE installation is incomplete "
                     "(data/clayff/clayff.ff); reinstall ECCE")


def minff_dir():
    """Directory that contains min.ff/: the bundled copy, else ECCE_MINFF_DIR."""
    d = bundled_minff_dir()
    if d:
        return d
    d = os.environ.get("ECCE_MINFF_DIR")
    if d:
        if not os.path.isfile(os.path.join(d, "min.ff", "forcefield.itp")):
            raise MinffError("ECCE_MINFF_DIR=%s has no min.ff/forcefield.itp" % d)
        return d
    raise MinffError("min.ff is missing: this ECCE installation is incomplete "
                     "(data/minff/min.ff); reinstall ECCE or set ECCE_MINFF_DIR")


def fetch_pinned():
    """Download the minff commit MINFF_COMMIT (min.ff and the example systems
    the tests use) into the cache; return its directory.  Not used by builds."""
    d = os.path.join(cache_root(), MINFF_COMMIT)
    if os.path.isfile(os.path.join(d, "Systems", "UC_conf", "Kaolinite_GII_0.0487.pdb")):
        return d
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
    b = bundled_atomipy_dir()
    if b and b not in sys.path:
        sys.path.insert(0, b)
    try:
        import atomipy as ap
        return ap
    except ImportError:
        raise MinffError(ATOMIPY_HINT)


def _atomipy_env():
    env = dict(os.environ)
    b = bundled_atomipy_dir()
    if b:
        env["PYTHONPATH"] = b + os.pathsep + env.get("PYTHONPATH", "")
    return env


def find_atomipy_python():
    """Python able to import atomipy: ECCE_ATOMIPY_PYTHON, else python3."""
    cands = [sys.executable, os.environ.get("ECCE_ATOMIPY_PYTHON"), "python3"]
    for c in cands:
        if not c:
            continue
        try:
            if subprocess.run([c, "-c", "import atomipy"], capture_output=True,
                              env=_atomipy_env()).returncode == 0:
                return c
        except OSError:
            pass
    return None


# ----------------------------------------------------------------- build

# Ion parameter sets min.ff's ions.itp defines for every water model: hydration
# free energy (HFE) or ion-oxygen distance (IOD) fitted.
ION_SETS = ("HFE", "IOD")
# For these waters ions.itp takes the monovalent IOD ions from one shared block.
SHARED_MONOVALENT_IOD = ("spce", "tip3p", "tip4pew")


def topol_text(variant, k, mineral, water, ion_set="HFE"):
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
         "#define %s_%s_LM   ; ion parameters matching the water model" % (ionsuf, ion_set)]
    if ion_set == "IOD" and water in SHARED_MONOVALENT_IOD:
        L.append("#define all_IOD_LM   ; monovalent IOD ions of this water model")
    L.append("")
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


def contiguous_z(path):
    """Rewrite a .gro so that the mineral is one block along z, the gap
    between its layers or periodic images at the top of the cell.  A layer
    split over the z boundary would be torn apart when the Prepare adds a
    water layer on top of the cell (its bonds would then span the new box).
    Atoms move by whole cell vectors only, so the periodic system is the
    same.  Returns the number of atoms moved."""
    lines = open(path, encoding="utf-8").read().split("\n")
    n = int(lines[1])
    box = [float(x) for x in lines[2 + n].split()]
    box += [0.0] * (9 - len(box))
    c, vx, vy = box[2], box[7], box[8]
    if c <= 0 or n == 0:
        return 0
    xyz = []
    for a in lines[2:2 + n]:
        x, y, z = float(a[20:28]), float(a[28:36]), float(a[36:44])
        k = math.floor(z / c)
        xyz.append([x - k * vx, y - k * vy, z - k * c])
    order = sorted(range(n), key=lambda i: xyz[i][2])
    # the largest gap, the wrap-around one (top to bottom + c) included
    best, cut = xyz[order[0]][2] + c - xyz[order[-1]][2], 0
    for j in range(1, n):
        gap = xyz[order[j]][2] - xyz[order[j - 1]][2]
        if gap > best:
            best, cut = gap, j
    moved = 0
    for j in range(cut):
        i = order[j]
        xyz[i][0] += vx
        xyz[i][1] += vy
        xyz[i][2] += c
        moved += 1
    for i, a in enumerate(lines[2:2 + n]):
        lines[2 + i] = a[:20] + "%8.3f%8.3f%8.3f" % tuple(xyz[i]) + a[44:]
    open(path, "w", encoding="utf-8").write("\n".join(lines))
    return moved


GAP_NM = 0.3


def read_gro_z(path):
    """(z of each atom, c) of a .gro; c is the box vector's z component."""
    lines = open(path, encoding="utf-8").read().split("\n")
    n = int(lines[1])
    box = [float(x) for x in lines[2 + n].split()]
    return [float(a[36:44]) for a in lines[2:2 + n]], box[2]


def itp_edges(path):
    """Atom index pairs (0-based) joined by an angle in min.itp: every M-O
    and O-H bond of the mineral."""
    edges, sec = set(), None
    with open(path, encoding="utf-8") as f:
        for line in f:
            t = line.split(";")[0].strip()
            if t.startswith("["):
                sec = t.strip("[] ").lower()
            elif t and sec == "angles" and t[0] != "#":
                i, j, k = (int(x) - 1 for x in t.split()[:3])
                edges.update(((min(i, j), max(i, j)), (min(j, k), max(j, k))))
    return sorted(edges)


def winding_bonds(z, c, edges):
    """Number of bonds that close a loop around the cell along c, i.e. join the
    mineral to its own periodic image.  Unwrap the bond graph along c from a
    spanning tree; a bond whose ends then differ by a whole cell is one."""
    nbr = {}
    for i, j in edges:
        nbr.setdefault(i, []).append(j)
        nbr.setdefault(j, []).append(i)
    u, bad = {}, 0

    def d(i, j):
        x = (z[j] - z[i]) / c
        return x - round(x)
    for root in nbr:
        if root in u:
            continue
        u[root] = 0.0
        todo = [root]
        while todo:
            i = todo.pop()
            for j in nbr[i]:
                if j not in u:
                    u[j] = u[i] + d(i, j)
                    todo.append(j)
    for i, j in edges:
        if abs(u[j] - u[i] - d(i, j)) > 0.5:
            bad += 1
    return bad


def widest_gaps(z, c, count=4, least=0.04):
    """[(z0, gap)]: the largest empty z-distances between atoms (the cell
    boundary included, at least `least` nm, so not within an atom plane) and
    the plane in the middle of each.  A layer is opened at one of them to
    lengthen the cell: the vacuum, unless that is narrower than the spacing
    of the layer's own atom planes, so the next ones are kept to try."""
    order = sorted(v % c for v in z)
    gaps = []
    for i, v in enumerate(order):
        nxt = order[i + 1] if i + 1 < len(order) else order[0] + c
        if nxt - v >= least:
            gaps.append((nxt - v, (v + (nxt - v) / 2.0) % c))
    gaps.sort(reverse=True)
    return [(z0, g) for g, z0 in gaps[:count]]


def regap(gro_in, gro_out, z0, need_nm, names):
    """Copy of a .gro opened at z = z0 (atoms move by whole cell vectors so
    the layer is contiguous from 0) with the cell lengthened along c by need_nm,
    the box's tilt kept.  `names` are the atom names the first pass started
    from: its output names were assigned from image-bonded environments and
    would carry the error into the second."""
    lines = open(gro_in, encoding="utf-8").read().split("\n")
    n = int(lines[1])
    box = [float(x) for x in lines[2 + n].split()]
    box += [0.0] * (9 - len(box))
    c, vx, vy = box[2], box[7], box[8]
    for i in range(n):
        a = lines[2 + i]
        x, y, z = float(a[20:28]), float(a[28:36]), float(a[36:44])
        k = math.floor((z - z0) / c)
        lines[2 + i] = (a[:10] + "%5s" % names[i][:5] + a[15:20] +
                        "%8.3f%8.3f%8.3f" % (x - k * vx, y - k * vy, z - k * c - z0) + a[44:])
    box[2] += need_nm
    lines[2 + n] = "".join("%10.5f" % v for v in box)
    open(gro_out, "w", encoding="utf-8").write("\n".join(lines))


def add_posres_noh(path, fc=1000):
    """Append `#ifdef POSRES_noH`: restrain every heavy atom (mass > 2 u).
    The name and the 1000 kJ/mol/nm2 force constant are the ones MINFF
    workflows use (-DPOSRES_noH); atomipy only writes POSRES and POSRES_MINFF.
    Returns the number of restrained atoms."""
    heavy, sec = [], None
    with open(path, encoding="utf-8") as f:
        for line in f:
            s = line.split(";")[0].strip()
            if s.startswith("["):
                sec = s.strip("[] ").lower()
            elif s and sec == "atoms" and s[0] != "#":
                p = s.split()
                if float(p[7]) > 2.0:
                    heavy.append(p[0])
    with open(path, "a", encoding="utf-8") as f:
        f.write("\n#ifdef POSRES_noH\n[ position_restraints ]\n"
                "; atom  type      fx      fy      fz\n")
        for i in heavy:
            f.write("%-6s 1     %d  %d  %d\n" % (i, fc, fc, fc))
        f.write("#endif\n")
    return len(heavy)


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
            elif sec == "atoms" and s[0] != "#":
                p = s.split()
                out.append((p[1], float(p[6])))
    return out


# ----------------------------------------------------------------- ClayFF

# Charges of Table 1 of Cygan, Liang and Kalinichev, J. Phys. Chem. B 2004,
# 108, 1255, under atomipy's type names.  atomipy's clayff() computes the
# oxygen charges from the valence sum instead and gives Lio 0.86, so the
# paper's values are written into min.itp and the differences are reported.
CLAYFF_CHARGES = {
    "H": 0.425, "Oh": -0.95, "Ob": -1.05, "Op": -1.05, "O": -1.05,
    "Omg": -1.1808, "Oalt": -1.1688, "Odsub": -1.2996, "Ohmg": -1.0808,
    "Sit": 2.1, "Alo": 1.575, "Alt": 1.575, "Mgo": 1.36, "Mgh": 1.05,
    "Cao": 1.36, "Cah": 1.05, "Feo": 1.575, "Lio": 0.525,
    "Ow": -0.82, "Hw": 0.41, "Na": 1.0, "K": 1.0, "Cs": 1.0, "Ca": 2.0,
    "Ba": 2.0, "Cl": -1.0,
}
CLAYFF_BENDS = ("none", "moh", "moh2004")
_CLAYFF_BEND_DEFINE = {"none": None, "moh": "CLAYFF_MOH", "moh2004": "CLAYFF_MOH_2004"}
_CLAYFF_HYDROXYL_O = ("Oh", "Ohmg")
_CLAYFF_MOH_METALS = ("Alo", "Mgo", "Mgh")


def clayff_types(ffdir):
    """Atom type names of clayff.ff/ffnonbonded.itp."""
    types, sec = set(), None
    with open(os.path.join(ffdir, "clayff.ff", "ffnonbonded.itp"), encoding="utf-8") as f:
        for line in f:
            s = line.split(";")[0].strip()
            if s.startswith("["):
                sec = s.strip("[] ")
            elif s and sec == "atomtypes":
                types.add(s.split()[0])
    return types


def topol_text_clayff(bend, rigid_water=False):
    d = _CLAYFF_BEND_DEFINE[bend]
    L = ["; ClayFF system written by ECCE minff_topology.py",
         "; Cygan, Liang and Kalinichev, J. Phys. Chem. B 2004, 108, 1255 (clayff.ff/forcefield.doc)",
         "; M-O-H bend: %s" % {"none": "none (the paper's bulk setting)",
                               "moh": "Pouvreau et al. 2017 (Al, Mg) with Morse O-H",
                               "moh2004": "optional term of the 2004 paper, all metals"}[bend],
         ""]
    if d:
        L.append("#define %s" % d)
    if rigid_water:
        L.append("#define RIGID_WATER")
    L += ['#include "clayff.ff/forcefield.itp"', '#include "clayff.ff/ions.itp"',
          '#include "clayff.ff/spc.itp"', '#include "min.itp"', "",
          "[ system ]", "ClayFF mineral", "",
          "[ molecules ]", "; name  count", "MIN    1", ""]
    return "\n".join(L)


def clayff_itp(path):
    """Make atomipy's min.itp a ClayFF one.  ClayFF has no mineral bond or
    angle terms, so the O-M-O angles atomipy writes (they are MINFF's) go;
    the O-H bonds stay, as harmonic or, inside CLAYFF_MOH, Morse bonds, and
    the M-O-H angles are listed only inside CLAYFF_MOH (Al, Mg) and
    CLAYFF_MOH_2004 (any metal); their parameters are clayff.ff's angletypes."""
    lines = open(path, encoding="utf-8").readlines()
    cut = next((n for n, l in enumerate(lines) if l.lstrip().startswith("#ifdef POSRES")),
               len(lines))
    tail = lines[cut:]                 # the position-restraint blocks, as written
    secs, cur, head = {}, None, []
    for line in lines[:cut]:
        t = line.split(";")[0].strip()
        if t.startswith("["):
            cur = t.strip("[] ").lower()
            secs[cur] = []
        elif cur is None:
            head.append(line)
        else:
            secs[cur].append(line)
    types = {}
    for line in secs["atoms"]:
        p = line.split(";")[0].split()
        if len(p) > 1:
            types[int(p[0])] = p[1]
    bonds = [tuple(int(x) for x in l.split(";")[0].split()[:2])
             for l in secs.get("bonds", []) if l.split(";")[0].split()]
    moh = []                          # (i, j, k, metal type)
    for l in secs.get("angles", []):
        p = l.split(";")[0].split()
        if len(p) < 3:
            continue
        i, j, k = int(p[0]), int(p[1]), int(p[2])
        if types[j] not in _CLAYFF_HYDROXYL_O:
            continue
        if types[i] == "H" and types[k] != "H":
            moh.append((k, j, i, types[k]))
        elif types[k] == "H" and types[i] != "H":
            moh.append((i, j, k, types[i]))
    out = head + ["\n[ moleculetype ]\n"] + secs["moleculetype"]
    out += ["\n[ atoms ]\n"] + secs["atoms"]
    out += ["\n#ifdef CLAYFF_MOH\n[ bonds ]\n; i  j  funct (Morse)\n"]
    out += ["%d %d 3\n" % b for b in bonds]
    out += ["#else\n[ bonds ]\n; i  j  funct (harmonic)\n"]
    out += ["%d %d 1\n" % b for b in bonds]
    out += ["#endif\n"]
    hdr = "[ angles ]\n; M  O  H  funct\n"
    out += ["\n#ifdef CLAYFF_MOH\n" + hdr]
    out += ["%d %d %d 1  ; %s-O-H\n" % (i, j, k, m) for i, j, k, m in moh
            if m in _CLAYFF_MOH_METALS]
    out += ["#else\n#ifdef CLAYFF_MOH_2004\n" + hdr]
    out += ["%d %d %d 1  ; %s-O-H\n" % (i, j, k, m) for i, j, k, m in moh]
    out += ["#endif\n#endif\n"]
    out += ["\n"] + tail
    with open(path, "w", encoding="utf-8") as f:
        f.write("".join(out))


def clayff_retype(atoms):
    """atomipy types every six-coordinate Ca as Cao.  Table 1 has cah for the
    calcium of a hydroxide (portlandite), whose six oxygens all carry an H;
    retype those."""
    for a in atoms:
        if a["type"] != "Cao":
            continue
        os_ = [atoms[j] for j in a.get("neigh", []) if atoms[j].get("element") == "O"]
        if os_ and all(any(atoms[k].get("element") == "H" for k in o.get("neigh", []))
                       for o in os_):
            a["type"] = a["fftype"] = "Cah"


def clayff_charges(atoms):
    """Set the paper's Table 1 charge on every atom whose type it lists; return
    {type: [atomipy charges that differ]} for the report."""
    diff = {}
    for a in atoms:
        q = CLAYFF_CHARGES.get(a["type"])
        if q is None:
            continue
        if abs(a["charge"] - q) > 6e-5:
            diff.setdefault(a["type"], set()).add(round(a["charge"], 4))
        a["charge"] = q
    return {t: sorted(v) for t, v in diff.items()}


def build(cell, outdir, replicate=(1, 1, 1), substitutions=(), min_distance=5.5,
          seed=None, variant="gminff", mineral=None, angle_k=500, water="opc3",
          log=sys.stderr, add_gap=None, ion_set="HFE", gui=False,
          forcefield="minff", clayff_bend="none", clayff_rigid_water=False):
    if forcefield not in ("minff", "clayff"):
        raise MinffError("force field must be minff or clayff")
    if clayff_bend not in CLAYFF_BENDS:
        raise MinffError("ClayFF bend must be one of %s" % (CLAYFF_BENDS,))
    clay = forcefield == "clayff"
    if ion_set not in ION_SETS:
        raise MinffError("ion set must be one of %s" % (ION_SETS,))
    if variant not in ("gminff", "tminff"):
        raise MinffError("variant must be gminff or tminff")
    if angle_k not in ANGLE_KS:
        raise MinffError("angle k must be one of %s" % (ANGLE_KS,))
    if water not in WATER_MODELS:
        raise MinffError("water model must be one of %s" % sorted(WATER_MODELS))
    if not os.path.isfile(cell):
        raise MinffError("unit cell file not found: %s" % cell)
    ap = import_atomipy()
    ffdir = clayff_dir() if clay else minff_dir()
    if clay:
        variant, mineral, water = "clayff", None, "spc"
    elif variant == "tminff":
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
        names0 = [str(a.get("type", "")) for a in atoms]   # what minff() started from
        charge_diff = {}
        if clay:
            atoms = ap.clayff(atoms, box)
            clayff_retype(atoms)
            charge_diff = clayff_charges(atoms)
        else:
            atoms = ap.minff(atoms, box)
        itp_tmp = os.path.join(tmp, "min.itp")
        ap.write_itp(atoms, box, itp_tmp, molecule_name="MIN", KANGLE=angle_k,
                     water_model="spce" if clay else water)
        if clay:
            clayff_itp(itp_tmp)
        gro_tmp = os.path.join(tmp, "conf.gro")
        ap.write_gro(atoms, box, gro_tmp)
        shutil.copy(itp_tmp, os.path.join(outdir, "min.itp"))
        shutil.copy(gro_tmp, os.path.join(outdir, "conf.gro"))

    add_posres_noh(os.path.join(outdir, "min.itp"))
    z_moved = contiguous_z(os.path.join(outdir, "conf.gro"))
    ffname = "clayff.ff" if clay else "min.ff"
    dst = os.path.join(outdir, ffname)
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(os.path.join(ffdir, ffname), dst)
    known = clayff_types(ffdir) if clay else ff_block_types(ffdir, variant, angle_k, mineral)
    if variant == "tminff":
        with open(os.path.join(outdir, "min_bonded.itp"), "w", encoding="utf-8") as f:
            f.write(restrict_bonded(os.path.join(ffdir, "min.ff", "ffbonded.itp"), known))
    with open(os.path.join(outdir, "topol.top"), "w", encoding="utf-8") as f:
        f.write(topol_text_clayff(clayff_bend, clayff_rigid_water) if clay else
                topol_text(variant, angle_k, mineral, water, ion_set))

    itp = read_itp_atoms(os.path.join(outdir, "min.itp"))
    counts = {}
    for t, _ in itp:
        counts[t] = counts.get(t, 0) + 1
    # atomipy leaves an atom it cannot classify with its element as type
    untyped = [i + 1 for i, (t, _) in enumerate(itp) if t not in known]
    summary = {
        "variant": variant.upper() if variant in ("gminff", "clayff") else "TMINFF",
        "forcefield": forcefield,
        "clayff_bend": clayff_bend if clay else None,
        "clayff_rigid_water": bool(clay and clayff_rigid_water),
        "charges_differing_from_atomipy": {t: v for t, v in charge_diff.items()} if clay else None,
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
        "z_moved_atoms": z_moved,      # atoms shifted by a cell vector to make the layer contiguous
        "types_missing_from_ff": sorted(t for t in counts if t not in known),
        "minff_commit": None if clay else MINFF_COMMIT if not os.environ.get("ECCE_MINFF_DIR") else "ECCE_MINFF_DIR",
        "files": {"topology": "topol.top", "molecule": "min.itp", "structure": "conf.gro"},
    }
    z, c = read_gro_z(os.path.join(outdir, "conf.gro"))
    edges = itp_edges(os.path.join(outdir, "min.itp"))
    summary["warnings"] = []
    if clay and abs(summary["total_charge"] - round(summary["total_charge"])) > 1e-4:
        summary["warnings"].append(
            "the charges of Table 1 of the ClayFF paper give this structure a net charge "
            "of %.4f e; its oxygens are not all of the kinds the table lists "
            "(see charges_differing_from_atomipy in minff.json)" % summary["total_charge"])
    # ClayFF has no bonds across a layer, so a layer touching its image is no error
    wound = winding_bonds(z, c, edges) if z and c > 0 and not clay else 0
    if wound:
        cuts = widest_gaps(z, c)
        gap = cuts[0][1] if cuts else 0.0
        fixed = None
        for z0, g in (cuts if add_gap else ()):
            # rebuild from the structure opened here, the cell lengthened; the
            # right place leaves no false bonds and every atom typed
            wide = os.path.join(outdir, "conf_gap.gro")
            trial = tempfile.mkdtemp(dir=outdir, prefix="gap-")
            regap(os.path.join(outdir, "conf.gro"), wide, z0, max(add_gap - g, 0.0), names0)
            try:
                s2 = build(wide, trial, (1, 1, 1), (), min_distance, seed, variant,
                           mineral, angle_k, water, log, ion_set=ion_set, gui=gui,
                           forcefield=forcefield, clayff_bend=clayff_bend,
                           clayff_rigid_water=clayff_rigid_water)
                if not s2["warnings"] and not s2["untyped_atoms"]:
                    for fn in os.listdir(trial):
                        dst = os.path.join(outdir, fn)
                        if os.path.isdir(dst):
                            shutil.rmtree(dst)
                        elif os.path.exists(dst):
                            os.remove(dst)
                        shutil.move(os.path.join(trial, fn), dst)
                    s2.update(cell=summary["cell"], replicate=summary["replicate"],
                              substitutions=summary["substitutions"], seed=seed,
                              z_moved_atoms=z_moved, gap_added_nm=round(max(add_gap - g, 0.0), 3))
                    fixed = s2
            finally:
                os.remove(wide)
                shutil.rmtree(trial, ignore_errors=True)
            if fixed:
                break
        if fixed:
            summary = fixed
        else:
            if add_gap:
                # No gap opens without breaking bonds: the crystal is not
                # layered along c, so water cannot simply be stacked on it.
                summary["warnings"].append(
                    "%d bonds join the mineral to its own periodic image along c, "
                    "and no gap could be opened between layers: this crystal is not "
                    "layered along c. For the bulk crystal, set the water layer to "
                    "0. For water on a surface, start from a slab cell (the crystal "
                    "cut along a plane, with its surface terminated) and repeat it "
                    "once along c." % wound)
            else:
                summary["warnings"].append(
                    "%d bonds join the mineral to its own periodic image along c: the "
                    "cell has too little vacuum (at most about %.2f nm; MINFF needs "
                    "about %.1f nm), so its layer is bonded to itself. Lengthen the "
                    "cell along c%s." % (wound, gap, GAP_NM,
                                         "" if gui else ", or build with --add-gap"))
    for w in summary["warnings"]:
        print("minff_topology: WARNING: " + w, file=sys.stderr)
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
    b.add_argument("--add-gap", type=float, nargs="?", const=GAP_NM, default=None,
                   metavar="NM", help="lengthen the cell along c so that the mineral has "
                   "NM (default %.1f) of vacuum to its periodic image; off by default, "
                   "a bare cell is only warned about" % GAP_NM)
    b.add_argument("--ion-set", choices=ION_SETS, default="HFE",
                   help="ion parameter set of min.ff: HFE (hydration free energy, "
                   "default) or IOD (ion-oxygen distance)")
    b.add_argument("--forcefield", choices=("minff", "clayff"), default="minff",
                   help="minff (default) or clayff (Cygan et al. 2004, clayff.ff; "
                   "types with atomipy's clayff(), water is flexible SPC)")
    b.add_argument("--clayff-bend", choices=CLAYFF_BENDS, default="none",
                   help="with --forcefield clayff, the M-O-H bend written into "
                   "topol.top: none (default), moh (Pouvreau 2017) or moh2004")
    b.add_argument("--clayff-rigid-water", action="store_true",
                   help="with --forcefield clayff, write #define RIGID_WATER (SETTLE SPC)")
    b.add_argument("--gui", action="store_true",
                   help="worded for the ECCE editor: warnings do not name command-line flags")
    m = sub.add_parser("minerals", help="list the TMINFF minerals")
    m.add_argument("--angle-k", type=int, default=500, choices=ANGLE_KS)
    sub.add_parser("fetch", help="download the pinned minff commit (for the tests) and print its directory")
    a = p.parse_args(argv)
    if a.cmd == "build" and not os.environ.get("ECCE_MINFF_REEXEC"):
        try:
            import_atomipy()
        except MinffError:
            py = find_atomipy_python()
            if not py:
                print("minff_topology: " + ATOMIPY_HINT, file=sys.stderr)
                return 1
            env = dict(_atomipy_env(), ECCE_MINFF_REEXEC="1")
            return subprocess.call([py, os.path.abspath(__file__)] + (
                sys.argv[1:] if argv is None else list(argv)), env=env)
    try:
        if a.cmd == "fetch":
            print(fetch_pinned())
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
                      a.variant, a.mineral, a.angle_k, a.water, add_gap=a.add_gap,
                      ion_set=a.ion_set, gui=a.gui, forcefield=a.forcefield,
                      clayff_bend=a.clayff_bend,
                      clayff_rigid_water=a.clayff_rigid_water)
            print(json.dumps(s, indent=2))
    except MinffError as e:
        print("minff_topology: %s" % e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
