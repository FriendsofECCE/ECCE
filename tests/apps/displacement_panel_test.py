#!/usr/bin/env python3
"""Displacement panel in the Builder's Geometry Trace panel (#701).

    tests/apps/displacement_panel_test.py

Two copies of the Na+(H2O)6 trace fixture of ensemble_patterns_test (the
second with its frames in reverse order) are installed side by side.  Through
the Builder scene hooks the panel's numbers must equal ecce-displacement's
JSON for the same stored calculations, colour by displacement must recolour
the atoms, off must restore them, and Export must write the same table.
"""
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import ensemble_patterns_test as ep  # noqa: E402

CLI = os.path.join(ROOT, "scripts", "ecce_displacement.py")
fails = []


def check(name, ok, detail=""):
    print(("PASS  " if ok else "FAIL  ") + name + ("" if ok else "  " + str(detail)))
    if not ok:
        fails.append(name)


def cli(*args):
    out = subprocess.run([sys.executable, CLI] + list(args) + ["--json"],
                         capture_output=True, text=True, check=True).stdout
    return json.loads(out)


def blocks(log):
    """{command: [lines]} of the GTSTRESS output, in order of the commands."""
    out, cur = [], None
    for line in re.findall(r"GTSTRESS: (.*)$", log, re.M):
        m = re.match(r"(gtdisp\w*) ok=(\d)", line)
        if m:
            cur = {"cmd": m.group(1), "ok": m.group(2) == "1", "rows": [], "colour": [],
                   "summary": ""}
            out.append(cur)
        elif cur is not None:
            if line.startswith("disp "):
                cur["rows"].append([f.strip() for f in line[5:].split("|")])
            elif line.startswith("colour "):
                cur["colour"].append(line[7:])
            elif line.startswith("summary "):
                cur["summary"] = line[8:]
    return out


def close(a, b):
    return abs(float(a) - float(b)) < 2e-6


def panel(tmp):
    import apps
    import fixture as fx
    import isolate
    import xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fx.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    one, two = os.path.join(tmp, "na-disp1"), os.path.join(tmp, "na-disp2")
    ep.fixture(one, dav=True)
    ep.FRAMES = tuple(reversed(ep.FRAMES))
    ep.fixture(two, dav=True)
    export = os.path.join(tmp, "export.csv")
    want1 = cli(one)
    want2 = cli(one, two)
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                check("services came up", False, log)
                return
            fx.ensureRealUserAccount()
            restore = fx.settleUpgradeNotices()
            try:
                url, error = fx.install("na-disp1", source_dir=one)
                if not error:
                    _u, error = fx.install("na-disp2", source_dir=two)
                if error:
                    check("install the fixtures", False, error)
                    return
                scene = os.path.join(tmp, "scene")
                open(scene, "w").write(
                    "gtfocus\ngtdisp\ngtdispcolour off\ngtdispcolour on\n"
                    "gtdispcolour off\ngtdispexport %s\n"
                    "gtdispcompare na-disp2\ngtdispcolour on\n"
                    "gtdispselect within 3.5 of 1\ngtclose\nhold 1\n" % export)
                args = ("-context", url)
                if os.path.realpath(state) != os.path.realpath(os.path.expanduser("~")):
                    auth = fx.authFile(os.path.join(state, ".ECCE", "auth.pipe"), user=fx.USER)
                    args = ("-pipe", auth) + args
                result = apps.run(display, "builder", args=args, windowTimeout=120,
                                  settle=120,
                                  env={"ECCE_VIEWER_SCENE": scene,
                                       "ECCE_VIEWER_SCENE_OUT": tmp,
                                       "ECCE_REALUSER": fx.USER})
                log = result.log or ""
                check("builder did not crash", not result.crashed, result.signalName)
                failed = os.path.join(tmp, "FAILED")
                if os.path.exists(failed):
                    check("the scene ran", False, open(failed).read())
                b = blocks(log)
                if len(b) < 8:
                    check("the hooks ran", False, log[-4000:])
                    return
                disp, base, on, off, exp, cmp_, cmpcol, sel = b[:8]
                rows = disp["rows"]
                check("one calculation: rows equal the CLI's JSON",
                      len(rows) == len(want1["atoms"]) and all(
                          int(r[0]) == a["index"] and r[1] == a["element"]
                          and close(r[2], a["max"]) and close(r[3], a["rms"])
                          for r, a in zip(rows, want1["atoms"])), (rows[:2], want1["atoms"][:2]))
                check("some atom actually moves", any(a["max"] > 0.1 for a in want1["atoms"]))
                check("colour on changes the atom colours",
                      on["colour"] and on["colour"] != base["colour"])
                check("colour off restores them", off["colour"] == base["colour"],
                      (off["colour"][:2], base["colour"][:2]))
                check("export writes the same table",
                      os.path.exists(export) and
                      [l.split(",")[0] for l in open(export).read().split("\n")[1:] if l]
                      == [str(a["index"]) for a in want1["atoms"]])
                m = re.search(r"aligned RMSD: ([0-9.]+)", cmp_["summary"])
                check("compare: aligned RMSD equals the CLI's",
                      m and close(m.group(1), want2["rmsd"]), (cmp_["summary"], want2["rmsd"]))
                check("compare: per-atom displacements equal the CLI's",
                      len(cmp_["rows"]) == len(want2["atoms"]) and all(
                          int(r[0]) == a["index"] and close(r[-1], a["displacement"])
                          for r, a in zip(cmp_["rows"], want2["atoms"])))
                check("compare: colours differ from the normal ones",
                      cmpcol["colour"] != base["colour"])
                want3 = cli(one, two, "--select", "within 3.5 of 1")
                check("selection: the CLI's --select syntax is honoured",
                      sel["ok"] and 0 < len(sel["rows"]) < len(want2["atoms"]) and
                      [int(r[0]) for r in sel["rows"]] == [a["index"] for a in want3["atoms"]],
                      (sel["summary"], len(sel["rows"])))
            finally:
                restore()
                fx.remove("na-disp1")
                fx.remove("na-disp2")
        finally:
            apps.stopServices(display)


def main():
    tmp = tempfile.mkdtemp(prefix="disp-panel-")
    try:
        panel(tmp)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
