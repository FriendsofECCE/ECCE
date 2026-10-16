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
    short is refused at launch with pdb2gmx's message;
  * the Organizer's New menu (its test hook) makes the Prepare with the
    right application type.

    tests/launch/gromacs_prepare_test.py [--build build-cmake] [--keep]

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
            rc, out = s.launch(brk[1])
            check(rc != 0 and ("missing" in out and "TRP" in out),
                  "a side chain cut short is refused at launch with pdb2gmx's "
                  "message: %s" % re.sub(r"\s+", " ", out.strip())[-200:])

        # ---- the Organizer's New menu -----------------------------------
        organizer_new(s, build, gmx)
    finally:
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)

    say("%d failure(s)" % len(failures))
    return 1 if failures else 0


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
                for t in open(log, errors="replace").read().splitlines():
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        try:
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


if __name__ == "__main__":
    sys.exit(main())
