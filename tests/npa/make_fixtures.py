#!/usr/bin/env python3
"""Makes the NPA fixtures (#1054): ORCA B3LYP/def2-SVP single points with
`! NBO` (NBO 7's NPA is the independent oracle), each run through ECCE
(tests/launch harness: isolated HOME and data, ai.orca deck, Launch, job
store), so the results.h5 is the one ECCE writes.  Copies each results.h5
and NBO's natural charges/spin populations into tests/npa/fixtures.

    NBOEXE=<nbo7.i8.exe> ORCA=<orca> python3 tests/npa/make_fixtures.py <workdir>
"""
import glob, json, math, os, re, shutil, subprocess, sys, time

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BUILD = os.environ.get("ECCE_BUILD", os.path.join(REPO, "build-cmake"))
PARSERS = os.path.join(REPO, "scripts", "parsers")
FIX = os.path.join(REPO, "tests", "npa", "fixtures")
OUT = os.path.abspath(sys.argv[1])
ONLY = sys.argv[2:]
PROCS = int(os.environ.get("NPA_PROCS", "4"))
os.makedirs(OUT, exist_ok=True)


def log(t):
    print("NPA: " + t, flush=True)


def nboexe():
    for p in [os.environ.get("NBOEXE", "")] + [os.path.join(os.environ.get("REALHOME", "/nonexistent"), "nbo7/nbo7/bin/nbo7.i8.exe")]:
        if p and os.access(p, os.X_OK):
            return p
    return ""


def frag(title, atoms):
    return ("fragment\ntitle: %s\nnum_atoms: %d\natom_info: symbol\natom_list:\n%s" %
            (title, len(atoms), "".join("%s %.6f %.6f %.6f\n" % a for a in atoms)))


def mvm(title, atoms):
    """The chemical system a calculation made in the Builder holds; without it
    the job store writes no results.h5 and the Builder shows no panels."""
    bonds = []
    for i in range(len(atoms)):
        for j in range(i + 1, len(atoms)):
            d = math.dist(atoms[i][1:], atoms[j][1:])
            if d < (1.25 if "H" in (atoms[i][0], atoms[j][0]) else 1.75):
                bonds.append((i + 1, j + 1))
    return ("title: %s\ntype: molecule\nnum_atoms: %d\natom_info: symbol cart\natom_list: \n%s"
            "attr_list:\n%satom_type_list:\n%snum_bonds: %d\nbond_list: \n%s") % (
        title, len(atoms), "".join("%s %.6f %.6f %.6f\n" % a for a in atoms),
        " 0.000000   1   0   0   0\n" * len(atoms), "        0   1\n" * len(atoms),
        len(bonds), "".join("%d %d 1.00000 1\n" % b for b in bonds))



MOLS = {
    "water": (0, 1, [("O", 0, 0, 0.1173), ("H", 0, 0.7572, -0.4692), ("H", 0, -0.7572, -0.4692)]),
    "ethene": (0, 1, [("C", -0.667, 0.0, 0.0), ("C", 0.667, 0.0, 0.0), ("H", -1.235, 0.921, 0.0),
                      ("H", -1.235, -0.921, 0.0), ("H", 1.235, 0.921, 0.0), ("H", 1.235, -0.921, 0.0)]),
    "formaldehyde": (0, 1, [("C", 0, 0, -0.529), ("O", 0, 0, 0.676), ("H", 0, 0.935, -1.114),
                            ("H", 0, -0.935, -1.114)]),
    "acetate": (-1, 1, [("C", 0, 0, 0), ("O", 1.126, 0, 0.652), ("O", -1.126, 0, 0.652),
                        ("C", 0, 0, -1.55), ("H", 0, 1.03, -1.92), ("H", 0.89, -0.515, -1.92),
                        ("H", -0.89, -0.515, -1.92)]),
    "methyl": (0, 2, [("C", 0, 0, 0), ("H", 1.079, 0, 0), ("H", -0.5395, 0.9344, 0),
                      ("H", -0.5395, -0.9344, 0)]),
    "iodomethane": (0, 1, [("C", 0, 0, 0), ("I", 0, 0, 2.14), ("H", 1.03, 0, -0.36),
                           ("H", -0.515, 0.892, -0.36), ("H", -0.515, -0.892, -0.36)]),
}
fe = [("Fe", 0, 0, 0)]
for ax in range(3):
    for sg in (1, -1):
        o = [0.0, 0.0, 0.0]; o[ax] = 2.12 * sg
        fe.append(("O",) + tuple(o))
        for hs in (1, -1):
            h = [0.0, 0.0, 0.0]; h[ax] = 2.70 * sg; h[(ax + 1) % 3] = 0.76 * hs
            fe.append(("H",) + tuple(h))
MOLS["fe_h2o6"] = (2, 5, fe)


def param(charge, mult):
    return ("Category: DFT\nTheory: %s\nRunType: Energy\nCharge: %d\nChemSys.Multiplicity: %d\n"
            "ES.Theory.DFT.XCFunctionals: B3LYP\nES.Theory.UseSymmetry: 0\n"
            "ES.Runtype.ORCA.UseNBO: 1\nES.ChemSys.Multiplicity: %d\nSphericalBasis: TRUE\n" % ("UDFT" if mult > 1 else "RDFT", charge, mult, mult))


def deck(name, charge, mult, atoms):
    work = os.path.join(OUT, "decks", name)
    shutil.rmtree(work, ignore_errors=True); os.makedirs(work)
    shutil.copy(os.path.join(PARSERS, "orca.tpl"), os.path.join(work, "t.tpl"))
    open(os.path.join(work, "w.frag"), "w").write(mvm(name, atoms))  # import reads MVM
    open(os.path.join(work, "chemsys.mvm"), "w").write(mvm(name, atoms))
    open(os.path.join(work, "w.basis"), "w").write("useRouteCard def2-SVP\n")
    open(os.path.join(work, "w.param"), "w").write(param(charge, mult))
    # the basis as the Calculation Editor stores it (library name per element):
    # without it the job store cannot map the orbitals and writes no results.h5
    with open(os.path.join(work, "w.gbs"), "w") as g:
        g.write('NameBasis\nbasis "ao basis" spherical print\n')
        for el in sorted({a[0] for a in atoms}):
            g.write('%s library "def2-SVP"\n' % el)
        g.write("END\nEndNameBasis\n")
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f", "-b", "-n", "w", "-t", "t.tpl"],
                       cwd=work, env=dict(os.environ, ECCE_HOME=REPO), capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    text = open(os.path.join(work, "t.tpl")).read()
    open(os.path.join(work, "orca.orcain"), "w").write(text)
    return os.path.join(work, "orca.orcain")


def nboTables(text):
    """Each 'Summary of Natural Population Analysis' table: [(symbol, charge)]."""
    tabs = []
    for blk in re.split(r"Summary of Natural Population Analysis:", text)[1:]:
        # Atom No Charge Core Valence Rydberg Total [Spin density (open shell)]
        rows = re.findall(r"^\s+([A-Z][a-z]?)\s+(\d+)\s+(-?\d+\.\d+)(?:\s+-?\d+\.\d+){4,5}\s*$",
                          blk[:4000], re.M)
        seen, out = set(), []
        for sym, idx, q in rows:
            if idx in seen:
                continue
            seen.add(idx); out.append((sym, float(q)))
        tabs.append(out)
    return tabs


sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
import harness
orca = os.environ.get("ORCA") or (sorted(glob.glob("/opt/orca/orca_6*/orca")) or [shutil.which("orca")])[-1]
if not nboexe():
    sys.exit("NBOEXE (nbo7.i8.exe) is needed: NBO's NPA is the reference")
os.environ["NBOEXE"] = nboexe()
s = harness.Session(BUILD, "npafix", {"ORCA": orca}, local=True, keep=True)
rundir = os.path.join(s.state, "run"); os.makedirs(rundir, exist_ok=True)
try:
    assert s.services(True)
    for name, (charge, mult, atoms) in MOLS.items():
        if ONLY and name not in ONLY:
            continue
        path = deck(name, charge, mult, atoms)
        url, out = s.create(name, "orca_es", path, "orca.orcain", rundir,
                            extra=("procs=%d" % PROCS, "code=ORCA", "theory=DFT:B3LYP", "runtype=Energy"))
        if not url:
            raise RuntimeError("create %s: %s" % (name, out[-500:]))
        imp = os.path.join(OUT, "import", name)   # import() consumes its files
        shutil.rmtree(imp, ignore_errors=True)
        shutil.copytree(os.path.dirname(path), imp)
        rc, sout = s.driver("setup", url, imp, "w.out")
        if rc != 0:
            raise RuntimeError("setup %s: %s" % (name, sout[-800:]))
        calc = url[len("file://"):].rstrip("/")
        os.makedirs(os.path.join(calc, "Parameters"), exist_ok=True)
        shutil.copy(os.path.join(os.path.dirname(path), "chemsys.mvm"), os.path.join(calc, "Parameters", "chemsys.mvm"))
        t0 = time.time(); s.launch(url)
        st = s.waitState(url, seconds=3600)
        log("%s: %s in %.0f s" % (name, st, time.time() - t0))
        h5 = [p for p in glob.glob(os.path.join(calc, "**", "results.h5"), recursive=True)]
        outs = [p for p in glob.glob(os.path.join(rundir, "**", "*.orcaout"), recursive=True) if name in p] + \
            glob.glob(os.path.join(calc, "**", "*.orcaout"), recursive=True)
        if not h5 or not outs:
            log("%s: no results.h5 (%s) or output (%s)" % (name, h5, outs)); continue
        tabs = nboTables(open(outs[0], errors="replace").read())
        if not tabs:
            log("%s: no NBO summary in %s" % (name, outs[0])); continue
        ref = {"source": "NBO 7 NPA, ORCA B3LYP/def2-SVP, same run",
               "elements": [a[0] for a in tabs[0]], "charge": [a[1] for a in tabs[0]]}
        if mult > 1 and len(tabs) >= 3:   # alpha and beta tables: spin = q(beta) - q(alpha)
            ref["spin"] = [round(b[1] - a[1], 5) for a, b in zip(tabs[1], tabs[2])]
        d = os.path.join(FIX, name); os.makedirs(d, exist_ok=True)
        shutil.copy(h5[0], os.path.join(d, "results.h5"))
        json.dump(ref, open(os.path.join(d, "nbo_reference.json"), "w"), indent=1)
        shutil.copy(outs[0], os.path.join(OUT, name + ".orcaout"))
        log("%s: reference %s" % (name, ref))
finally:
    s.stop()
