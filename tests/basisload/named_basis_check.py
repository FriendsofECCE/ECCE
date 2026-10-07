#!/usr/bin/env python3
"""
Is a basis keyword the same basis as the one ECCE holds under that name?

    ./named_basis_check.py                         ORCA and Gaussian 16, all sets
    ./named_basis_check.py --code orca --only cc-pVTZ-DK
    ./named_basis_check.py -j 3 --out named_basis_check.results.txt

On demand only (slow: a few minutes per code per hundred sets); it is not
part of the fast run.  SKIPs (exit 0) when the code, apache2 or a build
tree (ECCE_TEST_BUILD) is missing.

wrORCAGBS.pm and wrGaussian16GBS.pm write a basis by name when ECCE's name
is in %NameToBasis.  An entry belongs there only if the code accepts the
keyword AND the keyword is the same basis ECCE holds -- neither is
implied by the spelling (Gaussian's def2SVPP is def2-SV(P), not def2-SVPP).
For every orbital set in data/admin/basissets, and for each spelling the
code might use, this runs an RHF energy of a small molecule built from the
elements the set covers twice with the same coordinate type: once with the
keyword and once with ECCE's primitives written out by the real
TGBSConfig::dump() -> std2<Code> path (loadBasis --dump, LOADBASIS_CODE).
Match means the same number of functions and |dE| < 1e-5 Eh.

The explicit deck is the oracle: it comes from ECCE's own data, so the
keyword is judged against what the user selected, not against another code.
ECP-bearing sets are not covered: the writers keep an element with an ECP
explicit regardless of the table.

Scratch lives under ECCE_NWBASIS_SCRATCH (default ~/tmp/ecce/nwbasis-scratch,
not /tmp: that is RAM on some machines) and is removed at the end.
"""

import argparse
import concurrent.futures
import glob
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as R  # noqa: E402
import nwchem_library_check as N  # noqa: E402

REPO = R.REPO
PARSERS = os.path.join(REPO, "scripts", "parsers")
ORBITAL_TYPES = [t for t in N.ORBITAL_TYPES if t != "ECPOrbital"]
#  --ecp (Gaussian only): sets with an ECP are checked on a molecule that
#  carries one, the explicit deck holding the library's ECP block and the
#  keyword deck leaving Gaussian to apply its own.
ECP_TYPE = "ECPOrbital"

#  Smallest closed-shell system for the elements a set covers; the first
#  whose elements are all covered is used, and a second-row one besides.
MOLS = [
    ("H2O", ["H", "O"], [("O", 0, 0, .117), ("H", 0, .757, -.469),
                         ("H", 0, -.757, -.469)]),
    ("N2", ["N"], [("N", 0, 0, 0), ("N", 0, 0, 1.098)]),
    ("F2", ["F"], [("F", 0, 0, 0), ("F", 0, 0, 1.412)]),
    ("Ne", ["Ne"], [("Ne", 0, 0, 0)]),
    ("He", ["He"], [("He", 0, 0, 0)]),
]
MOLS_ECP = [
    ("HI", ["H", "I"], [("I", 0, 0, 0), ("H", 0, 0, 1.609)]),
    #  closed-shell atoms: a 3d and a 5d core of different ECP shape
    ("Zn", ["Zn"], [("Zn", 0, 0, 0)]),
    ("Hg", ["Hg"], [("Hg", 0, 0, 0)]),
]
MOLS2 = [
    ("HCl", ["H", "Cl"], [("Cl", 0, 0, 0), ("H", 0, 0, 1.275)]),
    ("Cl2", ["Cl"], [("Cl", 0, 0, 0), ("Cl", 0, 0, 1.99)]),
    ("Ar", ["Ar"], [("Ar", 0, 0, 0)]),
]

#  Spellings that differ from ECCE's name in a way no rule derives.
ALIASES = {
    "ahlrichs vdz": ["SV", "def2-SV"],
    "ahlrichs pvdz": ["SVP", "def2-SVP"],
    "ahlrichs vtz": ["TZV", "def2-TZV"],
    "ahlrichs tzv": ["TZV", "def2-TZV"],
    "mini (huzinaga)": ["MINI"],
    "midi (huzinaga)": ["MIDI"],
    "dzp (dunning)": ["D95**"],
    "svp (dunning-hay)": ["D95V*"],
    "tz (dunning)": ["TZ"],
    "cc-pv(d+d)z": ["cc-pV(D+d)Z"],
    "def2-svp(p)": ["def2SVPP", "def2-SV(P)"],
    "dzvp (dft orbital)": ["DGDZVP"],
    "dzvp2 (dft orbital)": ["DGDZVP2"],
    "tzvp (dft orbital)": ["DGTZVP"],
    "ahlrichs tzvp": ["TZVP"],
}


def variants(name):
    """Spellings of ECCE's `name` to try as the code's keyword."""
    out = [name]
    if "(" not in name or "pv(" in name.lower():
        out.append(name.replace("-", ""))
    out += ALIASES.get(name.lower(), [])
    seen = []
    for v in out:
        if v not in seen:
            seen.append(v)
    return seen


def tableValues(code):
    path = os.path.join(PARSERS, {"orca": "wrORCAGBS.pm",
                                  "gaussian": "wrGaussian16GBS.pm"}[code])
    return dict((k, v) for k, v in re.findall(
        r'\$NameToBasis\{"([^"]+)"\}\s*=\s*"([^"]+)"', open(path).read()))


def geometry(atoms):
    return "\n".join("%-2s %12.6f %12.6f %12.6f" % a for a in atoms)


def orcaDeck(atoms, keyword, block):
    return "! RHF %s TightSCF\n%s* xyz 0 1\n%s\n*\n" % (
        keyword or "", block or "", geometry(atoms))


def gaussDeck(atoms, keyword, block):
    #  an explicit deck carries its ECP, if the set has one, after the basis
    pseudo = " pseudo=read" if block and "\n\n" in block.strip() else ""
    return ("%%nprocshared=1\n%%mem=2GB\n#p HF/%s%s 5D 7F nosymm scf=tight\n\n"
            "t\n\n0 1\n%s\n\n%s\n" % (
                keyword or "gen", pseudo if not keyword else "",
                geometry(atoms), "" if keyword else block + "\n"))


def run(code, text, workdir, timeout):
    os.makedirs(workdir, exist_ok=True)
    if code == "orca":
        orca = glob.glob("/opt/orca/orca_6_1_1_*/orca")[0]
        path = os.path.join(workdir, "job.inp")
        open(path, "w").write(text)
        cmd, stdin = [orca, "job.inp"], None
        env = dict(os.environ)
    else:
        path = os.path.join(workdir, "job.gjf")
        open(path, "w").write(text)
        cmd, stdin = ["/opt/gaussian/g16/g16"], open(path, "rb")
        env = dict(os.environ, g16root="/opt/gaussian", GAUSS_SCRDIR=workdir)
    try:
        p = subprocess.run(cmd, cwd=workdir, env=env, stdin=stdin,
                           capture_output=True, text=True, errors="replace",
                           timeout=timeout)
    except subprocess.TimeoutExpired:
        return {"status": "timeout"}
    out = p.stdout
    if code == "orca":
        e = re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", out)
        nb = re.search(r"Basis Dimension\s+Dim\s+\.+\s+(\d+)", out)
    else:
        e = re.findall(r"SCF Done:.*?=\s+(-?\d+\.\d+)", out)
        nb = re.search(r"NBasis=\s+(\d+)", out)
    if not e:
        why = re.findall(r"(?:ERROR|rror|not recogni|not found|illegal|"
                         r"Unrecognized|unknown).*", out, re.I)
        return {"status": "error", "why": (why[:1] or [out[-200:]])[0].strip()}
    return {"status": "ok", "energy": float(e[-1]),
            "nbf": int(nb.group(1)) if nb else None}


SECOND = False
#  Same basis, ECCE data rounded differently from the code's (STO-2G and
#  D95V differ by 3.4e-6 Eh on water); a different basis differs by >1e-4.
TOL = 1e-5
SKIP = re.compile(r"\(pt/|\(fi/|\(old\)|Core Set|Partridge|Rydberg|Feller|NASA|"
                  r"mcc|^pV[67]Z|GAMESS|McLean|Roos|Bauschlicher|Chipman|"
                  r"Diffuse|Blaudeau|seg-opt|-NR$|Binning|Huzinaga \(S", re.I)


def pick(atoms, gtype=None):
    have = set(atoms)
    chosen = []
    if gtype == ECP_TYPE:
        chosen += [m for m in MOLS_ECP if set(m[1]) <= have]
    for group in ((MOLS, MOLS2) if SECOND else (MOLS,)):
        for m in group:
            if set(m[1]) <= have:
                chosen.append(m)
                break
    return chosen


def explicitDeck(code, driver, e, base, name, gtype, mol, mode="explicit"):
    tag = " ".join(sorted(set(a[0] for a in mol[2])))
    env = dict(e, LOADBASIS_CODE="ORCA" if code == "orca" else "Gaussian-16")
    p = subprocess.run([driver, base, "--dump", name, gtype, tag, mode],
                       env=env, capture_output=True, text=True)
    if p.returncode != 0 or not p.stdout.strip():
        return None
    text = p.stdout
    if code == "gaussian":
        #  the dump may carry a trailing ECP section; sets here have none
        text = text.rstrip() + "\n"
    return text


def caseKey(*parts):
    return "|".join(parts)


def runName(args, code, name, gtype, atoms, scratch, driver, e, base, table):
    deck = orcaDeck if code == "orca" else gaussDeck
    res = {"basis": name, "code": code, "in_table": table.get(name.lower()),
           "mols": [], "keyword": None, "status": None, "tried": {}}
    mols = pick(atoms, gtype)
    if not mols:
        res["status"] = "not run (no small test molecule for its elements)"
        return res
    work = os.path.join(scratch, code, re.sub(r"\W", "_", name))
    ref = {}
    for mol in mols:
        text = explicitDeck(code, driver, e, base, name, gtype, mol)
        if text is None:
            res["status"] = "error: loadBasis produced no deck"
            return res
        ref[mol[0]] = run(code, deck(mol[2], None, text),
                          "%s/ref_%s" % (work, mol[0]), args.timeout)
        if ref[mol[0]]["status"] != "ok":
            res["status"] = "error: explicit deck: %s" % ref[mol[0]]
            return res
    keywords = variants(name)
    if res["in_table"] and res["in_table"] not in keywords:
        keywords.insert(0, res["in_table"])
    for kw in keywords:
        if re.search(r"\s", kw):
            continue
        verdicts = []
        for mol in mols:
            got = run(code, deck(mol[2], kw, None),
                      "%s/kw_%s_%s" % (work, re.sub(r"\W", "_", kw), mol[0]),
                      args.timeout)
            r = ref[mol[0]]
            if got["status"] != "ok":
                verdicts.append("%s: %s" % (mol[0], got["status"] + (
                    " (%s)" % got["why"] if got.get("why") else "")))
                break
            dE = got["energy"] - r["energy"]
            ok = got["nbf"] == r["nbf"] and abs(dE) < TOL
            verdicts.append("%s: %s nbf %s/%s dE %.2g" % (
                mol[0], "match" if ok else "DIFFERS", got["nbf"], r["nbf"], dE))
            if not ok:
                break
            res["mols"].append("%s nbf %s" % (mol[0], r["nbf"]))
        extra = []
        if gtype == ECP_TYPE and all(": match" in v for v in verdicts):
            #  What the writer really emits for the molecule (named where it
            #  may, explicit with its ECP where it must) against the
            #  explicit deck; "useRouteCard" means the whole thing is named.
            for mol in mols:
                if mol[0] != "HI":
                    continue
                text = explicitDeck(code, driver, e, base, name, gtype, mol,
                                    "named")
                if text is None or text.startswith("useRouteCard"):
                    continue
                got = run(code, deck(mol[2], None, text),
                          "%s/wr_%s" % (work, mol[0]), args.timeout)
                dE = got.get("energy", 0) - ref[mol[0]]["energy"]
                ok = got["status"] == "ok" and abs(dE) < TOL and \
                    got["nbf"] == ref[mol[0]]["nbf"]
                extra.append("%s writer deck: %s dE %.2g" % (
                    mol[0], "match" if ok else "DIFFERS", dE))
        res["tried"][kw] = verdicts + extra
        if any("DIFFERS" in v for v in extra):
            res["status"] = "keyword matches, writer deck DIFFERS"
            break
        if verdicts and all(": match" in v for v in verdicts) and \
                len(verdicts) == len(mols):
            res["keyword"] = kw
            res["status"] = "match"
            break
    if res["status"] is None:
        res["status"] = "no keyword matches"
    if not args.keep:
        shutil.rmtree(work, ignore_errors=True)
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--code", action="append", choices=["orca", "gaussian"])
    ap.add_argument("--only", action="append", default=[])
    ap.add_argument("--second", action="store_true",
                    help="also a second-row molecule per set (twice the runs)")
    ap.add_argument("--all", action="store_true",
                    help="include sets no code is expected to ship")
    ap.add_argument("--ecp", action="store_true",
                    help="also the ECP-bearing sets (Gaussian only)")
    ap.add_argument("--big", action="store_true",
                    help="include 5Z and larger sets")
    ap.add_argument("-j", "--jobs", type=int, default=3)
    ap.add_argument("--timeout", type=int, default=300)
    ap.add_argument("--out", default="named_basis_check.results.txt")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    global SECOND
    SECOND = args.second
    codes = args.code or ["orca", "gaussian"]
    args.jobs = max(1, min(args.jobs, 4))

    if "orca" in codes and not glob.glob("/opt/orca/orca_6_1_1_*/orca"):
        print("SKIP: no ORCA")
        return 0
    if "gaussian" in codes and not os.access("/opt/gaussian/g16/g16", os.X_OK):
        print("SKIP: no Gaussian 16")
        return 0
    if not shutil.which("apache2") and not os.path.exists("/usr/sbin/apache2"):
        print("SKIP: no apache2")
        return 0

    scratch = os.environ.get("ECCE_NWBASIS_SCRATCH") or os.path.expanduser(
        "~/tmp/ecce/nwbasis-scratch")
    state = os.path.join(scratch, "server")
    shutil.rmtree(state, ignore_errors=True)
    os.makedirs(state)
    cache = args.out + ".cache.json"
    done = json.load(open(cache)) if os.path.exists(cache) else {}
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
        started = True
        p = subprocess.run([os.path.join(R.DATASERVER, "ecce-dataserver-start")],
                           env=e, capture_output=True, text=True)
        if p.returncode != 0:
            sys.exit("could not start the data server:\n" + p.stderr)
        base = "http://127.0.0.1:%d%s" % (port, R.LIBPATH)

        sets = []
        for gtype in ORBITAL_TYPES + ([ECP_TYPE] if args.ecp else []):
            for ent in N.parseIndex(os.path.join(N.INDEXDIR, gtype)):
                if args.only and ent["name"] not in args.only:
                    continue
                if not args.all and not args.only and SKIP.search(ent["name"]):
                    continue
                if not args.big and re.search(r"[5-9]Z|\(\d\+d\)Z",
                                              ent["name"]) and not args.only:
                    continue
                sets.append((ent["name"], gtype, ent["atoms"]))
        tables = dict((c, tableValues(c)) for c in codes)
        jobs = [(c,) + s for c in codes for s in sets
                if caseKey(c, s[0]) not in done
                and not (c == "orca" and s[1] == ECP_TYPE)]
        print("%d cases to run" % len(jobs), flush=True)

        def one(job):
            c, name, gtype, atoms = job
            return job, runName(args, c, name, gtype, atoms, scratch, driver,
                                e, base, tables[c])

        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            for job, res in pool.map(one, jobs):
                done[caseKey(job[0], job[1])] = res
                print("%-9s %-28s %s %s" % (job[0], job[1], res["status"],
                                           res["keyword"] or ""), flush=True)
                json.dump(done, open(cache, "w"))
    finally:
        if started:
            subprocess.run([os.path.join(R.DATASERVER, "ecce-dataserver-stop")],
                           env=e, capture_output=True)
        if not args.keep:
            shutil.rmtree(scratch, ignore_errors=True)

    #  A run covering a subset (--only) updates its own lines of the file
    #  and leaves the rest as they were.
    lines = {}
    if os.path.exists(args.out):
        for ln in open(args.out).read().splitlines()[1:]:
            lines[caseKey(ln[:8].strip(), ln[9:37].strip())] = ln
    for k in done:
        r = done[k]
        detail = ""
        if r["keyword"]:
            detail = "  [%s]" % "; ".join(r["tried"][r["keyword"]])
        lines[k] = "%-8s %-28s %-22s %-22s %s%s" % (
            r["code"], r["basis"], r["keyword"] or "-",
            r["in_table"] or "-", r["status"], detail)
    with open(args.out, "w") as h:
        h.write("%-8s %-28s %-22s %-22s %s\n" % (
            "code", "ECCE name", "keyword (verified)", "in table now", "result"))
        for k in sorted(lines):
            h.write(lines[k] + "\n")
    print("table in", args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
