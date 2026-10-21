#!/usr/bin/env python3
"""Opt=QST2 / QST3 decks (#419) for HCN <-> HNC, from the end points of a
real IRC log with the product (and guess) in another atom order, mapped and
aligned by atommap_cli as CalcEd does.  Each deck is checked for the
structure blocks Gaussian requires and, when Gaussian 16 is present, RUN at
HF/3-21G: the job must converge on a stationary point with one imaginary
frequency.

  run_tests.py <source dir> <build dir>
"""
import os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
BUILD = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else os.path.join(ROOT, "build-cmake")
PARSERS = os.path.join(ROOT, "scripts", "parsers")
G16 = "/opt/gaussian/g16/g16"
SYM = {6: "C", 7: "N", 1: "H"}


def irc_points(stop_at_ts=False):
    """'Input orientation' geometries of the log with atoms C, N, H; with
    stop_at_ts only those of the Opt=TS stage (the last is the converged TS)."""
    pts = []
    lines = open(os.path.join(
        ROOT, "tests/parsers/fixtures/gaussian-16/hcn_irc.log")).read().splitlines()
    if stop_at_ts:
        lines = lines[:[i for i, l in enumerate(lines) if "Stationary point found" in l][0]]
    i = 0
    while i < len(lines):
        if "Input orientation" in lines[i]:
            j, atoms = i + 5, []
            while "-----" not in lines[j]:
                f = lines[j].split()
                atoms.append((SYM[int(f[1])], [float(x) for x in f[3:6]]))
                j += 1
            if [a[0] for a in atoms] == ["C", "N", "H"]:
                pts.append(atoms)
            i = j
        i += 1
    return pts


def dist(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5


def write_xyz(path, atoms):
    with open(path, "w") as f:
        f.write("%d\nx\n" % len(atoms))
        for e, c in atoms:
            f.write("%s %.8f %.8f %.8f\n" % ((e,) + tuple(c)))


def read_xyz(path):
    ls = open(path).read().splitlines()[2:]
    return [(l.split()[0], [float(x) for x in l.split()[1:4]]) for l in ls if l.strip()]


def write_frag(path, title, atoms):
    with open(path, "w") as f:
        f.write("ecce\nnum_atoms: %d\ntitle: %s\natom_info: symbol x y z\natom_list:\n"
                % (len(atoms), title))
        for e, c in atoms:
            f.write("%s %.10f %.10f %.10f\n" % ((e,) + tuple(c)))


def deck(work, mode, reactant, product, guess):
    """The files CalcEd writes: all structures already mapped."""
    shutil.copy(os.path.join(PARSERS, "g16.tpl"), os.path.join(work, "t.tpl"))
    structs = [("reactant", reactant), ("product", product)]
    if mode == "QST3":
        structs.append(("guess", guess))
    write_frag(os.path.join(work, "w.frag"), "hcn", reactant)
    for k, (name, a) in enumerate(structs):
        write_frag(os.path.join(work, "w.qst%d.frag" % (k + 1)), name, a)
    open(os.path.join(work, "w.basis"), "w").write("useRouteCard 3-21G\n")
    open(os.path.join(work, "w.param"), "w").write(
        "Category: SCF\nTheory: RHF\nRunType: GeoVib\nCharge: 0\n"
        "ChemSys.Multiplicity: 1\ntitle: w\n"
        "ES.Runtype.GeomOpt.SearchFor: Transition State (QST)\n")
    env = dict(os.environ, ECCE_HOME=ROOT)
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.gauss16"), "-p", "-f", "-b",
                        "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                       capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stdout + p.stderr)
    return open(os.path.join(work, "t.tpl")).read()


def main():
    pts = irc_points()
    hcn = min(pts, key=lambda a: dist(a[0][1], a[2][1]))   # shortest C-H
    hnc = min(pts, key=lambda a: dist(a[1][1], a[2][1]))   # shortest N-H
    ts = irc_points(True)[-1]   # converged TS of the log's Opt=TS stage
    work = tempfile.mkdtemp(prefix="ecce-qst-")
    fails = 0
    try:
        # product and guess written H, N, C like a structure built elsewhere
        for name, a in (("prod", hnc), ("guess", ts)):
            write_xyz(os.path.join(work, name + ".xyz"), [a[2], a[1], a[0]])
        write_xyz(os.path.join(work, "react.xyz"), hcn)
        outs = {}
        for name in ("prod", "guess"):
            p = subprocess.run([os.path.join(BUILD, "atommap_cli"), "react.xyz",
                                name + ".xyz", "--write", name + "_out.xyz"], cwd=work,
                               capture_output=True, text=True)
            print(p.stdout)
            if p.returncode:
                print(p.stderr)
                return 1
            outs[name] = read_xyz(os.path.join(work, name + "_out.xyz"))
        for mode in ("QST2", "QST3"):
            d = os.path.join(work, mode)
            os.mkdir(d)
            text = deck(d, mode, hcn, outs["prod"], outs["guess"])
            print("----- %s deck\n%s" % (mode, text))
            nblocks = len(re.findall(r"^0 1\b", text, re.M))
            ok = ("Opt=(%s " % mode) in text and "Freq" in text \
                and nblocks == (2 if mode == "QST2" else 3)
            for blk in re.findall(r"^0 1\n((?: *[A-Z][a-z]? .*\n)+)", text, re.M):
                if [l.split()[0] for l in blk.splitlines()] != ["C", "N", "H"]:
                    ok = False
            print("%-4s %s structure blocks and atom order" % ("ok" if ok else "FAIL", mode))
            fails += not ok
            if not os.path.exists(G16):
                print("SKIP running %s: no Gaussian 16 at %s" % (mode, G16))
                continue
            env = dict(os.environ, g16root="/opt/gaussian",
                       GAUSS_EXEDIR="/opt/gaussian/g16/bsd:/opt/gaussian/g16",
                       GAUSS_ARCHDIR="/opt/gaussian/g16/arch",
                       GAUSS_BSDDIR="/opt/gaussian/g16/bsd", GAUSS_SCRDIR=d)
            open(os.path.join(d, "in.com"), "w").write(text)
            with open(os.path.join(d, "in.com")) as fin, \
                    open(os.path.join(d, "out.log"), "w") as fout:
                subprocess.run([G16], stdin=fin, stdout=fout, stderr=subprocess.STDOUT,
                               cwd=d, env=env, timeout=900)
            log = open(os.path.join(d, "out.log")).read()
            freqs = re.findall(r"Frequencies --\s+(-?[\d.]+)", log)
            neg = [f for f in freqs if f.startswith("-")]
            ok = "Stationary point found" in log and len(neg) == 1 \
                and "Normal termination" in log
            print("%-4s %s Gaussian run: frequencies %s" % ("ok" if ok else "FAIL", mode, freqs[:3]))
            if not ok:
                print(log[-3000:])
            fails += not ok
    finally:
        if fails == 0:
            shutil.rmtree(work, ignore_errors=True)
        else:
            print("kept", work)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
