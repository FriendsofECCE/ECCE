#!/usr/bin/env python3
"""Project-table multi-selection (#925) and the workflow folder's table
and boxes (#951), headless.

Clicks go through the grid window's own handlers (hook "tableclick ROW
[ctrl] [shift]"): a click selects one row, Shift+click extends or shrinks a
range from the anchor, Ctrl+click toggles a row, and an automatic reload
keeps the rows and the Shift anchor.  Exit 77 (skip) without an install or
Xvfb.
"""
import os
import re
import subprocess
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
SRC = os.path.join(fixture.FIXTURES, "calc-water-opt")
NAMES = ["c1", "c2", "c3", "c4"]
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + str(detail)) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-tsel-data-")
    work = tempfile.mkdtemp(prefix="ecce-tsel-")
    try:
        project = os.path.join(data, "users", "local", "tsel")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        for n in NAMES:
            shutil.copytree(SRC, os.path.join(project, n), symlinks=True)
        u = "file://" + project
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def send(*lines):
            with open(cmds, "a") as h:
                for line in lines:
                    h.write(line + "\n")
            time.sleep(1.5)

        def drive(_display):
            send("select " + u)
            time.sleep(3)
            # Which calculation is in which row.
            send("tableclick 0", "tableclick 1", "tableclick 2", "tableclick 3")
            send("tableclick 0")
            send("tableclick 2 shift")      # extend 0..2
            send("tableclick 1 shift")      # shrink to 0..1, inside the selection
            send("tableclick 3 ctrl")       # add 3
            send("tableclick 1 ctrl")       # remove 1
            send("tableclick 0 ctrl", "tableclick 1 ctrl")   # now rows 1 and 3
            # A reload of the shown folder (#889) with nothing changed.
            stray = os.path.join(project, "stray")
            open(stray, "w").close()
            os.remove(stray)
            time.sleep(6)
            send("autoreloads x", "tableselected x")
            send("tableclick 3 shift")      # from the anchor, row 1
            time.sleep(2)
            # Anchor on row 2, Ctrl+click it off, reload, Shift+click row 3.
            # The reload re-sorts the rows; "table" reports the new order.
            send("tableclick 2")
            send("tableclick 2 ctrl")
            open(stray, "w").close()
            os.remove(stray)
            time.sleep(6)
            send("autoreloads x", "table x", "tableselected x")
            send("tableclick 3 shift")
            time.sleep(2)

        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": u,
                               "ECCE_LOCAL_DATA": data}, inspect=drive)
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return

        def ans(word):
            return re.findall(r"ECCE_TEST_ORGANIZER: %s .*?: (.*)$" % word, log, re.M)
        clicks = ans("tableclick")
        if not check(len(clicks) == 15 and all(c.startswith("ok") for c in clicks),
                     "the click hook answered", log[-2000:]):
            return

        def names(answer):
            return sorted(os.path.basename(x.rstrip("/")) for x in answer.split()[1:])
        row = [names(c) for c in clicks[:4]]
        if not check(all(len(r) == 1 for r in row) and sorted(sum(row, [])) == NAMES,
                     "a click selects that row alone", row):
            return
        row = [r[0] for r in row]

        def rows(*idx):
            return sorted(row[i] for i in idx)
        steps = [("a click selects one row", rows(0)),
                 ("Shift+click extends a range", rows(0, 1, 2)),
                 ("Shift+click inside the selection shrinks the range", rows(0, 1)),
                 ("Ctrl+click adds a row", rows(0, 1, 3)),
                 ("Ctrl+click on a selected row removes it", rows(0, 3)),
                 ("Ctrl+click toggles off", rows(3)),
                 ("two rows apart are selected", rows(1, 3))]
        for (what, want), got in zip(steps, clicks[4:11]):
            check(names(got) == want, what, "got %s, want %s" % (names(got), want))
        reloads, kept = ans("autoreloads"), ans("tableselected")
        check(reloads and int(reloads[0]) >= 1, "the folder was reloaded", reloads)
        check(kept and names(kept[0]) == rows(1, 3),
              "the reload keeps both rows", kept)
        # The anchor survives a refill (#925): it stays on its calculation,
        # wherever the re-sort puts it, instead of falling back to row 0.
        check(names(clicks[13]) == [], "Ctrl+click clears the only selected row",
              "got %s" % names(clicks[13]))
        # Nothing selected before the reload, so nothing after it (#939).
        check(len(kept) == 2 and names(kept[1]) == [],
              "a reload keeps an emptied selection empty", kept)
        anchor = names(clicks[12])[0]
        table = ans("table")
        order = [c.split("=")[0].strip() for c in table[-1].split("|")[1:]] \
            if table else []
        if check(sorted(order) == NAMES, "the reloaded table lists the rows", table):
            a = order.index(anchor)
            want = sorted(order[min(a, 3):max(a, 3) + 1])
            check(names(clicks[14]) == want,
                  "Shift+click after an auto-refresh extends from the anchor",
                  "got %s, want %s (rows %s, anchor %s)"
                  % (names(clicks[14]), want, order, anchor))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def run_workflow(display):
    """A workflow folder's table and its boxes (#951), with real X clicks on
    the test's own Xvfb display: a click selects one step, Ctrl+click adds
    and removes, Shift+click selects a range, and the tools (Energy
    Calculator) act on what the boxes show.  The steps are made in an order
    other than the table's."""
    if not shutil.which("xdotool"):
        print("SKIP workflow table: no xdotool")
        return
    env = display.env()
    if env.get("DISPLAY", ":0") in (":0", ":0.0", os.environ.get("ECCE_LIVE_DISPLAY", "-")):
        check(False, "a test display of its own", env.get("DISPLAY"))
        return
    data = tempfile.mkdtemp(prefix="ecce-wsel-data-")
    work = tempfile.mkdtemp(prefix="ecce-wsel-")
    try:
        project = os.path.join(data, "users", "local", "wsel")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        shutil.copytree(SRC, os.path.join(project, "water"), symlinks=True)
        u = "file://" + project
        wf = u + "/Workflow"
        cmds = os.path.join(work, "commands")
        answers = os.path.join(work, "answers")
        open(cmds, "w").close()
        open(answers, "w").close()
        got = []

        def send(line, wait=1.5):
            n = len(open(answers).read().splitlines())
            with open(cmds, "a") as h:
                h.write(line + "\n")
            for _ in range(int(wait * 20) + 100):
                for l in open(answers).read().splitlines()[n:]:
                    if l.startswith(line + ": "):
                        got.append(l)
                        return l[len(line) + 2:]
                time.sleep(0.05)
            got.append(line + ": (no answer)")
            return ""

        def click(xy, *keys):
            x, y = xy
            seq = ["xdotool", "mousemove", "--sync", str(x), str(y)]
            for k in keys:
                seq += ["keydown", k]
            seq += ["click", "1"]
            for k in keys:
                seq += ["keyup", k]
            subprocess.run(seq, env=env, timeout=20)
            time.sleep(1.0)

        def names(answer):
            return sorted(os.path.basename(x.rstrip("/"))
                          for x in answer.split()[1:])

        def drive(_display):
            for line in ["new %s workflow" % u,
                         "workflowcopy %s %s/water seed" % (wf, u),
                         "workflowstep %s xtb_es zeta %s/seed" % (wf, wf),
                         "workflowstep %s xtb_es alpha %s/seed" % (wf, wf),
                         "workflowstep %s xtb_es mid %s/seed" % (wf, wf)]:
                send(line, 10)
            send("select " + wf, 5)
            time.sleep(3)
            rows = []
            for i in range(4):
                a = send("tablerow %d" % i).split()
                if len(a) == 4 and a[0] == "ok":
                    rows.append(((int(a[1]), int(a[2])), os.path.basename(a[3].rstrip("/"))))
            out = {"rows": [r[1] for r in rows]}
            if len(rows) != 4:
                return out
            pos = [r[0] for r in rows]
            res = []

            def state():
                res.append((names(send("tableselected x")), names(send("toolselection x"))))
            click(pos[0]); state()
            click(pos[2], "ctrl"); state()
            click(pos[3], "ctrl"); state()
            click(pos[2], "ctrl"); state()
            click(pos[1]); state()
            click(pos[3], "shift"); state()
            click(pos[0]); state()
            out["table"] = res
            # The boxes, by step name, with the view tall enough to show them.
            send("workflowsash 700", 3)
            time.sleep(1)
            box = {}
            # The third row of boxes is below a 1024x768 screen.
            for n in ("seed", "zeta", "alpha", "mid"):
                a = send("boxpoint %s/%s" % (wf, n)).split()
                if len(a) == 3 and a[0] == "ok":
                    box[n] = (int(a[1]), int(a[2]))
            out["box"] = {}
            if len(box) == 4:
                bres = []

                def bstate():
                    bres.append((names(send("tableselected x")), names(send("toolselection x"))))
                click(box["zeta"]); bstate()
                click(box["alpha"], "ctrl"); bstate()
                click(box["seed"], "ctrl"); bstate()
                click(box["alpha"], "ctrl"); bstate()
                click(box["seed"]); bstate()
                out["boxes"] = bres
                out["box"] = box
            return out

        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_TEST_ORGANIZER_ANSWERS": answers,
                               "ECCE_ORGANIZER_OPEN": u,
                               "ECCE_LOCAL_DATA": data}, inspect=drive)
        if result.crashed:
            check(False, "the Organizer ran (workflow)", "CRASHED (%s)" % result.signalName)
            return
        out = result.inspected or {}
        row = out.get("rows", [])
        if not check(sorted(row) == ["alpha", "mid", "seed", "zeta"],
                     "the workflow table lists the steps", "\n".join(got[-20:])):
            return
        check(row != ["seed", "zeta", "alpha", "mid"],
              "the table is not in creation order", row)

        def rows(*idx):
            return sorted(row[i] for i in idx)
        want = [("click selects that row", rows(0)),
                ("Ctrl+click adds a row", rows(0, 2)),
                ("Ctrl+click adds another", rows(0, 2, 3)),
                ("Ctrl+click removes a row", rows(0, 3)),
                ("a click selects that row alone", rows(1)),
                ("Shift+click selects the range", rows(1, 2, 3)),
                ("a click after a range selects one row", rows(0))]
        for (what, w), (t, tools) in zip(want, out.get("table", [])):
            check(t == w, "workflow table: " + what, "got %s, want %s" % (t, w))
            check(tools == w, "workflow table: tools act on it: " + what,
                  "got %s, want %s" % (tools, w))
        check(len(out.get("table", [])) == len(want), "the table clicks were answered",
              "\n".join(got[-30:]))
        bwant = [("a box click selects that step", ["zeta"]),
                 ("Ctrl+click on a box adds it", ["alpha", "zeta"]),
                 ("Ctrl+click on another adds it", ["alpha", "seed", "zeta"]),
                 ("Ctrl+click on a picked box removes it", ["seed", "zeta"]),
                 ("a plain box click selects it alone", ["seed"])]
        for (what, w), (t, tools) in zip(bwant, out.get("boxes", [])):
            check(t == w, "boxes: " + what + " (table)", "got %s, want %s" % (t, w))
            check(tools == w, "boxes: " + what + " (Energy Calculator)",
                  "got %s, want %s" % (tools, w))
        check(len(out.get("boxes", [])) == len(bwant), "the box clicks were answered " + str(out.get("box")),
              "\n".join(got[-30:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        run_workflow(display)
        if failures:
            results.fail("tableselection", "table selection wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
