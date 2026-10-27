#!/usr/bin/env python3
"""
Out of contact and the folding Progress box (#284), headless.

A local-data project with two running calculations, one of them stored as
out of contact (ecce:contact, what a restarted Organizer reads), is opened
through the ECCE_TEST_ORGANIZER hook.  "contact <url> lost <epoch>|ok" hands
the Organizer the job store's notice; "progressbox", "treestate" and
"table" print what is drawn; "snap" saves the window:

- out of contact: the summary's state icon and the tree icon are the amber
  "running?" ring, the box says when the job was last heard from, greys the
  progress and shows its three buttons, and a machine check's lines;
- folded: one line, amber "no contact for n min" without unfolding;
- contact again: back to normal by itself, folded line "Opt step 4 ..
  updated n s ago";
- the table: amber "no contact for 17 min" and the "running?" state for the
  stored case;
- the fold is remembered: a second Organizer opens the box folded.

ECCE_PROGRESS_PNGS=<dir> keeps the window captures there.  Same installed
tree and isolation as progress_box_test.py.  Exit 77 (skip) without an
install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps               # noqa: E402
import progress_box_test as box   # noqa: E402
import run_tests          # noqa: E402

NS = "http://www.emsl.pnl.gov/ecce:"
WARN = "\u26a0"
DOT = "\u00b7"
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def field(answer, key):
    m = re.search(r"\b%s=(\S+)" % key, answer)
    return m.group(1) if m else None


def lines(answer):
    return answer.split(" | ")[1:]


def organizer(display, cmds, data, opened):
    result = apps.run(display, "organizer", windowTimeout=60, settle=45,
                      env={"ECCE_TEST_ORGANIZER": cmds,
                           "ECCE_ORGANIZER_OPEN": "file://" + opened,
                           "ECCE_LOCAL_DATA": data})
    if result.crashed:
        check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
    return result.log or ""


def run(display, pngs):
    data = tempfile.mkdtemp(prefix="ecce-contact-data-")
    work = tempfile.mkdtemp(prefix="ecce-contact-")
    try:
        project = os.path.join(data, "users", "local", "contact-test")
        os.makedirs(project)
        shutil.copy(box.PROJECT_META, os.path.join(project, ".ecce-meta"))
        now = time.time()
        stamp = {"sent": now - 20, "at": int(now - 20)}
        running = box.makeCalc(project, "water-running", "Running",
                               box.STORED.format(**stamp))
        stored = box.makeCalc(project, "water-lost", "Running",
                              box.STORED.format(**stamp))
        with open(os.path.join(stored, ".ecce-meta"), "a") as handle:
            handle.write(".\t%scontact\t\tlost %d\n" % (NS, int(now - 17 * 60 - 5)))
        # Queued (#606): submitted, id known, no snapshot.
        queued = box.makeCalc(project, "water-queued", "Submitted", "")
        with open(os.path.join(queued, ".ecce-meta"), "a") as handle:
            handle.write(".\t%ssubmitdate\t\t%d\n" % (NS, int(now - 300)))
            handle.write(".\t%sjob_jobid\t\t4711\n" % NS)
        # First in the table, which selects its first row: the amber cell
        # is drawn in an unselected row.
        box.makeCalc(project, "water-done", "Complete", box.DONE.format(**stamp))
        png = lambda n: os.path.join(pngs, n)
        url = "file://" + running
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("\n".join([
                "size 1100 1000",
                "summary " + url,
                "progressbox",
                "treestate " + url,
                "contact %s lost %d" % (url, int(now - 600)),
                "progressbox",
                "treestate " + url,
                "snap " + png("contact-lost.png"),
                "checklines %s Checked at 14:22:|Process 4242 is still on the "
                "machine.|Output ecceqm.qmout last written 14:12 (10 min ago), "
                "2010 bytes." % url,
                "progressbox",
                "snap " + png("contact-lost-checked.png"),
                "fold shut",
                "progressbox",
                "snap " + png("contact-lost-folded.png"),
                "contact %s ok" % url,
                "progressbox",
                "treestate " + url,
                "snap " + png("contact-back-folded.png"),
                "table file://" + project,
                "tablestates",
                "snap " + png("contact-table.png"),
                "summary file://" + stored,
                "progressbox",
                "fold open",
                "summary file://" + queued,
                "progressbox",
                "contact file://%s ok %d" % (queued, int(now - 20)),
                "progressbox",
                "contact file://%s lost %d" % (queued, int(now - 240)),
                "progressbox",
                "snap " + png("contact-queued-lost.png"),
                "fold shut",
            ]) + "\n")
        log = organizer(display, cmds, data, running)
        boxes = box.answers(log, "progressbox")
        trees = re.findall(r"ECCE_TEST_ORGANIZER: treestate.*?: (image=.*)", log)
        tables = re.findall(r"ECCE_TEST_ORGANIZER: table.*?: (panel=.*)", log)
        states = box.answers(log, "tablestates")
        if not check(len(boxes) == 9 and len(trees) == 3 and len(tables) == 1
                     and len(states) == 1,
                     "every command answered (%d boxes, %d trees, %d tables, %d states)"
                     % (len(boxes), len(trees), len(tables), len(states)),
                     run_tests._tail(log, 30)):
            return
        for b in boxes + trees + tables + states:
            print("      " + b)
        before, lost, checked, folded, back, fromStored, qBox, qChecked, qLost = boxes

        check(field(before, "stateicon") == "plain" and
              "heading open" in lines(before) and
              not any(l.startswith("contact") for l in lines(before)),
              "in contact: the box open, plain state icon", before)
        t0, t1, t2 = trees
        check(field(t0, "image") == field(t0, "running") and field(t0, "outOfContact") == "0",
              "in contact: the tree shows running", t0)

        check(field(lost, "stateicon") == "running?", "lost: the summary's icon is running?", lost)
        check(field(t1, "image") == field(t1, "lost") and field(t1, "lost") != field(t1, "running"),
              "lost: the tree icon is the out-of-contact ring", t1)
        check(any(re.match(r"contact lost, last heard \d\d:\d\d \(10 min ago\)", l)
                  for l in lines(lost)), "lost: last heard hh:mm (10 min ago)", lost)
        check("buttons 3" in lines(lost), "lost: three action buttons shown", lost)
        check(any(l.startswith("(grey) Optimisation: step 4") for l in lines(lost)) and
              not any(l.startswith("updated") for l in lines(lost)),
              "lost: the progress greyed, no 'updated'", lost)
        check("check: Process 4242 is still on the machine." in lines(checked),
              "the machine check's lines in the box", checked)

        check("heading folded" in lines(folded) and
              "folded: %s no contact for 10 min" % WARN in lines(folded),
              "folded while lost: one amber line, not opened", folded)
        check("buttons 0" in lines(folded), "folded: no buttons", folded)

        check(field(back, "stateicon") == "plain" and
              not any(l.startswith("contact") for l in lines(back)),
              "contact again: normal by itself", back)
        check(any(re.match(r"folded: Opt step 4 %s SCF 7 %s updated \d+ s ago" % (DOT, DOT), l)
                  for l in lines(back)), "contact again: the folded live line", back)
        check(field(t2, "image") == field(t2, "running"), "contact again: tree running", t2)

        table = tables[0]
        cells = dict(c.split("=", 1) for c in lines(table))
        stateCells = dict(c.split("=", 1) for c in lines(states[0]))
        check(cells.get("water-lost") == "%s no contact for 17 min" % WARN,
              "table: stored out of contact %r" % cells.get("water-lost"), table)
        check(cells.get("water-running") == "Opt step 4 %s SCF 7" % DOT,
              "table: in contact %r" % cells.get("water-running"), table)
        check(stateCells.get("water-lost") == "running?" and
              stateCells.get("water-running", "").lower() == "running",
              "table: state cells %r" % stateCells, table)

        check(field(fromStored, "stateicon") == "running?" and
              any("(17 min ago)" in l for l in lines(fromStored)),
              "stored ecce:contact: lost after a restart of the Organizer", fromStored)

        check(re.search(r"Status: Queued since \d\d:\d\d \(5 min ago\)", qBox) and
              "Status: Job id 4711" in lines(qBox) and
              not any("Optimisation" in l or l.startswith("updated")
                      for l in lines(qBox)) and
              field(qBox, "shown") == "1" and field(qBox, "stateicon") == "plain",
              "queued: since, job id, no scheduler state, no progress", qBox)
        check(any(re.match(r"updated (2\d|3\d) s ago", l) for l in lines(qChecked)) and
              any(l.startswith("Status: Queued since") for l in lines(qChecked)),
              "queued: the monitor's last queue check is 'updated n s ago'", qChecked)
        check(any(re.match(r"contact lost, last heard \d\d:\d\d \(4 min ago\)", l)
                  for l in lines(qLost)) and "buttons 3" in lines(qLost) and
              any(l.startswith("(grey) Status: Queued since") for l in lines(qLost)) and
              field(qLost, "stateicon") == "plain",
              "queued: no contact greys it, last heard shown", qLost)

        ini = os.path.join(os.environ.get("ECCE_REALUSERHOME", ""), ".ECCE")
        found = [os.path.join(r, f) for r, _, fs in os.walk(ini) for f in fs
                 if f == "wxorganizer.ini"]
        text = open(found[0]).read() if found else ""
        check("Collapsed=1" in text, "the fold is written to wxorganizer.ini",
              "%s: %r" % (found, text))

        # A second Organizer: the box opens folded, then unfold it again.
        with open(cmds, "w") as handle:
            handle.write("\n".join([
                "summary " + url,
                "progressbox",
                "fold open",
                "progressbox",
            ]) + "\n")
        log = organizer(display, cmds, data, running)
        again = box.answers(log, "progressbox")
        if check(len(again) == 2, "second Organizer answered", run_tests._tail(log, 30)):
            check("heading folded" in lines(again[0]),
                  "the fold is remembered by the next Organizer", again[0])
            check("heading open" in lines(again[1]), "and opens again", again[1])

        for n in ("contact-lost.png", "contact-lost-checked.png",
                  "contact-lost-folded.png", "contact-back-folded.png",
                  "contact-table.png", "contact-queued-lost.png"):
            check(os.path.exists(png(n)) and os.path.getsize(png(n)) > 1000,
                  "saved " + n)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        keep = os.environ.get("ECCE_PROGRESS_PNGS")
        pngs = keep or tempfile.mkdtemp(prefix="ecce-contact-png-")
        os.makedirs(pngs, exist_ok=True)
        try:
            run(display, pngs)
        finally:
            if not keep:
                shutil.rmtree(pngs, ignore_errors=True)
        if failures:
            results.fail("contact", "out-of-contact display wrong: %s"
                         % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
