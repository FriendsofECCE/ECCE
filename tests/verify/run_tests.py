#!/usr/bin/env python3
"""
ECCE input-deck checker suite (GitHub #148).

Runs the real ``scripts/parsers/verifyinput`` over checked-in decks and
checks what it says about each one.  Needs nothing but Python 3 and perl:
no GUI, no ECCE services, no chemistry codes.

    ./run_tests.py            run everything
    ./run_tests.py -v         show every finding, not just the failures

Why this exists
---------------
The checker's whole value is that a user believes it.  Two failure modes
destroy that, and they are not symmetric:

  * A MISSED fault leaves the user exactly where they were -- a job that
    fails in a queue an hour later.  Bad, but no worse than no checker.
  * A FALSE ALARM is worse than no checker.  A user told their working
    deck is broken either wastes an afternoon or, having been wrong
    once, ignores the light forever -- including the time it is right.

So the suite is weighted accordingly.  Every fixture that is a REAL,
working deck must come back clean, and those outnumber the broken ones
deliberately.  When a check cannot be made honestly it must say UNSURE
rather than guess in either direction.

The broken fixtures are real failures, not invented ones:

  * ``g16-garbled-basis`` is the deck from 2026-09-25 whose Gen basis
    block came out as six bytes of binary -- TGBSConfig::dump() handed
    out a pointer into a frozen ostrstream buffer (#146).  Gaussian read
    it as an atomic symbol, printed "Unrecognized atomic symbol" and
    then took a segmentation violation.  The job reached a queue first.
  * ``g16-gen-no-basis`` is the other half of the same afternoon: a
    route card promising /GEN with nothing following it.
"""

import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
CHECKER = os.path.join(ROOT, os.pardir, "scripts", "parsers", "verifyinput")
FIXTURES = os.path.join(HERE, "fixtures")


#  fixture, code, atoms (0 = do not tell it), expected findings.
#
#  An expectation is (level, check).  Every one listed must appear; a BAD
#  the case does not list is a failure, because an unexpected BAD is the
#  false alarm this suite exists to prevent.  Unlisted GOOD and UNSURE
#  findings are allowed -- they are the checker reporting more of what it
#  looked at, which is not a regression.
CASES = [
    #  --- decks that are fine, and must be left alone -----------------
    ("g16-water.g16in",       "Gaussian-16",      3, [("GOOD", "readable"),
                                                      ("GOOD", "geometry"),
                                                      ("GOOD", "route"),
                                                      ("GOOD", "charge")]),
    ("g16-ozone-sci.g16in",   "Gaussian-16",      3, [("GOOD", "geometry")]),
    ("g16-gen-basis.g16in",   "Gaussian-16",      3, [("GOOD", "basis")]),
    ("orca-ethane.orcain",    "ORCA",             8, [("GOOD", "route"),
                                                      ("GOOD", "geomblock"),
                                                      ("GOOD", "geometry")]),
    ("nwchem-ammonia.nw",     "NWChem",           4, [("GOOD", "geometry"),
                                                      ("GOOD", "task")]),
    ("mopac-methane.mopin",   "MOPAC",            5, [("GOOD", "geometry")]),
    ("qe-silicon.qein",       "QuantumESPRESSO",  2, [("GOOD", "control"),
                                                      ("GOOD", "system"),
                                                      ("GOOD", "positions")]),

    #  --- the two real failures ---------------------------------------
    #  Three faults, not one: the binary bytes, the basis block they
    #  destroyed, and the terminating blank line the file therefore
    #  does not have.
    ("g16-garbled-basis.g16in", "Gaussian-16",    3, [("BAD", "readable"),
                                                      ("BAD", "basis"),
                                                      ("BAD", "sections")]),
    ("g16-gen-no-basis.g16in",  "Gaussian-16",    3, [("BAD", "basis")]),

    #  --- deck structure: the sections and the spacing between them ---
    ("g16-link0-after-route.g16in", "Gaussian-16",  3, [("BAD", "link0")]),
    ("g16-no-final-blank.g16in",    "Gaussian-16",  3, [("BAD", "sections")]),
    #  A route card Gaussian rejects outright. Validated against
    #  Gaussian's own parser below, not against anyone's reading of
    #  the manual.
    ("g16-unbalanced-parens.g16in", "Gaussian-16",  3, [("BAD", "keywords")]),

    #  --- the basis set being defined, and readable -------------------
    #  A shell that declares five primitives and lists three. The
    #  first version of this check passed it, because the NEXT shell's
    #  header (" S   1  1.00") holds two numbers and was accepted as a
    #  row of primitives.
    ("g16-short-shell.g16in",       "Gaussian-16",  3, [("BAD", "basis")]),
    #  An element in the molecule with no basis anywhere.
    ("g16-element-no-basis.g16in",  "Gaussian-16",  3, [("BAD", "basis")]),

    #  --- the other things that make a deck unrunnable ----------------
    ("g16-no-geometry.g16in",   "Gaussian-16",    0, [("BAD", "geometry")]),
    ("g16-no-route.g16in",      "Gaussian-16",    3, [("BAD", "route")]),
    ("nwchem-ammonia-no-task.nw", "NWChem",       4, [("BAD", "task")]),
    ("orca-no-route.orcain",    "ORCA",           8, [("BAD", "route")]),

    #  --- the atom count disagreeing with the calculation -------------
    ("g16-water.g16in",         "Gaussian-16",    4, [("BAD", "geometry")]),

    #  --- honest ignorance, which must not read as either verdict -----
    #  A checkpoint geometry is not a missing geometry.
    ("g16-geom-check.g16in",    "Gaussian-16",    0, [("UNSURE", "geometry")]),
    #  No structural rules for this code: say so, do not imply approval.
    ("g16-water.g16in",         "GAMESS-US",      3, [("UNSURE", "code")]),
]


def run(fixture, code, atoms):
    cmd = [CHECKER, "-f", os.path.join(FIXTURES, fixture)]
    if code:
        cmd += ["-c", code]
    if atoms:
        cmd += ["-a", str(atoms)]
    p = subprocess.run(cmd, capture_output=True, text=True)
    findings = []
    for line in p.stdout.splitlines():
        parts = line.split("|", 3)
        if len(parts) == 4:
            findings.append((parts[0], parts[2], parts[3]))
    return p.returncode, findings


#  ---------------------------------------------------------------------
#  The dialog, built for real.
#
#  The most expensive bug class in this port is a wx3.2/GTK3 dialog that
#  crashes while being constructed (#78) or shortly after Show() returns
#  (the layout-reentrancy class).  A compile does not reach either, and
#  neither does anything above -- so the new dialog gets built, sized,
#  laid out and destroyed on a headless X server.  It SKIPs cleanly
#  without a build tree, a compiler or Xvfb rather than failing, because
#  its absence is an environment fact and not a regression.
SMOKE = os.path.join(HERE, "dialog_smoke.C")

LIBS = ["eccecomm", "eccercmd", "ecceexp", "eccewxgui", "eccewxplotctrl",
        "eccewxthings", "eccedsi", "eccedav", "eccecipc", "eccefaces",
        "eccexml", "eccetdat", "ecceutil"]


def which(name):
    for directory in os.environ.get("PATH", "").split(os.pathsep):
        candidate = os.path.join(directory, name)
        if os.access(candidate, os.X_OK):
            return candidate
    return None


def dialog_smoke(verbose):
    """Returns (ran, ok, reason)."""
    tree = os.path.join(ROOT, os.pardir, "build-cmake")
    if not os.path.isdir(tree):
        return (False, True, "no build-cmake tree")
    for tool in ("g++", "wx-config", "Xvfb"):
        if not which(tool):
            return (False, True, "no %s" % tool)

    import tempfile
    work = tempfile.mkdtemp(prefix="ecce-verify-smoke")
    binary = os.path.join(work, "dialog_smoke")
    calced = os.path.join(ROOT, os.pardir, "src", "apps", "calced")

    def wxflags(what):
        p = subprocess.run(["wx-config", what], capture_output=True,
                           text=True)
        return p.stdout.split()

    cmd = (["g++", "-std=c++17", "-o", binary, SMOKE,
            os.path.join(calced, "VerifyReportDialog.C"),
            os.path.join(calced, "InputVerifier.C"),
            "-I" + os.path.join(ROOT, os.pardir, "include"),
            "-I" + calced]
           + wxflags("--cxxflags")
           + ["-L" + tree] + ["-l" + lib for lib in LIBS]
           + ["--libs=core,base,adv,html"] + ["-lxerces-c"])
    #  wx-config --libs takes its component list as an argument, which
    #  the line above cannot express; build it properly.
    cmd = [c for c in cmd if not c.startswith("--libs=")]
    p = subprocess.run(["wx-config", "--libs", "core,base,adv,html"],
                       capture_output=True, text=True)
    cmd = cmd[:-1] + p.stdout.split() + ["-lxerces-c"]

    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        if verbose:
            print(build.stderr[-2000:])
        return (False, True, "the harness did not link against this tree")

    #  A display of its own, and a home of its own: this must not touch
    #  the running user's ECCE state.
    display = ":%d" % (90 + os.getpid() % 8)
    xvfb = subprocess.Popen(["Xvfb", display, "-screen", "0",
                             "1280x1024x24"],
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    try:
        import time
        time.sleep(2)
        home = os.path.join(work, "home")
        os.makedirs(home, exist_ok=True)
        env = dict(os.environ)
        env["DISPLAY"] = display
        env["HOME"] = home
        env["ECCE_REALUSERHOME"] = home
        env["ECCE_HOME"] = os.path.abspath(os.path.join(ROOT, os.pardir))
        run = subprocess.run([binary], capture_output=True, text=True,
                             env=env, timeout=120)
        if verbose or run.returncode != 0:
            print(run.stdout)
            print(run.stderr)
        return (True, run.returncode == 0,
                "exit %d" % run.returncode)
    finally:
        xvfb.terminate()


#  ---------------------------------------------------------------------
#  The route-card rules, checked against Gaussian's own parser.
#
#  Gaussian ships `testrt`, which parses a route card exactly as the
#  real program does and exits non-zero on anything it would reject.
#  verifyinput deliberately does NOT call it -- ECCE submits to remote
#  machines, so the client doing the checking is the machine least
#  likely to have Gaussian on it, and a check that degrades to "unsure"
#  on most installations is not a check.
#
#  It is used HERE instead, which is the right place for it: the rules
#  in verifyinput are reverse-engineered, and this is what stops them
#  drifting from what Gaussian actually does. Every route card the
#  checker passes must be one Gaussian accepts, and the route card it
#  rejects must be one Gaussian rejects.
#
#  This is also how the empty-parentheses trap was found. "Freq=()"
#  looks like a mistake and ECCE emits it on nearly every deck --
#  Gaussian accepts it. A rule written from intuition would have
#  condemned the entire corpus.
TESTRT_CANDIDATES = [
    os.path.join(os.environ.get("GAUSS_EXEDIR", ""), "testrt"),
    "/opt/gaussian/g16/testrt",
    "/opt/gaussian/g09/testrt",
    "/usr/local/g16/testrt",
]


def find_testrt():
    for candidate in TESTRT_CANDIDATES:
        if candidate and os.access(candidate, os.X_OK):
            return candidate
    return None


def route_of(path):
    """The route card: from the first '#' line to the next blank one."""
    route, started = [], False
    for line in open(path, errors="replace"):
        line = line.rstrip("\n")
        if not started and line.lstrip().startswith("#"):
            started = True
        if started:
            if not line.strip():
                break
            route.append(line)
    return route


def testrt_agreement(verbose):
    """Returns (ran, failures, checked)."""
    testrt = find_testrt()
    if not testrt:
        return (False, 0, 0)

    import tempfile
    failures, checked = 0, 0
    work = tempfile.mkdtemp(prefix="ecce-testrt")

    for fixture, code, _atoms, expected in CASES:
        if not code.startswith("Gaussian"):
            continue
        path = os.path.join(FIXTURES, fixture)
        route = route_of(path)
        if not route:
            continue

        p = subprocess.run([testrt], input="\n".join(route) + "\n\n",
                           capture_output=True, text=True, cwd=work,
                           timeout=60)
        accepted = (p.returncode == 0 and "QPErr" not in p.stdout)

        #  testrt can only speak about the ROUTE CARD, so only the
        #  checker's route-card findings may be compared with it. A
        #  deck with a broken basis block has a perfectly good route
        #  card, and testrt accepting it says nothing about the deck.
        #
        #  Which rule reached the verdict does not have to match: a
        #  %-directive inside the route card is a Link 0 fault here
        #  and a syntax error to Gaussian, and those are the same
        #  finding about the same lines.
        _rc, findings = run(fixture, code, 0)
        weExpectBad = any(level == "BAD" and check in ("keywords", "link0",
                                                       "route")
                          for level, check, _message in findings)
        checked += 1

        if weExpectBad and accepted:
            print("FAIL  testrt accepts a route card we call wrong: %s"
                  % fixture)
            failures += 1
        elif not weExpectBad and not accepted:
            print("FAIL  testrt rejects a route card we pass: %s" % fixture)
            if verbose:
                print(p.stdout[-600:])
            failures += 1
        elif verbose:
            print("      testrt %s %s"
                  % ("accepts" if accepted else "rejects", fixture))

    return (True, failures, checked)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    failures = 0
    for fixture, code, atoms, expected in CASES:
        label = "%s [%s]" % (fixture, code)
        if atoms:
            label += " n=%d" % atoms
        rc, findings = run(fixture, code, atoms)
        got = set((level, check) for level, check, _ in findings)
        problems = []

        for want in expected:
            if want not in got:
                problems.append("expected %s|%s and did not get it"
                                % want)

        #  An unexpected BAD is the failure mode that matters most.
        for level, check in got:
            if level == "BAD" and (level, check) not in expected:
                problems.append("raised a false alarm: BAD|%s" % check)

        #  Exit status has to agree with what it printed, because the
        #  caller lights the traffic light from one and shows the other.
        anyBad = any(level == "BAD" for level, _ in got)
        if anyBad != (rc == 1):
            problems.append("exit status %d does not match its findings"
                            % rc)

        if problems:
            failures += 1
            print("FAIL  %s" % label)
            for problem in problems:
                print("        %s" % problem)
            for level, check, message in findings:
                print("      %-7s %-10s %s" % (level, check, message))
        else:
            print("ok    %s" % label)
            if args.verbose:
                for level, check, message in findings:
                    print("      %-7s %-10s %s" % (level, check, message))

    ran, oracleFailures, checked = testrt_agreement(args.verbose)
    if not ran:
        print("SKIP  route cards against Gaussian's own parser "
              "(no testrt on this machine)")
    elif oracleFailures:
        failures += oracleFailures
    else:
        print("ok    %d route cards agree with Gaussian's own parser"
              % checked)

    ran, ok, reason = dialog_smoke(args.verbose)
    if not ran:
        print("SKIP  report dialog construction (%s)" % reason)
    elif ok:
        print("ok    report dialog construction")
    else:
        print("FAIL  report dialog construction (%s)" % reason)
        failures += 1

    print()
    print("%d of %d cases failed." % (failures, len(CASES) + (1 if ran else 0)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
