#!/usr/bin/env python3
"""
Is a basis set written by name the same basis as the one ECCE holds?

    ./nwchem_library_check.py                 every candidate set (slow)
    ./nwchem_library_check.py --only aug-cc-pVDZ --only IGLO-II
    ./nwchem_library_check.py -j 4 --timeout 600 --out result.txt

On demand only; it is not part of the fast test run.  SKIPs (exit 0) when
there is no nwchem, no apache2 or no build tree (ECCE_TEST_BUILD).

wrNWChemGBS.pm forces explicit exponents and coefficients for the whole
calculation when any element's basis name matches its blocklist.  Whether a
name belongs on that list is not "does NWChem accept it" but "is NWChem's
set of that name the same basis ECCE holds under it" -- ECCE may have merged
components that NWChem keeps apart.  For every orbital set in
data/admin/basissets whose name the CURRENT blocklist catches, and every
element of H Li B C N O F Na Si P S Cl Br the set covers, this runs an RHF
energy of the element's hydride in NWChem twice with the same coordinate
type: once with the basis named (what the writer would emit without the
blocklist) and once with ECCE's primitives written out.  Match means the
same number of functions and |dE| < 1e-6 Eh; NWChem refusing the name is
"unavailable".  Hydrogen always carries the same set, so it is covered too.

Both decks come from the real TGBSConfig::dump() -> std2NWChem path
(loadBasis --dump).  The named form needs the blocklist out of the way, so
a scratch copy of scripts/ with that one test disabled is used; the
repository is never modified.  Results are cached per case in the output
directory so an interrupted run resumes.

Scratch lives under ECCE_NWBASIS_SCRATCH (default ~/tmp/ecce/nwbasis-scratch,
not /tmp: that is RAM on some machines) and is removed at the end.
"""

import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as R  # noqa: E402

REPO = R.REPO
INDEXDIR = os.path.join(REPO, "data", "admin", "basissets")
ORBITAL_TYPES = ["pople", "correlation_consistent", "other_generally_contracted",
                 "other_segmented", "ECPOrbital", "DFTOrbital"]
#  The rule being tested, as it stands in wrNWChemGBS.pm.
BLOCKLIST = [r'^\s*"\s*aug', r'^\s*"\s*d-aug', r'-pcv', r'IGLO-II']

#  element -> (molecule, geometry in angstrom)
MOLECULES = {
    "H": ("H2", [("H", 0, 0, 0), ("H", 0, 0, 0.74)]),
    "Li": ("LiH", [("Li", 0, 0, 0), ("H", 0, 0, 1.595)]),
    "B": ("BH3", [("B", 0, 0, 0), ("H", 1.19, 0, 0),
                  ("H", -0.595, 1.0306, 0), ("H", -0.595, -1.0306, 0)]),
    "C": ("CH4", [("C", 0, 0, 0), ("H", 0.629, 0.629, 0.629),
                  ("H", -0.629, -0.629, 0.629), ("H", -0.629, 0.629, -0.629),
                  ("H", 0.629, -0.629, -0.629)]),
    "N": ("NH3", [("N", 0, 0, 0.116), ("H", 0.939, 0, -0.271),
                  ("H", -0.4695, 0.8132, -0.271),
                  ("H", -0.4695, -0.8132, -0.271)]),
    "O": ("H2O", [("O", 0, 0, 0.117), ("H", 0, 0.757, -0.469),
                  ("H", 0, -0.757, -0.469)]),
    "F": ("HF", [("F", 0, 0, 0), ("H", 0, 0, 0.917)]),
    "Na": ("NaH", [("Na", 0, 0, 0), ("H", 0, 0, 1.887)]),
    "Si": ("SiH4", [("Si", 0, 0, 0), ("H", 0.857, 0.857, 0.857),
                    ("H", -0.857, -0.857, 0.857),
                    ("H", -0.857, 0.857, -0.857),
                    ("H", 0.857, -0.857, -0.857)]),
    "P": ("PH3", [("P", 0, 0, 0.127), ("H", 1.19, 0, -0.636),
                  ("H", -0.595, 1.0306, -0.636),
                  ("H", -0.595, -1.0306, -0.636)]),
    "S": ("H2S", [("S", 0, 0, 0.103), ("H", 0, 0.962, -0.822),
                  ("H", 0, -0.962, -0.822)]),
    "Cl": ("HCl", [("Cl", 0, 0, 0), ("H", 0, 0, 1.275)]),
    "Br": ("HBr", [("Br", 0, 0, 0), ("H", 0, 0, 1.414)]),
}


ORDER = list(MOLECULES)
SLOW = {}   # set name -> index in ORDER of the lightest element that timed out


def parseIndex(path):
    entries, cur = [], None
    with open(path, errors="replace") as handle:
        for line in handle:
            line = line.rstrip("\n")
            if line.startswith("name="):
                cur = {"name": line[5:].strip(), "atoms": []}
                entries.append(cur)
            elif line.startswith("atoms=") and cur is not None:
                cur["atoms"] += line[6:].split()
    return entries


def candidates(only):
    found = []
    for gtype in ORBITAL_TYPES:
        for entry in parseIndex(os.path.join(INDEXDIR, gtype)):
            name = entry["name"]
            if only and name not in only:
                continue
            if not only and not blocked('"%s"' % name):
                continue
            found.append((name, gtype, entry["atoms"]))
    return found


def blocked(libname):
    return any(re.search(p, libname, re.I) for p in BLOCKLIST)


def patchedHome(state, home):
    """A second ECCE_HOME whose wrNWChemGBS.pm never forces explicit output."""
    home2 = os.path.join(state, "ecce-home-named")
    os.makedirs(os.path.join(home2, "scripts"))
    shutil.copytree(os.path.join(REPO, "scripts", "parsers"),
                    os.path.join(home2, "scripts", "parsers"),
                    symlinks=True)
    for name in os.listdir(home):
        if name != "scripts" and not os.path.exists(os.path.join(home2, name)):
            os.symlink(os.path.join(home, name), os.path.join(home2, name))
    path = os.path.join(home2, "scripts", "parsers", "wrNWChemGBS.pm")
    with open(path) as handle:
        text = handle.read()
    text, n = re.subn(r"if \(\$gbs\{\$atom\} =~ .*?\) \{(\s*\$useExplicitBasis = 1;)",
                      r"if (0) {\1", text, count=1, flags=re.S)
    if n != 1:
        sys.exit("wrNWChemGBS.pm: cannot find the blocklist test to disable")
    with open(path, "w") as handle:
        handle.write(text)
    return home2


def dump(driver, e, base, name, gtype, tag, form):
    proc = subprocess.run([driver, base, "--dump", name, gtype, tag, form],
                          env=e, capture_output=True, text=True)
    if proc.returncode != 0 or "basis" not in proc.stdout:
        return None
    return proc.stdout


def deck(title, atoms, basis, workdir, extra=""):
    bl, el = split(basis)
    basis = "\n".join(bl + ["END"]) + ("\necp\n" + "\n".join(el) + "\nend"
                                       if el else "")
    geo = "\n".join("  %-2s %12.6f %12.6f %12.6f" % a for a in atoms)
    return """title "%s"
start chk
echo
scratch_dir %s/scratch
permanent_dir %s/perm
memory total 2 gb
geometry units angstrom noautosym
%s
end
%s
scf
  rhf
  singlet
  thresh 1e-8
  direct
end
%stask scf energy
""" % (title, workdir, workdir, geo, basis, extra)


def runNwchem(text, workdir, timeout):
    os.makedirs(os.path.join(workdir, "scratch"), exist_ok=True)
    os.makedirs(os.path.join(workdir, "perm"), exist_ok=True)
    with open(os.path.join(workdir, "in.nw"), "w") as handle:
        handle.write(text)
    env = dict(os.environ, OMP_NUM_THREADS="1")
    try:
        proc = subprocess.run(["nwchem", "in.nw"], cwd=workdir, env=env,
                              capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return {"status": "timeout"}
    out = proc.stdout
    with open(os.path.join(workdir, "out.txt"), "w") as handle:
        handle.write(out)
    energy = re.findall(r"Total SCF energy =\s+(-?\d+\.\d+)", out)
    if not energy:
        why = re.findall(r"(?:ERROR|library|basis set).*", out, re.I)
        return {"status": "error", "why": (why[:2] or [out[-300:]])[0].strip()}
    mm = re.search(r"^\s*functions\s*=\s*(\d+)", out, re.M)
    nb = int(mm.group(1)) if mm else None
    return {"status": "ok", "energy": float(energy[-1]), "nbf": nb,
            "lindep": "deemed linearly dependent" in out}


def partnerHydrogen(name):
    """The set supplying hydrogen to a set that has none (core-valence ...)."""
    m = re.search(r"(?:CV|V)\(?([DTQ56])", name.replace("aug-cc-p", "", 1),
                  re.I)
    if not m:
        return None
    prefix = "d-aug-" if name.lower().startswith("d-aug") else (
        "aug-" if "aug" in name.lower() else "")
    return "%scc-pV%sZ" % (prefix, m.group(1).upper())


def split(text):
    """A dump as (basis lines without END, ECP lines or [])."""
    lines = text.splitlines()
    cut = next((i for i, l in enumerate(lines) if l.strip() == "ECP"),
               len(lines))
    basis = [l for l in lines[:cut] if l.strip() != "END"]
    ecp = [l for l in lines[cut + 1:] if l.strip() != "END"]
    return basis, ecp


def merge(first, second):
    """One dump from two: second's basis lines join first's block."""
    b1, e1 = split(first)
    b2, e2 = split(second)
    ecp = ["ECP"] + e1 + e2 + ["END"] if e1 or e2 else []
    return "\n".join(b1 + b2[1:] + ["END"] + ecp) + "\n"


def runCase(args, name, gtype, element, scratch, driver, e, e2, base, hset):
    mol, atoms = MOLECULES[element]
    tag = element if hset else ("H" if element == "H" else element + " H")
    work = os.path.join(scratch, "run", re.sub(r"\W", "_", name) + "_" + element)
    result = {"basis": name, "element": element, "molecule": mol}
    named = dump(driver, e2, base, name, gtype, tag, "named")
    explicit = dump(driver, e, base, name, gtype, tag, "explicit")
    if named is None or explicit is None:
        result["status"] = "error: loadBasis produced no deck"
        return result
    if hset:
        #  Hydrogen from the plain set of the same zeta; only the element's
        #  own set is under test.
        hn = dump(driver, e2, base, hset, "correlation_consistent", "H",
                  "named")
        he = dump(driver, e, base, hset, "correlation_consistent", "H",
                  "explicit")
        if hn is None or he is None:
            result["status"] = "error: no hydrogen set " + hset
            return result
        named, explicit = merge(named, hn), merge(explicit, he)
        result["hydrogen from"] = hset
    lib = re.findall(r'^\s*(\w+)\s+library\s+(".*")\s*$',
                     "\n".join(split(named)[0]), re.M)
    result["libnames"] = dict(lib)
    result["blocked"] = blocked(dict(lib).get(element, ""))
    if not lib:
        result["status"] = "not named (ECCE writes it out in full anyway)"
        return result
    #  A heavier element of a set that already timed out will too.
    if name in SLOW and ORDER.index(element) >= SLOW[name]:
        result["status"] = "skipped (a lighter element timed out)"
        return result
    a = runNwchem(deck(name + " named", atoms, named, work + "_named"),
                  work + "_named", args.timeout)
    b = ({"status": "timeout"} if a["status"] == "timeout" else
         runNwchem(deck(name + " explicit", atoms, explicit,
                        work + "_explicit"), work + "_explicit", args.timeout))
    for w in (work + "_named", work + "_explicit"):
        if not args.keep:
            shutil.rmtree(w, ignore_errors=True)
    result["named"], result["explicit"] = a, b
    if a["status"] == "timeout" or b["status"] == "timeout":
        result["status"] = "timeout"
        SLOW[name] = min(SLOW.get(name, 99), ORDER.index(element))
    elif a["status"] == "error":
        result["status"] = "unavailable"
        result["why"] = a.get("why")
    elif b["status"] == "error":
        result["status"] = "error: explicit " + (b.get("why") or "")
    else:
        dE = a["energy"] - b["energy"]
        result["dE"] = dE
        if a["nbf"] == b["nbf"] and abs(dE) < 1e-6:
            result["status"] = "match"
        elif a["nbf"] == b["nbf"] and (a["lindep"] or b["lindep"]):
            #  ECCE's contractions can repeat primitives that are also
            #  free, so the deck spans the same space but NWChem drops a
            #  vector as linearly dependent.  Same basis, if the energies
            #  agree once nothing is dropped.
            tight = "set lindep:tol 1.0d-12\n"
            a2 = runNwchem(deck(name + " named", atoms, named, work + "_n2",
                                tight), work + "_n2", args.timeout)
            b2 = runNwchem(deck(name + " explicit", atoms, explicit,
                                work + "_e2", tight), work + "_e2",
                           args.timeout)
            for w in (work + "_n2", work + "_e2"):
                if not args.keep:
                    shutil.rmtree(w, ignore_errors=True)
            if a2["status"] == b2["status"] == "ok" and \
                    abs(a2["energy"] - b2["energy"]) < 1e-6:
                result["status"] = ("match (explicit deck is linearly "
                                    "dependent, dE %.2g untightened)" % dE)
            elif "timeout" in (a2["status"], b2["status"]):
                result["status"] = ("inconclusive (tightened run timed out; "
                                    "dE %.3g untightened)" % dE)
            else:
                result["status"] = "differs (nbf %s vs %s, dE %.3g)" % (
                    a["nbf"], b["nbf"], dE)
        else:
            result["status"] = "differs (nbf %s vs %s, dE %.3g)" % (
                a["nbf"], b["nbf"], dE)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--only", action="append", default=[],
                        help="a set name (repeatable); default: all candidates")
    parser.add_argument("--element", action="append", default=[])
    parser.add_argument("-j", "--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--out", default="nwchem_library_check.txt")
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args()
    args.jobs = max(1, min(args.jobs, 4))

    if not shutil.which("nwchem"):
        print("SKIP: no nwchem")
        return 0
    if not shutil.which("apache2") and not os.path.exists("/usr/sbin/apache2"):
        print("SKIP: no apache2")
        return 0

    scratch = os.environ.get("ECCE_NWBASIS_SCRATCH") or os.path.expanduser(
        "~/tmp/ecce/nwbasis-scratch")
    os.makedirs(scratch, exist_ok=True)
    state = os.path.join(scratch, "server")
    shutil.rmtree(state, ignore_errors=True)
    os.makedirs(state)
    cache = args.out + ".cache.json"
    done = json.load(open(cache)) if os.path.exists(cache) else {}
    for res in done.values():
        if res["status"] == "timeout":
            SLOW[res["basis"]] = min(SLOW.get(res["basis"], 99),
                                     ORDER.index(res["element"]))
    started, e = False, None
    try:
        driver = os.path.join(state, "loadBasis")
        try:
            R.buildDriver(driver)
        except R.Skip as why:
            print("SKIP: %s" % why)
            return 0
        port = R.freePort()
        home = R.makeHome(state, port)
        e = R.env(state, home, port)
        e2 = dict(e, ECCE_HOME=patchedHome(state, home))
        started = True
        proc = subprocess.run(
            [os.path.join(R.DATASERVER, "ecce-dataserver-start")], env=e,
            capture_output=True, text=True)
        if proc.returncode != 0:
            sys.exit("could not start the data server:\n" + proc.stderr)
        base = "http://127.0.0.1:%d%s" % (port, R.LIBPATH)

        jobs = []
        for name, gtype, atoms in candidates(args.only):
            hset = None if "H" in atoms else partnerHydrogen(name)
            if "H" not in atoms and not hset:
                continue
            for element in MOLECULES:
                if args.element and element not in args.element:
                    continue
                if element in atoms:
                    jobs.append((name, gtype, element, hset))
        #  smallest first, so a timeout wastes the least
        zeta = lambda n: (len(re.findall(r"5Z|6Z|7Z|8Z", n)),
                          len(re.findall(r"QZ", n)), n)
        jobs.sort(key=lambda j: (zeta(j[0]), ORDER.index(j[2])))
        todo = [j for j in jobs if "|".join(j[:3]) not in done]
        print("%d cases, %d to run" % (len(jobs), len(todo)), flush=True)

        def one(job):
            return job, runCase(args, job[0], job[1], job[2], scratch,
                                driver, e, e2, base, job[3])

        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            for job, res in pool.map(one, todo):
                done["|".join(job[:3])] = res
                print("%-28s %-3s %s" % (job[0], job[2], res["status"]),
                      flush=True)
                json.dump(done, open(cache, "w"))
    finally:
        if started:
            subprocess.run([os.path.join(R.DATASERVER, "ecce-dataserver-stop")],
                           env=e, capture_output=True)
        if not args.keep:
            shutil.rmtree(scratch, ignore_errors=True)

    rows = [done[k] for k in sorted(done)]
    with open(args.out, "w") as handle:
        handle.write("%-28s %-3s %-4s %-7s %s\n" % (
            "basis", "el", "mol", "blocked", "result"))
        for r in rows:
            handle.write("%-28s %-3s %-4s %-7s %s\n" % (
                r["basis"], r["element"], r["molecule"],
                "yes" if r.get("blocked") else "no", r["status"]))
    bad = [r for r in rows if not r["status"].startswith("match")]
    print("%d cases, %d not a match; table in %s" % (len(rows), len(bad),
                                                    args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
