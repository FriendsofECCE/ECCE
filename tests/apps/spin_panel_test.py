#!/usr/bin/env python3
"""Spin per atom at a glance (#616): the MOs panel's Spin Density view, headless.

    ECCE_TEST_HOME=... ECCE_TEST_WRAPPERS=... tests/apps/spin_panel_test.py

Opens the O2 triplet (ROKS) and checks the summary line names the atom with
the largest |spin|, the per-element table adds up to 2S, and that "Show on
structure" recolours the atoms and putting it off restores the colours.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
import ecceqm_motable_test as mt   # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402


def colourBlocks(log, name):
    blocks, cur = {}, None
    for line in log.splitlines():
        m = re.match(r"PLOTSHOT-SPINCOLOURS %s (\w+)" % re.escape(name), line)
        if m:
            cur = blocks.setdefault(m.group(1), [])
        elif line.startswith("PLOTSHOT-SPINCOLOUR ") and cur is not None:
            cur.append(line.split(" ", 1)[1])
    return blocks


def checkApp(display, name, results, verbose=False):
    folder = os.path.join(fixture.stateDir(), "spin-localdata")
    urls = installLocal(folder, {"o2": mt.CALCS["o2"]})
    os.environ["ECCE_LOCAL_DATA"] = folder
    restore = fixture.settleUpgradeNotices()
    try:
        scene = os.path.join(folder, "scene")
        out = os.path.join(folder, "shots")
        os.makedirs(out, exist_ok=True)
        with open(scene, "w") as h:
            h.write("plotshot spinv 0 0 MOs:dockedspin\n"
                    "plotshot spinc 0 0 MOs:dockedspinpaint\nhold 1\n")
        log = apps.run(display, "builder", args=("-context", urls["o2"]),
                       windowTimeout=120, settle=300,
                       env={"ECCE_PANEL_FULLSCREEN": "1", "ECCE_VIEWER_SCENE": scene,
                            "ECCE_VIEWER_SCENE_OUT": out}).log or ""
        text = mt.parseState(log, "spinv").get("spinView", "")
        m = re.search(r"largest spin: (O\d) (-?\d+\.\d+) \(Mulliken\)", text)
        if not m or abs(abs(float(m.group(2))) - 1.0) > 0.1:
            results.fail("spin", "largest-spin line wrong: %r" % text)
        lines = text.split("\\n")
        i = [k for k, l in enumerate(lines) if l.startswith("Element | ")]
        sums = {}
        for l in lines[i[0] + 1:] if i else []:
            c = l.split(" | ")
            if len(c) == 2:
                sums[c[0].strip()] = float(c[1])
        if set(sums) != {"O"} or abs(sums["O"] - 2.0) > 0.05:
            results.fail("spin", "element sums should be O 2.0 (2S): %r" % sums)
        b = colourBlocks(log, "spinc")
        if set(b) != {"before", "on", "off"}:
            results.fail("spin", "no colour blocks: %r" % list(b))
        else:
            if not b["on"] or b["on"] == b["before"]:
                results.fail("spin", "Show on structure left the colours unchanged")
            if b["off"] != b["before"]:
                results.fail("spin", "off did not restore: %r vs %r" % (b["off"], b["before"]))
            if not all(l.startswith("1 ") for l in b["on"]):
                results.fail("spin", "atoms not coloured per atom while on: %r" % b["on"])
        print("  spin: %s, sums %r, colour blocks %s" % (
            m.group(0) if m else None, sums, {k: len(v) for k, v in b.items()}))
    finally:
        restore()
        os.environ.pop("ECCE_LOCAL_DATA", None)


if __name__ == "__main__":
    xdisplay.SCREEN = mt.SCREEN
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    sys.exit(run_tests.main())
