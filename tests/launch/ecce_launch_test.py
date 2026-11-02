#!/usr/bin/env python3
"""ecce-launch (#687): the command line launches Ready calculations inside
the running Organizer's session.

Two Ready calculations (one given as a path, one as a URL) and one that is
already submitted go to `ecce-launch` against an isolated session with the
Organizer running; each gets its line and the exit status is 1. A second
call on the launched ones is refused. Without a session the command says so
and exits 2.

    tests/launch/ecce_launch_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

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
    exe = os.path.join(build, "ecce-launch")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-launch is not built in %s" % build)
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-launch", {"CP2K": "/bin/true"},
                        local=True, keep=args.keep)
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
    s.registerMachine("localhost", "Shell", {"CP2K": "/bin/true"})
    #  #741: a staged Slurm machine without the TL capability stops after the
    #  settings are worked out; its entry passes no time limit.
    s.registerMachine("notl", "Slurm", {"CP2K": "/bin/true"},
                      config={"userSubmit": "true"})
    mym = os.path.join(s.state, ".ECCE", "MyMachines")
    lines = open(mym).read().splitlines()
    with open(mym, "w") as h:
        for l in lines:
            h.write((l.replace(":TL", "") if l.startswith("notl\t") else l) + "\n")
    with open(os.path.join(s.state, ".ECCE", "MachPrefs"), "w") as h:
        for m in ("notl", "localhost"):
            rd = os.path.join(s.state, "run-" + m)
            h.write("%s:\t%s|ssh|normal|%s|%s|acct-1|1|1|60|0|0|1\n"
                    % (m, s.user(), rd, rd))
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    deck = os.path.join(work, "a.cp2kin")
    open(deck, "w").write("&GLOBAL\n  RUN_TYPE ENERGY\n&END GLOBAL\n")

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        urls = {}
        #  Made before the Organizer starts, moved into a new study folder
        #  after it has read the tree (#721).
        late_url, out = s.create("late", "cp2k_es", deck, "cp2k.cp2kin", rundir,
                                 extra=("procs=1", "queue=normal", "nodes=1",
                                        "wall=0 1:0"))
        if not check(late_url is not None, "calculation late created"):
            say(out)
            return 1
        late_dir = late_url[len("file://"):]
        late_hold = os.path.join(s.state, "late-hold")
        shutil.move(late_dir, os.path.join(late_hold))
        for name in ("one", "two", "gone"):
            #  #742: annotation= on create is stored as the Annotation.
            url, out = s.create(name, "cp2k_es", deck, "cp2k.cp2kin", rundir,
                                extra=("procs=1", "queue=normal", "nodes=1",
                                       "wall=0 1:0")
                                + (("annotation=made by create",)
                                   if name == "one" else ()))
            if not check(url is not None, "calculation %s created" % name):
                say(out)
                return 1
            urls[name] = url
        #  "gone" is submitted outside the Organizer first: not Ready any more.
        rc, out = s.launch(urls["gone"])
        st = s.waitState(urls["gone"], seconds=120,
                         want=harness.FINAL_STATES + ("submitted", "running"))
        check(st not in ("ready", "created", ""), "gone is no longer ready (%s)" % st)

        def run(*argv, **kw):
            r = subprocess.run([exe] + list(argv), env=s.env(kw.get("env")),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=120)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        #  No session: said so, nothing launched.
        empty = tempfile.mkdtemp(prefix="ecce-launch-nosession-")
        try:
            rc, out, err = run(urls["one"], env={
                "ECCE_SESSION_ID": "0123456789abcdef",
                "ECCE_REALUSERHOME": empty})
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(rc == 2 and "ECCE is not running: start ECCE first" in err,
              "without a session: exit 2 and the message (rc=%s %r)" % (rc, err))
        check(s.state_of(urls["one"]) == "ready", "nothing was launched without a session")

        def annotation(url):
            rc, out = s.driver("meta", url, "annotation")
            return out.strip() if rc == 0 else ""

        check("made by create" in annotation(urls["one"]),
              "create annotation= is stored (%r)" % annotation(urls["one"]))
        #  #742: without a session a local calculation is written through
        #  the data layer.
        annot = os.path.join(build, "ecce-annotate")
        empty2 = tempfile.mkdtemp(prefix="ecce-annotate-nosession-")
        try:
            r_ = subprocess.run([annot, urls["two"], "written offline"],
                                env=s.env({"ECCE_SESSION_ID": "0123456789abcdef",
                                           "ECCE_REALUSERHOME": empty2}),
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=120)
        finally:
            shutil.rmtree(empty2, ignore_errors=True)
        check(r_.returncode == 0 and b"annotated" in r_.stdout and
              "written offline" in annotation(urls["two"]),
              "ecce-annotate without a session writes the local calculation "
              "(rc=%s %r)" % (r_.returncode, r_.stdout + r_.stderr))

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
        import time

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

        def selected():
            m = re.search(r" selected=(\S+)", send("tree"))
            return m.group(1) if m else None

        #  #781: a launch from the command line leaves the selection alone.
        before = selected()
        check(before is not None, "the tree hook reports the selection (%s)" % before)

        #  The Organizer needs a moment to subscribe: ack arrives, or retry.
        path_one = urls["one"][len("file://"):]
        rc = out = err = None
        end = time.time() + 120
        while time.time() < end:
            rc, out, err = run(path_one, urls["two"], urls["gone"])
            if rc != 2:
                break
            time.sleep(3)
        say(out)
        say(err)
        check(rc == 1, "exit status 1 as one calculation was not launched (%s)" % rc)
        lines = out.splitlines()
        check(any(re.match(r"^one: job id \S+$", l) for l in lines),
              "path argument: 'one: job id N'")
        check(any(re.match(r"^two: job id \S+$", l) for l in lines),
              "URL argument: 'two: job id N'")
        check(any(l.startswith("gone: not launched: not ready") for l in lines),
              "submitted calculation refused with the reason")
        after = selected()
        check(after == before,
              "the Organizer's selection is unchanged (%s -> %s)" % (before, after))
        for name in ("one", "two"):
            st = s.waitState(urls[name], seconds=120,
                             want=harness.FINAL_STATES + ("submitted", "running"))
            check(st not in ("ready", "created", ""), "%s left ready: %s" % (name, st))

        rc, out, err = run(path_one, urls["two"])
        say(out)
        check(rc == 1 and out.count("not launched: not ready") == 2 and
              "job id" not in out,
              "a second call on the launched calculations is refused (rc=%s)" % rc)
        rc, out, err = run(os.path.join(s.state, "decks"))
        check(rc == 1 and "not inside the data folder" in out,
              "a path outside the data folder is refused (rc=%s)" % rc)

        #  #742: through the running Organizer, by path and by URL, with a
        #  line for a path that is no calculation.
        r_ = subprocess.run([annot, path_one, urls["two"], "through the Organizer"],
                            env=s.env(), stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=120)
        say(r_.stdout.decode())
        check(r_.returncode == 0 and r_.stdout.decode().count(": annotated") == 2 and
              all("through the Organizer" in annotation(urls[n]) for n in ("one", "two")),
              "ecce-annotate through the Organizer sets both (rc=%s)" % r_.returncode)
        r_ = subprocess.run([annot, os.path.join(s.state, "decks"), "x"],
                            env=s.env(), stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=120)
        check(r_.returncode == 1 and b"not annotated" in r_.stdout,
              "a path outside the data folder is refused by ecce-annotate")

        #  #721: a whole study copied in after the tree was read is found.
        study = os.path.join(os.path.dirname(late_dir), "newstudy")
        os.makedirs(study)
        shutil.move(late_hold, os.path.join(study, "late"))
        rc, out, err = run(os.path.join(study, "late"))
        say(out)
        check(re.search(r"^late: job id \S+$", out, re.M) is not None,
              "a calculation in a study copied in later is launched (rc=%s)" % rc)
        check(selected() == before,
              "the selection is unchanged after the re-read launch")

        #  #741: queue and wall time overrides, and what was filled in or dropped.
        for name, extra in (("noq", ("procs=1", "nodes=1", "wall=0 1:0")),
                            ("ovr", ("procs=1", "queue=normal", "nodes=1",
                                     "wall=0 1:0"))):
            url, out = s.create(name, "cp2k_es", deck, "cp2k.cp2kin", rundir,
                                extra=extra)
            if not check(url is not None, "calculation %s created" % name):
                say(out)
                return 1
            urls[name] = url
        rc, out, err = run("on=notl", urls["noq"])
        say(out)
        check("noq: note: no queue stored: using " in out,
              "a launch with no stored queue says which queue it used")
        check("does not pass a time limit: wall time 1:00 h is not applied" in out,
              "a machine without TL says the wall time is dropped")
        rc, out, err = run("on=notl", "queue=debug", "wall=0:20", urls["ovr"])
        say(out)
        rc, ld = s.driver("launchdata", urls["ovr"])
        stored = dict(l.split("=", 1) for l in ld.splitlines() if "=" in l)
        check(stored.get("queue") == "debug", "queue= is stored (%s)" % stored.get("queue"))
        #  wall= with no queue manager in the way (Shell machine, TL).
        rc, out, err = run("on=localhost", "wall=1:30", urls["ovr"])
        say(out)
        rc, ld = s.driver("launchdata", urls["ovr"])
        stored = dict(l.split("=", 1) for l in ld.splitlines() if "=" in l)
        check(stored.get("wall") == "0 1:30",
              "wall= is stored (%s)" % stored.get("wall"))
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
