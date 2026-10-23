#!/usr/bin/env python3
"""
Capture (or check) the MO-diagram golden dumps in tests/modiagram/expected/.

These are the byte-identity oracle for a later refactor that moves the MO
diagram engine (MoDiagramPanel::buildOnce() and friends) out of wx and into
a plain library -- see the ECCE_OPEN_PANEL/ECCE_MODIAGRAM_DUMP/
ECCE_EXIT_AFTER_DUMP hooks added to Builder.C and MoDiagramPanel.C for #171.
The refactor is trusted only as far as it reproduces these dumps verbatim
against the same fixtures.

Usage:
    ECCE_TEST_HOME=/path/to/an/install python3 capture.py [--update] [name ...]

With no names, every fixture below is run.  Without --update, a captured
dump is compared against tests/modiagram/expected/<name>.dump and any
difference is reported; with --update, that file is (over)written.

Reuses tests/apps' isolate/xdisplay/apps/fixture helpers exactly as
run_tests.py does -- this is one more caller of the same "bring up an
isolated gateway + data server, launch one app headlessly" machinery, not
a separate suite with its own service-startup code (see tests/apps/README).
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
TESTS_APPS = os.path.join(os.path.dirname(HERE), "apps")
sys.path.insert(0, TESTS_APPS)

import apps          # noqa: E402
import fixture        # noqa: E402
import isolate         # noqa: E402
import xdisplay          # noqa: E402

FIXTURES_DIR = os.path.join(HERE, "fixtures")
EXPECTED_DIR = os.path.join(HERE, "expected")

# name -> source directory.  calc-water-vib is tests/apps' own fixture
# (C2v water, VIB+MO+GEOMTRACE); the rest are tests/modiagram's own,
# trimmed to just what MoDiagramPanel::build() reads -- see
# tests/modiagram/README.
FIXTURES = {
    "calc-water-vib": os.path.join(os.path.dirname(HERE), "apps",
                                   "fixtures", "calc-water-vib"),
    "orca-co": os.path.join(FIXTURES_DIR, "orca-co"),
    "gaussian-water": os.path.join(FIXTURES_DIR, "gaussian-water"),
    "gaussian-co": os.path.join(FIXTURES_DIR, "gaussian-co"),
    "orca-crco6": os.path.join(FIXTURES_DIR, "orca-crco6"),
    #  The same calculation drawn by the SKELETON construction, which was
    #  the metal default before the fragment-orbital diagram (#301).
    "orca-crco6-skeleton": os.path.join(FIXTURES_DIR, "orca-crco6"),
    "orca-crco6-lfview": os.path.join(FIXTURES_DIR, "orca-crco6"),
    "nwchem-water": os.path.join(FIXTURES_DIR, "nwchem-water"),
    #  #132: unsymmetrised input, symmetry on in the deck (makecalc.py).
    "orca-nh3-usesym": os.path.join(FIXTURES_DIR, "orca-nh3-usesym"),
    "gaussian-water-cs": os.path.join(FIXTURES_DIR, "gaussian-water-cs"),
    "orca-co2-c2v": os.path.join(FIXTURES_DIR, "orca-co2-c2v"),
    "orca-nh3-c1": os.path.join(FIXTURES_DIR, "orca-nh3-c1"),
    #  #132: ECCE-QM runs (makecalc.py ecceqm), which print no orbital
    #  symmetry labels and keep the input frame.
    "ecceqm-c2": os.path.join(FIXTURES_DIR, "ecceqm-c2"),
    "ecceqm-he2": os.path.join(FIXTURES_DIR, "ecceqm-he2"),
    "ecceqm-n2": os.path.join(FIXTURES_DIR, "ecceqm-n2"),
    "ecceqm-co-rotated": os.path.join(FIXTURES_DIR, "ecceqm-co-rotated"),
    "ecceqm-water": os.path.join(FIXTURES_DIR, "ecceqm-water"),
    #  #301: ethene and cyclopropane (makecalc.py ecceqm c2h4 / c3h6 0
    #  --no-rotate), and the atom picks that choose a diagram.
    "ecceqm-c2h4": os.path.join(FIXTURES_DIR, "ecceqm-c2h4"),
    #  #458: staggered ethane, RHF/6-31G* (makecalc.py ecceqm c2h6 0 DIR
    #  --no-rotate --method hf), CH3 + CH3.
    "ecceqm-c2h6": os.path.join(FIXTURES_DIR, "ecceqm-c2h6"),
    "ecceqm-c2h4-pick-c1c2": os.path.join(FIXTURES_DIR, "ecceqm-c2h4"),
    "ecceqm-c3h6-pick-ring": os.path.join(FIXTURES_DIR, "ecceqm-c3h6"),
    "ecceqm-water-pick-o1": os.path.join(FIXTURES_DIR, "ecceqm-water"),
    "orca-crco6-pick-cr1": os.path.join(FIXTURES_DIR, "orca-crco6"),
    #  #301: a non-metal pick that the one-atom CENTRAL cannot draw cleanly
    #  goes to the fragment-orbital route (makecalc.py ecceqm ch3oh 0
    #  --no-rotate): the methyl carbon, and the hydroxyl oxygen.
    "ecceqm-methanol-pick-c1": os.path.join(FIXTURES_DIR, "ecceqm-methanol"),
    "ecceqm-methanol-pick-o2": os.path.join(FIXTURES_DIR, "ecceqm-methanol"),
    #  #301 validation set (ORCA BP86/def2-SVP, idealised geometries, no
    #  UseSym, so the labels are the diagram's own).  The open-shell and
    #  charged ones carry their multiplicity and charge in .davprops.
    "orca-bf3": os.path.join(FIXTURES_DIR, "orca-bf3"),
    "orca-benzene": os.path.join(FIXTURES_DIR, "orca-benzene"),
    "orca-benzene-pi": os.path.join(FIXTURES_DIR, "orca-benzene"),
    "ecceqm-c2h4-pi": os.path.join(FIXTURES_DIR, "ecceqm-c2h4"),
    "orca-o2-triplet": os.path.join(FIXTURES_DIR, "orca-o2-triplet"),
    "orca-no2": os.path.join(FIXTURES_DIR, "orca-no2"),
    "orca-b2h6": os.path.join(FIXTURES_DIR, "orca-b2h6"),
    "orca-nico4": os.path.join(FIXTURES_DIR, "orca-nico4"),
    "orca-cocl4-quartet": os.path.join(FIXTURES_DIR, "orca-cocl4-quartet"),
    #  A bond pick that breaks the molecule's symmetry: drawn and labelled in
    #  the subgroup that keeps the two sides (Td -> C3v, Oh -> C4v).
    "orca-cocl4-pick-cocl3": os.path.join(FIXTURES_DIR, "orca-cocl4-quartet"),
    "orca-crco6-pick-crc": os.path.join(FIXTURES_DIR, "orca-crco6"),
    "orca-ptcl4-overlaps": os.path.join(FIXTURES_DIR, "orca-ptcl4"),
    "orca-bf3-overlaps": os.path.join(FIXTURES_DIR, "orca-bf3"),
    "orca-crco6-overlaps": os.path.join(FIXTURES_DIR, "orca-crco6"),
    "orca-benzene-overlaps": os.path.join(FIXTURES_DIR, "orca-benzene"),
    #  Ligand-set phase sketches (#368): Oh and Td, with every level drawn.
    "orca-crco6-all": os.path.join(FIXTURES_DIR, "orca-crco6"),
    "orca-cocl4-overlaps": os.path.join(FIXTURES_DIR, "orca-cocl4-quartet"),
    "orca-nico4-overlaps": os.path.join(FIXTURES_DIR, "orca-nico4"),
    "orca-cocl4-compute": os.path.join(FIXTURES_DIR, "orca-cocl4-quartet"),
    "orca-benzene-compute-ring": os.path.join(FIXTURES_DIR, "orca-benzene"),
    "orca-agnh3": os.path.join(FIXTURES_DIR, "orca-agnh3"),
    #  [PtCl4]2-, d8 square planar: the metal d electrons come from the MOs.
    "orca-ptcl4": os.path.join(FIXTURES_DIR, "orca-ptcl4"),
}


#  Extra environment per fixture: a construction asked for by name.
FIXTURE_ENV = {
    "orca-crco6-skeleton": {"ECCE_MODIAGRAM_CONSTRUCTION": "skeleton"},
    "orca-crco6-lfview": {"ECCE_MODIAGRAM_LFVIEW": "1"},
    "orca-crco6-all": {"ECCE_MODIAGRAM_SHOWALL": "1"},
    #  ECCE_MODIAGRAM_PICK: the atom (or "A,B" bond) the user picked.
    "ecceqm-c2h4-pick-c1c2": {"ECCE_MODIAGRAM_PICK": "C1,C2"},
    "ecceqm-c3h6-pick-ring": {"ECCE_MODIAGRAM_PICK": "C1,C2"},
    "ecceqm-water-pick-o1": {"ECCE_MODIAGRAM_PICK": "O1"},
    "orca-crco6-pick-cr1": {"ECCE_MODIAGRAM_PICK": "Cr1"},
    "orca-cocl4-pick-cocl3": {"ECCE_MODIAGRAM_PICK": "Co1,Cl2"},
    "orca-crco6-pick-crc": {"ECCE_MODIAGRAM_PICK": "Cr1,C2"},
    "ecceqm-methanol-pick-c1": {"ECCE_MODIAGRAM_PICK": "C1"},
    "ecceqm-methanol-pick-o2": {"ECCE_MODIAGRAM_PICK": "O2"},
    #  ECCE_MODIAGRAM_PIONLY: the "pi system only" box ticked.
    "orca-benzene-pi": {"ECCE_MODIAGRAM_PIONLY": "1"},
    "ecceqm-c2h4-pi": {"ECCE_MODIAGRAM_PIONLY": "1"},
}


#  Scripted checks run in the Builder after the diagram is up (the scene
#  commands of Builder.C): name -> (scene text, {output file: regex it must
#  match}).  Scene fixtures share a source directory with a plain one.
FIXTURE_SCENE = {
    #  No two labels or arrows of the model column on top of each other.
    "orca-ptcl4-overlaps": ("hold 2\nmodiagram overlaps overlaps 0\n",
                            {"overlaps": r"overlaps 0\b"}),
    #  Model and calculation columns: one lane, no overlapping rows, and the
    #  0 eV tick between the negative and the positive model levels.
    "orca-bf3-overlaps": ("hold 2\nmodiagram overlaps overlaps 0\n",
                          {"overlaps": r"overlaps 0\b"}),
    "orca-crco6-overlaps": ("hold 2\nmodiagram overlaps overlaps 0\n",
                            {"overlaps": r"overlaps 0\b"}),
    "orca-cocl4-overlaps": ("hold 2\nmodiagram overlaps overlaps 0\n",
                            {"overlaps": r"overlaps 0\b"}),
    "orca-nico4-overlaps": ("hold 2\nmodiagram overlaps overlaps 0\n",
                            {"overlaps": r"overlaps 0\b"}),
    "orca-benzene-overlaps": ("hold 2\nmodiagram overlaps overlaps 0\n",
                              {"overlaps": r"overlaps 0\b"}),
    #  The live Compute button: pick a bond, press it, and the diagram is
    #  redrawn in the subgroup that keeps the two sides.
    "orca-cocl4-compute": ("hold 2\nmodipick pick Co1 Cl2\n",
                           {"pick": r"labels in C.*3.*lowered from"}),
    #  A bond that cannot be cut (a ring bond) is said on the panel.
    "orca-benzene-compute-ring": ("hold 2\nmodipick pick C1 C2\n",
                                  {"pick": r"part of a ring"}),
}


def setProperties(url, props):
    """Store ecce: properties over WebDAV: {relative path: {name: value}}.
    The fixtures' stored metadata is a binary property database made from
    an ORCA template, so a calculation of another code is told its code
    and basis kind here instead."""
    import base64
    import urllib.request
    user = fixture.realUser()
    token = base64.b64encode(("%s:%s" % (
        user, fixture.passwordFor(user))).encode()).decode()
    for path, values in props.items():
        sets = "".join("<E:%s>%s</E:%s>" % (k, v, k) for k, v in values.items())
        body = ('<?xml version="1.0"?><D:propertyupdate xmlns:D="DAV:" '
                'xmlns:E="http://www.emsl.pnl.gov/ecce:"><D:set><D:prop>%s'
                '</D:prop></D:set></D:propertyupdate>' % sets).encode()
        target = url + "/" + path if path else url + "/"
        request = urllib.request.Request(target, data=body, method="PROPPATCH")
        request.add_header("Authorization", "Basic " + token)
        request.add_header("Content-Type", "text/xml")
        urllib.request.urlopen(request, timeout=30).read()


def captureOne(name, source, outfile, timeout, png=None):
    """Run builder headlessly against `source`, dump to `outfile`.

    Returns an error string, or None on success.  Brings its own isolated
    gateway/data server/X display up and down -- callers that capture
    several fixtures in one process still get a fresh service pair per
    fixture, which costs ~10s each but keeps one fixture's failure from
    wedging the rest.

    @param png  optional path: also screenshot the "ECCE Builder" window
                (the MO Diagram panel open inside it) before the app
                closes.  Uses ECCE_EXIT_AFTER_DUMP_DELAY_MS to hold the
                window open a few seconds after the dump is written --
                see tools/screenshots/capture.py for the same "import
                -window <id>" technique on this same private display.
    """
    #  Each call picks its own free ports -- isolate.apply() treats an
    #  already-set ECCE_DATASERVER_PORT/ECCE_BROKER_PORT as an explicit
    #  request and insists on reusing exactly that port, which races the
    #  previous call's own teardown when this runs several fixtures (or
    #  several repeats of one) in the same process.
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"

    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        after = apps.serviceState(display)
        if not all(after.values()):
            apps.stopServices(display)
            return "services did not come up: %s (%s)" % (after, log)

        #  RunMgmt/processmachine dies without ~/.ECCE/Queues -- normally
        #  created the first time a user does Tools > Register Machines.
        queuesPath = os.path.join(state, ".ECCE", "Queues")
        os.makedirs(os.path.dirname(queuesPath), exist_ok=True)
        if not os.path.exists(queuesPath):
            open(queuesPath, "w").close()

        fixture.ensureRealUserAccount()
        restorePrefs = fixture.settleUpgradeNotices()
        url, error = fixture.install(name, source_dir=source)
        if error:
            apps.stopServices(display)
            restorePrefs()
            return "fixture install failed: %s" % error

        marker = os.path.join(source, ".davprops")
        if os.path.exists(marker):
            import json
            try:
                setProperties(url, json.load(open(marker)))
            except Exception as err:
                apps.stopServices(display)
                restorePrefs()
                return "could not set the stored properties: %s" % err

        #  ECCE-QM's MO diagram is off by default (Preferences).
        prefs = os.path.join(state, ".ECCE", "EcceGlobal")
        os.makedirs(os.path.dirname(prefs), exist_ok=True)
        if not os.path.exists(prefs) or "EcceQmMoDiagram" not in open(prefs).read():
            with open(prefs, "a") as handle:
                handle.write("EcceQmMoDiagram:\ttrue\n")

        if os.path.exists(outfile):
            os.remove(outfile)
        os.makedirs(os.path.dirname(outfile), exist_ok=True)

        env = display.env()
        env["ECCE_OPEN_PANEL"] = "MO Diagram"
        env["ECCE_MODIAGRAM_DUMP"] = outfile
        env.update(FIXTURE_ENV.get(name, {}))
        sceneOut = None
        if name in FIXTURE_SCENE:
            #  The scene runs after the first build and ends the app itself.
            sceneOut = tempfile.mkdtemp(prefix=name + "-scene-",
                                        dir=os.path.dirname(state))
            with open(os.path.join(sceneOut, "script"), "w") as handle:
                handle.write(FIXTURE_SCENE[name][0])
            env["ECCE_VIEWER_SCENE"] = os.path.join(sceneOut, "script")
            env["ECCE_VIEWER_SCENE_OUT"] = sceneOut
        else:
            env["ECCE_EXIT_AFTER_DUMP"] = "1"
        if png:
            #  Long enough to find the window, let it settle and shoot it;
            #  short enough not to make a --png run noticeably slower.
            env["ECCE_EXIT_AFTER_DUMP_DELAY_MS"] = "6000"

        #  -pipe avoids the "ECCE Authentication" modal, which otherwise
        #  blocks the event loop before the calc ever loads.
        authPath = os.path.join(state, "auth.pipe")
        fixture.authFile(authPath, port=int(settings["ECCE_DATASERVER_PORT"]))

        command = [os.path.join(apps.WRAPPERS, "ecce-builder"),
                  "-pipe", authPath, "-context", url]
        proc = subprocess.Popen(command, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                start_new_session=True)
        deadline = time.time() + timeout
        shot = False
        while time.time() < deadline:
            if os.path.exists(outfile) and os.path.getsize(outfile) > 0:
                if png and not shot:
                    #  Give the window a moment to paint the diagram
                    #  (the dump is written from inside the same build()
                    #  call that fills the canvas, but the paint itself
                    #  is a separate, later event) before shooting it --
                    #  well inside the delay above, which is what keeps
                    #  the app from closing under us.
                    time.sleep(2)
                    #  The MO Diagram panel opens as its own top-level
                    #  window (an AUI floating pane, not a tab inside the
                    #  Builder/Viewer frame) -- confirmed against a live
                    #  window listing, which is why this is not "ECCE
                    #  Builder"/"ECCE Viewer" as in tools/screenshots.
                    wid = None
                    for w, title in display.windows():
                        if title == "MO Diagram":
                            wid = w
                            break
                    if wid:
                        subprocess.run(["import", "-display", display.name,
                                       "-window", wid, png], env=display.env(),
                                       check=False)
                    shot = True
                if not png:
                    break
            if proc.poll() is not None:
                break
            time.sleep(0.5)

        try:
            proc.wait(timeout=60 if sceneOut else 15)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), 15)
                proc.wait(timeout=10)
            except Exception:
                try:
                    os.killpg(os.getpgid(proc.pid), 9)
                except Exception:
                    pass

        out = b""
        try:
            out = proc.stdout.read() or b""
        except Exception:
            pass

        apps.stopServices(display)
        restorePrefs()

        if sceneOut:
            import re
            problem = None
            for fileName, pattern in FIXTURE_SCENE[name][1].items():
                path = os.path.join(sceneOut, fileName + ".txt")
                text = open(path).read() if os.path.exists(path) else ""
                if not re.search(pattern, text):
                    failed = os.path.join(sceneOut, "FAILED")
                    problem = ("scene check %s: %r does not match %r%s"
                               % (fileName, text[:300], pattern,
                                  (" (" + open(failed).read() + ")")
                                  if os.path.exists(failed) else ""))
                    break
            shutil.rmtree(sceneOut, ignore_errors=True)
            if problem:
                return problem

        if not os.path.exists(outfile):
            tail = out.decode("utf-8", "replace")[-4000:]
            return "no dump written; builder's last output:\n%s" % tail
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("names", nargs="*", help="fixture names to run "
                        "(default: all)")
    parser.add_argument("--update", action="store_true",
                        help="write tests/modiagram/expected/<name>.dump "
                             "instead of comparing against it")
    parser.add_argument("--repeat", type=int, default=1,
                        help="capture each fixture N times and require "
                             "byte-identical dumps (determinism check)")
    parser.add_argument("--timeout", type=float, default=90)
    parser.add_argument("--out", metavar="DIR",
                        help="write each dump to DIR/<name>.dump and compare "
                             "against nothing (for exploring a fixture that "
                             "has no golden yet)")
    parser.add_argument("--png", metavar="DIR",
                        help="also screenshot the ECCE Builder window "
                             "(MO Diagram panel open) for each captured "
                             "fixture, as DIR/<name>.png")
    args = parser.parse_args()
    if args.png:
        os.makedirs(args.png, exist_ok=True)

    names = args.names or list(FIXTURES)
    #  A path to a calculation directory runs as its own fixture.
    for n in list(names):
        if n not in FIXTURES and os.path.isdir(n):
            FIXTURES[os.path.basename(os.path.normpath(n))] = os.path.abspath(n)
            names[names.index(n)] = os.path.basename(os.path.normpath(n))
    unknown = [n for n in names if n not in FIXTURES]
    if unknown:
        print("unknown fixture(s): %s" % ", ".join(unknown))
        return 2

    failures = []
    for name in names:
        source = FIXTURES[name]
        if not os.path.isdir(source):
            failures.append("%s: no such fixture directory %s"
                            % (name, source))
            continue

        dumps = []
        ok = True
        for run in range(max(1, args.repeat)):
            scratch = os.path.join("/tmp", "modiagram-capture-%s-%d.dump"
                                   % (name, run))
            png = os.path.join(args.png, name + ".png") if args.png else None
            error = captureOne(name, source, scratch, args.timeout, png=png)
            if error:
                failures.append("%s: %s" % (name, error))
                ok = False
                break
            dumps.append(open(scratch, "rb").read())
            os.remove(scratch)
        if not ok:
            continue

        for i in range(1, len(dumps)):
            if dumps[i] != dumps[0]:
                failures.append("%s: dump differs between run 1 and run %d "
                                "-- not deterministic" % (name, i + 1))
                ok = False
                break
        if not ok:
            continue

        expected = os.path.join(EXPECTED_DIR, "%s.dump" % name)
        if args.out:
            os.makedirs(args.out, exist_ok=True)
            open(os.path.join(args.out, name + ".dump"), "wb").write(dumps[0])
            print("%s: wrote %s" % (name, os.path.join(args.out, name + ".dump")))
        elif args.update:
            os.makedirs(EXPECTED_DIR, exist_ok=True)
            open(expected, "wb").write(dumps[0])
            print("%s: wrote %s (%d bytes)"
                 % (name, expected, len(dumps[0])))
        elif not os.path.exists(expected):
            failures.append("%s: no golden at %s (run with --update first)"
                            % (name, expected))
        elif open(expected, "rb").read() != dumps[0]:
            failures.append("%s: dump differs from %s" % (name, expected))
        else:
            print("%s: matches golden (%d bytes)" % (name, len(dumps[0])))

    if failures:
        print("\nFAILURES:")
        for f in failures:
            print(" ", f)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
