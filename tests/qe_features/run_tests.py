#!/usr/bin/env python3
"""Quantum ESPRESSO: molecular dynamics, DFT+U and dispersion (#329).

Decks are generated with the real scripts/parsers/ai.qe from a .param/.frag
pair, checked line by line, passed through scripts/parsers/verifyinput, and
(with pw.x and the SSSP pseudopotentials installed) run for real.  The pw.x
output is then replayed through the real qe.desc and the parser scripts, and
what they extract is compared with numbers read from pw.x's own output by a
separate regular expression, never with values worked out here.

    tests/qe_features/run_tests.py            decks only, plus runs if pw.x is found
    tests/qe_features/run_tests.py --no-run   decks only

QE_FEATURES_KEEP=<dir> keeps every deck and pw.x output there (the parser fixtures).

Exit 77 (skip) is not used: the deck part always runs, the pw.x part says
"skipped" when pw.x or the pseudopotentials are missing.
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

PSEUDO = "/usr/share/espresso/pseudo"
RY = 0.5          # Hartree per Rydberg

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
AR2 = dict(
    title="ar2",
    lattice=["10.0 0.0 0.0", "0.0 10.0 0.0", "0.0 0.0 10.0"],
    atoms=[("Ar", 0.0, 0.0, 0.0), ("Ar", 0.0, 0.0, 3.8)])
CO = dict(
    title="co",
    lattice=["6.5 0.0 0.0", "0.0 6.5 0.0", "0.0 0.0 6.5"],
    atoms=[("C", 0.0, 0.0, 0.0), ("O", 0.0, 0.0, 1.13)])
# NiO primitive rocksalt cell, ferromagnetic.
NIO = dict(
    title="nio",
    lattice=["0.0 2.085 2.085", "2.085 0.0 2.085", "2.085 2.085 0.0"],
    atoms=[("Ni", 0.0, 0.0, 0.0), ("O", 2.085, 2.085, 2.085)])

SMALL = ["ES.Theory.PW.EcutWfc: 18.0"]
GAMMA = ["ES.Theory.PW.KPointScheme: Gamma point only"]
K222 = ["ES.Theory.PW.KPointScheme: Monkhorst-Pack grid",
        "ES.Theory.PW.KPointsNk1: 2", "ES.Theory.PW.KPointsNk2: 2",
        "ES.Theory.PW.KPointsNk3: 2"]


def deck(system, runtype, params, work=None):
    """Return (deck text, ai.qe stderr)."""
    own = work is None
    work = work or tempfile.mkdtemp(prefix="ecce-qefeat-")
    try:
        tpl = os.path.join(work, "w.pwin")
        shutil.copy(os.path.join(PARSERS, "qe.tpl"), tpl)
        atoms = system["atoms"]
        with open(os.path.join(work, "w.frag"), "w") as f:
            f.write("ECCE fragment\ntitle: %s\ntype: crystal\nnum_atoms: %d\n"
                    "atom_info: symbol cart\nlattice_vectors:\n%s\natom_list:\n%s"
                    % (system["title"], len(atoms), "\n".join(system["lattice"]) + "\n",
                       "".join("%s %.8f %.8f %.8f\n" % a for a in atoms)))
        p = ["title: " + system["title"], "RunType: " + runtype, "Charge: 0",
             "ChemSys.Multiplicity: 1", "ES.Theory.PW.PseudoDir: " + PSEUDO] + list(params)
        with open(os.path.join(work, "w.param"), "w") as f:
            f.write("\n".join(p) + "\n")
        env = dict(os.environ, ECCE_HOME=ROOT)
        r = subprocess.run(["perl", os.path.join(PARSERS, "ai.qe"), "-n", "w", "-p", "-f",
                            "-t", tpl], cwd=work, env=env, capture_output=True, text=True)
        if r.returncode:
            raise RuntimeError(r.stderr)
        return open(tpl).read(), r.stderr
    finally:
        if own:
            shutil.rmtree(work, ignore_errors=True)


def verify(text, name):
    d = tempfile.mkdtemp(prefix="ecce-qeverify-")
    try:
        path = os.path.join(d, name)
        open(path, "w").write(text)
        r = subprocess.run(["perl", os.path.join(PARSERS, "verifyinput"), "-f", path,
                            "-c", "QuantumESPRESSO"], capture_output=True, text=True,
                           env=dict(os.environ, ECCE_HOME=ROOT))
        return [l.split("|", 3) for l in r.stdout.splitlines() if l.count("|") >= 3]
    finally:
        shutil.rmtree(d, ignore_errors=True)


def has(findings, level, check_name):
    return any(f[0] == level and f[2] == check_name for f in findings)


def namelist(text, name):
    m = re.search(r"^&%s\n(.*?)^/" % name, text, re.S | re.M | re.I)
    return m.group(1) if m else None


# ---------------------------------------------------------------- deck tests
def deck_tests():
    print("molecular dynamics")
    md = ["ES.Runtype.PW.MDTimeStep: 1.0", "ES.Runtype.PW.MDSteps: 7"] + SMALL + GAMMA
    cases = [
        ("Constant energy (no thermostat)", ["ion_temperature = 'not_controlled'"], ["tempw", "nraise"]),
        ("Constant energy, from an initial temperature",
         ["ion_temperature = 'initial'", "tempw = 450"], ["nraise", "tolp"]),
        ("Berendsen thermostat", ["ion_temperature = 'berendsen'", "tempw = 450", "nraise = 12"], ["tolp"]),
        ("Andersen thermostat", ["ion_temperature = 'andersen'", "tempw = 450", "nraise = 12"], ["tolp"]),
        ("Stochastic velocity rescaling (SVR)", ["ion_temperature = 'svr'", "tempw = 450", "nraise = 12"], ["tolp"]),
        ("Velocity rescaling", ["ion_temperature = 'rescaling'", "tempw = 450", "tolp = 80"], ["nraise"]),
    ]
    for label, want, absent in cases:
        text, err = deck(SI, "Dynamics", md + [
            "ES.Runtype.PW.IonThermostat: " + label, "ES.Runtype.PW.MDTemperature: 450",
            "ES.Runtype.PW.MDCouplingSteps: 12", "ES.Runtype.PW.MDRescaleTol: 80"])
        ions = namelist(text, "IONS") or ""
        ctl = namelist(text, "CONTROL") or ""
        check("calculation = 'md'" in ctl and "ion_dynamics = 'verlet'" in ions
              and all(w in ions for w in want)
              and not any(re.search(r"\b%s\b" % a, ions) for a in absent),
              "%s: &IONS %s" % (label, " / ".join(w.strip() for w in ions.split("\n") if w.strip())))
    text, _ = deck(SI, "Dynamics", md + ["ES.Runtype.PW.IonThermostat: Berendsen thermostat"])
    ctl = namelist(text, "CONTROL")
    m = re.search(r"dt = (\S+)", ctl)
    check(m and abs(float(m.group(1)) - 1.0 / 0.04837768652) < 1e-3 and "nstep = 7" in ctl,
          "1.0 fs becomes dt = %s Rydberg atomic units, nstep = 7" % (m.group(1) if m else "?"))
    text, _ = deck(SI, "Energy", SMALL)
    check(namelist(text, "IONS") is None and "dt =" not in text, "an Energy deck has no &IONS and no dt")
    f = verify(deck(SI, "Dynamics", md + ["ES.Runtype.PW.IonThermostat: Berendsen thermostat"])[0], "md.pwin")
    check(has(f, "GOOD", "md") and not any(x[0] == "BAD" for x in f), "Verify marks the md deck as checked")
    bad = verify(text.replace("&ELECTRONS", "&IONS\n  ion_dynamics = 'verlet'\n/\n&ELECTRONS", 1), "bad.pwin")
    check(has(bad, "BAD", "ions"), "Verify flags an &IONS namelist in an scf deck")

    print("dispersion")
    for label, want, extra in [
            ("None", None, []),
            ("Grimme D2", "vdw_corr = 'grimme-d2'", []),
            ("Grimme D3 (zero damping)", "vdw_corr = 'grimme-d3'", ["dftd3_version = 3"]),
            ("Grimme D3 (Becke-Johnson damping)", "vdw_corr = 'grimme-d3'", ["dftd3_version = 4"]),
            ("Tkatchenko-Scheffler", "vdw_corr = 'ts'", []),
            ("XDM", "vdw_corr = 'xdm'", [])]:
        text, _ = deck(SI, "Energy", SMALL + ["ES.Theory.PW.Dispersion: " + label])
        sysn = namelist(text, "SYSTEM")
        ok = ("vdw_corr" not in sysn) if want is None else (want in sysn and all(e in sysn for e in extra))
        check(ok and "dftd3_threebody" not in sysn, "%s -> %s" % (label, want or "no vdw_corr"))
        if want:
            check(has(verify(text, "d.pwin"), "GOOD", "dispersion"), "  Verify marks it as checked")

    print("DFT+U")
    u = SMALL + ["ES.Theory.PW.HubbardElement1: Ni", "ES.Theory.PW.HubbardOrbital1: 3d",
                 "ES.Theory.PW.HubbardU1: 4.5", "ES.Theory.PW.HubbardElement2: O",
                 "ES.Theory.PW.HubbardOrbital2: 2p", "ES.Theory.PW.HubbardU2: 1.0",
                 "ES.Theory.PW.HubbardElement3: Fe", "ES.Theory.PW.HubbardU3: 3.0",
                 "ES.Theory.PW.SpinType: Unrestricted"]
    # Unset syntax gives the pre-7.1 keywords (Debian's pw.x is 6.7).
    text, _ = deck(NIO, "Energy", u)
    check("lda_plus_u" in text and "HUBBARD" not in text,
          "default (unset) syntax writes lda_plus_u, not the HUBBARD card")
    card = ["ES.Theory.PW.HubbardSyntax: QE 7.1 or later (HUBBARD card)"]
    text, err = deck(NIO, "Energy", u + card)
    m = re.search(r"^HUBBARD \{ortho-atomic\}\n  U Ni-3d 4.5\n  U O-2p 1.0\n", text, re.M)
    check(m and "Fe" not in text.split("HUBBARD")[1] and "lda_plus_u" not in text,
          "QE 7.1 syntax: HUBBARD {ortho-atomic} card, U Ni-3d 4.5 and U O-2p 1.0")
    check("Fe, which is not in the structure" in err or "Fe" in err,
          "a row for an element not in the structure is ignored with a note")
    f = verify(text, "u.pwin")
    check(has(f, "GOOD", "hubbard") and not has(f, "UNSURE", "hubbard"), "Verify marks the HUBBARD card as checked")
    text, _ = deck(NIO, "Energy", u + card + ["ES.Theory.PW.HubbardProjector: Atomic"])
    check("HUBBARD {atomic}" in text, "projector Atomic -> HUBBARD {atomic}")
    text, _ = deck(NIO, "Energy", u + ["ES.Theory.PW.HubbardSyntax: QE 6.x to 7.0 (lda_plus_u keywords)"])
    sysn = namelist(text, "SYSTEM")
    check("HUBBARD" not in text and "lda_plus_u = .true." in sysn and "Hubbard_U(1) = 4.5" in sysn
          and "Hubbard_U(2) = 1.0" in sysn and "U_projection_type = 'ortho-atomic'" in sysn,
          "QE 6.x to 7.0 syntax: lda_plus_u, Hubbard_U(1) = 4.5, Hubbard_U(2) = 1.0")
    check(has(verify(text, "u6.pwin"), "GOOD", "hubbard"), "Verify marks the lda_plus_u form as checked")
    nospin, _ = deck(NIO, "Energy", [x for x in u if "SpinType" not in x])
    check(has(verify(nospin, "ns.pwin"), "UNSURE", "hubbard"),
          "Verify warns (does not block) when U is set on Ni without spin polarisation")
    check(not any(x[0] == "BAD" for x in verify(nospin, "ns.pwin")), "  and finds nothing BAD")
    text, _ = deck(NIO, "Energy", SMALL + ["ES.Theory.PW.HubbardElement1: "])
    check("HUBBARD" not in text and "lda_plus_u" not in text, "no element typed -> no U")


# ---------------------------------------------------------------- real runs
def pw_version():
    try:
        with tempfile.TemporaryDirectory(prefix="ecce-qever-") as d:
            r = subprocess.run(["pw.x"], input="", capture_output=True, text=True, timeout=60, cwd=d)
    except (OSError, subprocess.TimeoutExpired):
        return None
    m = re.search(r"Program PWSCF v\.(\d+)\.(\d+)", r.stdout)
    return (int(m.group(1)), int(m.group(2)), re.search(r"v\.(\S+)", r.stdout).group(1)) if m else None


def run_pw(text, work, name, threads=2):
    pwin = os.path.join(work, name + ".pwin")
    pwout = os.path.join(work, name + ".pwout")
    open(pwin, "w").write(text)
    env = dict(os.environ, OMP_NUM_THREADS=str(threads))
    subprocess.run(["pw.x", "-in", pwin], cwd=work, env=env, stdout=open(pwout, "w"),
                   stderr=subprocess.STDOUT, timeout=900)
    out = open(pwout).read()
    if os.environ.get("QE_FEATURES_KEEP"):
        for ext in ("pwin", "pwout"):
            shutil.copy(os.path.join(work, name + "." + ext), os.environ["QE_FEATURES_KEEP"])
    return pwout, out


def parsed(pwout, runtype):
    """Replay pw.x output through the real qe.desc and parser scripts:
    {key: [flat record of every block]}."""
    desc = sim.read_desc(os.path.join(PARSERS, "qe.desc"))
    result = sim.replay(desc, pwout)
    got = {}
    work = tempfile.mkdtemp(prefix="ecce-qefeat-parse-")
    try:
        for entry in desc.live_entries():
            for block in result.delivered_for(entry):
                if not entry.script:
                    continue
                o, _, _ = sim.run_parser(PARSERS, entry, block, (".", runtype, "PW", "PW", "0"),
                                         workdir=work)
                for rec in sim.parse_parser_output(o):
                    got.setdefault(rec["key"], []).append(rec["flat"])
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return got


def vals(recs, key):
    return [float(r["values"]) for r in recs.get(key, [])]


def near(a, b, tol):
    return len(a) == len(b) and all(abs(x - y) <= tol for x, y in zip(a, b))


def floats(rx, out):
    return [float(x.replace("D", "E")) for x in re.findall(rx, out, re.M)]


def run_tests(work):
    ver = pw_version()
    if ver is None or not os.path.isdir(PSEUDO):
        print("pw.x or %s not found: real runs skipped" % PSEUDO)
        return
    print("pw.x %s" % ver[2])

    print("(a) silicon, 5 molecular dynamics steps with a Berendsen thermostat")
    text, _ = deck(SI, "Dynamics", SMALL + K222 + [
        "ES.Runtype.PW.MDTimeStep: 1.0", "ES.Runtype.PW.MDSteps: 5",
        "ES.Runtype.PW.IonThermostat: Berendsen thermostat",
        "ES.Runtype.PW.MDTemperature: 600", "ES.Runtype.PW.MDCouplingSteps: 1"])
    pwout, out = run_pw(text, work, "si_md")
    check("JOB DONE" in out, "pw.x finished (the &IONS keywords were accepted)")
    got = parsed(pwout, "Dynamics")
    n = len(re.findall(r"Entering Dynamics", out))
    check(n == 5, "pw.x ran %d dynamics steps for nstep = 5" % n)
    check(len(got.get("GEOMTRACE", [])) == 5, "GEOMTRACE has %d frames" % len(got.get("GEOMTRACE", [])))
    temps = floats(r"^\s+temperature\s+=\s+(\S+) K", out)
    check(len(temps) == 5 and near(vals(got, "TEMPVEC"), temps, 1e-6),
          "TEMPVEC %s equals the temperatures pw.x printed" % vals(got, "TEMPVEC"))
    check(abs(temps[0] - 600.0) < 1e-6, "the run starts at the requested 600 K")
    const = floats(r"Ekin \+ Etot \(const\)\s+=\s+(\S+) Ry", out)
    check(near(vals(got, "TEVEC"), [c * RY for c in const], 1e-8),
          "TEVEC equals pw.x's Ekin + Etot")
    ekin = floats(r"kinetic energy \(Ekin\)\s+=\s+(\S+) Ry", out)
    check(near(vals(got, "KEVEC"), [e * RY for e in ekin], 1e-8), "KEVEC equals Ekin")
    etot = floats(r"^!\s+total energy\s+=\s+(\S+) Ry", out)
    check(near(vals(got, "PEVEC"), [e * RY for e in etot[:5]], 1e-7),
          "PEVEC equals the DFT energies pw.x printed (one per step, the first is the starting geometry)")
    fs = re.search(r"Time step\s+=\s+\S+ a\.u\.,\s+(\S+) femto-seconds", out)
    check(fs and abs(float(fs.group(1)) - 1.0) < 1e-3, "pw.x reports a time step of %s fs" % (fs.group(1) if fs else "?"))
    check(len(vals(got, "TEVEC")) <= len(got["GEOMTRACE"]), "TEVEC is no longer than GEOMTRACE")

    print("(b) NiO, U on Ni 3d, %s" % ("lda_plus_u form" if ver[:2] < (7, 1) else "HUBBARD card"))
    legacy = ["ES.Theory.PW.HubbardSyntax: QE 6.x to 7.0 (lda_plus_u keywords)"] if ver[:2] < (7, 1) else []
    base = SMALL + ["ES.Theory.PW.EcutWfc: 30.0", "ES.Theory.PW.Occupations: Smearing (metal)",
                    "ES.Theory.PW.SpinType: Unrestricted", "ES.Theory.PW.StartingMagnetization: 0.5",
                    "ES.Theory.PW.MixingBeta: 0.3", "ES.Theory.PW.ConvergenceEnergy: 1e-7"] + K222
    res = {}
    for label, extra in (("U0", []), ("U4", ["ES.Theory.PW.HubbardElement1: Ni",
                                              "ES.Theory.PW.HubbardOrbital1: 3d",
                                              "ES.Theory.PW.HubbardU1: 4.0"] + legacy)):
        text, _ = deck(NIO, "Energy", base + extra)
        pwout, out = run_pw(text, work, "nio_" + label, threads=4)
        res[label] = (out, parsed(pwout, "Energy"))
        check("JOB DONE" in out, "%s: pw.x finished" % label)
    o0, g0 = res["U0"]
    o4, g4 = res["U4"]
    e0 = floats(r"^!\s+total energy\s+=\s+(\S+) Ry", o0)
    e4 = floats(r"^!\s+total energy\s+=\s+(\S+) Ry", o4)
    check(e0 and e4 and abs(e4[-1] - e0[-1]) > 1e-3, "U = 4 eV changes the total energy (%.6f -> %.6f Ry)" % (e0[-1], e4[-1]))
    check(vals(g4, "TE") and abs(vals(g4, "TE")[-1] - e4[-1] * RY) < 1e-8, "TE is the energy pw.x printed")
    hub = floats(r"Hubbard energy\s+=\s+(\S+) Ry", o4)
    check(hub and near(vals(g4, "EHUBBARD")[-1:], [hub[-1] * RY], 1e-8),
          "EHUBBARD equals the Hubbard energy pw.x printed (%s Ry)" % (hub[-1:] or "none"))
    check("Hubbard energy" not in o0 and "EHUBBARD" not in g0, "U = 0: no Hubbard energy printed, none parsed")
    check("enter write_ns" in o4 and re.search(r"Tr\[ns\(na\)\]", o4), "pw.x printed the Hubbard occupations")
    check(re.search(r"Simplified LDA\+U|DFT\+U|Hubbard", o4) and
          re.search(r"U\s*=?\s*4\.0|Hubbard_U\s*\(eV\)|\s4\.0000", o4), "pw.x echoes U = 4.0 eV")

    print("(c) dispersion, two argon atoms")
    base = ["ES.Theory.PW.EcutWfc: 30.0"] + GAMMA
    runs = {}
    for label in ("None", "Grimme D2", "Grimme D3 (zero damping)",
                  "Grimme D3 (Becke-Johnson damping)", "Tkatchenko-Scheffler"):
        text, _ = deck(AR2, "Energy", base + ["ES.Theory.PW.Dispersion: " + label])
        pwout, out = run_pw(text, work, "ar2_" + label.split()[0].lower() + str(len(runs)))
        runs[label] = (out, parsed(pwout, "Energy"))
        check("JOB DONE" in out, "%s: pw.x finished" % label)
    out0, g0 = runs["None"]
    te0 = floats(r"^!\s+total energy\s+=\s+(\S+) Ry", out0)[-1]
    check("EDISP" not in g0, "no correction: no dispersion energy parsed")
    for label, rx in (("Grimme D2", r"Dispersion Correction\s+=\s+(\S+) Ry"),
                      ("Grimme D3 (zero damping)", r"DFT-D3 Dispersion\s+=\s+(\S+) Ry"),
                      ("Grimme D3 (Becke-Johnson damping)", r"DFT-D3 Dispersion\s+=\s+(\S+) Ry"),
                      ("Tkatchenko-Scheffler", r"Dispersion T-S Correction\s+=\s+(\S+) Ry")):
        out, g = runs[label]
        printed = floats(rx, out)
        te = floats(r"^!\s+total energy\s+=\s+(\S+) Ry", out)[-1]
        check(printed and near(vals(g, "EDISP")[-1:], [printed[-1] * RY], 1e-8),
              "%s: EDISP %s Ha equals pw.x's %s Ry" % (label, vals(g, "EDISP")[-1:], printed[-1:]))
        # The correction is added after the SCF cycle (Tkatchenko-Scheffler also feeds back
        # into the density, hence the looser bound).
        tol = 2e-6 if "Tkatchenko" in label else 2e-7
        check(printed and abs((te - te0) - printed[-1]) < tol,
              "%s: total energy differs from no correction by %.8f Ry (printed %.8f)"
              % (label, te - te0, printed[-1]))
    d3zero = floats(r"DFT-D3 Dispersion\s+=\s+(\S+) Ry", runs["Grimme D3 (zero damping)"][0])[-1]
    d3bj = floats(r"DFT-D3 Dispersion\s+=\s+(\S+) Ry", runs["Grimme D3 (Becke-Johnson damping)"][0])[-1]
    check(abs(d3zero - d3bj) > 1e-6, "zero damping and Becke-Johnson damping give different energies")

    print("(c2) dispersion, XDM (needs PAW): carbon monoxide")
    text, _ = deck(CO, "Energy", ["ES.Theory.PW.EcutWfc: 25.0", "ES.Theory.PW.UseEcutRho: 1",
                                  "ES.Theory.PW.EcutRho: 200"] + GAMMA +
                   ["ES.Theory.PW.Dispersion: XDM", "ES.Theory.PW.PseudoType: Prefer PAW"])
    pwout, out = run_pw(text, work, "co_xdm", threads=4)
    g = parsed(pwout, "Energy")
    printed = floats(r"Dispersion XDM Correction\s+=\s+(\S+) Ry", out)
    check("JOB DONE" in out and printed and near(vals(g, "EDISP")[-1:], [printed[-1] * RY], 1e-8),
          "XDM: EDISP equals pw.x's %s Ry" % printed[-1:])


if __name__ == "__main__":
    print("decks")
    deck_tests()
    if "--no-run" not in sys.argv:
        work = tempfile.mkdtemp(prefix="ecce-qefeat-run-")
        try:
            run_tests(work)
        finally:
            shutil.rmtree(work, ignore_errors=True)
    print("\n%d failure(s)" % len(failures))
    sys.exit(1 if failures else 0)
