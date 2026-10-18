#!/usr/bin/env python3
"""A GROMACS Prepare task, then the tasks after it, through the real launch path.

Local data, no data server.  The tree's own binaries, never the real ~/.ECCE:

  * a GROMACS MD study with a Prepare and an Optimize task is made through
    Resource::createChild and Session::addMemberAsTarget (launchjob
    gromacsprepstudy); the Prepare's source structure is attached as its
    .gmxpdb and its recipe written as the editor will (the task model XML
    through md.gmxprep);
  * Prepare (pdb2gmx, editconf, solvate, genion) runs through the real
    Launch, gensub, eccejobmaster, eccejobstore and eccejobmonitor and ends
    Complete with the topology (.gmxtop), the structure (.gro), the
    restraints (posre.itp) and chemsys.pdb stored as its outputs;
  * Optimize then launches with nothing attached, finds the topology, the
    structure and posre.itp on the Prepare, and completes;
  * a Prepare that fails in its job (an ion name genion does not know) ends
    Failed with gmx's own words as the reason, and a PDB with a side chain cut
    short is reported at launch with pdb2gmx's message (Cancel stops it);
  * the Organizer's New menu (its test hook) makes the Prepare with the
    right application type;
  * the editor (gmxprepare, private Xvfb, ECCE_TEST_MDED): a Prepare as New
    makes it is opened, the force field, water model, salt concentration and
    source PDB are set as a person would, Save stores the recipe and the
    model, the water models follow the force field, and a Launch of that
    task ends Complete; the Organizer names GMXPrepare for a GROMACS Prepare
    and MDPrepare for an NWChem one.

    tests/launch/gromacs_prepare_test.py [--build build-cmake] [--keep] [--png DIR]

Exit status 77 (CTest SKIP) without gmx or the built binaries;
ECCE_GROMACS_REQUIRE=1 turns the missing gmx into a failure.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

FIXTURES = os.path.join(REPO, "tests", "gromacs_md", "fixtures")
NA_UNK = os.path.join(FIXTURES, "na_unk.pdb")

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def outputs(task_url):
    d = os.path.join(task_url.replace("file://", "").rstrip("/"), "Outputs")
    return sorted(os.listdir(d)) if os.path.isdir(d) else []


def last_run_file(rundir, name):
    found = sorted(glob.glob(os.path.join(rundir, "**", name), recursive=True),
                   key=os.path.getmtime)
    return found[-1] if found else None


def urls_of(out):
    return [l.rstrip("/") + "/" for l in out.splitlines() if l.startswith("file://")]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--png", help="where the editor's screenshot goes "
                    "(default: the test's state directory, with --keep)")
    ap.add_argument("--minff-only", action="store_true",
                    help="run only the MINFF section")
    ap.add_argument("--editor-only", action="store_true",
                    help="run only the editor section")
    ap.add_argument("--builder-save-only", action="store_true",
                    help="run only the Builder-save section")
    ap.add_argument("--builder-flow-only", action="store_true",
                    help="run only the Builder (Na ion) section")
    ap.add_argument("--organizer-only", action="store_true",
                    help="run only the Organizer section")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    gmx = shutil.which("gmx")
    if not gmx:
        if os.environ.get("ECCE_GROMACS_REQUIRE"):
            say("FAIL: gmx is not installed and ECCE_GROMACS_REQUIRE is set")
            return 1
        harness.skip("gmx is not installed")
    harness.prerequisites(build, ())

    s = harness.Session(build, "gmxprep", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
    s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
    rundir = os.path.join(s.state, "run")
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        if args.minff_only:
            minff_section(s, build, rundir, args.png or os.path.join(s.state, "png"))
            failures.extend(gromacs_test_failures())
            return 1 if failures else 0

        if args.builder_save_only:
            builder_save_section(s, rundir)
            return 1 if failures else 0

        if args.builder_flow_only:
            builder_flow_section(s, build, rundir)
            return 1 if failures else 0

        if args.organizer_only:
            organizer_new(s, build, gmx)
            return 1 if failures else 0

        if args.editor_only:
            editor_section(s, build, rundir, args.png or os.path.join(s.state, "png"))
            failures.extend(gromacs_test_failures())
            return 1 if failures else 0

        # ---- Prepare then Optimize --------------------------------------
        rc, out = s.driver("gromacsprepstudy", s.userUrl(), "trpcage",
                           os.path.join(FIXTURES, "1l2y.pdb"),
                           "ff=amber99sb-ildn", "water=tip3p", "conc=0.15")
        urls = urls_of(out)
        if not check(rc == 0 and len(urls) == 3,
                     "a study with a Prepare and an Optimize was made: %s"
                     % out.strip().replace("\n", " | ")[:300]):
            return 1
        study, prep, opt = urls
        for u in (prep, opt):
            rc, out = s.driver("mdsetup", u, "localhost", rundir, s.user())
            check(rc == 0, "launch settings stored for %s"
                  % u.rstrip("/").rsplit("/", 1)[-1])
        rc, out = s.driver("apptype", prep)
        check(out.split()[:2] == ["GROMACS", "ecceMdPrepare"],
              "the Prepare is a GROMACS ecceMdPrepare: %s" % out.strip())
        ptask = prep.replace("file://", "").rstrip("/")
        inputs = sorted(os.listdir(os.path.join(ptask, "Inputs")))
        say("  Prepare Inputs/: %s" % inputs)
        check("gromacs.gmxprep" in inputs and "1l2y.gmxpdb" in inputs,
              "the recipe and the source structure are stored as inputs")
        recipe = open(os.path.join(ptask, "Inputs", "gromacs.gmxprep")).read()
        check("FF=amber99sb-ildn" in recipe and "CONC=0.15" in recipe
              and "HIS=auto" in recipe,
              "md.gmxprep turned the task model into the recipe")

        say("running Prepare")
        rc, out = s.launch(prep)
        check(rc == 0, "Launch accepted the Prepare: %s"
              % out.strip().splitlines()[-3:])
        state = s.waitState(prep, 600)
        check(state in ("completed", "loaded"), "Prepare ran to %s" % state)
        outs = outputs(prep)
        say("  Prepare Outputs/: %s" % outs)
        check("ecceSession_ecceMdPrepare.gro" in outs or
              any(o.endswith(".gro") for o in outs),
              "the structure (.gro) was stored")
        check(any(o.endswith(".gmxtop") for o in outs),
              "the topology (.gmxtop) was stored")
        check("posre.itp" in outs, "posre.itp was stored")
        check("chemsys.pdb" in outs, "chemsys.pdb was stored")
        top = [o for o in outs if o.endswith(".gmxtop")]
        if top:
            text = open(os.path.join(ptask, "Outputs", top[0])).read()
            check("[ molecules ]" in text and "SOL" in text and "Protein" in text,
                  "the topology lists the protein and the solvent")

        rc, stored = s.driver("progress", prep)
        say("  stored progress: %s" % stored.strip().replace("\n", " | ")[:400])
        check(re.search(r"^prepare point=4/4 .*tool=genion", stored, re.M)
              and "done=yes" in stored and re.search(r"atoms=\d+", stored)
              and "box=dodecahedron" in stored and "pos=" in stored,
              "the stored progress has the Prepare's step 4/4 and its result")
        props = s.props(prep)
        say("  Prepare properties: %s" % props)
        want = ["NATOMS", "NRESIDUES", "NWATERS", "NIONSPOS", "NIONSNEG",
                "SYSCHARGE0", "SYSCHARGE", "BOXA", "BOXB", "BOXC", "BOXALPHA",
                "BOXBETA", "BOXGAMMA"]
        check(all(k in props for k in want),
              "the Prepare stored its system as properties: missing %s"
              % [k for k in want if k not in props])
        logf = [o for o in outs if o.endswith(".gmxlog")]
        if logf:
            log = open(os.path.join(ptask, "Outputs", logf[0])).read()
            # the job log is appended to the output, so the steps appear twice
            check(set(re.findall(r"^ECCE-PREPARE step (\d)/4 ", log, re.M))
                  == {"1", "2", "3", "4"} and "ECCE-PREPARE done" in log,
                  "the output file announces four steps and the end")

        say("running Optimize with nothing attached")
        otask = opt.replace("file://", "").rstrip("/")
        check(not os.path.isdir(os.path.join(otask, "Inputs")) or
              not [f for f in os.listdir(os.path.join(otask, "Inputs"))
                   if f.endswith((".gmxtop", ".gro", ".itp"))],
              "the Optimize has no topology, structure or include attached")
        rc, out = s.launch(opt)
        check(rc == 0, "Launch accepted the Optimize: %s"
              % out.strip().splitlines()[-3:])
        state = s.waitState(opt, 900)
        check(state in ("completed", "loaded"), "Optimize ran to %s" % state)
        r = last_run_file(rundir, "topol.top")
        d = os.path.dirname(r) if r else None
        if d and os.path.exists(os.path.join(d, "conf.gro")):
            check(os.path.exists(os.path.join(d, "posre.itp")),
                  "posre.itp was staged in the Optimize's run directory")
            conf = os.path.join(d, "conf.gro")
            prepgro = [o for o in outs if o.endswith(".gro")]
            check(prepgro and open(conf).read() == open(
                os.path.join(ptask, "Outputs", prepgro[0])).read(),
                "the Optimize started from the Prepare's structure")
        else:
            check(False, "the Optimize's run directory has topol.top and conf.gro")
        props = s.props(opt)
        check("TE" in props, "the Optimize stored its energies: %s" % props)

        # ---- failures ----------------------------------------------------
        # more salt than the box has water for: genion, the last step, refuses it
        rc, out = s.driver("gromacsprepstudy", s.userUrl(), "saltbox",
                           os.path.join(FIXTURES, "1l2y.pdb"), "conc=300")
        bad = urls_of(out)
        if check(rc == 0 and len(bad) == 3, "a second study was made"):
            s.driver("mdsetup", bad[1], "localhost", rundir, s.user())
            rc, out = s.launch(bad[1])
            check(rc == 0, "Launch accepted it (pdb2gmx is fine with this PDB)")
            state = s.waitState(bad[1], 300)
            check(state in ("unsuccessful", "failed"),
                  "a Prepare whose last step fails ends %s" % state)
            rc, out = s.driver("reason", bad[1])
            reason = out.strip().splitlines()[0] if out.strip() else ""
            say("  reason: %s" % reason[:300])
            check("Fatal error" in reason or "solvent" in reason.lower(),
                  "the reason is gmx's own words, not a bare 'failed'")

        rc, out = s.driver("gromacsprepstudy", s.userUrl(), "broken",
                           os.path.join(FIXTURES, "1l2y_broken.pdb"))
        brk = urls_of(out)
        if check(rc == 0 and len(brk) == 3, "a study on a broken PDB was made"):
            s.driver("mdsetup", brk[1], "localhost", rundir, s.user())
            rc, out = s.driver("launch", brk[1], extra={"LAUNCHJOB_WARNING": "cancel"})
            check(rc != 0 and ("missing" in out and "TRP" in out),
                  "a side chain cut short is reported at launch with pdb2gmx's "
                  "message: %s" % re.sub(r"\s+", " ", out.strip())[-200:])

        builder_save_section(s, rundir)

        # ---- the Organizer's New menu -----------------------------------
        organizer_new(s, build, gmx)

        # ---- the editor ---------------------------------------------------
        editor_section(s, build, rundir, args.png or os.path.join(s.state, "png"))
        minff_section(s, build, rundir, args.png or os.path.join(s.state, "png"))
        failures.extend(gromacs_test_failures())
    finally:
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)

    say("%d failure(s)" % len(failures))
    return 1 if failures else 0


def builder_save_section(s, rundir):
    """A structure the Builder saved once is the Prepare's source (G-g).

    The first Builder save is stored as the task's original fragment only;
    Launch used to look for the input fragment and refused the launch.
    """
    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "bldsave",
                       os.path.join(FIXTURES, "1l2y.pdb"),
                       "frag=" + os.path.join(FIXTURES, "1l2y.pdb"))
    urls = urls_of(out)
    if not check(rc == 0 and len(urls) == 3, "a study whose Prepare has one Builder save: %s"
                 % out.strip().replace("\n", " | ")[:300]):
        return
    prep = urls[1]
    ptask = prep.replace("file://", "").rstrip("/")
    s.driver("mdsetup", prep, "localhost", rundir, s.user())
    rc, out = s.launch(prep)
    check(rc == 0, "Launch finds the structure of a Prepare saved once in the Builder: %s"
          % re.sub(r"\s+", " ", out.strip())[-200:])
    state = s.waitState(prep, 600)
    check(state in ("completed", "loaded"), "that Prepare ran to %s" % state)

    # nothing at all: the message says what to do
    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "nostruct",
                       os.path.join(FIXTURES, "1l2y.pdb"), "frag=-")
    urls = urls_of(out)
    if check(rc == 0 and len(urls) == 3, "a study with an empty Prepare"):
        s.driver("mdsetup", urls[1], "localhost", rundir, s.user())
        rc, out = s.driver("launch", urls[1], extra={"LAUNCHJOB_WARNING": "cancel"})
        text = re.sub(r"\s+", " ", out)
        check(rc != 0 and "has no structure" in text and "Builder" in text,
              "a Prepare with no structure is refused and told what to do: %s" % text[-200:])


def run_builder(s, build, display, ctx, script, tag):
    """The real Builder on a task, driven by ECCE_BUILDER_SCRIPT; its stderr."""
    harness.link(os.path.join(build, "builder"), os.path.join(s.home, "bin", "builder"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers-bld")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    path = os.path.join(s.state, tag + ".script")
    with open(path, "w") as h:
        h.write("\n".join(script) + "\n")
    env = s.env({"DISPLAY": display.name, "ECCE_BUILDER_SCRIPT": path,
                 "ECCE_FEEDBACK_STDERR": "1",
                 "PATH": wrappers + os.pathsep + s.env()["PATH"]})
    r = subprocess.run([os.path.join(wrappers, "ecce-builder"), "-context", ctx],
                       env=env, cwd=os.path.join(s.home, "bin"),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=240)
    log = r.stdout.decode("utf-8", "replace")
    open(os.path.join(s.state, tag + ".log"), "w").write(log)
    return log


def builder_flow_section(s, build, rundir):
    """One Na drawn in the Builder, residue renamed in the Residue Table,
    saved once; then the Prepare as the editor leaves it (G-b, G-d, G-h)."""
    if not shutil.which("Xvfb") or not os.access(os.path.join(build, "builder"), os.X_OK):
        say("  (no Xvfb or no builder: the Builder is not checked)")
        return
    import xdisplay
    if not s.services(True):
        check(False, "services came up for the Builder")
        return
    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "naion",
                       os.path.join(FIXTURES, "1l2y.pdb"), "frag=-",
                       "ff=amber99sb-ildn", "water=tip3p", "box=cubic",
                       "dist=1.0", "conc=0")
    urls = urls_of(out)
    if not check(rc == 0 and len(urls) == 3, "a study with an empty Prepare"):
        return
    study, prep, opt = urls
    for u in (prep, opt):
        s.driver("mdsetup", u, "localhost", rundir, s.user())
    with xdisplay.Display() as display:
        log = run_builder(s, build, display, prep, [
            "wait 5000", "add Na Terminal 0 0 0", "save", "wait 3000",
            "panel Residue Table", "wait 1500", "residues",
            "cell residue 0 2 ZZ esc", "cell residue 0 2 NA enter", "residues",
            "panel Atom Table", "wait 1500", "cell atom 0 2 0.5 enter",
            "cell atom 0 2 7 esc", "savestate", "save",
            "wait 3000", "quit"], "naion")
    say("  " + "\n  ".join(l for l in log.splitlines()
                           if "residues=" in l or "cell " in l))
    ptask = prep.replace("file://", "").rstrip("/")
    names = [n.strip() for n in re.findall(r"BUILDER: residues=(.*)", log)]
    def cell(what):
        m = re.search(r"cell %s: value=\[(.*?)\] editing=(\w+)" % what, log)
        return m.groups() if m else None
    check(cell("residue 0 2 NA enter") == ("NA", "no"),
          "Enter commits a Residue Table edit and closes the editor: %s"
          % (cell("residue 0 2 NA enter"),))
    check(cell("residue 0 2 ZZ esc")[0].strip() == "UNK",
          "Esc cancels a Residue Table edit: %s" % (cell("residue 0 2 ZZ esc"),))
    check(cell("atom 0 2 0.5 enter") == ("0.5", "no") and
          cell("atom 0 2 7 esc")[0] == "0.5",
          "Enter commits and Esc cancels in the Atom Table: %s, %s"
          % (cell("atom 0 2 0.5 enter"), cell("atom 0 2 7 esc")))
    check(names[:2] == ["UNK", "NA"],
          "the table showed the saved UNK residue and the edit reached the structure: %s" % names)
    st = re.findall(r"BUILDER: dirty=(\d) save_enabled=(\d)", log)
    check(st and st[-1] == ("1", "1"), "the rename marks the structure unsaved: %s" % st)
    pdbs = sorted(glob.glob(os.path.join(ptask, "Parameters", "*.pdb")),
                  key=os.path.getmtime)
    texts = [open(f).read() for f in pdbs]
    say("  stored: %s" % [os.path.basename(f) for f in pdbs])
    check(len(texts) == 2 and " UNK " in texts[0] and " NA " in texts[1]
          and " UNK " not in texts[1],
          "the second save stored residue NA (first save kept UNK)")
    # closing the Builder ended the session; start it again
    if not s.services(True):
        check(False, "services came up again after the Builder")
        return
    prepare_then_optimize(s, rundir, prep, opt, "Na+ in water", charged=False)

    # the same ion left UNK by the Builder (no Residue Table edit): the job names it
    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "naunk",
                       os.path.join(FIXTURES, "1l2y.pdb"), "frag=" + NA_UNK,
                       "box=cubic", "dist=1.0", "conc=0")
    urls = urls_of(out)
    if check(rc == 0 and len(urls) == 3, "a study with a lone UNK Na as the Builder saves it"):
        for u in urls[1:]:
            s.driver("mdsetup", u, "localhost", rundir, s.user())
        prepare_then_optimize(s, rundir, urls[1], urls[2], "UNK Na+ in water", charged=False)

    # "Neutralise" unticked: the +1 stays, and the Optimize launches without
    # the Launch-check dialog (LAUNCHJOB_WARNING=cancel would make it fail)
    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "naoff", os.path.join(FIXTURES, "1l2y.pdb"),
                       "frag=" + NA_UNK, "box=cubic", "dist=1.0", "conc=0", "neut=0")
    urls = urls_of(out)
    if check(rc == 0 and len(urls) == 3, "a study with Neutralise unticked"):
        for u in urls[1:]:
            s.driver("mdsetup", u, "localhost", rundir, s.user())
        prepare_then_optimize(s, rundir, urls[1], urls[2], "Na+ unneutralised", charged=True)


def prepare_then_optimize(s, rundir, prep, opt, what, charged):
    """Launch the Prepare, check its system, then the Optimize after it."""
    rc, out = s.launch(prep)
    check(rc == 0, "%s: Launch accepted the Prepare: %s" % (what, re.sub(r"\s+", " ", out.strip())[-150:]))
    state = s.waitState(prep, 600)
    check(state in ("completed", "loaded"), "%s: Prepare ran to %s" % (what, state))
    ptask = prep.replace("file://", "").rstrip("/")
    top = [o for o in outputs(prep) if o.endswith(".gmxtop")]
    mol = open(os.path.join(ptask, "Outputs", top[0])).read() if top else ""
    mols = [l.split()[0] for l in mol.split("[ molecules ]")[-1].splitlines()
            if l.strip() and not l.lstrip().startswith(";")]
    say("  %s: molecules %s" % (what, " ".join(mols[:12])))
    check(("NA" in mols or "Ion" in mols) and "SOL" in mols and ("CL" in mols) == (not charged),
          "%s: the system holds Na+ and water%s" % (what, "" if charged else " and the Cl- that neutralises it"))
    rc, stored = s.driver("progress", prep)
    m = re.search(r"charge0=(\S+) charge1=(\S+)", stored)
    say("  %s: net charge before/after ions %s" % (what, m.groups() if m else stored.strip()[:200]))
    want = "1" if charged else "0"
    check(m is not None and float(m.group(2)) == (1.0 if charged else 0.0),
          "%s: the stored net charge after the ions is %s" % (what, want))
    rc, out = s.driver("launch", opt, extra={"LAUNCHJOB_WARNING": "cancel"})
    check(rc == 0, "%s: Launch accepted the Optimize without a dialog: %s"
          % (what, re.sub(r"\s+", " ", out.strip())[-150:]))
    state = s.waitState(opt, 900)
    check(state in ("completed", "loaded"), "%s: Optimize ran to %s" % (what, state))


def editor_section(s, build, rundir, png):
    """gmxprepare through ECCE_TEST_MDED, then Launch of what it saved."""
    if not shutil.which("Xvfb"):
        say("  (no Xvfb: the editor is not checked)")
        return
    for exe in ("gmxprepare", "organizer"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            say("  (%s is not built: the editor is not checked)" % exe)
            return
    import xdisplay
    import gromacs_test
    from md_editors_test import summary
    for exe in ("gmxprepare", "organizer"):
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers-ed")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    os.makedirs(png, exist_ok=True)

    # closing the Organizer above ended the session; start it again
    if not s.services(True):
        check(False, "services came up again")
        return

    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "edprep",
                       os.path.join(FIXTURES, "1l2y.pdb"), "bare")
    urls = urls_of(out)
    if not check(rc == 0 and len(urls) == 3, "a study with a bare Prepare was made: %s"
                 % out.strip().replace("\n", " | ")[:300]):
        return
    prep = urls[1]
    ptask = prep.replace("file://", "").rstrip("/")
    check(not os.path.isdir(os.path.join(ptask, "Inputs")) or not os.listdir(
        os.path.join(ptask, "Inputs")), "the new Prepare has no inputs yet")
    rc, out = s.driver("nwchemmdstudy", s.userUrl(), "edprepnw")
    nw = [u.rstrip("/") + "/" for u in urls_of(out) if u.rstrip("/").endswith("/prepare")]

    with xdisplay.Display() as display:
        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir, exist_ok=True)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        ed = gromacs_test.Editor(s, display, genv, wrappers, cmddir,
                                 "gmxprepare", "GMXPrepare")
        try:
            if ed.up():
                check(ed.cmd("open " + prep) == "ok", "the Prepare opens in gmxprepare")
                check(ed.cmd("title") == "ECCE GROMACS MD Prepare",
                      "the title: %s" % ed.cmd("title"))
                check(ed.cmd("tabs") == "System", "one page: %s" % ed.cmd("tabs"))
                recipe = ed.cmd("recipe")
                check("FF=amber99sb-ildn" in recipe and "WATER=tip3p" in recipe
                      and "MERGE=1" in recipe and "CONC=0.15" in recipe,
                      "opening a new Prepare stores the default recipe: %s" % recipe)
                fields = ed.cmd("fields")
                check("structure=builder" in fields and "waters=tip3p,tip4p,tip4pew,tip5p,spc,spce " in fields,
                      "the defaults and the AMBER water models: %s" % fields)

                # G3: Verify shows what the job will run
                ver = ed.cmd("verify")
                check("gmx pdb2gmx -f <structure.pdb> -ff amber99sb-ildn -water tip3p -ignh -merge all" in ver
                      and "gmx editconf -f step1.gro -o step2.gro -c -bt dodecahedron -d 1" in ver
                      and "gmx solvate -cp step2.gro -cs spc216.gro" in ver
                      and "-pname NA -pq 1 -nname CL -neutral -conc 0.15" in ver
                      and "rcoulomb = 1.0" in ver and "FF=amber99sb-ildn" in ver,
                      "Verify lists the protein Prepare's commands and ions.mdp: %s" % ver)
                sz = [int(x) for x in re.findall(r"\d+", ed.cmd("size"))]
                check(len(sz) == 6 and sz[0] <= 1000 and sz[1] <= 760,
                      "the editor opens compact: %s" % sz)

                # the water models follow the force field
                check(ed.cmd("set ff oplsaa") == "ok", "OPLS-AA chosen")
                fields = ed.cmd("fields")
                check("waters=tip4p,tip4pew,tip3p,tip5p,tip5pe,spc,spce " in fields
                      and "water=tip3p" in fields,
                      "OPLS-AA offers its own water models and keeps TIP3P: %s" % fields)
                check(ed.cmd("set ff gromos54a7") == "ok", "GROMOS chosen")
                fields = ed.cmd("fields")
                check("waters=spc,spce " in fields and "water=spc " in fields,
                      "GROMOS offers SPC and SPC/E, SPC chosen: %s" % fields)
                check(ed.cmd("set water tip3p").startswith("refused"),
                      "TIP3P is refused for GROMOS")
                check(ed.cmd("set ff amber99sb-ildn") == "ok", "AMBER99SB-ILDN chosen")
                check(ed.cmd("set water tip3p") == "ok", "TIP3P chosen")

                # a bad number is not saved
                ed.cmd("set conc abc")
                ed.cmd("save")
                check("CONC=0.15" in ed.cmd("recipe"),
                      "a concentration that is not a number is not saved")
                check(ed.cmd("set conc 0.15") == "ok", "0.15 mol/L set")
                check(ed.cmd("set structure " + os.path.join(FIXTURES, "1l2y.pdb")) == "ok",
                      "a PDB file chosen as the structure")
                check("structure=1l2y.gmxpdb" in ed.cmd("fields"), "the editor shows it")
                shot = os.path.join(png, "gmxprepare.png")
                check(ed.cmd("snap " + shot) == "saved" and os.path.exists(shot),
                      "window shot: %s" % shot)
                ed.cmd("save")
                recipe = ed.cmd("recipe")
                check("FF=amber99sb-ildn" in recipe and "WATER=tip3p" in recipe
                      and "CONC=0.15" in recipe and "BOX=dodecahedron" in recipe,
                      "Save stored the recipe: %s" % recipe)
                model = ed.cmd("model")
                check("<ForceField>amber99sb-ildn</ForceField>" in model
                      and "<WaterModel>tip3p</WaterModel>" in model,
                      "and the model: %s" % model[:200])
                inputs = sorted(os.listdir(os.path.join(ptask, "Inputs")))
                check("1l2y.gmxpdb" in inputs and "gromacs.gmxprep" in inputs,
                      "the task holds the source PDB and the recipe: %s" % inputs)
                check(ed.proc.poll() is None, "gmxprepare is still up")

                # the editor stays open: closing the last window of a session
                # ends the session, and with it the broker the job needs
                rc, out = s.driver("mdsetup", prep, "localhost", rundir, s.user())
                check(rc == 0, "launch settings stored for the edited Prepare")
                rc, out = s.launch(prep)
                check(rc == 0, "Launch accepted the edited Prepare: %s"
                      % out.strip().splitlines()[-3:])
                state = s.waitState(prep, 600)
                check(state in ("completed", "loaded"),
                      "the edited Prepare ran to %s" % state)
                outs = outputs(prep)
                check(any(o.endswith(".gmxtop") for o in outs) and
                      any(o.endswith(".gro") for o in outs),
                      "with its topology and structure")
                check(ed.proc.poll() is None, "gmxprepare is still up after the run")
                viewer_section(s, build, display, genv, wrappers, prep, png)
        finally:
            ed.close()

        # the Organizer opens the right editor for each code's Prepare
        if nw:
            answers, menus = summary(s, display, genv, wrappers, [prep, nw[0]], png)
            for url, want, other in ((prep, "GROMACS MD Prepare", "NWChem MD Prepare"),
                                     (nw[0], "NWChem MD Prepare", "GROMACS MD Prepare")):
                tools = re.findall(r"tool=(.*?)(?= tool=|$)", answers.get(url, ""))
                check(want in tools and other not in tools,
                      "the Organizer's button for %s: %s" % (want, tools))
                items = re.findall(r"item=(.*?)(?= item=|$)", menus.get(url, ""))
                check(any(i.startswith(want) for i in items) and
                      not any(i.startswith(other) for i in items),
                      "the Tools menu for %s: %s" % (want, items))


def viewer_section(s, build, display, genv, wrappers, prep, png):
    """The finished Prepare in the Builder (the Viewer): structure and cell."""
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        say("  (builder is not built: the Viewer is not checked)")
        return
    harness.link(os.path.join(build, "builder"), os.path.join(s.home, "bin", "builder"))
    out = os.path.join(s.state, "viewer")
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    shot = os.path.join(png, "prepare-viewer.png")
    shot_cell = os.path.join(png, "prepare-viewer-cell.png")
    shot_props = os.path.join(png, "prepare-viewer-properties.png")
    scene = os.path.join(out, "scene")
    with open(scene, "w") as h:
        h.write("\n".join(["background 1 1 1", "viewall", "xshot " + shot,
                           "fragdump before", "pbcopen", "viewall",
                           "xshot " + shot_cell, "fragdump after", "columntab 1",
                           "xshot " + shot_props, ""]))
    env = dict(genv, ECCE_VIEWER_SCENE=scene, ECCE_VIEWER_SCENE_OUT=out,
               ECCE_PANEL_FULLSCREEN="1")
    log = os.path.join(s.state, "viewer.log")
    proc = subprocess.Popen(
        [os.path.join(wrappers, "ecce-builder"), "-context", prep], env=env,
        cwd=os.path.join(s.home, "bin"), stdout=open(log, "w"),
        stderr=subprocess.STDOUT, start_new_session=True)
    try:
        end = time.time() + 180
        while time.time() < end and not os.path.exists(shot_props):
            if proc.poll() is not None:
                break
            time.sleep(1)
        time.sleep(2)
    finally:
        import gromacs_test
        gromacs_test.stop(proc)
    for name in ("before", "after"):
        f = os.path.join(out, name + ".txt")
        text = open(f).read() if os.path.exists(f) else ""
        say("  viewer %s: %s" % (name, " | ".join(text.splitlines()[:8])))
    check(os.path.exists(shot) and os.path.exists(shot_cell),
          "the Viewer's screenshots were taken")
    before = open(os.path.join(out, "before.txt")).read() \
        if os.path.exists(os.path.join(out, "before.txt")) else ""
    m = re.search(r"^atoms (\d+)$", before, re.M)
    check(m is not None and int(m.group(1)) > 3000,
          "the Viewer shows the solvated system (thousands of atoms)")
    check("lattice 1" in before, "the structure carries the box as its cell")
    # G4: opened in the Viewer the box is drawn, with no panel opened by hand
    check("latticeshown 1" in before, "the Viewer draws the box when it opens")
    # the cell against gmx's own box line of the .gro the Prepare stored
    gro = [o for o in outputs(prep) if o.endswith(".gro")]
    vec = [[float(x) * 0.1 for x in l.split()[1:4]]
           for l in before.splitlines() if l.startswith("vector ")]
    if gro and len(vec) == 3:
        box = [float(x) for x in open(os.path.join(
            prep.replace("file://", "").rstrip("/"), "Outputs",
            gro[0])).read().split("\n")[-2].split()]
        v = [[box[0], box[3], box[4]], [box[5], box[1], box[6]],
             [box[7], box[8], box[2]]]
        off = max(abs(vec[i][k] - v[i][k]) for i in range(3) for k in range(3))
        check(off < 0.002, "the cell vectors are the .gro box's (largest "
              "difference %.4f nm)" % off)


def gromacs_test_failures():
    import gromacs_test
    return gromacs_test.failures


def organizer_new(s, build, gmx):
    """File > New > GROMACS MD Prepare, through the Organizer's test hook."""
    for tool in ("Xvfb",):
        if not shutil.which(tool):
            say("  (no %s: the Organizer's New menu is not checked)" % tool)
            return
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        say("  (organizer is not built: the New menu is not checked)")
        return
    import xdisplay
    harness.link(os.path.join(build, "organizer"),
                 os.path.join(s.home, "bin", "organizer"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    rc, out = s.driver("gromacsstudy", s.userUrl(), "menu")
    urls = urls_of(out)
    if not check(rc == 0 and urls, "a project to make the study in"):
        return
    proj = urls[0].rsplit("/", 2)[0] + "/"
    # a study with tasks already has its own; make an empty one with the hook
    cmds = os.path.join(s.state, "organizer-commands")
    open(cmds, "w").close()
    log = os.path.join(s.state, "organizer.log")
    with xdisplay.Display() as display:
        env = s.env({"DISPLAY": display.name, "ECCE_TEST_ORGANIZER": cmds,
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        proc = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")], env=env,
            cwd=os.path.join(s.home, "bin"), stdout=open(log, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=120):
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if proc.poll() is not None:
                    return "the Organizer ended (%s)" % proc.returncode
                for t in open(log, errors="replace").read().splitlines():
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        try:
            # selecting each task builds its summary panel from the tool
            # views of its content type (an Equilibrate had none: SIGSEGV)
            for u in urls[1:]:
                ans = send("summary " + u)
                check(ans.startswith("ok") and proc.poll() is None,
                      "the Organizer shows the %s task: %s"
                      % (u.rstrip("/").rsplit("/", 1)[-1], ans[:60]))
            ans = send("new %s gromacs_md_study" % proj)
            check(ans == "ok", "Organizer New: a GROMACS MD Study: %s" % ans)
            pdir = proj.replace("file://", "")
            before = set(["menu"])
            new_study = None
            for x in sorted(os.listdir(pdir)):
                full = os.path.join(pdir, x)
                if x not in before and os.path.isdir(full) and \
                        not any(os.path.isdir(os.path.join(full, y)) for y in os.listdir(full)):
                    new_study = full
            say("  project holds: %s -> new study %s" % (os.listdir(pdir), new_study))
            if check(new_study, "the study made by the hook was found"):
                ans = send("new file://%s gromacs_md_prepare" % new_study)
                check(ans == "ok", "Organizer New: a GROMACS MD Prepare in the study: %s" % ans)
                made = [x for x in os.listdir(new_study)
                        if os.path.isdir(os.path.join(new_study, x))]
                say("  study holds: %s" % made)
                prep = [m for m in made if "repare" in m]
                if check(prep, "the Prepare was created in the study"):
                    rc, out = s.driver("apptype", "file://%s/%s" % (new_study, prep[0]))
                    check(out.split()[:2] == ["GROMACS", "ecceMdPrepare"],
                          "with the GROMACS application type: %s" % out.strip())
        finally:
            for sig in (15, 9):
                try:
                    os.killpg(proc.pid, sig)
                    proc.wait(timeout=10)
                    break
                except Exception:
                    pass


def minff_section(s, build, rundir, png):
    """The MINFF source: build in the editor, Prepare, then an Optimize.

    The mineral is the public pyrophyllite unit cell of the minff repository
    (fetched with min.ff, not stored here), repeated 4 x 3 x 3 with four
    Al -> Mg substitutions, so it carries a charge of -4 for the ions to
    neutralise.  Skipped without atomipy, min.ff or the editors."""
    sys.path.insert(0, os.path.join(REPO, "scripts"))
    try:
        import minff_topology as mt
        py = mt.find_atomipy_python()
        ffdir = mt.fetch_pinned()
    except Exception as e:
        say("  (MINFF is not checked: %s)" % e)
        return
    if not py:
        say("  (MINFF is not checked: atomipy is not installed)")
        return
    if not shutil.which("Xvfb"):
        say("  (no Xvfb: the MINFF editor is not checked)")
        return
    for exe in ("gmxprepare", "mdoptimize"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            say("  (%s is not built: MINFF is not checked)" % exe)
            return
    import xdisplay
    import gromacs_test
    for exe in ("gmxprepare", "mdoptimize"):
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers-minff")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    if not s.services(True):
        check(False, "services came up for the MINFF section")
        return

    cell = os.path.join(ffdir, "Systems", "UC_conf",
                        "Pyrophyllite_Lee_Guggenheim_1981_GII_0.071.pdb")
    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "minffprep",
                       os.path.join(FIXTURES, "1l2y.pdb"), "bare")
    urls = urls_of(out)
    if not check(rc == 0 and len(urls) == 3, "a study with a bare Prepare was made"):
        return
    prep, opt = urls[1], urls[2]
    ptask = prep.replace("file://", "").rstrip("/")
    otask = opt.replace("file://", "").rstrip("/")

    with xdisplay.Display() as display:
        cmddir = os.path.join(s.state, "mded-minff")
        os.makedirs(cmddir, exist_ok=True)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "ECCE_ATOMIPY_PYTHON": py, "ECCE_MINFF_DIR": ffdir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        ed = gromacs_test.Editor(s, display, genv, wrappers, cmddir,
                                 "gmxprepare", "GMXPrepare")
        try:
            if not ed.up():
                return
            check(ed.cmd("open " + prep) == "ok", "the Prepare opens in gmxprepare")
            check(ed.cmd("set source minff") == "ok", "source: Mineral (MINFF)")
            fields = ed.cmd("fields")
            check("source=minff" in fields and "water=opc3" in fields
                  and "waters=opc3,opc,spce,tip3p,tip3p-fb,tip4p-fb,tip4pew " in fields
                  and "dist=0 " in fields and "conc=0 " in fields
                  and "ions=Na,Cl" in fields and "built=0" in fields,
                  "MINFF's water models, ions and no water layer: %s" % fields)
            check(ed.cmd("set mcell " + cell) == "ok", "the unit cell is chosen")
            # G7: a 6 x 4 x 1 cell is wide but 0.9 nm high; the editor warns
            # until the water layer lengthens c (synthetic stand-in for the
            # .gro of the report)
            # G1: compact and scrollable, with the tall mineral page showing
            sz = [int(x) for x in re.findall(r"\d+", ed.cmd("size"))]
            check(len(sz) == 6 and sz[0] <= 1000 and sz[1] <= 760
                  and sz[5] > sz[3],
                  "the editor opens compact and scrolls its page: %s" % sz)
            ver = ed.cmd("verify")
            check("SOURCE=minff" in ver and "no water layer: step2.gro" in ver
                  and "(mineral)" in ver,
                  "Verify before a build names the mineral recipe: %s" % ver[:200])
            check(ed.cmd("set mrep 6 4 1") == "ok", "repeat 6 4 1")
            out = ed.cmd("set mbuild go", timeout=600)
            check(out == "ok", "Build topology of the flat cell: %s" % out)
            check("cellwarning=1" in ed.cmd("fields"),
                  "no water: the editor warns the cell is too small for the cut-off")
            check(ed.cmd("set dist 2") == "ok" and "cellwarning=0" in ed.cmd("fields"),
                  "2 nm of water: no warning")
            ver = ed.cmd("verify")
            check("gmx solvate -cp step2.gro -cs spc216.gro" in ver
                  and re.search(r"rcoulomb = 0\.[0-5]\d*\b", ver) and "periodic-molecules = yes" in ver
                  and "-pname Na" in ver and "-neutral" in ver,
                  "Verify lists solvate, genion and the mineral's ions.mdp: %s" % ver[-420:])
            check(ed.cmd("set dist 0") == "ok" and "cellwarning=1" in ed.cmd("fields"),
                  "water layer off again: the warning returns")
            check(ed.cmd("set mrep 4 3 3") == "ok" and
                  ed.cmd("set msubs Al,Mgo,4") == "ok" and
                  ed.cmd("set mseed 1") == "ok", "repeat 4 3 3, 4 Al -> Mgo, seed 1")
            shot = os.path.join(png, "gmxprepare-minff.png")
            os.makedirs(png, exist_ok=True)
            out = ed.cmd("set mbuild go", timeout=600)
            check(out == "ok", "Build topology ran minff_topology.py: %s" % out)
            fields = ed.cmd("fields")
            say("  fields: %s" % fields)
            check("built=1" in fields and "1440 atoms, charge -4.0" in fields,
                  "the editor reports 1440 atoms and charge -4: %s"
                  % fields.split("mineral=")[-1])
            check(ed.cmd("snap " + shot) == "saved", "window shot: %s" % shot)
            # a changed option marks the build as stale, but the files stay
            check(ed.cmd("set pname Ca") == "ok" and ed.cmd("set dist 2") == "ok"
                  and ed.cmd("set conc 0") == "ok", "Ca counter-ions, 2 nm of water")
            check(ed.cmd("set pname NA").startswith("refused"),
                  "a protein ion name is not offered for a mineral")
            ed.cmd("save")
            recipe = ed.cmd("recipe")
            check("SOURCE=minff" in recipe and "WATER=opc3" in recipe
                  and "DIST=2" in recipe and "PNAME=Ca" in recipe and "PQ=2" in recipe
                  and "CONC=0 " in recipe + " ", "Save stored the recipe: %s" % recipe)
            inputs = sorted(os.listdir(os.path.join(ptask, "Inputs")))
            say("  Prepare Inputs/: %s" % inputs)
            check(all(f in inputs for f in ("conf.gmxmin", "topol.gmxmin", "min.itp",
                                            "min.ff__SLASH__forcefield.itp",
                                            "min.ff__SLASH__ions.itp",
                                            "min.ff__SLASH__opc3.itp",
                                            "gromacs.gmxprep"))
                  and not any(f.endswith(".gmxpdb") for f in inputs),
                  "the task holds the structure, topology and includes")
            model = ed.cmd("model")
            check("<Source>minff</Source>" in model and "<MinffAngleK>500</MinffAngleK>" in model
                  and "<MinffReplicate>4 3 3</MinffReplicate>" in model,
                  "and the options in the model")

            rc, out = s.driver("mdsetup", prep, "localhost", rundir, s.user())
            check(rc == 0, "launch settings stored for the MINFF Prepare")
            rc, out = s.launch(prep)
            check(rc == 0, "Launch accepted the MINFF Prepare (no Builder structure, "
                  "no PDB): %s" % out.strip().splitlines()[-3:])
            state = s.waitState(prep, 900)
            check(state in ("completed", "loaded"), "the MINFF Prepare ran to %s" % state)
            outs = outputs(prep)
            say("  Prepare Outputs/: %s" % outs)
            check(any(o.endswith(".gmxtop") for o in outs)
                  and any(o.endswith(".gro") for o in outs) and "chemsys.pdb" in outs,
                  "with its topology, structure and chemsys.pdb")
            rc, stored = s.driver("progress", prep)
            say("  stored progress: %s" % stored.strip().replace("\n", " | ")[:400])
            check("done=yes" in stored and re.search(r"atoms=\d+", stored)
                  and "box=cell" in stored, "the Progress box has the summary")
            props = s.props(prep)
            say("  Prepare properties: %s" % props)
            check("NATOMS" in props and "NWATERS" in props, "and the properties")

            # ---- the Optimize after it, nothing attached ----------------
            oed = gromacs_test.Editor(s, display, genv, wrappers, cmddir,
                                      "mdoptimize", "MDOptimize")
            try:
                if oed.up():
                    check(oed.cmd("open " + opt) == "ok", "the Optimize opens in its editor")
                    check(oed.cmd("set-steps 50") == "ok", "50 steps")
                    check(oed.cmd("save") == "ok", "Save")
                    mdp = open(os.path.join(otask, "Inputs", "gromacs.mdp")).read()
                    check(re.search(r"periodic-molecules\s*=\s*yes", mdp) is not None,
                          "the .mdp has periodic-molecules = yes from the cell")
                    rc, out = s.driver("mdsetup", opt, "localhost", rundir, s.user())
                    rc, out = s.launch(opt)
                    check(rc == 0, "Launch accepted the Optimize: %s"
                          % out.strip().splitlines()[-3:])
                    state = s.waitState(opt, 900)
                    check(state in ("completed", "loaded"), "the Optimize ran to %s" % state)
                    props = s.props(opt)
                    say("  Optimize properties: %s" % props)
                    check("TE" in props, "the Optimize stored its energies")
                    r = last_run_file(rundir, "min.ff__SLASH__forcefield.itp") or \
                        last_run_file(rundir, "forcefield.itp")
                    check(r is not None, "the force field includes reached the Optimize's run")
            finally:
                oed.close()
        finally:
            ed.close()


if __name__ == "__main__":
    sys.exit(main())
