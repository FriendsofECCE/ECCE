#!/usr/bin/env python3
"""Counterpoise correction for ECCE-QM and ORCA (#1091), water dimer.

    tests/launch/counterpoise_test.py --offline [--qm ECCE_QM_EXE] [--orca ORCA_EXE]
    tests/launch/counterpoise_test.py [--build build-cmake] [--keep]

--offline: the real ai.ecceqm / ai.orca write the decks from a .frag with two
fragments; the decks are checked (N+1+N runs, ghosts, charges, multiplicity),
the monomer decks are split off as gensub does and, when an engine is given,
run; counterpoise.parse turns the energies into the properties.  Without
--offline: ECCE-QM B3LYP-D3(BJ)/def2-SVP through Launch, gensub and the job
monitor on localhost, then the stored properties are checked.

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import types

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))
sys.path.insert(0, os.path.join(REPO, "tests", "qm"))
sys.path.insert(0, HERE)

failures = []
KJ = 2625.499638
BASIS = "def2-svp"
ATOMS = [("O", 0.0, 0.0, 0.0), ("H", 0.965, 0.0, 0.0), ("H", -0.24, 0.93, 0.0),
         ("O", 2.91, 0.0, 0.0), ("H", 3.229, 0.491, 0.757), ("H", 3.229, 0.491, -0.757)]


def say(text):
    print(text, flush=True)


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def writeFiles(work, name, code, counterpoise=True, own=True):
    import ecceqm_test as q
    with open(os.path.join(work, name + ".frag"), "w") as out:
        out.write("fragment\ntitle: %s\nnum_atoms: %d\natom_info: symbol\natom_list:\n"
                  % (name, len(ATOMS)))
        for a in ATOMS:
            out.write("%s %.10f %.10f %.10f\n" % a)
        if counterpoise:
            out.write("fragment_states: 2\n1 0 1\n2 0 1\natom_fragment_list:\n")
            out.write("1\n1\n1\n2\n2\n2\n")
    param = ["Category: DFT", "Theory: DFT", "RunType: Energy", "Charge: 0",
             "ChemSys.Multiplicity: 1", "ES.Theory.DFT.XCFunctionals: B3LYP"]
    param.append("ES.Theory.ECCEQM.Dispersion: 1" if code == "ecceqm"
                 else "ES.Theory.SCF.Dispersion: D3BJ")
    if counterpoise:
        param.append("ES.Runtype.Counterpoise: 1")
        param.append("ES.Runtype.CounterpoiseOwn: %d" % (1 if own else 0))
    with open(os.path.join(work, name + ".param"), "w") as out:
        out.write("\n".join(param) + "\n")
    q.writeGbs(os.path.join(work, name + ".gbs"), ["H", "O"], BASIS)


def generate(work, name, code):
    """ai.<code> on the files, as DeckWriter calls it; returns the deck text."""
    env = dict(os.environ, ECCE_HOME=REPO)
    std = "std2ECCEQM" if code == "ecceqm" else "std2ORCA"
    with open(os.path.join(work, name + ".gbs")) as gbs:
        r = subprocess.run(["perl", os.path.join(PARSERS, std)], stdin=gbs, env=env,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    with open(os.path.join(work, name + ".basis"), "wb") as out:
        out.write(r.stdout)
    shutil.copy(os.path.join(PARSERS, code + ".tpl"), os.path.join(work, name + ".tpl"))
    r = subprocess.run(["perl", os.path.join(PARSERS, "ai." + code), "-p", "-f", "-b",
                        "-n", name, "-t", name + ".tpl"], cwd=work, env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if r.returncode != 0:
        say(r.stdout.decode(errors="replace"))
        return None
    with open(os.path.join(work, name + ".tpl")) as handle:
        return handle.read()


def splitDeck(text):
    """What gensub's cpSplit does: {name: deck}, 'complex' first."""
    parts, name = {"complex": []}, "complex"
    for line in text.splitlines(True):
        m = re.match(r"# ecce-run (\S+)", line)
        if m:
            name = m.group(1)
            parts[name] = []
        else:
            parts[name].append(line)
    return {k: "".join(v) for k, v in parts.items()}


def checkDecks(parts, code):
    check(sorted(parts) == ["complex", "fragment-1", "fragment-1-own", "fragment-2",
                            "fragment-2-own"], "5 decks: complex and 2 + 2 monomers (%s)" % sorted(parts))
    for name in ("fragment-1", "fragment-2"):
        deck = parts.get(name, "")
        if code == "ecceqm":
            ghosts = len(re.findall(r"^ \w+\s+\S+\s+\S+\s+\S+\s+ghost$", deck, re.M))
            atoms = len(re.findall(r"^ [A-Z][a-z]?\s+[-\d.]+\s+[-\d.]+\s+[-\d.]+( ghost)?$", deck, re.M))
            check("charge 0\n" in deck and "multiplicity 1\n" in deck, "%s: charge 0, multiplicity 1" % name)
            check("write_orbitals" not in deck, "%s writes no orbital file" % name)
        else:
            ghosts = len(re.findall(r"^\s*[A-Z][a-z]?:\s", deck, re.M))
            atoms = len(re.findall(r"^\s*[A-Z][a-z]?:?\s+[-\d.]+\s+[-\d.]+\s+[-\d.]+$", deck, re.M))
            check("* xyz 0 1" in deck, "%s: charge 0, multiplicity 1" % name)
        check(atoms == 6 and ghosts == 3, "%s: six centres, three ghosts (%d, %d)" % (name, atoms, ghosts))
        own = parts.get(name + "-own", "")
        check(len(re.findall(r"ghost|[A-Z]:\s", own)) == 0 and
              (len(re.findall(r"^ [A-Z][a-z]?\s+[-\d.]+\s+[-\d.]+\s+[-\d.]+$", own, re.M)) == 3 if code == "ecceqm"
               else len(re.findall(r"^\s*[A-Z][a-z]?\s+[-\d.]+\s+[-\d.]+\s+[-\d.]+$", own, re.M)) == 3),
              "%s-own: three real atoms, no ghosts" % name)


def parseBlock(energies):
    text = "begin counterpoise\n" + "".join("%s %.10f\n" % kv for kv in energies) + "end counterpoise\n"
    r = subprocess.run(["perl", os.path.join(PARSERS, "counterpoise.parse")], input=text.encode(),
                       stdout=subprocess.PIPE)
    out = r.stdout.decode()
    return {m.group(1): float(m.group(2)) for m in
            re.finditer(r"key: (\w+)\nsize:\n1\nvalues:\n(-?[\d.]+)\n", out)}, out


def runSplit(work, parts, code, exe):
    """Run every deck as the job script does; returns {name: energy}."""
    res = {}
    for name, deck in parts.items():
        ext = "qmin" if code == "ecceqm" else "inp"
        inp = os.path.join(work, "run-%s.%s" % (name, ext))
        with open(inp, "w") as handle:
            handle.write(deck)
        r = subprocess.run([exe, os.path.basename(inp)], cwd=work, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, env=dict(os.environ, OMP_NUM_THREADS="2"))
        text = r.stdout.decode(errors="replace")
        pat = r"^energy_total\s+(\S+)" if code == "ecceqm" else r"FINAL SINGLE POINT ENERGY\s+(\S+)"
        m = re.findall(pat, text, re.M)
        check(r.returncode == 0 and bool(m), "%s ran (%s)" % (name, "rc=%d" % r.returncode))
        if m:
            res[name] = float(m[-1])
    return res


def offline(opts):
    parserCheck()
    for code, exe in (("ecceqm", opts.qm), ("orca", opts.orca)):
        say("--- %s input generation" % code)
        work = tempfile.mkdtemp(prefix="cp_%s_" % code)
        writeFiles(work, "dimer", code)
        text = generate(work, "dimer", code)
        if not check(text is not None, "ai.%s wrote the deck" % code):
            continue
        parts = splitDeck(text)
        checkDecks(parts, code)
        # No fragments stored: the option is refused with a message.
        writeFiles(work, "nofrag", code, counterpoise=False)
        with open(os.path.join(work, "nofrag.param"), "a") as handle:
            handle.write("ES.Runtype.Counterpoise: 1\n")
        check(generate(work, "nofrag", code) is None, "no fragments: the generator refuses")
        if not exe:
            continue
        say("--- %s runs" % code)
        res = runSplit(work, parts, code, exe)
        if len(res) == 5:
            e, props = energies(res)
            check(props["EBSSE"] > 0, "BSSE %.2f kJ/mol > 0" % (props["EBSSE"] * KJ))
            check(props["EINTCP"] > props["EINTRAW"],
                  "dE_CP %.2f kJ/mol is less negative than raw %.2f kJ/mol"
                  % (props["EINTCP"] * KJ, props["EINTRAW"] * KJ))
            say("  %s: raw %.3f  BSSE %.3f  dE_CP %.3f kJ/mol"
                % (code, props["EINTRAW"] * KJ, props["EBSSE"] * KJ, props["EINTCP"] * KJ))


def energies(res):
    items = [("complex", res["complex"])] + [(k, v) for k, v in sorted(res.items()) if k != "complex"]
    props, _ = parseBlock(items)
    return items, props


def parserCheck():
    say("--- counterpoise.parse")
    props, out = parseBlock([("complex", -152.65), ("fragment-1", -76.3250), ("fragment-2", -76.3240),
                             ("fragment-1-own", -76.3240), ("fragment-2-own", -76.3235)])
    check(abs(props["EINTCP"] - (-152.65 + 76.3250 + 76.3240)) < 1e-8, "dE_CP = E_AB - sum E_X(AB)")
    check(abs(props["EINTRAW"] - (-152.65 + 76.3240 + 76.3235)) < 1e-8, "raw = E_AB - sum E_X(X)")
    check(abs(props["EBSSE"] - (props["EINTCP"] - props["EINTRAW"])) < 1e-8 and props["EBSSE"] > 0,
          "BSSE = dE_CP - raw > 0")
    check("CPTABLE" in out and "kcal/mol" in out, "the summary string carries both units")
    props, out = parseBlock([("complex", -152.65), ("fragment-1", -76.3250), ("fragment-2", -76.3240)])
    check("EINTCP" in props and "EINTRAW" not in props, "without own-basis runs only dE_CP")


# --- full run through ECCE -----------------------------------------------------

def launch(opts):
    import ecceqm_test as q
    import isolate
    import run_tests as lt
    build = os.path.abspath(opts.build)
    qm = os.path.join(build, "src", "qm", "ecce-qm")
    for exe in ("launchjob", "eccejobstore", "eccejobmaster", "ecmd"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            lt.skip("%s is not built in %s" % (exe, build))
    if not os.access(qm, os.X_OK):
        lt.skip("ecce-qm is not built")
    for tool in ("mosquitto", "perl"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            lt.skip("%s is not installed" % tool)
    if not os.path.exists(os.path.join(build, "siteconfig-local", "DataServers")):
        lt.skip("%s/siteconfig-local is missing (run cmake)" % build)

    state = isolate.resolveStateDir(isolate.runState("counterpoise", keep=opts.keep))
    os.makedirs(state, exist_ok=True)
    lt.sweep(state)
    for sub in ("jobs", "tmp", "folder", "localdata", "work"):
        shutil.rmtree(os.path.join(state, sub), ignore_errors=True)
    os.makedirs(os.path.join(state, "tmp"))
    work = os.path.join(state, "work", "dimer")
    os.makedirs(work)
    os.environ["ECCE_TMPDIR"] = os.path.join(state, "tmp")
    install = lt.treeInstall(state, build)
    lt.link(qm, os.path.join(install, "bin", "ecce-qm"))
    settings = isolate.apply(install, state)
    home = settings["ECCE_HOME"]
    os.unlink(os.path.join(home, "siteconfig", "DataServers"))
    os.environ["ECCE_TEST_HOME"] = install
    args = types.SimpleNamespace(
        local=True, folder=False, machine="localhost", cwd=None, legacy_transport=None,
        job_comms=None, shared_connection=False, hold=False, drop=False, remote_user="",
        expect_keepalive=False)
    suite = lt.Suite(args, build, state, home)
    env = suite.env()
    env["ECCE_HOME"] = REPO
    try:
        if not suite.services(True):
            check(False, "services started")
            return
        # The counterpoise deck is what the job runs; a plain deck gives the
        # import (Import Calculation) something to build the calculation from.
        writeFiles(work, "cp", "ecceqm")
        cpDeck = generate(work, "cp", "ecceqm")
        check(cpDeck is not None and cpDeck.count("# ecce-run ") == 4, "the counterpoise deck has 4 monomer runs")
        writeFiles(work, "dimer", "ecceqm", counterpoise=False)
        q.generateDeck(env, work, "dimer")
        plain = os.path.join(work, "ecceqm.qmin")
        output = q.importFromOutput(env, work, "dimer", qm, plain)
        deck = os.path.join(work, "cp.qmin")
        with open(deck, "w") as handle:
            handle.write(cpDeck)
        rundir = os.path.join(state, "jobs")
        os.makedirs(rundir, exist_ok=True)
        calcName = "cp-dimer-%d" % int(time.time())
        rc, out = suite.driver("create", suite.userUrl(), calcName, q.RESOURCE, deck,
                               "ecceqm.qmin", "localhost", rundir, suite.user())
        if not check(rc == 0, "calculation created"):
            say(out)
            return
        url = out.strip().splitlines()[-1]
        rc, out = suite.driver("setup", url, work, os.path.basename(output))
        check(rc == 0, "molecule, basis and theory stored")
        rc, out = suite.driver("launch", url)
        check(rc == 0, "Launch ran to the end")
        st = suite.waitState(url)
        check(st == "completed", "the run reached completed (last: %s)" % st)
        if st != "completed":
            suite.diagnose(url)
            return
        time.sleep(2)
        calc = (url[len("file://"):] if url.startswith("file://") else url).rstrip("/")
        props = suite.props(url)
        for key in ("TE", "EINTCP", "EINTRAW", "EBSSE", "CPTABLE"):
            check(key in props, "%s present in Props/" % key)
        if "EBSSE" in props:
            raw, bsse, cp = (q.propertyValue(calc, k) for k in ("EINTRAW", "EBSSE", "EINTCP"))
            say("  water dimer B3LYP-D3(BJ)/def2-SVP: raw %.3f  BSSE %.3f  dE_CP %.3f kJ/mol"
                % (raw * KJ, bsse * KJ, cp * KJ))
            check(bsse > 0 and cp > raw, "BSSE > 0 and dE_CP less negative than raw")
            check(abs(cp - (raw + bsse)) < 1e-8, "dE_CP = raw + BSSE")
    finally:
        if not opts.keep:
            suite.services(False)
            lt.sweep(state)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--offline", action="store_true")
    ap.add_argument("--qm", default="")
    ap.add_argument("--orca", default="")
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    opts = ap.parse_args()
    opts.orca = opts.orca or os.environ.get("ECCE_ORCA", "")
    if not opts.orca:
        import glob
        for pat in ("~/orca_*/orca", "~/opt/orca/*/orca", "/opt/orca/*/orca"):
            found = sorted(glob.glob(os.path.expanduser(pat)))
            if found:
                opts.orca = found[-1]
                break
    if opts.offline:
        offline(opts)
    else:
        launch(opts)
    say("")
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
