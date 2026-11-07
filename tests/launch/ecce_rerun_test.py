#!/usr/bin/env python3
"""ecce-rerun: the Organizer copies finished jobs without changing the original.

Uses stored NWChem setup and a sleeping stand-in executable, not a chemistry
calculation. Checks Ready copies, setup preservation, naming, launch and refusals.
"""

import argparse
import hashlib
import re
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    exe = os.path.join(build, "ecce-rerun")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-rerun is not built")
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-rerun", {"NWChem": "/bin/true"},
                        local=True, keep=args.keep)
    slow = os.path.join(s.state, "slow-nwchem")
    with open(slow, "w") as h:
        # NWChem's monitor detects Running from its parse file, not stdout.
        h.write("#!/bin/sh\n: > ecce.out\nsleep 300\nexec /bin/true \"$@\"\n")
    os.chmod(slow, 0o755)
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
    s.registerMachine("localhost", "Shell", {"NWChem": slow})
    s.registerMachine("stored", "Shell", {"NWChem": slow})
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    # Retargeting uses the destination machine's default allocation profile.
    with open(os.path.join(s.state, ".ECCE", "MachPrefs"), "w") as h:
        h.write("localhost:\t%s|ssh|normal|%s|%s||1|1|60|0|0|1\n"
                % (s.user(), rundir, rundir))
    deck = os.path.join(REPO, "tests", "apps", "fixtures", "calc-water-opt", "Inputs", "nwch.nw")

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        def run(*argv, env=None):
            r = subprocess.run([exe] + list(argv), env=s.env(env),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=300)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        empty = tempfile.mkdtemp(prefix="ecce-rerun-nosession-", dir=s.state)
        try:
            rc, out, err = run("a", env={"ECCE_SESSION_ID": "0123456789abcdef",
                                         "ECCE_REALUSERHOME": empty})
            check(rc == 2 and "ECCE is not running: start ECCE first" in err,
                  "no session: exit 2 and the reason")
        finally:
            shutil.rmtree(empty)
        check(all(run(*a)[0] == 64 for a in ((), ("launch",), ("on=localhost",),
                                             ("--bad",), ("on=", "a"))),
              "invalid usage: exit 64")

        # Install complete setup fixtures before the Organizer reads them.
        urls = {}
        fixture = os.path.join(REPO, "tests", "apps", "fixtures", "calc-water-opt")
        for name in ("killed", "completed", "failed", "running", "submitted", "waiting"):
            url, out = s.create(name, "nwchem_es", deck, "nwch.nw", rundir)
            if not check(url is not None, "created " + name):
                say(out)
                return 1
            path = url[len("file://"):]
            shutil.rmtree(path)
            shutil.copytree(fixture, path)
            rc, out = s.driver("mdsetup", url, "stored", rundir, s.user(),
                               "queue=normal", "procs=1", "nodes=1", "wall=0 1:0")
            check(rc == 0, "launch settings stored for " + name)
            urls[name] = url

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        olog = os.path.join(s.state, "organizer.log")
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=120):
            seen = len(open(olog, errors="replace").read().splitlines())
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

        check("selected=" in send("tree"), "the Organizer is up")
        for name, url in urls.items():
            check(send("setstate %s %s" % (url, name)) == "ok", "state set: " + name)

        def go(*argv):
            end = time.time() + 120
            while True:
                rc, out, err = run(*argv)
                if rc != 2 or time.time() > end:
                    return rc, out, err
                time.sleep(3)

        def path(url):
            return url[len("file://"):]

        def snapshot(url, sub=""):
            root = os.path.join(path(url), sub)
            result = {}
            for folder, _, files in os.walk(root):
                for name in files:
                    if name.endswith(".lock"):
                        continue
                    file = os.path.join(folder, name)
                    result[os.path.relpath(file, root)] = hashlib.sha256(open(file, "rb").read()).hexdigest()
            return result

        originals = {name: snapshot(url) for name, url in urls.items()}
        rc, out, err = go(path(urls["killed"]), urls["completed"], urls["failed"])
        say(out + err)
        check(rc == 0 and out.splitlines() == ["%s: rerun as %s_r2" % (n, n)
                                               for n in ("killed", "completed", "failed")],
              "killed, completed and failed: one successful line each")
        for name in ("killed", "completed", "failed"):
            source = urls[name]
            copy = source + "_r2"
            check(s.state_of(copy) == "ready", name + " copy is Ready")
            check(snapshot(copy, "Parameters") == snapshot(source, "Parameters"),
                  name + " structure, basis and setup values preserved")
            for prop in ("application", "theory", "runtype"):
                rc, original = s.driver("meta", source, prop)
                rc2, copied = s.driver("meta", copy, prop)
                check(rc == rc2 == 0 and original.strip() and original == copied,
                      name + " same " + prop)
            check(s.driver("launchdata", copy) == s.driver("launchdata", source),
                  name + " same launch settings")
            rc, props = s.driver("props", copy)
            check(rc == 0 and not props.strip(), name + " copy has no old results")
            check(snapshot(source) == originals[name], name + " original untouched")
            check(s.state_of(source) == name, name + " original state unchanged")

        rc, out, err = go(urls["killed"])
        check(rc == 0 and out.strip() == "killed: rerun as killed_r3",
              "name collision takes _r3")

        rc, out, err = go("on=localhost", "launch", urls["completed"])
        say(out + err)
        check(rc == 0 and re.fullmatch(r"completed: rerun as completed_r3, job id \S+\n", out),
              "launch on localhost prints copy and job id")
        copy = urls["completed"] + "_r3"
        state = s.waitState(copy, seconds=120, want=("running",) + harness.FINAL_STATES)
        check(state == "running", "the new copy runs (%s)" % state)
        rc, settings = s.driver("launchdata", copy)
        check("machine=localhost" in settings, "on= overrides the stored machine")
        check(snapshot(urls["completed"]) == originals["completed"],
              "launch left original untouched")
        check(s.state_of(urls["completed"]) == "completed",
              "launch left original Completed")

        # A failed submission still identifies the new Ready copy.
        rc, out, err = go("on=no-such-machine", "launch", urls["failed"])
        check(rc == 1 and "failed: rerun as failed_r3, not launched:" in out and
              s.state_of(urls["failed"] + "_r3") == "ready",
              "launch failure reports and keeps the Ready copy")

        rc, out, err = go(urls["running"], urls["submitted"], urls["waiting"], urls["failed"])
        say(out + err)
        check(rc == 1 and len(out.splitlines()) == 4 and
              all(n + ": not rerun: state is " in out for n in ("running", "submitted", "waiting"))
              and "failed: rerun as failed_r4" in out,
              "active states refused with reasons; another item still copied")
        for name in ("running", "submitted", "waiting"):
            check(not os.path.exists(path(urls[name]) + "_r2") and
                  snapshot(urls[name]) == originals[name], name + " unchanged")
        parent = os.path.dirname(path(urls["killed"]))
        rc, out, err = go(parent)
        check(rc == 1 and "not a calculation" in out, "folder refused with reason")
        for arg in (work, "file://" + work):
            rc, out, err = go(arg)
            check(rc == 1 and "not rerun:" in out and org.poll() is None,
                  "outside-data argument refused: " + arg)
        check(send("tree").startswith("ok"), "Organizer still answers")

        # --last-geometry: the copy starts from the last trace step.
        def trace_last(url):
            text = open(os.path.join(path(url), "Props", "GEOMTRACE")).read()
            step = re.findall(r"<step[^>]*>(.*?)</step>", text, re.S)[-1]
            return [float(v) for v in step.split()]

        def mvm_coords(url):
            lines = open(os.path.join(path(url), "Parameters", "chemsys.mvm")).read().splitlines()
            at = lines.index("atom_list: ") + 1
            n = int([l for l in lines if l.startswith("num_atoms:")][0].split()[1])
            return [float(v) for l in lines[at:at + n] for v in l.split()[1:4]]

        want = trace_last(urls["killed"])
        before = mvm_coords(urls["killed"])
        check(max(abs(a - b) for a, b in zip(want, before)) > 1e-3,
              "fixture: the last trace step differs from the input structure")
        rc, out, err = go("--last-geometry", "cores=4", "wall=1 2:30", urls["killed"])
        say(out + err)
        m = re.fullmatch(r"killed: rerun as (killed_r\d+)\n", out)
        check(rc == 0 and m is not None, "--last-geometry: one successful line")
        if m:
            copy2 = os.path.join(os.path.dirname(urls["killed"]), m.group(1))
            got = mvm_coords(copy2)
            check(len(got) == len(want) and max(abs(a - b) for a, b in zip(want, got)) < 1e-4,
                  "copy's structure equals the last trace step")
            check(s.state_of(copy2) == "ready", "last-geometry copy is Ready")
            deck_atoms = []
            deck_text = open(os.path.join(path(copy2), "Inputs", "nwch.nw")).read()
            in_geom = False
            for l in deck_text.splitlines():
                w = l.split()
                if w and w[0].lower() == "geometry":
                    in_geom = True
                elif in_geom and w and w[0].lower() == "end":
                    break
                elif in_geom and len(w) == 4:
                    deck_atoms += [float(v) for v in w[1:]]
            check(len(deck_atoms) == len(want) and
                  max(abs(a - b) for a, b in zip(want, deck_atoms)) < 1e-4,
                  "copy's input deck holds the last trace geometry")
            # ecce-launch refuses a Created calculation.
            check(send("setstate %s created" % copy2) == "ok", "copy set to Created")
            lx = os.path.join(build, "ecce-launch")
            r = subprocess.run([lx, copy2], env=s.env(), stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, timeout=120)
            lout = r.stdout.decode("utf-8", "replace")
            check(r.returncode == 1 and "not ready: state Created" in lout,
                  "ecce-launch refuses a Created calculation: " + lout.strip())
            check(send("setstate %s ready" % copy2) == "ok", "copy set back to Ready")
            rc, settings = s.driver("launchdata", copy2)
            say(settings)
            check(re.search(r"(procs|totalprocs)\D*=\s*4\b", settings) is not None and
                  "wall=0 1:0" not in settings and "1 2:30" in settings.replace("wall=", ""),
                  "cores= and wall= reach the copy's launch settings")
            rc, ann = s.driver("meta", copy2, "annotation")
            check("continued from killed (last geometry)" in ann, "annotation names the original")
            check(snapshot(urls["killed"]) == originals["killed"], "original untouched")

        rc, out, err = go("cores=2", "wall=0:45", urls["failed"])
        m = re.fullmatch(r"failed: rerun as (failed_r\d+)\n", out)
        check(rc == 0 and m is not None, "cores=/wall= without --last-geometry")
        if m:
            rc, settings = s.driver("launchdata", urls["failed"] + m.group(1)[len("failed"):])
            check(re.search(r"(procs|totalprocs)\D*=\s*2\b", settings) is not None,
                  "cores= applied to a plain rerun copy")
        rc, out, err = go("wall=soon", urls["failed"])
        check(rc == 1 and "not rerun: wall time" in out, "bad wall= refused")
        check(all(run(*a)[0] == 64 for a in (("cores=0", "a"), ("cores=x", "a"), ("wall=", "a"))),
              "bad cores=/wall= usage: exit 64")

        # A running job with no stored geometry is refused and keeps running.
        check(s.state_of(copy) == "running", "the launched copy still runs")
        names = set(os.listdir(os.path.dirname(path(copy))))
        rc, out, err = go("--last-geometry", copy)
        check(rc == 1 and "not rerun: no stored geometry yet" in out and
              set(os.listdir(os.path.dirname(path(copy)))) == names and
              s.state_of(copy) == "running",
              "no stored geometry: clear error, no copy, job not terminated")

        # With a trace present, the running job is ended and continued.
        os.makedirs(os.path.join(path(copy), "Props"), exist_ok=True)
        shutil.copy(os.path.join(path(urls["killed"]), "Props", "GEOMTRACE"),
                    os.path.join(path(copy), "Props", "GEOMTRACE"))
        rc, out, err = go("--last-geometry", copy)
        say(out + err)
        check(rc == 0 and re.fullmatch(r"completed_r3: rerun as completed_r3_r\d+\n", out) is not None,
              "running job with a trace: terminated and continued")
        state = s.waitState(copy, seconds=60, want=harness.FINAL_STATES)
        check(state == "killed", "the running original was terminated (%s)" % state)
        s.driver("kill", copy)
    finally:
        if org and org.poll() is None:
            org.terminate()
        if display:
            display.__exit__(None, None, None)
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
