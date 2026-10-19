#!/usr/bin/env python3
"""CP2K (Quickstep, #329): input generation, the input checker and real runs.

Decks are made by the real scripts/parsers/ai.cp2k from a .param/.frag pair,
read back line by line, checked with scripts/parsers/verifyinput and, with
CP2K installed, with `cp2k --check`.  Two real runs (bulk silicon, and a
spin-polarised NiO cell with U on Ni that is optimised for three steps) are
then replayed through the real cp2k.desc and parser scripts, and what they
extract is compared with numbers read from CP2K's own output by a separate
regular expression, never with values worked out here.

    tests/cp2k_features/run_tests.py            decks, plus runs if cp2k.psmp is found
    tests/cp2k_features/run_tests.py --no-run   decks only

CP2K_FEATURES_KEEP=<dir> keeps every deck and output there (the fixtures of
tests/parsers/fixtures/cp2k).  The runs use at most 4 MPI ranks.

Exit 77 (skip) when the CP2K data directory is missing, since ai.cp2k will
not write a deck without it; the run part says "skipped" without cp2k.psmp.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
sys.path.insert(0, os.path.join(ROOT, "tests", "parsers"))
import eccejobmonitor_sim as sim  # noqa: E402

LIB = "/usr/share/cp2k"

failures = []


def check(cond, what):
    print("  %s %s" % ("ok  " if cond else "FAIL", what))
    if not cond:
        failures.append(what)


# ---------------------------------------------------------------- systems
SI = dict(
    title="si",
    lattice=["-2.715 0.0 2.715", "0.0 2.715 2.715", "-2.715 2.715 0.0"],
    atoms=[("Si", 0.0, 0.0, 0.0), ("Si", -1.30, 1.40, 1.35)])
# NiO primitive rocksalt cell with the oxygen moved off its site so that
# there are forces to follow.
NIO = dict(
    title="nio",
    lattice=["0.0 2.085 2.085", "2.085 0.0 2.085", "2.085 2.085 0.0"],
    atoms=[("Ni", 0.0, 0.0, 0.0), ("O", 2.135, 2.115, 2.085)])

CHEAP = ["ES.Theory.GPW.Cutoff: 100", "ES.Theory.GPW.RelCutoff: 30"]
NIO_SPIN = ["ChemSys.Multiplicity: 3", "ES.Theory.GPW.BasisLevel: SZV-MOLOPT-SR-GTH",
            "ES.Theory.GPW.Cutoff: 150", "ES.Theory.GPW.RelCutoff: 30",
            "ES.Theory.GPW.HubbardElement1: Ni", "ES.Theory.GPW.HubbardOrbital1: d",
            "ES.Theory.GPW.HubbardU1: 4.0", "ES.Theory.GPW.OTMinimizer: CG",
            "ES.Theory.GPW.OTPreconditioner: FULL_ALL", "ES.Theory.GPW.MaxScf: 300"]


def deck(system, runtype, params, work=None, lattice=True):
    """Return (deck text, ai.cp2k stderr, return code)."""
    own = work is None
    work = work or tempfile.mkdtemp(prefix="ecce-cp2kfeat-")
    try:
        tpl = os.path.join(work, "w.cp2kin")
        shutil.copy(os.path.join(PARSERS, "cp2k.tpl"), tpl)
        atoms = system["atoms"]
        cell = ("lattice_vectors:\n%s\n" % "\n".join(system["lattice"])) if lattice else ""
        with open(os.path.join(work, "w.frag"), "w") as f:
            f.write("ECCE fragment\ntitle: %s\ntype: crystal\nnum_atoms: %d\n"
                    "atom_info: symbol cart\n%satom_list:\n%s"
                    % (system["title"], len(atoms), cell,
                       "".join("%s %.8f %.8f %.8f\n" % a for a in atoms)))
        p = ["title: " + system["title"], "RunType: " + runtype, "Charge: 0",
             "ChemSys.Multiplicity: 1"] + list(params)
        with open(os.path.join(work, "w.param"), "w") as f:
            f.write("\n".join(p) + "\n")
        r = subprocess.run(["perl", os.path.join(PARSERS, "ai.cp2k"), "-n", "w", "-p", "-f",
                            "-t", tpl], cwd=work, env=dict(os.environ, ECCE_HOME=ROOT),
                           capture_output=True, text=True)
        return open(tpl).read(), r.stderr, r.returncode
    finally:
        if own:
            shutil.rmtree(work, ignore_errors=True)


def verify(text, atoms=None):
    d = tempfile.mkdtemp(prefix="ecce-cp2kverify-")
    try:
        path = os.path.join(d, "x.cp2kin")
        open(path, "w").write(text)
        cmd = ["perl", os.path.join(PARSERS, "verifyinput"), "-f", path, "-c", "CP2K"]
        if atoms:
            cmd += ["-a", str(atoms)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        return [l.split("|", 3) for l in r.stdout.splitlines() if l.count("|") >= 3]
    finally:
        shutil.rmtree(d, ignore_errors=True)


def has(findings, level, name):
    return any(f[0] == level and f[2] == name for f in findings)


def section(text, name):
    m = re.search(r"^(\s*)&%s\b.*?\n(.*?)^\1&END %s" % (name, name), text, re.S | re.M)
    return m.group(2) if m else None


def cp2k_check(text):
    """True/False from `cp2k --check`, None without cp2k."""
    exe = shutil.which("cp2k.psmp") or shutil.which("cp2k.ssmp") or shutil.which("cp2k")
    if not exe:
        return None
    d = tempfile.mkdtemp(prefix="ecce-cp2kchk-")
    try:
        path = os.path.join(d, "x.inp")
        open(path, "w").write(text)
        r = subprocess.run([exe, "--check", path], capture_output=True, text=True, cwd=d, timeout=120)
        return "SUCCESS" in r.stdout
    finally:
        shutil.rmtree(d, ignore_errors=True)


# What the library files hold, read here with a pattern of its own (not
# ai.cp2k's) so that the generator is not compared with itself.
def library_names(filename):
    names = {}
    for line in open(os.path.join(LIB, filename)):
        m = re.match(r"^\s*([A-Z][a-z]?)\s+(\S+.*)$", line)
        if m and not line.lstrip().startswith("#") and re.match(r"[A-Za-z]", m.group(2)):
            names.setdefault(m.group(1), []).extend(m.group(2).split())
    return names


# ---------------------------------------------------------------- deck tests
def key_tests():
    """The dialogs, the generator and the editor's summary use one set of keys."""
    print("keys")
    dialog = set()
    for f in ("cp2ktheory.py", "cp2kruntype.py"):
        text = open(os.path.join(ROOT, "scripts", "codereg", f)).read()
        for m in re.finditer(r'name="(ES\.[A-Za-z0-9_.]+?)(%d)?"', text):
            dialog.add(m.group(1) + ("N" if m.group(2) else ""))
    gen = set()
    for m in re.finditer(r'"(ES\.[A-Za-z0-9_.]+?)(\$i)?"',
                         open(os.path.join(PARSERS, "ai.cp2k")).read()):
        gen.add(m.group(1) + ("N" if m.group(2) else ""))
    check(len(dialog) > 20 and dialog == gen,
          "every dialog widget is read by ai.cp2k and the reverse (%d keys; "
          "dialog only %s, ai.cp2k only %s)" % (len(dialog), sorted(dialog - gen), sorted(gen - dialog)))
    edml = open(os.path.join(ROOT, "data", "client", "cap", "CP2K.edml")).read()
    summary = set(re.findall(r'<item key="(ES\.[A-Za-z0-9_.]+)"', edml))
    plain = {k for k in dialog if not k.endswith("N")}
    check(summary and summary <= plain, "every key of the editor's summary is a dialog widget %s"
          % sorted(summary - plain))


def deck_tests():
    key_tests()
    print("a silicon energy deck")
    text, err, rc = deck(SI, "Energy", CHEAP)
    check(rc == 0, "ai.cp2k exits 0")
    check("RUN_TYPE ENERGY_FORCE" in text and "PROJECT si" in text,
          "RUN_TYPE ENERGY_FORCE (the forces box is ticked by default), PROJECT si")
    check("BASIS_SET DZVP-MOLOPT-SR-GTH-q4" in text and "POTENTIAL GTH-PBE-q4" in text,
          "Si: DZVP-MOLOPT-SR-GTH-q4 with GTH-PBE-q4")
    check("BASIS_SET_FILE_NAME BASIS_MOLOPT" in text and "POTENTIAL_FILE_NAME GTH_POTENTIALS" in text,
          "bare file names while the directory is CP2K's own")
    check("UKS" not in text and "MULTIPLICITY" not in text and "CHARGE 0" in text,
          "a singlet is restricted")
    check(re.search(r"A \[angstrom\]\s+-2\.7150000000\s+0\.0000000000\s+2\.7150000000", text)
          and "PERIODIC XYZ" in text, "the cell is the fragment's lattice, periodic in XYZ")
    check("&MOTION" not in text, "no &MOTION for an energy")
    check(len(re.findall(r"^\s+Si\s+-?\d", text, re.M)) == 2, "two Si atoms in &COORD")
    ok = cp2k_check(text)
    check(ok in (True, None), "cp2k --check accepts it" if ok else "cp2k --check skipped (no cp2k)")
    f = verify(text, 2)
    check(has(f, "GOOD", "kinds") and has(f, "GOOD", "sections") and has(f, "GOOD", "geometry")
          and not any(x[0] == "BAD" for x in f), "Verify finds nothing wrong")

    print("energy without forces")
    text, _, _ = deck(SI, "Energy", CHEAP + ["ES.Runtype.GPW.PrintForces: 0"])
    check("RUN_TYPE ENERGY\n" in text and "&FORCES" not in text, "RUN_TYPE ENERGY, no &PRINT &FORCES")

    print("functionals: section, potential family, D3 reference")
    lib_pot = library_names("GTH_POTENTIALS")
    for label, want, fam, ref in [
            ("PBE", "&PBE", "GTH-PBE", "PBE"),
            ("revPBE", "PARAMETRIZATION REVPBE", "GTH-PBE", "revPBE"),
            ("BLYP", "&BECKE88", "GTH-BLYP", "BLYP"),
            ("BP86", "&P86C", "GTH-BP", "BP"),
            ("LDA (Pade)", "&PADE", "GTH-PADE", None)]:
        p = CHEAP + ["ES.Theory.GPW.Functional: " + label]
        text, err, rc = deck(SI, "Energy", p + (["ES.Theory.GPW.Dispersion: DFT-D3 (zero damping)"] if ref else []))
        xc = section(text, "XC_FUNCTIONAL") or ""
        pot = re.search(r"POTENTIAL (\S+)", text)
        ok_fam = pot and pot.group(1).startswith(fam + "-q") and pot.group(1) in lib_pot.get("Si", [])
        check(want in xc and ok_fam and (ref is None or ("REFERENCE_FUNCTIONAL %s\n" % ref) in text),
              "%s -> %s, potential %s%s" % (label, want, pot.group(1) if pot else "?",
                                           ", D3 reference " + ref if ref else ""))
        ok = cp2k_check(text)
        check(ok in (True, None), "  cp2k --check accepts it")
    text, err, rc = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.Functional: LDA (Pade)",
                                                "ES.Theory.GPW.Dispersion: DFT-D3 (zero damping)"])
    check(rc != 0 and "DFT-D3 has no parameters" in err, "LDA with D3 is refused, with a reason")

    print("dispersion")
    text, _, _ = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.Dispersion: DFT-D3 (Becke-Johnson damping)"])
    check("TYPE DFTD3(BJ)" in text and "PARAMETER_FILE_NAME dftd3.dat" in text, "D3(BJ) -> TYPE DFTD3(BJ)")
    check(cp2k_check(text) in (True, None), "  cp2k --check accepts it")
    text, _, _ = deck(SI, "Energy", CHEAP)
    check("VDW_POTENTIAL" not in text, "no dispersion by default")

    print("basis levels and fallback")
    lib_bas = library_names("BASIS_MOLOPT")
    for lvl in ("DZVP-MOLOPT-GTH", "SZV-MOLOPT-SR-GTH", "TZVP-MOLOPT-GTH", "TZV2P-MOLOPT-GTH"):
        text, _, rc = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.BasisLevel: " + lvl])
        want = "%s-q4" % lvl
        check(rc == 0 and ("BASIS_SET %s\n" % want) in text and want in lib_bas["Si"],
              "level %s -> %s" % (lvl, want))
    # An element with a plain MOLOPT set but no short-range one falls back.
    only_plain = [el for el, n in lib_bas.items()
                  if any(x.startswith("DZVP-MOLOPT-GTH-q") for x in n)
                  and not any(x.startswith("DZVP-MOLOPT-SR-GTH-q") for x in n)
                  and any(x.startswith("GTH-PBE-q") for x in lib_pot.get(el, []))]
    if only_plain:
        el = only_plain[0]
        text, err, rc = deck(dict(title="x", lattice=SI["lattice"], atoms=[(el, 0, 0, 0)]),
                             "Energy", CHEAP)
        check(rc == 0 and re.search(r"BASIS_SET DZVP-MOLOPT-GTH-q\d+\n", text),
              "%s has no short-range set: DZVP-MOLOPT-GTH is used instead" % el)
    text, err, rc = deck(dict(title="x", lattice=SI["lattice"], atoms=[("Yb", 0, 0, 0)]), "Energy", CHEAP)
    check(rc != 0 and "Yb" in err and "no basis set and potential" in err,
          "an element the library has no MOLOPT set for is refused by name")

    print("per-element basis and potential")
    fe = dict(title="fe", lattice=["2.87 0 0", "0 2.87 0", "0 0 2.87"], atoms=[("Fe", 0, 0, 0)])
    text, _, rc = deck(fe, "Energy", CHEAP)
    check("POTENTIAL GTH-PBE-q16" in text and "BASIS_SET DZVP-MOLOPT-SR-GTH-q16" in text,
          "Fe: the default potential is the file's GTH-PBE (q16), with the q16 basis")
    text, err, rc = deck(fe, "Energy", CHEAP + ["ES.Theory.GPW.KindElement1: Fe",
                                               "ES.Theory.GPW.KindPotential1: GTH-PBE-q8"])
    check(rc != 0 and "Fe" in err, "a q8 potential has no q8 basis: refused, naming Fe")
    text, err, rc = deck(fe, "Energy", CHEAP + [
        "ES.Theory.GPW.KindElement1: Fe", "ES.Theory.GPW.KindPotential1: GTH-PBE-q8",
        "ES.Theory.GPW.KindBasis1: DZVP-MOLOPT-SR-GTH-q16"])
    check(rc == 0 and "POTENTIAL GTH-PBE-q8" in text and "BASIS_SET DZVP-MOLOPT-SR-GTH-q16" in text,
          "both named in the row: written as given")
    text, err, rc = deck(fe, "Energy", CHEAP + ["ES.Theory.GPW.KindElement1: Cu",
                                               "ES.Theory.GPW.KindBasis1: X"])
    check(rc == 0 and "Cu" in err and "X\n" not in text, "a row for an element not in the structure is ignored with a note")

    print("spin")
    text, _, _ = deck(NIO, "Energy", CHEAP + ["ChemSys.Multiplicity: 3"])
    check("UKS" in text and "MULTIPLICITY 3" in text, "multiplicity 3 forces UKS")
    text, _, _ = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.SpinType: Unrestricted (UKS)"])
    check("UKS" in text and "MULTIPLICITY 1" in text, "Unrestricted with multiplicity 1: UKS, MULTIPLICITY 1")
    bad = re.sub(r"^\s*UKS\n", "", deck(NIO, "Energy", CHEAP + ["ChemSys.Multiplicity: 3"])[0], flags=re.M)
    check(has(verify(bad, 2), "BAD", "spin"), "Verify flags a multiplicity without UKS")

    print("DFT+U")
    u = CHEAP + ["ChemSys.Multiplicity: 3", "ES.Theory.GPW.HubbardElement1: Ni",
                 "ES.Theory.GPW.HubbardOrbital1: d", "ES.Theory.GPW.HubbardU1: 4.5",
                 "ES.Theory.GPW.HubbardElement2: Fe", "ES.Theory.GPW.HubbardU2: 3.0"]
    text, err, rc = deck(NIO, "Energy", u)
    ni = re.search(r"&KIND Ni\n(.*?)&END KIND", text, re.S).group(1)
    o = re.search(r"&KIND O\n(.*?)&END KIND", text, re.S).group(1)
    check("&DFT_PLUS_U" in ni and "L 2" in ni and "U_MINUS_J [eV] 4.5" in ni and "DFT_PLUS_U" not in o,
          "U-J = 4.5 eV on the Ni 3d kind only")
    check("Fe" in err, "a row for an element not in the structure is ignored with a note")
    check(cp2k_check(text) in (True, None), "cp2k --check accepts it")
    check(has(verify(text, 2), "GOOD", "plus_u"), "Verify marks DFT+U as seen")
    text, _, _ = deck(NIO, "Energy", u[:2] + ["ES.Theory.GPW.HubbardElement1: Ni",
                                              "ES.Theory.GPW.HubbardOrbital1: f",
                                              "ES.Theory.GPW.HubbardU1: 2.0"])
    check("L 3" in text, "orbital f -> L 3")

    print("SCF")
    text, _, _ = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.UseOT: 0", "ES.Theory.GPW.SmearTemperature: 300"])
    scf = section(text, "SCF")
    check("&OT" not in scf and "&DIAGONALIZATION" in scf and "&MIXING" in scf and "ADDED_MOS 20" in scf
          and "ELECTRONIC_TEMPERATURE [K] 300" in scf, "OT off: diagonalisation, mixing and smearing")
    check(cp2k_check(text) in (True, None), "  cp2k --check accepts it")
    text, _, _ = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.OTMinimizer: CG",
                                             "ES.Theory.GPW.OTPreconditioner: FULL_ALL",
                                             "ES.Theory.GPW.EpsScf: 1e-8", "ES.Theory.GPW.MaxScf: 77"])
    scf = section(text, "SCF")
    check("MINIMIZER CG" in scf and "LINESEARCH 2PNT" in scf and "PRECONDITIONER FULL_ALL" in scf
          and "EPS_SCF 1e-8" in scf and "MAX_SCF 77" in scf, "OT with CG, FULL_ALL, EPS_SCF 1e-8, MAX_SCF 77")
    check(cp2k_check(text) in (True, None), "  cp2k --check accepts it")

    print("a geometry optimisation deck")
    text, _, rc = deck(NIO, "Geometry", CHEAP + ["ES.Runtype.GPW.MaxIter: 7",
                                                  "ES.Runtype.GPW.Optimizer: LBFGS",
                                                  "ES.Runtype.GPW.MaxForce: 1.0E-3",
                                                  "ES.Runtype.GPW.MaxDR: 6.0E-3"])
    mo = section(text, "MOTION") or ""
    check("RUN_TYPE GEO_OPT" in text and "OPTIMIZER LBFGS" in mo and "MAX_ITER 7" in mo
          and "MAX_FORCE 1.0E-3" in mo and "RMS_FORCE 0.000666667" in mo and "MAX_DR 6.0E-3" in mo
          and "RMS_DR 0.003" in mo, "GEO_OPT with LBFGS, 7 steps and the criteria")
    check("FILENAME __STD_OUT__" in mo and "&FORCES ON" in text,
          "the trajectory goes into the output; forces are printed")
    check(cp2k_check(text) in (True, None), "cp2k --check accepts it")
    f = verify(text, 2)
    check(has(f, "GOOD", "run") and not any(x[0] == "BAD" for x in f), "Verify finds nothing wrong")
    check(has(verify(text.replace("&MOTION", "&XMOTION").replace("&END MOTION", "&END XMOTION"), 2),
              "BAD", "run"), "Verify flags GEO_OPT without &MOTION")

    print("what the generator refuses, and says why")
    text, err, rc = deck(SI, "Energy", CHEAP, lattice=False)
    check(rc != 0 and "Periodic Builder" in err, "no lattice: refused, naming the Periodic Builder")
    text, err, rc = deck(SI, "Vibration", CHEAP)
    check(rc != 0 and "not supported for CP2K" in err, "an unsupported runtype is refused")
    with tempfile.TemporaryDirectory(prefix="ecce-cp2klib-") as d:
        text, err, rc = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.LibraryDir: " + d])
        check(rc != 0 and "cannot be read" in err, "an unreadable library directory is refused")
        for n in ("BASIS_MOLOPT", "GTH_POTENTIALS"):
            shutil.copy(os.path.join(LIB, n), d)
        text, err, rc = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.LibraryDir: " + d])
        check(rc == 0 and ("BASIS_SET_FILE_NAME %s/BASIS_MOLOPT" % d) in text,
              "a directory typed in the dialog is written in full")
    check(not any(x for x in re.findall(r"##\w+##", deck(SI, "Energy", CHEAP)[0])), "no ##tag## is left in a deck")


# ---------------------------------------------------------------- real runs
def run_cp2k(text, work, name, ranks=4):
    inp = os.path.join(work, name + ".cp2kin")
    out = os.path.join(work, name + ".cp2kout")
    open(inp, "w").write(text)
    env = dict(os.environ, OMP_NUM_THREADS="1")
    cmd = ["mpirun", "-np", str(ranks), "cp2k.psmp", "-i", inp] if shutil.which("mpirun") \
        else ["cp2k.psmp", "-i", inp]
    subprocess.run(["nice", "-n", "19"] + cmd, cwd=work, env=env, stdout=open(out, "w"),
                   stderr=subprocess.STDOUT, timeout=1500)
    text_out = open(out).read()
    keep = os.environ.get("CP2K_FEATURES_KEEP")
    if keep:
        os.makedirs(keep, exist_ok=True)
        for ext in ("cp2kin", "cp2kout"):
            shutil.copy(os.path.join(work, name + "." + ext), keep)
    return out, text_out


def parsed(out, runtype):
    """Replay CP2K output through the real cp2k.desc and parser scripts:
    {key: [flat record of every block]}."""
    desc = sim.read_desc(os.path.join(PARSERS, "cp2k.desc"))
    result = sim.replay(desc, out)
    got = {}
    work = tempfile.mkdtemp(prefix="ecce-cp2kfeat-parse-")
    try:
        for entry in desc.live_entries():
            for block in result.delivered_for(entry):
                if not entry.script:
                    continue
                o, _, _ = sim.run_parser(PARSERS, entry, block, (".", runtype, "QS", "GPW", "0"),
                                         workdir=work)
                for rec in sim.parse_parser_output(o):
                    got.setdefault(rec["key"], []).append(rec["flat"])
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return got


def nums(rx, out):
    return [float(x) for x in re.findall(rx, out, re.M)]


def vals(recs, key):
    return [[float(v) for v in r["values"].split()] for r in recs.get(key, [])]


def near(a, b, tol):
    return len(a) == len(b) and all(abs(x - y) <= tol for x, y in zip(a, b))


def run_tests(work):
    if not shutil.which("cp2k.psmp"):
        print("cp2k.psmp not found: real runs skipped")
        return
    ver = subprocess.run(["cp2k.psmp", "--version"], capture_output=True, text=True).stdout
    print("CP2K %s" % (re.search(r"version (\S+)", ver) or [0, "?"])[1])

    print("(a) bulk silicon, energy and forces")
    text, _, _ = deck(SI, "Energy", CHEAP)
    out, o = run_cp2k(text, work, "si")
    check("PROGRAM STOPPED" in o and "SCF run converged" in o, "CP2K ran to the end and converged")
    got = parsed(out, "Energy")
    e = nums(r"ENERGY\| Total FORCE_EVAL \( QS \) energy \[hartree\]\s+(\S+)", o)
    check(e and vals(got, "TE") and abs(vals(got, "TE")[-1][0] - e[-1]) < 1e-9,
          "TE is the energy CP2K printed (%.9f Hartree)" % e[-1])
    steps = nums(r"^\s+\d+\s+OT DIIS\s+\S+\s+\S+\s+\S+\s+(-\d+\.\d+)\s+\S+\s*$", o)
    check(len(steps) >= 5 and len(got.get("EWVEC", [])) == len(steps)
          and near([v[0] for v in vals(got, "EWVEC")], steps, 1e-9),
          "EWVEC has the %d SCF iteration energies" % len(steps))
    check(len(got.get("DELTAE", [])) == len(steps), "DELTAE has one change per iteration")
    mull = [float(x) for x in re.findall(r"^\s+\d\s+Si\s+1\s+\S+\s+(-?\d+\.\d+)\s*$", o, re.M)]
    check(len(mull) == 2 and near(vals(got, "MULLIKEN")[-1], mull, 1e-9),
          "MULLIKEN equals the net charges of the Mulliken table %s" % mull)
    check("SPINMOM" not in got, "no spin moments for a restricted run")
    check(vals(got, "HIRSHFELD") and len(vals(got, "HIRSHFELD")[-1]) == 2, "HIRSHFELD has one charge per atom")
    forces = nums(r"FORCES\|\s+\d\s+(\S+)\s+(\S+)\s+(\S+)\s+\S+\s*$", o) if False else \
        [float(x) for m in re.findall(r"FORCES\|\s+\d\s+(\S+)\s+(\S+)\s+(\S+)\s+\S+\s*$", o, re.M) for x in m]
    check(len(forces) == 6 and near(vals(got, "EGRADVEC")[-1], [-f for f in forces], 1e-8),
          "EGRADVEC is minus the forces CP2K printed")
    check(vals(got, "ETIME"), "ETIME is read from the timing table")
    check(got.get("VERSION") and got["VERSION"][0]["values"] == "CP2K " + re.search(r"CP2K version (\S+)", o).group(1),
          "VERSION is read")
    check("GEOMTRACE" not in got, "an energy run has no geometry trace")

    print("(b) silicon without OT: diagonalisation, Broyden mixing and smearing")
    text, _, _ = deck(SI, "Energy", CHEAP + ["ES.Theory.GPW.UseOT: 0", "ES.Theory.GPW.SmearTemperature: 300",
                                             "ES.Runtype.GPW.PrintForces: 0"])
    out, o = run_cp2k(text, work, "si_diag")
    check("PROGRAM STOPPED" in o and "SCF run converged" in o, "CP2K ran to the end and converged")
    got = parsed(out, "Energy")
    steps = nums(r"^\s+\d+\s+\S+/\S+\s+\S+\s+\S+\s+\S+\s+(-\d+\.\d+)\s+\S+\s*$", o)
    check(len(steps) >= 3 and near([v[0] for v in vals(got, "EWVEC")], steps, 1e-9),
          "EWVEC has the %d iteration energies of the diagonalisation table" % len(steps))
    e = nums(r"ENERGY\| Total FORCE_EVAL \( QS \) energy \[hartree\]\s+(\S+)", o)
    check(e and abs(vals(got, "TE")[-1][0] - e[-1]) < 1e-9, "TE is the energy CP2K printed")
    check("EGRADVEC" not in got, "no forces were asked for, none are read")

    print("(c) NiO, U on the Ni 3d, UKS with multiplicity 3, three optimisation steps")
    text, _, _ = deck(NIO, "Geometry", NIO_SPIN + ["ES.Runtype.GPW.MaxIter: 3"])
    out, o = run_cp2k(text, work, "nio")
    check("PROGRAM STOPPED" in o and "MAXIMUM NUMBER OF OPTIMIZATION STEPS REACHED" in o,
          "CP2K ran its three steps")
    check("DFT+U energy" in o, "CP2K applied the U (it printed a DFT+U energy)")
    check("Spin restricted" not in o and re.search(r"Multiplicity\s+3", o), "UKS with multiplicity 3")
    got = parsed(out, "Geometry")
    frames = re.findall(r"^\s+i =\s+(\d+), E =\s+(\S+)\s*$", o, re.M)
    check(len(frames) == 3 and len(got.get("GEOMTRACE", [])) == 3,
          "GEOMTRACE has %d frames for %d printed" % (len(got.get("GEOMTRACE", [])), len(frames)))
    check(near([v[0] for v in vals(got, "TEVEC")], [float(x[1]) for x in frames], 1e-9),
          "TEVEC equals the frame energies")
    check(len(got["TEVEC"]) <= len(got["GEOMTRACE"]), "TEVEC is no longer than GEOMTRACE")
    last = re.findall(r"^\s+i =\s+3, E =.*\n((?:\s+[A-Z][a-z]?\s+\S+\s+\S+\s+\S+\n){2})", o, re.M)
    rows = [float(x) for x in re.findall(r"-?\d+\.\d+", last[0])] if last else []
    check(rows and near(vals(got, "GEOMTRACE")[-1] if False else
                        [float(x) for x in got["GEOMTRACE"][-1]["values"].split()], rows, 1e-9),
          "the last GEOMTRACE frame is the last frame CP2K printed")
    spin = vals(got, "SPINMOM")
    tot = nums(r"# Total charge and spin\s+\S+\s+\S+\s+\S+\s+(\S+)", o)
    check(spin and tot and abs(sum(spin[-1]) - tot[-1]) < 1e-6 and abs(tot[-1] - 2.0) < 1e-3,
          "SPINMOM sums to the total spin %.3f (multiplicity 3 -> 2)" % tot[-1])
    chg = nums(r"^\s+\d\s+(?:Ni|O)\s+\d\s+\S+\s+\S+\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s*$", o)
    check(len(chg) >= 2 and near(vals(got, "MULLIKEN")[-1], chg[-2:], 1e-9),
          "MULLIKEN equals the net charges of the last step %s" % chg[-2:])
    check(len(got.get("EGRADVEC", [])) == 1 and len(vals(got, "EGRADVEC")[0]) == 6, "EGRADVEC for two atoms")
    e = nums(r"ENERGY\| Total FORCE_EVAL \( QS \) energy \[hartree\]\s+(\S+)", o)
    check(e and abs(vals(got, "TE")[-1][0] - e[-1]) < 1e-9, "TE is the last energy (%.9f)" % e[-1])


def main():
    if not os.path.isfile(os.path.join(LIB, "BASIS_MOLOPT")) or \
            not os.path.isfile(os.path.join(LIB, "GTH_POTENTIALS")):
        print("SKIP: %s has no BASIS_MOLOPT and GTH_POTENTIALS (package cp2k-data)" % LIB)
        return 77
    deck_tests()
    if "--no-run" not in sys.argv:
        work = tempfile.mkdtemp(prefix="ecce-cp2krun-")
        try:
            run_tests(work)
        finally:
            shutil.rmtree(work, ignore_errors=True)
    print()
    print("%d failure(s)" % len(failures) if failures else "all passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
