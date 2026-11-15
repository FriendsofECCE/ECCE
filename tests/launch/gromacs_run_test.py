#!/usr/bin/env python3
"""A GROMACS calculation made from the user's own files (#1038), launched.

File > New > GROMACS Calculation from Files... through its Organizer hook
`gmxrun PARENT MDP|- GRO|- TOP|- [TPR]`: a gromacs_md_run (its own content
type, so a fresh process reloads it as itself); the inputs are stored, Launch
ships them, the job script runs grompp (not for a .tpr) and mdrun, and the
monitor stores energies, the trajectory and the final structure.

Hand-offs (case 1's run): New from This > Rerun and Continue from Last
Geometry (`duplicate URL [last]`; the copy's .gro is the final frame, and it
runs), In Another Code from GROMACS to xTB (`anothercode`), and from that xTB
calculation back to GROMACS (`anothercodegmx`: the coordinates go into the
user's .gro; a .gro whose atoms differ is refused).

    tests/launch/gromacs_run_test.py [--build DIR] [--keep]

Cases: the small water fixture (.mdp + .gro + .top); the same system as a
ready .tpr; real MINFF inputs with a force-field folder and an index file,
when they exist on this machine.  Exit status 77 (CTest SKIP) without gmx,
Xvfb or the built binaries.
"""

import argparse
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
from gromacs_test import check, failures, stop  # noqa: E402

FIX = os.path.join(REPO, "tests", "e2e", "fixtures", "gromacs")
MINFF = "/home/andy/tmp/Andy/0_MD_gromacs_minff/minff_MD"


def where(u):
    return u[len("file://"):].rstrip("/")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "gmxrun", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
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
    s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
    work = os.path.join(s.state, "inputs")
    os.makedirs(work)

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        rc, out = s.driver("gromacsstudy", s.userUrl(), "scratch")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and urls, "a project to work in"):
            return 1
        # gromacsstudy makes its study in this project
        proj = s.userUrl().rstrip("/") + "/scratch-project/"

        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=120):
            seen = len(open(olog, errors="replace").read().splitlines()) \
                if os.path.exists(olog) else 0
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if org.poll() is not None:
                    return "the Organizer ended (%s)" % org.returncode
                for t in open(olog, errors="replace").read().splitlines()[seen:]:
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        projdir = where(proj)

        def made_by(line):
            before = set(os.listdir(projdir))
            ans = send(line)
            new = sorted(set(os.listdir(projdir)) - before)
            return ans, new

        def handoffs(url):
            task = where(url)
            final = [o for o in os.listdir(os.path.join(task, "Outputs"))
                     if o.endswith(".gro")]
            final = open(os.path.join(task, "Outputs", final[0])).read() \
                if final else ""
            start = open(os.path.join(task, "Inputs", "conf.gro")).read()

            say("hand-off: New from This > Rerun")
            ans, new = made_by("duplicate %s" % url.rstrip("/"))
            if check(ans.startswith("ok") and len(new) == 1,
                     "rerun: one copy made: %s %s" % (ans, new)):
                cu = proj + new[0] + "/"
                rc, out = s.driver("apptype", cu)
                check(out.strip().endswith("gromacs_md_run"),
                      "rerun: the copy is a gromacs_md_run: %s" % out.strip())
                check(s.state_of(cu) == "ready", "rerun: the copy is Ready")
                check(open(os.path.join(where(cu), "Inputs", "conf.gro")).read()
                      == start, "rerun: the copy starts from the same .gro")

            say("hand-off: Continue from Last Geometry")
            ans, new = made_by("duplicate %s last" % url.rstrip("/"))
            if check(ans.startswith("ok") and len(new) == 1,
                     "last geometry: one copy made: %s %s" % (ans, new)):
                cu = proj + new[0] + "/"
                check(s.state_of(cu) == "ready", "last geometry: the copy is Ready")
                got = open(os.path.join(where(cu), "Inputs", "conf.gro")).read()
                check(final and got == final,
                      "last geometry: the copy's .gro is the final frame")
                check(not os.path.isdir(os.path.join(where(cu), "Outputs")) or
                      not os.listdir(os.path.join(where(cu), "Outputs")),
                      "last geometry: the copy has no results")
                run_it("continued", "", ("conf.gro", "gromacs.mdp"), new[0],
                       url=cu)

            say("hand-off: In Another Code, GROMACS -> xTB")
            ans, new = made_by("anothercode xtb_es _xtb - %s" % url)
            xurl = None
            if check(" made" in ans and new == ["gromacs_xtb"],
                     "to xTB: made: %s %s" % (ans, new)):
                xurl = proj + new[0] + "/"
                rc, out = s.driver("apptype", xurl)
                check("xtb_es" in out, "to xTB: an xTB calculation: %s" % out.strip())
            if not xurl:
                return

            say("hand-off: In Another Code, xTB -> GROMACS")
            ans, new = made_by("anothercodegmx %s %s %s _gmx - %s"
                               % (mdp, gro, top, xurl))
            if check("made, Ready" in ans and new == ["gromacs_xtb_gmx"],
                     "to GROMACS: made: %s %s" % (ans, new)):
                gu = proj + new[0] + "/"
                got = open(os.path.join(where(gu), "Inputs", "conf.gro")).read()

                def xyz(text):
                    rows = text.splitlines()[2:-1]
                    return [tuple(float(r[20 + 8 * k:28 + 8 * k]) for k in range(3))
                            for r in rows]
                a, b = xyz(got), xyz(final)
                check(len(a) == len(b) == 648 and
                      max(abs(p - q) for u, v in zip(a, b) for p, q in zip(u, v))
                      < 0.0011,
                      "to GROMACS: the .gro holds the xTB structure's coordinates")
                check(got.splitlines()[-1] == start.splitlines()[-1],
                      "to GROMACS: the user's box line is kept")
                run_it("from-xtb", "", ("conf.gro",), new[0], url=gu)

            say("hand-off: a .gro with other atoms is refused")
            bad = os.path.join(work, "swapped.gro")
            rows = start.splitlines()
            rows[2], rows[3] = rows[3], rows[2]
            open(bad, "w").write("\n".join(rows) + "\n")
            ans, new = made_by("anothercodegmx %s %s %s _bad - %s"
                               % (mdp, bad, top, xurl))
            check("not made" in ans and "same order" in ans and not new,
                  "refused, with the reason: %s" % ans)

        def run_it(case, args_, expect_inputs, name, url=None):
            if url is None:
                ans = send("gmxrun %s %s" % (proj, args_))
                if not check(ans.startswith("ok "), "%s: gmxrun: %s" % (case, ans)):
                    return None
                url = ans.split()[1].rstrip("/") + "/"
            check(url.rstrip("/").endswith("/" + name),
                  "%s: the calculation is named %s: %s" % (case, name, url))
            rc, out = s.driver("apptype", url)
            check(out.strip().endswith("GROMACS ecceMdRun gromacs_md_run"),
                  "%s: it reloads as a gromacs_md_run: %s" % (case, out.strip()))
            task = where(url)
            have = sorted(os.listdir(os.path.join(task, "Inputs")))
            say("  Inputs/: %s" % have)
            for f in expect_inputs:
                check(f in have, "%s: %s is stored" % (case, f))
            check(s.state_of(url) == "ready", "%s: the run is Ready" % case)
            rundir = os.path.join(s.state, "run-" + case)
            rc, out = s.driver("mdsetup", url, "localhost", rundir, s.user())
            check(rc == 0, "%s: launch settings stored" % case)
            rc, out = s.launch(url)
            check(rc == 0, "%s: Launch accepted the run: %s"
                  % (case, out.strip().splitlines()[-3:]))
            state = s.waitState(url, 600)
            check(state in ("completed", "loaded"), "%s: ran to %s" % (case, state))
            props = s.props(url)
            say("  properties: %s" % props)
            check("TE" in props or "TEVEC" in props,
                  "%s: energies were stored" % case)
            outs = sorted(os.listdir(os.path.join(task, "Outputs"))) \
                if os.path.isdir(os.path.join(task, "Outputs")) else []
            say("  Outputs/: %s" % outs)
            check(any(o.endswith(".gro") for o in outs) and "chemsys.pdb" in outs,
                  "%s: the final structure (.gro, chemsys.pdb) was stored" % case)
            check(any(o.endswith(".xtc") or o.endswith(".trr") for o in outs)
                  or any(o.endswith(".edr") for o in outs),
                  "%s: the trajectory or energy file was stored" % case)
            return url

        # ---- 1: .mdp + .gro + .top -------------------------------------
        say("case 1: .mdp, .gro, .top")
        mdp, gro, top = (os.path.join(FIX, f)
                         for f in ("gromacs.mdp", "conf.gro", "topol.top"))
        first = run_it("files", "%s %s %s" % (mdp, gro, top),
                       ("gromacs.mdp", "conf.gro", "topol.gmxtop"), "gromacs")
        if first:
            handoffs(first)

        # ---- 2: a ready .tpr --------------------------------------------
        say("case 2: a ready .tpr")
        tpr = os.path.join(work, "ready.tpr")
        p = subprocess.run([gmx, "grompp", "-f", mdp, "-c", gro, "-p", top,
                            "-o", tpr, "-maxwarn", "99"], cwd=work,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if check(p.returncode == 0 and os.path.exists(tpr), "a .tpr was made"):
            run_it("tpr", "- - - %s" % tpr, ("run.gmxtpr", "gromacs.mdp"), "ready")

        # ---- 3: MINFF, force-field folder and index file ------------------
        em = os.path.join(MINFF, "EM", "em.mdp")
        pre = os.path.join(MINFF, "gmx_minff", "preem.gro")
        mtop = os.path.join(MINFF, "gmx_minff", "topol.top")
        if all(os.path.exists(f) for f in (em, pre, mtop)):
            say("case 3: MINFF")
            short = os.path.join(work, "em.mdp")
            text = re.sub(r"(?m)^nsteps\b.*$", "nsteps = 20", open(em).read())
            open(short, "w").write(text)
            run_it("minff", "%s %s %s" % (short, pre, mtop),
                   ("gromacs.mdp", "preem.gro", "topol.gmxtop"), "em")
            ans = send("gmxrun %s %s %s %s" % (proj, short, pre, mtop))
            check(ans.startswith("ok ") and ans.split()[1].rstrip("/").endswith("/em_1"),
                  "a second run of the same name is made unique: %s" % ans)
        else:
            say("case 3: the MINFF inputs are not on this machine; skipped")
    finally:
        if org is not None:
            stop(org)
        if display is not None:
            display.__exit__(None, None, None)
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)
    say("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
