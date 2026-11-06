#!/usr/bin/env python3
"""Several stored calculations into one workflow, with links (#874).

Five stored SN2 example calculations (reactant, scan, product, QST3, IRC)
are copied into one folder.  The proposed links follow the structures:
reactant > scan > product and QST3 > IRC, each a "geometry" link.  The
project table's menu offers Make Workflow from Selected...; that action
(its dialog answered Move) moves reactant, scan and product into a new
workflow with both links, and the boxes keep their Loaded state.  Then
ecce-workflow create moves QST3 and IRC into a second workflow with
--auto-links, a copy of the product into the first workflow (a link from its
scan step), and two more copies with an explicit --link.  Refusals: a step
already in a workflow, two folders, a link to an unknown name; no session
(exit 2) and wrong usage (64).

    tests/launch/ecce_workflow_create_test.py [--build DIR] [--keep] [--shots DIR]

--shots keeps the link dialog and the workflow canvas as PNGs.  Exit 77
(CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import os
import re
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

SN2 = os.path.join(REPO, "data", "client", "examples", "workflows", "sn2-cl-ch3br")
PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
STEPS = ("reactant", "scan", "product", "qst3", "irc")

failures = []


def check(ok, what, detail=""):
    say("  %s %s%s" % ("ok  " if ok else "FAIL", what,
                       ("\n        " + str(detail)) if detail and not ok else ""))
    if not ok:
        failures.append(what)
    return ok


def links_of(info):
    return set(info.split(" links:")[1].split()) if " links:" in info else set()


def steps_of(info):
    table = {}
    if info.startswith("ok "):
        info = info[3:]
    for entry in info.split(" links:")[0].split("|"):
        f = entry.split()
        if len(f) >= 4:
            table[f[0]] = {"state": f[2], "source": f[3]}
    return table


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shots")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    wfexe = os.path.join(build, "ecce-workflow")
    if not os.access(wfexe, os.X_OK):
        say("FAIL: ecce-workflow is not built")
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-workflow-create", {}, local=True,
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
    shots = args.shots or os.path.join(s.state, "shots")
    os.makedirs(shots, exist_ok=True)

    home = os.path.join(s.localData(), "users", "local")
    proj = os.path.join(home, "imp")
    other = os.path.join(home, "other")
    for d in (proj, other):
        os.makedirs(d, exist_ok=True)
        shutil.copy(PROJECT_META, os.path.join(d, ".ecce-meta"))
    calcs = {}
    for step in STEPS:
        stored = os.path.join(SN2, "g16-" + step, "stored", "sn2_" + step)
        calcs[step] = os.path.join(proj, "sn2_" + step)
        shutil.copytree(stored, calcs[step], symlinks=True)
    for copy, step in (("p2", "product"), ("r3", "reactant"), ("i3", "irc"),
                       ("q4", "qst3")):
        shutil.copytree(os.path.join(SN2, "g16-" + step, "stored", "sn2_" + step),
                        os.path.join(proj, copy), symlinks=True)
    shutil.copytree(os.path.join(SN2, "g16-irc", "stored", "sn2_irc"),
                    os.path.join(other, "elsewhere"), symlinks=True)
    U = lambda p: "file://" + p

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        def run(*argv, env=None):
            r = subprocess.run([wfexe] + list(argv), env=s.env(env),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=300)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        empty = tempfile.mkdtemp(prefix="ecce-wfcreate-nosession-", dir=s.state)
        try:
            rc, out, err = run("create", "x", calcs["qst3"],
                               env={"ECCE_SESSION_ID": "0123456789abcdef",
                                    "ECCE_REALUSERHOME": empty})
            check(rc == 2 and "ECCE is not running" in err,
                  "create without a session: exit 2", (rc, err))
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(run("create")[0] == 64 and run("create", "x")[0] == 64 and
              run("create", "x", "a", "--bogus")[0] == 64,
              "create without calculations or with an unknown option: 64")

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
        all5 = " ".join(U(calcs[k]) for k in STEPS)
        got = send("workflowpropose " + all5)
        proposed = set(x for x in got[3:].split(",") if x) if got.startswith("ok ") else set()
        want = {"sn2_reactant>sn2_scan:geometry:geometry",
                "sn2_scan>sn2_product:geometry:geometry",
                "sn2_qst3>sn2_irc:geometry:geometry"}
        check(proposed == want,
              "links proposed from final to starting structures", got)

        rsp = " ".join(U(calcs[k]) for k in ("reactant", "scan", "product"))
        menu = send("tablecontextmenu " + rsp)
        labels = [x.lstrip("~") for x in menu.split("|")]
        check("Make Workflow from Selected..." in labels and
              "~Make Workflow from Selected..." not in menu,
              "the table menu offers Make Workflow from Selected...", menu)
        check("Move Selected into Workflow" in labels,
              "and Move Selected into Workflow", menu)

        dialog_png = os.path.join(shots, "link-dialog.png")
        got = send("workflowcreatedialog %s new %s" % (dialog_png, rsp))
        check(got.startswith("ok ") and
              "sn2_reactant → sn2_scan (geometry)" in got and
              "sn2_scan → sn2_product (geometry)" in got and
              os.path.exists(dialog_png),
              "the dialog lists both links and is saved", got)

        got = send("workflowcreatemenu new " + rsp)
        # Named after the first of the rows in the table's order.
        m = re.fullmatch(r"ok (file://\S+/(sn2_\w+_workflow)) members=3", got)
        check(m is not None, "the menu action made the workflow", got)
        wf, wfname = (m.group(1), m.group(2)) if m else ("none", "none")
        check(all(os.path.isdir(os.path.join(proj, wfname, "sn2_" + k))
                  and not os.path.exists(calcs[k])
                  for k in ("reactant", "scan", "product")),
              "the three calculations moved into it on disk")
        info = send("workflowinfo " + wf)
        table = steps_of(info)
        check(set(table) == {"sn2_reactant", "sn2_scan", "sn2_product"} and
              all(v["state"] == "Loaded" for v in table.values()),
              "three boxes, each with its stored state", info)
        check(links_of(info) == {"sn2_reactant>sn2_scan:geometry",
                                 "sn2_scan>sn2_product:geometry"},
              "both geometry links stored", info)
        check(table.get("sn2_scan", {}).get("source") == "sn2_reactant" and
              table.get("sn2_product", {}).get("source") == "sn2_scan",
              "each box drawn from its source", info)
        canvas_png = os.path.join(shots, "workflow-canvas.png")
        got = send("workflowshot %s %s" % (canvas_png, wf))
        check(got.startswith("ok") and "label-overlaps=0" in got and
              os.path.exists(canvas_png), "the canvas is drawn with clear labels", got)

        def go(*argv):
            end = time.time() + 120
            while True:
                rc, out, err = run(*argv)
                if rc != 2 or time.time() > end:
                    return rc, out, err
                time.sleep(3)

        rc, out, err = go("create", "ts_wf", calcs["qst3"], calcs["irc"], "--auto-links")
        ts = U(os.path.join(proj, "ts_wf"))
        check(rc == 0 and out.startswith("ok " + ts) and
              "links=sn2_qst3>sn2_irc:geometry" in out,
              "ecce-workflow create --auto-links: exit 0 and the link", (rc, out, err))
        info = send("workflowinfo " + ts)
        check(links_of(info) == {"sn2_qst3>sn2_irc:geometry"} and
              steps_of(info).get("sn2_irc", {}).get("state") == "Loaded",
              "the second workflow holds the link and the states", info)

        rc, out, err = go("create", os.path.join(proj, wfname),
                          os.path.join(proj, "p2"), "--auto-links")
        check(rc == 0 and "links=sn2_scan>p2:geometry" in out,
              "into an existing workflow: linked from its own step", (rc, out, err))

        rc, out, err = go("create", "manual", os.path.join(proj, "r3"),
                          os.path.join(proj, "i3"), "--link", "r3:i3")
        check(rc == 0 and out.strip().endswith("links=r3>i3:geometry"),
              "--link makes the link given", (rc, out, err))
        info = send("workflowinfo " + U(os.path.join(proj, "manual")))
        check(links_of(info) == {"r3>i3:geometry"}, "and stores it", info)

        rc, out, err = go("create", "again",
                          os.path.join(proj, "ts_wf", "sn2_irc"))
        check(rc == 1 and "already in a workflow" in out,
              "a step of a workflow is refused", (rc, out))
        rc, out, err = go("create", "two", os.path.join(other, "elsewhere"),
                          os.path.join(proj, "q4"))
        check(rc == 1 and "one folder" in out and os.path.isdir(os.path.join(proj, "q4")),
              "calculations from two folders are refused", (rc, out))
        rc, out, err = go("create", "bad", os.path.join(other, "elsewhere"),
                          "--link", "elsewhere:nobody")
        check(rc == 1 and "link" in out and
              os.path.isdir(os.path.join(other, "elsewhere")) and
              not os.path.exists(os.path.join(other, "bad")),
              "a link to an unknown name is refused before anything moves", (rc, out))
        check(send("tree").startswith("ok"), "the Organizer still answers")
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
