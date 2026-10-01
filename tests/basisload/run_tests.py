#!/usr/bin/env python3
"""
Does a basis set picked in ECCE arrive in the saved calculation whole?

    ./run_tests.py         run everything
    ./run_tests.py -v      show each driver's output

Loads basis sets from a REAL data server -- a private per-user Apache
started by the repository's own ecce-dataserver-start, serving the
repository's basis-set library on a free loopback port -- through the real
EDSIGaussianBasisSetLibrary linked from build-cmake's static libraries, and
checks the BasisSet.ecce_basisset document DavCalculation's own writers
produce from it (see loadBasis.C).  SKIPs when there is no build tree or no
apache2.

Why this exists
---------------
6-31G* saved for CH4 with no carbon d shell (2026-09-27, reproduced live
twice): the stored file named both components "6-31G*" of type
other_generally_contracted, TGBSGroup::insertGBS() saw one basis set twice
and kept the first, and 6-31GS.BAS's polarization functions vanished with
no error anywhere.  The identity came from DAV dead properties that the
deleted populate-gbs-metadata.py (b3273d2) PROPPATCHed onto the component
files of a live data server; lookup() consulted them before the index
files.  A fix made against a clean server (3e9b524) could not see it.

So every check runs twice: against a clean library, and again after
PROPPATCHing exactly what that script wrote.  Nothing in a clean seed has
readable dead properties (the legacy sdbm stores in the seed tar are
unreadable to a modern mod_dav_fs), so the second pass is the one that
reproduces a data server that has been in use.

Whole-file aggregates
---------------------
An aggregate such as 6-31G* is one file (its "-AGG.BAS", written by
tools/basissets/merge_aggregates.py) and loads as ONE basis set, the way
cc-pVDZ does.  The file decides the path: a library whose placeholders are
empty (an older server) is read from the component files, as before.  So the
suite runs the cases against the shipped library (whole sets), again after
PROPPATCHing the dead properties, and once more with the placeholders
emptied on top of that (the older server, the #164 reproduction), each with
its own expectations.  In the clean phase it also loads every multi-file
aggregate both ways and requires the shells in dump() order, and dump()'s
own text (by name and in full), to be identical: the component load is the
oracle for the whole file.

Only this suite's own server is ever started or stopped; it listens on
loopback alone, and its config -- the shipped template with PROPPATCH
permitted on the library, which the real one denies -- never leaves the
throwaway state directory.

Exit status is 0 only if every check passed (or the suite was skipped).
"""

import argparse
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))), "tools", "basissets"))
import merge_aggregates  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
DATASERVER = os.path.join(REPO, "packaging", "dataserver")
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "eccecipc", "ecceutil", "eccecomm", "eccercmd"]
NS = "http://www.emsl.pnl.gov/ecce:"
LIBPATH = "/Ecce/system/GaussianBasisSetLibrary"

#  What b3273d2's populate-gbs-metadata.py PROPPATCHed, in its order (a
#  file written twice keeps the second).  .POT files were typed ecp.
POISON = [
    ("6-31G.BAS", "6-31G", "other_generally_contracted"),
    ("6-31G.BAS", "6-31G*", "other_generally_contracted"),
    ("6-31GS.BAS", "6-31G*", "other_generally_contracted"),
    ("cc-pVDZ.BAS", "cc-pVDZ", "correlation_consistent"),
    ("cc-pVTZ.BAS", "cc-pVTZ", "correlation_consistent"),
    ("AUG-CC-PVDZ.BAS", "aug-cc-pVDZ", "correlation_consistent"),
    ("AUG-CC-PVTZ.BAS", "aug-cc-pVTZ", "correlation_consistent"),
    ("def2-svp.BAS", "def2-svp", "ECPOrbital"),
    ("def2-svp.POT", "def2-svp", "ecp"),
    ("def2-svpd.BAS", "def2-svpd", "ECPOrbital"),
    ("def2-svpd.POT", "def2-svpd", "ecp"),
    ("def2-tzvp.BAS", "def2-tzvp", "ECPOrbital"),
]

#  (label, name, picked-from list or "quick", tag, expectations).
#  An expectation is (set name, set type, element, shells it must have);
#  shells=None means an ECP entry for that element, and element "*" means
#  the set must not be in the saved data at all.
#  COMPOSITE_CASES is what a library with empty aggregate placeholders gives:
#  the aggregate as its components, each with an identity of its own.
COMPOSITE_CASES = [
    ("6-31G* CH4", "6-31G*", "pople", "C H", [
        ("6-31G", "pople", "C", ["S", "SP", "SP"]),
        ("6-31G", "pople", "H", ["S", "S"]),
        ("6-31G* Polarization", "polarization", "C", ["D"]),
    ]),
    ("6-31G* CH4 quick pick", "6-31G*", "quick", "C H", [
        ("6-31G", "pople", "C", ["S", "SP", "SP"]),
        ("6-31G* Polarization", "polarization", "C", ["D"]),
    ]),
    ("6-31G** CH4", "6-31G**", "pople", "C H", [
        ("6-31G", "pople", "C", ["S", "SP", "SP"]),
        ("6-31G** Polarization", "polarization", "C", ["D"]),
        ("6-31G** Polarization", "polarization", "H", ["P"]),
    ]),
    #  A mixed basis gives each element its own group (#164: WaterX,
    #  H STO-3G + O 6-31G*, was stored with no O d shell).
    ("6-31G* on O alone, mixed basis", "6-31G*", "pople", "O", [
        ("6-31G", "pople", "O", ["S", "SP", "SP"]),
        ("6-31G* Polarization", "polarization", "O", ["D"]),
    ]),
    ("6-31G alone keeps its own name", "6-31G", "pople", "C H", [
        ("6-31G", "pople", "C", ["S", "SP", "SP"]),
    ]),
    ("STO-3G* SiH4", "STO-3G*", "pople", "Si H", [
        ("STO-3G", "pople", "Si", ["S", "SP", "SP"]),
        ("STO-3G* Polarization", "polarization", "Si", ["D"]),
    ]),
]

#  What the shipped library gives: the same sets whole, one entry each.
WHOLE_CASES = [
    ("6-31G* CH4", "6-31G*", "pople", "C H", [
        ("6-31G*", "pople", "C", ["S", "SP", "SP", "D"]),
        ("6-31G*", "pople", "H", ["S", "S"]),
        ("6-31G", "pople", "*", None),
        ("6-31G* Polarization", "polarization", "*", None),
    ]),
    ("6-31G* CH4 quick pick", "6-31G*", "quick", "C H", [
        ("6-31G*", "pople", "C", ["S", "SP", "SP", "D"]),
        ("6-31G* Polarization", "polarization", "*", None),
    ]),
    ("6-31G** CH4", "6-31G**", "pople", "C H", [
        ("6-31G**", "pople", "C", ["S", "SP", "SP", "D"]),
        ("6-31G**", "pople", "H", ["S", "S", "P"]),
        ("6-31G** Polarization", "polarization", "*", None),
    ]),
    ("6-31G* on O alone, mixed basis", "6-31G*", "pople", "O", [
        ("6-31G*", "pople", "O", ["S", "SP", "SP", "D"]),
    ]),
    ("6-31G alone keeps its own name", "6-31G", "pople", "C H", [
        ("6-31G", "pople", "C", ["S", "SP", "SP"]),
    ]),
    ("STO-3G* SiH4", "STO-3G*", "pople", "Si H", [
        ("STO-3G*", "pople", "Si", ["S", "SP", "SP", "D"]),
        ("STO-3G* Polarization", "polarization", "*", None),
    ]),
    ("aug-cc-pVDZ water", "aug-cc-pVDZ", "correlation_consistent", "O H", [
        ("aug-cc-pVDZ", "correlation_consistent", "O", ["S", "P", "D"]),
        ("aug-cc-pVDZ Diffuse", "diffuse", "*", None),
    ]),
    ("6-31+G* CH4 (diffuse from a shared file)", "6-31+G*", "pople", "C H", [
        ("6-31+G*", "pople", "C", ["S", "SP", "SP", "SP", "D"]),
        ("6-31+G*", "pople", "H", ["S", "S"]),
    ]),
]

#  Sets that were never aggregates load as they always did.
COMMON_CASES = [
    ("cc-pVDZ-PP I", "cc-pVDZ-PP", "ECPOrbital", "I", [
        ("cc-pVDZ-PP", "ECPOrbital", "I", ["S", "P", "D"]),
        ("Stuttgart-Koeln MCDHF RSC ECP", "ecp", "I", None),
    ]),
    ("def2-svp HI", "def2-svp", "ECPOrbital", "H I", [
        ("def2-svp", "ECPOrbital", "H", ["S"]),
        ("def2-svp", "ECPOrbital", "I", ["S", "P", "D"]),
        ("Def2-ECP", "ecp", "I", None),
    ]),
]

#  aggregate-with-ECP sets keep the component path in every phase.
KEPT_COMPOSITE = ["aug-cc-pVDZ-PP", "aug-cc-pVTZ-PP", "aug-cc-pVQZ-PP",
                  "aug-cc-pV5Z-PP", "LANL2DZdp ECP", "SDB-aug-cc-pVTZ",
                  "SDB-aug-cc-pVQZ"]


class Skip(Exception):
    pass


def freePort(start=8496):
    for port in range(start, start + 100):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(0.3)
            if sock.connect_ex(("127.0.0.1", port)) != 0:
                return port
    raise Skip("no free port near %d" % start)


def buildDriver(out):
    if not all(os.path.exists(os.path.join(BUILD, "lib%s.a" % l)) for l in LIBS):
        raise Skip("no built static libraries in %s (set ECCE_TEST_BUILD)"
                   % BUILD)
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"),
            "-o", out, os.path.join(HERE, "loadBasis.C"), "-L" + BUILD]
           + ["-l" + l for l in LIBS] * 3 + ["-lxerces-c"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        sys.exit("could not build loadBasis:\n" + proc.stderr[-3000:])


def makeHome(state, port):
    """An ECCE_HOME holding just what ecce-dataserver-start reads."""
    home = os.path.join(state, "ecce-home")
    os.makedirs(os.path.join(home, "server", "httpd-conf"))
    #  The repository's own data: the library it is testing, the seed tree
    #  and the client tables (element data) the C++ loads.
    os.symlink(os.path.join(REPO, "data"), os.path.join(home, "data"))
    #  dump() pipes its text through std2NWChem, which finds its perl
    #  modules under $ECCE_HOME/scripts/parsers.
    os.symlink(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
    #  The C++ reads siteconfig/DataServers on first contact with a server.
    #  Point it at this run's port, never at the real 8096.
    os.makedirs(os.path.join(home, "siteconfig"))
    with open(os.path.join(REPO, "siteconfig", "DataServers")) as handle:
        servers, n = re.subn(r"(http://[^:<\s]+):\d+", r"\1:%d" % port,
                             handle.read())
    if not n or ":8096" in servers:
        sys.exit("siteconfig/DataServers: could not repoint it at :%d" % port)
    with open(os.path.join(home, "siteconfig", "DataServers"), "w") as handle:
        handle.write(servers)
    with open(os.path.join(DATASERVER, "httpd.conf.ecce")) as handle:
        conf = handle.read()
    #  Loopback-only is the template's own default now (#138):
    #  ecce-dataserver-start expands ##LISTEN## and env() below makes sure
    #  no inherited ECCE_DATASERVER_LISTEN widens it.  Check the line is
    #  still there rather than trusting it -- a bare "Listen ##PORT##" would
    #  put this suite's PROPPATCH-enabled server on every interface.
    if not re.search(r"(?m)^##LISTEN##$", conf) or \
            re.search(r"(?m)^Listen ", conf):
        sys.exit("httpd.conf.ecce: expected a ##LISTEN## line and no "
                 "literal Listen")
    #  Permit PROPPATCH on the library, to plant the dead properties.
    conf, n = re.subn(
        r"(<Directory \"##DATAROOT##/Ecce\">.*?)"
        r"<Limit HEAD GET POST OPTIONS PROPFIND>(.*?)"
        r"<Limit MKCOL PUT DELETE LOCK UNLOCK COPY MOVE PROPPATCH>",
        r"\1<Limit HEAD GET POST OPTIONS PROPFIND PROPPATCH>\2"
        r"<Limit MKCOL PUT DELETE LOCK UNLOCK COPY MOVE>", conf, count=1,
        flags=re.S)
    if n != 1:
        sys.exit("httpd.conf.ecce: the Ecce/ ACL is not where this suite "
                 "expects it; update makeHome()")
    with open(os.path.join(home, "server", "httpd-conf", "httpd.conf.ecce"),
              "w") as handle:
        handle.write(conf)
    return home


def env(state, home, port):
    e = dict(os.environ)
    e.update(ECCE_HOME=home, ECCE_REALUSERHOME=state, ECCE_REALUSER="tester",
             ECCE_DATASERVER_PORT=str(port),
             PATH=os.path.join(REPO, "scripts", "parsers") + os.pathsep
             + e.get("PATH", ""))
    e.pop("ECCE_DATASERVER_LISTEN", None)
    return e


def proppatch(base, filename, name, gbstype):
    body = ("""<?xml version="1.0" encoding="utf-8"?>
<D:propertyupdate xmlns:D="DAV:" xmlns:e="%s"><D:set><D:prop>
<e:name>%s</e:name><e:type>%s</e:type>
<e:spherical>Y</e:spherical><e:contraction_type>Segmented</e:contraction_type>
</D:prop></D:set></D:propertyupdate>""" % (NS, name, gbstype))
    req = urllib.request.Request(base + "/" + urllib.parse.quote(filename),
                                 data=body.encode(), method="PROPPATCH")
    req.add_header("Content-Type", "text/xml")
    try:
        with urllib.request.urlopen(req) as resp:
            return resp.status
    except urllib.error.HTTPError as err:
        return err.code


def propfindName(base, filename):
    body = ('<?xml version="1.0"?><D:propfind xmlns:D="DAV:">'
            '<D:allprop/></D:propfind>')
    req = urllib.request.Request(base + "/" + urllib.parse.quote(filename),
                                 data=body.encode(), method="PROPFIND")
    req.add_header("Depth", "0")
    req.add_header("Content-Type", "text/xml")
    with urllib.request.urlopen(req) as resp:
        text = resp.read().decode()
    m = re.search(r"<ns\d+:name>([^<]*)</ns\d+:name>", text)
    return m.group(1) if m else None


def parseData(text):
    """{(set name, type): {element: [shells] or None for an ECP}}"""
    data = text.split("<Data>", 1)[-1]
    sets, current, element = {}, None, None
    lines = data.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("BasisSet="):
            name = line[len("BasisSet="):]
            gtype = re.match(r"Type=(\S+)", lines[i + 1]).group(1)
            current = sets.setdefault((name, gtype), {})
        elif line.startswith("element=") and current is not None:
            element = line.split()[0][len("element="):]
            current[element] = None if "ncore=" in line else []
        elif line.startswith("contraction shell=") and current is not None:
            shell = line.split()[1][len("shell="):]
            #  A general contraction is written "SSSS": one S shell, four
            #  contractions sharing its exponents.
            if len(set(shell)) == 1:
                shell = shell[0]
            current[element].append(shell)
    return sets


def runCollide(driver, e):
    proc = subprocess.run([driver, "-", "--collide"], env=e,
                          capture_output=True, text=True)
    if proc.returncode != 0:
        return ["driver exited %d: %s" % (proc.returncode, proc.stderr[-500:])]
    same, _, twins = proc.stderr.partition("CASE twins")
    problems = []
    marker = "two of its parts are both named"
    if marker in same:
        problems.append("warned about one part inserted twice")
    if marker not in twins:
        problems.append("no warning for two distinct parts named 6-31G*")
    return problems


def runCase(driver, e, base, case, verbose):
    label, name, how, tag, expect = case
    proc = subprocess.run([driver, base, name, how, tag], env=e,
                          capture_output=True, text=True)
    if verbose:
        print(proc.stderr + proc.stdout)
    if proc.returncode != 0:
        return ["driver exited %d: %s" % (proc.returncode, proc.stderr[-500:])]
    sets = parseData(proc.stdout)
    problems = []
    for setName, gtype, element, shells in expect:
        got = sets.get((setName, gtype))
        if element == "*":
            if got is not None:
                problems.append("%s (%s) is in the saved data; it should "
                                "be part of the whole set" % (setName, gtype))
        elif got is None:
            problems.append("no %s (%s) in the saved data; it has %s"
                            % (setName, gtype, sorted(sets)))
        elif element not in got:
            problems.append("%s (%s) has no %s" % (setName, gtype, element))
        elif shells is None and got[element] is not None:
            problems.append("%s (%s) for %s is not an ECP"
                            % (setName, gtype, element))
        elif shells is not None and not all(
                got[element].count(s) >= shells.count(s) for s in set(shells)):
            problems.append("%s (%s) %s shells %s, want at least %s"
                            % (setName, gtype, element, got[element], shells))
    return problems


def runSweep(driver, e, base, verbose, whole):
    """Every multi-file aggregate.

    whole=True: the ones tools/basissets/merge_aggregates.py fills come
    back as ONE set carrying the aggregate's own name and type; the rest
    (ECP, fitting) come back as components.  whole=False: all components,
    each its own identity.
    """
    proc = subprocess.run([driver, base, "--sweep"], env=e,
                          capture_output=True, text=True)
    if proc.returncode != 0:
        return 0, ["sweep exited %d" % proc.returncode]
    aggs, cur = [], None
    for line in proc.stdout.splitlines():
        if line.startswith("AGG "):
            index, name, files = line[4:].split("|")
            cur = dict(index=index, name=name, files=files.split(), comps=[])
            aggs.append(cur)
        elif line.startswith("COMP "):
            cur["comps"].append(line[5:].split("|"))
    merged = {(a["type"], a["name"]) for a, verdict, _ in
              merge_aggregates.plan(merge_aggregates.DEFAULT_DIR)
              if verdict == "merge"}
    problems, wholeCount = [], 0
    for agg in aggs:
        where = "%s %s" % (agg["index"], agg["name"])
        comps = agg["comps"]
        if whole and (agg["index"], agg["name"]) in merged:
            wholeCount += 1
            if len(comps) != 1:
                problems.append("%s: %d sets came back, want the one whole "
                                "set" % (where, len(comps)))
            else:
                _, cname, ctype, count = comps[0]
                if (cname, ctype) != (agg["name"], agg["index"]):
                    problems.append("%s: whole set is %s (%s)"
                                    % (where, cname, ctype))
                if count == "0":
                    problems.append("%s: the whole set has no elements"
                                    % where)
            continue
        if len(comps) != len(agg["files"]):
            problems.append("%s: %d of %d components came back"
                            % (where, len(comps), len(agg["files"])))
            continue
        keys = [(c[1], c[2]) for c in comps]
        if len(set(keys)) != len(keys):
            problems.append("%s: components share an identity %s -- "
                            "insertGBS() would drop one" % (where, keys))
        for filename, cname, ctype, count in comps:
            isPot = filename.endswith(".POT")
            if isPot != (ctype == "ecp"):
                problems.append("%s: %s typed %s" % (where, filename, ctype))
            if count == "0":
                problems.append("%s: %s has no elements" % (where, filename))
    if whole and wholeCount != len(merged):
        problems.append("%d aggregates came back whole, %d were merged"
                        % (wholeCount, len(merged)))
    if verbose:
        print("  swept %d aggregates, %d whole" % (len(aggs), wholeCount))
    return len(aggs), problems


def parseOracle(text):
    """{aggregate: (sets returned, body)}"""
    result, key = {}, None
    for line in text.splitlines(True):
        if line.startswith("AGG "):
            head, _, count = line.rstrip("\n").rpartition("|")
            key = head
            result[key] = [int(count), []]
        elif key is not None:
            result[key][1].append(line)
    return result


def runOracle(driver, e, base):
    """The whole file against the components it was merged from: same
    shells, in dump() order, and the same dump() text."""
    texts = {}
    for mode in ("whole", "composite"):
        args = [driver, base, "--oracle"] + ([mode] if mode == "composite"
                                             else [])
        proc = subprocess.run(args, env=e, capture_output=True, text=True)
        if proc.returncode != 0:
            return 0, 0, ["--oracle %s exited %d: %s"
                          % (mode, proc.returncode, proc.stderr[-500:])]
        texts[mode] = parseOracle(proc.stdout)
    whole, comp = texts["whole"], texts["composite"]
    problems = []
    if set(whole) != set(comp) or not whole:
        problems.append("the two loads did not cover the same aggregates")
    single = compared = 0
    for key in sorted(whole):
        if key not in comp:
            continue
        compared += 1
        if whole[key][0] == 1 and comp[key][0] > 1:
            single += 1
        if whole[key][1] != comp[key][1]:
            a, b = whole[key][1], comp[key][1]
            first = next((i for i in range(min(len(a), len(b)))
                          if a[i] != b[i]), min(len(a), len(b)))
            problems.append("%s: whole file differs from its components at "
                            "line %d: %r vs %r" % (
                                key, first,
                                a[first] if first < len(a) else None,
                                b[first] if first < len(b) else None))
    return compared, single, problems


def runComplete(driver, e, base):
    """A calculation stored by an older release holds 6-31G* as its
    components; completing its group for a new molecule must not add the
    polarization set a second time next to the whole set."""
    proc = subprocess.run([driver, base, "--complete", "6-31G*", "pople",
                           "C H"], env=e, capture_output=True, text=True)
    if proc.returncode != 0:
        return ["--complete exited %d: %s" % (proc.returncode,
                                              proc.stderr[-500:])]
    sets = [l[4:] for l in proc.stdout.splitlines() if l.startswith("SET ")]
    ref = subprocess.run([driver, base, "6-31G*", "pople", "C H"], env=e,
                         capture_output=True, text=True)
    want = parseData(ref.stdout)[("6-31G*", "pople")]
    lines = proc.stdout.splitlines()
    shells, element = {}, None
    for l in lines:
        if l.startswith("element "):
            element = l.split()[1]
            shells[element] = []
        elif l.startswith("shell "):
            shells[element].append(l.split()[1])
    problems = []
    if sets != ["6-31G*|pople"]:
        problems.append("completed group holds %s, want only 6-31G*" % sets)
    for element, expect in want.items():
        got = shells.get(element)
        if got != expect:
            problems.append("%s: completed group has %s, the whole set %s"
                            % (element, got, expect))
    return problems


def report(label, problems, verbose=False):
    print("  %s %s" % ("FAIL" if problems else "ok  ", label))
    for p in problems:
        print("       " + p)
    return bool(problems)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    if not shutil.which("apache2") and not os.path.exists("/usr/sbin/apache2"):
        print("SKIP: no apache2")
        return 0

    parent = os.path.join(os.environ.get("XDG_CACHE_HOME") or
                          os.path.expanduser("~/.cache"))
    os.makedirs(parent, exist_ok=True)
    state = tempfile.mkdtemp(prefix="ecce-basisload-", dir=parent)
    failures = 0
    started = False
    e = None
    try:
        driver = os.path.join(state, "loadBasis")
        try:
            buildDriver(driver)
        except Skip as why:
            print("SKIP: %s" % why)
            return 0
        port = freePort()
        home = makeHome(state, port)
        e = env(state, home, port)
        proc = subprocess.run(
            [os.path.join(DATASERVER, "ecce-dataserver-start")], env=e,
            capture_output=True, text=True)
        started = True
        if proc.returncode != 0:
            sys.exit("could not start the suite's data server:\n"
                     + proc.stdout + proc.stderr)
        base = "http://127.0.0.1:%d%s" % (port, LIBPATH)

        libdir = os.path.join(state, ".ECCE", "dataserver", "htdocs", "Ecce",
                              "system", "GaussianBasisSetLibrary")

        def runCases(cases, whole):
            failed = 0
            for case in cases:
                problems = runCase(driver, e, base, case, args.verbose)
                failed += report(case[0], problems)
            count, problems = runSweep(driver, e, base, args.verbose, whole)
            failed += report("every multi-file aggregate (%d)" % count,
                             problems or ([] if count else ["none found"]))
            return failed

        #  The committed placeholders are what the script writes today.
        stale = subprocess.run(
            [sys.executable, os.path.join(REPO, "tools", "basissets",
                                          "merge_aggregates.py"), "--check"],
            capture_output=True, text=True)
        failures += report("-AGG.BAS files are current (merge_aggregates.py "
                           "--check)", [] if stale.returncode == 0
                           else [stale.stdout.strip()[-400:]])

        failures += report("two same-named parts of one set are reported "
                           "(#164's shape); one part inserted twice is not",
                           runCollide(driver, e))

        for phase in ("clean", "poisoned"):
            if phase == "poisoned":
                for filename, name, gtype in POISON:
                    status = proppatch(base, filename, name, gtype)
                    if status != 207:
                        sys.exit("PROPPATCH %s returned %d" % (filename, status))
                #  The reproduction is only a reproduction if the server
                #  really serves the wrong identity now.
                served = propfindName(base, "6-31GS.BAS")
                if served != "6-31G*":
                    sys.exit("poisoning did not take: 6-31GS.BAS is %r"
                             % served)
            print("%s library:" % phase)
            failures += runCases(WHOLE_CASES + COMMON_CASES, True)
            if phase == "clean":
                compared, single, problems = runOracle(driver, e, base)
                failures += report(
                    "whole file == its components, shell for shell and "
                    "dump() for dump() (%d aggregates, %d loaded whole)"
                    % (compared, single), problems)
                failures += report(
                    "a stored calculation's components complete without "
                    "doubling the polarization set",
                    runComplete(driver, e, base))

        #  An older server: same files, empty placeholders.  Poisoned too,
        #  which is the state #164 was reproduced in.
        emptied = 0
        for name in os.listdir(libdir):
            if name.endswith("-AGG.BAS"):
                open(os.path.join(libdir, name), "w").close()
                emptied += 1
        if not emptied:
            sys.exit("no placeholders found in %s" % libdir)
        print("older library (%d placeholders empty), poisoned:" % emptied)
        failures += runCases(COMPOSITE_CASES + COMMON_CASES, False)

        #  Upgrading that server in place: the start script's stamp differs
        #  from the package's, so its sync copies the shipped files over.
        with open(os.path.join(libdir, ".ecce-library-stamp"), "w") as stamp:
            stamp.write("stale\n")
        subprocess.run([os.path.join(DATASERVER, "ecce-dataserver-start")],
                       env=e, capture_output=True, text=True)
        filled = os.path.getsize(os.path.join(libdir, "6-31GS-AGG.BAS"))
        print("after the in-place upgrade (6-31GS-AGG.BAS is %d bytes):"
              % filled)
        failures += report("the sync refilled the placeholders",
                           [] if filled else ["still empty"])
        failures += runCases(WHOLE_CASES + COMMON_CASES, True)
    finally:
        if started:
            subprocess.run([os.path.join(DATASERVER, "ecce-dataserver-stop")],
                           env=e, capture_output=True)
        shutil.rmtree(state, ignore_errors=True)

    print("%s" % ("FAILED (%d)" % failures if failures else "all passed"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
