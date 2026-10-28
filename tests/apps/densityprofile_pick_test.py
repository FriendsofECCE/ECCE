#!/usr/bin/env python3
"""
Density Profile / Density Map details of #580, #581 and #583, headless:

  580  atoms clicked in the viewer set the line (two) or the centre (one)
  581  the key keeps to two rows and the plot to a usable height; a molecule
       with many occupied orbitals opens on Total + the largest few
  583  the Density Map names every atom near the plane, also the
       screen-parallel plane of a molecule pointing at the viewer

  629  on a large cluster a click holds the GUI thread under a second (the
       curves are computed on a worker); needs ECCE_DP_CLUSTER=<calc folder>
       of an ORCA cluster (e.g. Nb10O28Li, ~1400 basis functions; 18 MB, too
       big to ship), else that case is skipped

    ECCE_DP_PNG=<dir>   also copy the pictures there
    ECCE_DP_REPORT=1    print the numbers but do not fail

Same installed tree and isolation as densityprofile_panel_test.py
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402
from densityprofile_panel_test import blocks   # noqa: E402

EX = os.path.join(REPO, "data", "client", "examples")


def stored(topic, name):
    d = os.path.join(EX, topic, name, "stored")
    return os.path.join(d, [x for x in os.listdir(d) if os.path.isdir(os.path.join(d, x))][0])


CALCS = {
    "acala": stored("spectroscopy/amides-and-peptides", "orca-ac_ala_nhme"),
    "hf": stored("genchem/molecular-orbitals-of-diatomics", "ecceqm-HF"),
    "lih": stored("genchem/molecular-orbitals-of-diatomics", "ecceqm-LiH"),
}
if os.environ.get("ECCE_DP_CLUSTER"):
    CALCS["cluster"] = CALCS["clusterpre"] = os.environ["ECCE_DP_CLUSTER"]
SCREEN = os.environ.get("ECCE_DP_SCREEN", "1366x768x24")
#  Longest the GUI thread may be held by one click or control (#629).
MAX_BLOCK_MS = 1000

SCENES = {
    "acala": """dpopen
dpstate
plotshot {t}-acala 0 0 Density Profile:docked
dppick 3
dpstate
dppick 1 2
dpstate
dpshow all
dpstate
dppng {t}-acala-all 1000 650
plotshot {t}-acala-all-docked 0 0 Density Profile:docked
hold 1
""",
    "hf": """dpopen
dpstate
dpselect 1
dpstate
dpselect 1 2
dpstate
hold 1
""",
    "cluster": """dpopen
dpselect 1
dpstate
dpselect 2 5
dpstate
dpshow all
dpstate
dpcancelafter 1500
dpmode atom
dpstate
dpshow total
dpselect 3
dpstate
hold 1
""",
    "clusterpre": """select 6
dpopen
dpstate
hold 1
""",
    "lih": """dmopen
dmpng {t}-lih-map 900 700
dmoffset 0.5
dmpng {t}-lih-map-offset 900 700
hold 1
""",
}


#  ECCE_DP_BEFORE=1: only pictures, with commands an older build has.
BEFORE = {
    "acala": "dpopen\ndpshow all\ndppng {t}-acala-all 1000 650\nplotshot {t}-acala-docked 0 0 Density Profile:docked\nhold 1\n",
    "lih": SCENES["lih"],
}
if os.environ.get("ECCE_DP_BEFORE"):
    SCENES = BEFORE
    CALCS = {k: v for k, v in CALCS.items() if k in BEFORE}


def state(b, key):
    return b.get("s_" + key, "")


def check(case, log):
    p = []
    opened = blocks(log, "dpopen")
    seq = blocks(log, "dpstate")
    if case == "acala":
        first = opened[0]
        if len(first["curves"]) > 6 or len(first["curves"]) < 3 or first["curves"][-1][0] != "Total density":
            p.append("default curves %s" % first["curves"])
        if int(state(first, "keyrows") or 9) > 2:
            p.append("key rows %s" % state(first, "keyrows"))
        if int(state(first, "plotheight") or 0) < 100:
            p.append("plot height %s" % state(first, "plotheight"))
        if len(seq) < 4:
            return p + ["only %d states" % len(seq)]
        if state(seq[1], "mode") != "atom" or state(seq[1], "centre") != "3":
            p.append("one atom: mode %s centre %s" % (state(seq[1], "mode"), state(seq[1], "centre")))
        if state(seq[2], "mode") != "line" or state(seq[2], "line") != "1 2":
            p.append("two atoms: mode %s line %s" % (state(seq[2], "mode"), state(seq[2], "line")))
        if len(seq[3]["curves"]) < 20:
            p.append("All occupied has %d curves" % len(seq[3]["curves"]))
        if int(state(seq[3], "keyrows") or 9) > 2 or int(state(seq[3], "plotheight") or 0) < 100:
            p.append("all occupied: key rows %s plot height %s" % (state(seq[3], "keyrows"), state(seq[3], "plotheight")))
    elif case == "hf":
        first = opened[0]
        if state(first, "auto") != "0":
            p.append("a diatomic should keep Total + all occupied")
        if len(seq) < 3:
            return p + ["only %d states" % len(seq)]
        if state(seq[1], "mode") != "atom" or state(seq[1], "centre") != "1":
            p.append("click on atom 1: mode %s centre %s" % (state(seq[1], "mode"), state(seq[1], "centre")))
        if state(seq[2], "mode") != "line" or state(seq[2], "line") != "1 2":
            p.append("click on atoms 1+2: mode %s line %s" % (state(seq[2], "mode"), state(seq[2], "line")))
    elif case == "cluster":
        held = [l.split() for l in log.splitlines() if l.startswith("DENSITYPROFILE-BLOCK:")]
        if len(held) < 7:
            p.append("%d timed commands" % len(held))
        for w in held:
            # DENSITYPROFILE-BLOCK: cmd init N ms, command M ms
            if int(w[6]) > MAX_BLOCK_MS:
                p.append("%s held the GUI thread %s ms" % (w[1], w[6]))
        if len(seq) >= 4:
            if not state(seq[0], "line").startswith("1 "):
                p.append("one atom on a cluster: line %s" % state(seq[0], "line"))
            if len(seq[2]["curves"]) < 20:
                p.append("All occupied has %d curves" % len(seq[2]["curves"]))
            if state(seq[3], "progress") != "1":
                p.append("the average showed no progress dialog")
            if state(seq[3], "mode") != "atom" or state(seq[3], "stopped") != "1":
                p.append("Cancel in the progress dialog did not stop the average")
            if len(seq) < 5 or state(seq[4], "stopped") != "0" or state(seq[4], "centre") != "3" \
                    or len(seq[4]["curves"]) < 1:
                p.append("the average after a Cancel did not finish")
        else:
            p.append("only %d states" % len(seq))
        if os.environ.get("ECCE_DP_REPORT"):
            print("\n".join(" ".join(w) for w in held))
    elif case == "clusterpre":
        if not opened or not state(opened[0], "line").startswith("6 "):
            p.append("atom selected before the panel opened: line %s"
                     % (state(opened[0], "line") if opened else "-"))
    elif case == "lih":
        lab = [l.split()[1:] for l in log.splitlines() if l.startswith("labelled")]
        if len(lab) < 2:
            p.append("%d labelled lines in the dm output" % len(lab))
        for l in lab:
            if len(l) != 2:
                p.append("labelled %s, not both atoms" % l)
    return p


def main():
    pngdir = os.environ.get("ECCE_DP_PNG")
    report = bool(os.environ.get("ECCE_DP_REPORT"))

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "dppick-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        bad = []
        try:
            t = "dpk"
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            for case, url in urls.items():
                scene = os.path.join(folder, "scene")
                with open(scene, "w") as h:
                    h.write(SCENES[case].format(t=t))
                result = apps.run(display, "builder", args=("-context", url),
                                  windowTimeout=240,
                                  #  Each average on the cluster takes a minute or more.
                                  settle=900 if case == "cluster" else 300,
                                  env={"ECCE_PANEL_FULLSCREEN": "1",
                                       "ECCE_VIEWER_SCENE": scene,
                                       "ECCE_VIEWER_SCENE_OUT": out})
                log = result.log or ""
                problems = [] if os.environ.get("ECCE_DP_BEFORE") else check(case, log)
                if problems and os.environ.get("ECCE_DP_LOG"):
                    print("\n".join(l for l in log.splitlines() if "DENSITY" in l or "FAIL" in l or "atomclick" in l or "selection" in l.lower())[-3000:])
                if pngdir:
                    os.makedirs(pngdir, exist_ok=True)
                    for png in os.listdir(out):
                        if png.startswith(t + "-" + case):
                            shutil.copy(os.path.join(out, png), pngdir)
                print("%s  %s%s" % ("ok  " if not problems else "FAIL", case,
                                    "" if not problems else ": " + "; ".join(problems)))
                bad += ["%s: %s" % (case, q) for q in problems]
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if bad and not report:
            results.fail("densityprofile_pick", "; ".join(bad))

    run_tests.checkApp = checkApp
    xdisplay.SCREEN = SCREEN
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
