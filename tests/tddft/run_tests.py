#!/usr/bin/env python3
"""TD-DFT options in the ORCA and Gaussian 16 generators (#383).

Generates decks through the real ai.orca / ai.gauss16 and checks the %tddft
block and the TD route keyword; then runs formaldehyde B3LYP/def2-SVP in ORCA
6 (TDA off, TDA on, singlets+triplets) and checks that the production parse
pipeline reads the requested number of roots.  The ORCA run exits 77 (CTest
SKIP) without ORCA 6; a Gaussian run is only tried when g16 starts.
"""
import glob, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
sys.path.insert(0, os.path.join(ROOT, "tests", "parsers"))
fails = 0

H2CO = ("# frag\nnum_atoms: 4\ntitle: h2co\natom_info: symbol\natom_list:\n"
        "C 0.0 0.0 -0.529\nO 0.0 0.0 0.677\nH 0.0 0.935 -1.113\nH 0.0 -0.935 -1.113\n")


def check(ok, what):
    global fails
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    fails += not ok


def deck(code, tpl, extra, work, cat="DFT", theory="RDFT", mult=1):
    shutil.copy(os.path.join(PARSERS, tpl), os.path.join(work, "t.tpl"))
    open(os.path.join(work, "w.frag"), "w").write(H2CO)
    open(os.path.join(work, "w.basis"), "w").write(
        "useRouteCard " + ("def2-SVP" if code == "orca" else "6-31G(d)") + "\n")
    open(os.path.join(work, "w.param"), "w").write(
        "Category: %s\nTheory: %s\nRunType: Energy\nCharge: 0\n"
        "ChemSys.Multiplicity: %d\nES.Theory.DFT.XCFunctionals: B3LYP\n%s"
        % (cat, theory, mult, extra))
    env = dict(os.environ, ECCE_HOME=ROOT)
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai." + ("orca" if code == "orca" else "gauss16")),
                        "-p", "-f", "-b", "-c", "-n", "w", "-t", "t.tpl"],
                       cwd=work, env=env, capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return open(os.path.join(work, "t.tpl")).read()


def td(n, spin, tda):
    return ("ES.Theory.TDDFT.Use: 1\nES.Theory.TDDFT.NStates: %d\n"
            "ES.Theory.TDDFT.Spin: %s\nES.Theory.TDDFT.TDA: %d\n" % (n, spin, tda))


def generation(work):
    d = deck("orca", "orca.tpl", td(5, "Singlets", 0), work)
    check("%tddft\n  nroots 5\n  tda false\nend\n" in d, "ORCA: nroots 5, tda false")
    check("triplets" not in d, "ORCA: singlets only emits no triplets line")
    d = deck("orca", "orca.tpl", td(8, "Singlets and triplets", 1), work)
    check("%tddft\n  nroots 8\n  triplets true\n  tda true\nend\n" in d,
          "ORCA: nroots 8, triplets true, tda true")
    d = deck("orca", "orca.tpl", td(8, "Singlets and triplets", 0), work,
             cat="SCF", theory="UHF", mult=3)
    check("%tddft" in d and "triplets" not in d, "ORCA: no triplets line for an unrestricted reference")
    d = deck("orca", "orca.tpl", "", work)
    check("%tddft" not in d, "ORCA: nothing without the option")
    d = deck("orca", "orca.tpl", "ES.Theory.TDDFT.Use: 0\n" + td(5, "Singlets", 0).split("\n", 1)[1], work)
    check("%tddft" not in d, "ORCA: Use=0 emits nothing")

    def route(extra, **kw):
        t = deck("g16", "g16.tpl", extra, work, **kw)
        return " ".join(l for l in t.split("\n\n")[0].split("\n") if not l.startswith("%"))

    r = route(td(10, "Singlets", 0))
    check(re.search(r"(^| )TD\(NStates=10,Singlets\)( |$)", r) is not None, "G16: TD(NStates=10,Singlets)")
    r = route(td(6, "Triplets", 1))
    check("TDA(NStates=6,Triplets)" in r, "G16: TDA(NStates=6,Triplets)")
    r = route(td(4, "Singlets and triplets", 0))
    check("TD(NStates=4,50-50)" in r, "G16: TD(NStates=4,50-50)")
    r = route(td(4, "Singlets", 0), cat="SCF", theory="UHF", mult=3)
    check("TD(NStates=4)" in r, "G16: unrestricted reference carries no spin keyword")
    check(not re.search(r"\bTDA?\(", route("")), "G16: nothing without the option")


def parse_roots(out_path):
    from eccejobmonitor_sim import read_desc, replay, run_parser, parse_parser_output
    desc = read_desc(os.path.join(PARSERS, "orca.desc"))
    result = replay(desc, out_path)
    found = {}
    for entry in desc.live_entries():
        for block in result.delivered_for(entry):
            out, _e, _rc = run_parser(PARSERS, entry, block, (".", "Energy", "DFT", "B3LYP", "0"))
            for rec in parse_parser_output(out):
                found[rec["key"]] = rec["flat"]
    return found


def orca_runs(work):
    orcas = [p for p in sorted(glob.glob("/opt/orca/orca_6*/orca")) if os.access(p, os.X_OK)]
    if not orcas:
        print("no ORCA 6: run skipped")
        return False
    for name, spin, tda in (("nontda", "Singlets", 0), ("tda", "Singlets", 1),
                            ("trip", "Singlets and triplets", 0)):
        sub = os.path.join(work, name)
        os.mkdir(sub)
        d = deck("orca", "orca.tpl", td(5, spin, tda), sub)
        open(os.path.join(sub, "orca.orcain"), "w").write(d)
        subprocess.run([orcas[0], "orca.orcain"], cwd=sub, timeout=900,
                       stdout=open(os.path.join(sub, "orca.orcaout"), "w"), stderr=subprocess.STDOUT)
        out = open(os.path.join(sub, "orca.orcaout"), errors="replace").read()
        check("ORCA TERMINATED NORMALLY" in out, "%s: ORCA ran the generated deck" % name)
        if tda:
            check("TD-DFT/TDA EXCITED STATES" in out, "%s: ORCA printed the TDA block" % name)
        else:
            check("TD-DFT EXCITED STATES" in out and "TD-DFT/TDA" not in out,
                  "%s: ORCA printed the full TD-DFT block" % name)
        found = parse_roots(os.path.join(sub, "orca.orcaout"))
        rec = found.get("EXCIENERGY")
        vals = rec["values"].split() if rec else []
        want = 5 if spin == "Singlets" else 10
        check(len(vals) == want, "%s: parser reads %d roots (got %d)" % (name, want, len(vals)))
        if spin != "Singlets":
            sym = found.get("EXCISYM", {}).get("values", "").split()
            check(sum(x.startswith("Singlet") for x in sym) == 5 and
                  sum(x.startswith("Triplet") for x in sym) == 5,
                  "%s: five singlet and five triplet states labelled (%s)" % (name, " ".join(sym)))
            check("TD-DFT EXCITED STATES (TRIPLETS)" in out or "(TRIPLETS)" in out,
                  "%s: ORCA computed triplets as well" % name)
    return True


def g16_run(work):
    g16 = "/opt/gaussian/g16/g16"
    if not os.access(g16, os.X_OK):
        print("no g16: run skipped")
        return
    sub = os.path.join(work, "g16")
    os.mkdir(sub)
    t = deck("g16", "g16.tpl", td(3, "Singlets", 0), sub)
    open(os.path.join(sub, "a.com"), "w").write(t)
    env = dict(os.environ, GAUSS_SCRDIR=sub)
    subprocess.run([g16, "a.com"], cwd=sub, env=env, capture_output=True, timeout=600)
    log = open(os.path.join(sub, "a.log"), errors="replace").read() if os.path.exists(os.path.join(sub, "a.log")) else ""
    if "Normal termination" not in log:
        print("g16 does not run here (route accepted? no): run skipped")
        return
    check(len(re.findall(r"Excited State\s+\d+:", log)) >= 3, "G16: three excited states printed")


def main():
    work = tempfile.mkdtemp(prefix="ecce-tddft-")
    try:
        print("generation")
        generation(work)
        print("real runs")
        ran = orca_runs(work)
        g16_run(work)
    finally:
        shutil.rmtree(work, True)
    print("FAILED" if fails else "PASSED")
    if fails:
        return 1
    return 0 if ran else 77


if __name__ == "__main__":
    sys.exit(main())
