#!/usr/bin/env python3
"""Fragment analysis recipe and the Fragment orbitals box (#201), headless.

1. The recipe applied after a finished Co complex (an ORCA example) proposes
   Co | the rest with charges by electron count, builds one single point of the
   complex and one per fragment, each fragment holding its own atoms, charge and
   multiplicity, linked from the complex; overlapping or incomplete fragments
   are refused in the panel.
2. A fresh workflow from the recipe whose steps hold the Cr(CO)6 fixtures of
   tests/fragorb_h5 (Cr | (CO)6, ORCA RHF/def2-SVP): its Fragment orbitals box
   must show what `ecce-fragorb analyse --json` gives for the same files, the
   Details window's CSV must equal the CLI's composition matrix, and its plots
   must carry the CLI's PDOS and OPDOS curves.
ECCE_FRAGORB_SHOTS=<dir> keeps the PNGs.  Same install and isolation as
rate_gui_test.py; exit 77 (skip) without an install or Xvfb.
"""
import csv
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

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
COMPLEX = os.path.join(ROOT, "data", "client", "examples", "spectroscopy",
                       "sulfate-coordination", "orca-co_nh3_5_oso3", "stored", "co_nh3_5_oso3")
FIXTURES = os.path.join(ROOT, "tests", "fragorb_h5", "fixtures")
DEGENERATE = 0.003      # eV, as FragOrbBox.C
failures = []


def check(what, cond, why=""):
    print(("ok    " if cond else "FAIL  ") + what + ("" if cond else "\n      " + str(why)[:900]))
    if not cond:
        failures.append(what)
    return cond


def cli(args):
    env = dict(os.environ, ECCE_HOME=apps.INSTALL)
    r = subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-fragorb")] + args + ["--json"],
                       env=env, capture_output=True, text=True)
    return json.loads(r.stdout) if r.returncode == 0 else None


def steps(info):
    """name -> {field: value} from a workflowinfo answer."""
    table = {}
    for entry in info.split(" links:")[0].split("|"):
        f = entry.split()
        if len(f) < 4:
            continue
        rec = dict(zip(("name", "code", "state", "source"), f[:4]))
        for kv in f[4:]:
            k, _, v = kv.partition("=")
            rec[k] = v
        table[f[0]] = rec
    return table


def organizer(display, work, name, commands):
    script = os.path.join(work, "commands-" + name)
    with open(script, "w") as f:
        f.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=90,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    with open(os.path.join(work, name + ".log"), "w") as f:
        f.write(log)
    if result.crashed:
        check("the Organizer ran (%s)" % name, False, "CRASHED (%s)\n%s" % (result.signalName, log[-1500:]))
        return None
    return log


def answers(log, prefix):
    return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix), log, re.M)


def expected_rows(ref, names):
    """The box's rows from the CLI's JSON, grouped as FragOrbBox.C groups them."""
    rows = []
    open_shell = len(ref["channels"]) > 1
    for ch in ref["channels"]:
        fo = ch["fo"]
        homo = {}
        for f in fo:
            if f["occupation"] > 1e-6:
                homo[f["fragment"]] = f["index"]
        levels, k = [], 0
        while k < len(fo):
            j = k
            while (j + 1 < len(fo) and fo[j + 1]["fragment"] == fo[k]["fragment"] and
                   abs(fo[j + 1]["energy_ev"] - fo[k]["energy_ev"]) < DEGENERATE):
                j += 1
            h, lo, hi = homo.get(fo[k]["fragment"], -1), fo[k]["index"], fo[j]["index"]
            where = ("HOMO" if lo <= h <= hi else "HOMO−%d" % (h - hi) if hi < h
                     else "LUMO" if lo == h + 1 else "LUMO+%d" % (lo - h - 1))
            label = names[fo[k]["fragment"]] + " " + where + (" (×%d)" % (j - k + 1) if j > k else "")
            levels.append((k, j, label))
            k = j + 1
        mo = ch["mo"]
        mos, i = [], 0
        while i < len(mo):
            j = i
            while j + 1 < len(mo) and abs(mo[j + 1]["energy_ev"] - mo[i]["energy_ev"]) < DEGENERATE:
                j += 1
            mos.append((i, j))
            i = j + 1
        homo_level = max(n for n, (a, b) in enumerate(mos)
                         if any(mo[m]["occupation"] > 1e-6 for m in range(a, b + 1)))
        for n in range(min(len(mos) - 1, homo_level + 3), max(0, homo_level - 2) - 1, -1):
            a, b = mos[n]
            shares = []
            for (fa, fb, label) in levels:
                s = sum(mo[m]["composition"][q] for m in range(a, b + 1) for q in range(fa, fb + 1))
                shares.append((s / (b - a + 1), label))
            shares.sort(key=lambda x: -x[0])
            value = []
            for t, (s, label) in enumerate(shares[:3]):
                if t > 0 and s < ref["threshold"]:
                    break
                value.append("%.0f %% %s" % (100 * s, label))
            idx = ("MO %d" % mo[a]["index"]) if a == b else "MO %d-%d" % (mo[a]["index"], mo[b]["index"])
            label = (("β " if ch["spin"] else "α ") if open_shell else "") + idx
            label += " HOMO" if n == homo_level else " LUMO" if n == homo_level + 1 else ""
            label += "  %.2f eV" % mo[a]["energy_ev"]
            rows.append((label, ", ".join(value)))
    return rows


def run(display, data, work, shots):
    root = os.path.join(data, "users", "local")
    for name in ("frag", "box"):
        os.makedirs(os.path.join(root, name))
        shutil.copy(PROJECT_META, os.path.join(root, name, ".ecce-meta"))
    shutil.copytree(COMPLEX, os.path.join(root, "frag", "co_nh3_5_oso3"), symlinks=True)
    proj, bproj = "file://" + root + "/frag", "file://" + root + "/box"
    wf, bwf = proj + "/Workflow", bproj + "/Workflow"
    panel_png = os.path.join(shots, "fragment-recipe-panel.png")
    log = organizer(display, work, "recipe", [
        "new %s workflow" % proj, "new %s workflow" % bproj,
        "workflowcopy %s %s/co_nh3_5_oso3 co_complex" % (wf, proj),
        "workflowrecipelist",
        # the proposal on the finished complex, then two refused splits
        "workflowrecipepanel - %s fragment-analysis orca %s/co_complex" % (wf, wf),
        "workflowrecipepanel - %s fragment-analysis orca %s/co_complex fragments=1-3:1:1/3-26:0:1" % (wf, wf),
        "workflowrecipepanel - %s fragment-analysis orca %s/co_complex fragments=1:1:1/2-20:0:1" % (wf, wf),
        "workflowrecipecreate %s" % wf,
        "workflowrecipepanel %s %s fragment-analysis orca %s/co_complex" % (panel_png, wf, wf),
        "workflowrecipecreate %s" % wf,
        "workflowinfo %s" % wf,
        # a fresh workflow for the fixtures; the box waits for its steps
        "workflowrecipe %s fragment-analysis orca - fragments=1:0:1/2-13:0:1" % bwf,
        "workflowfragorb %s" % bwf,
    ])
    if log is None:
        return
    listed = answers(log, "workflowrecipelist")
    check("the recipe is listed", listed and " fragment-analysis:orca" in listed[0], listed)
    panels = answers(log, "workflowrecipepanel")
    check("four panel answers", len(panels) == 4, panels)
    if len(panels) < 4:
        return
    m = re.search(r"fragments=(\S+)", panels[0])
    proposal = m.group(1) if m else ""
    natoms = len([l for l in open(os.path.join(COMPLEX, "..", "..", "co_nh3_5_oso3.inp"))
                  if re.match(r"\s+[A-Z][a-z]?\s+-?\d", l)])
    # NH3 has 10 electrons and SO4 48 (both even): neutral ligands, Co takes the +1.
    check("proposal: Co | the rest, Co +1 singlet, ligands neutral singlet",
          proposal == "1:1:1/2-%d:0:1" % natoms, panels[0])
    check("proposal: the formulas of both fragments are shown",
          "formulas=Co;H15N5O4S;" in panels[0], panels[0])
    check("proposal: no problem, and the fragment heading comes before Where (4)",
          "fragment-problem= " in panels[0] and "where-heading=4  Where the new steps go" in panels[0],
          panels[0])
    check("shared atoms are refused", "atom 3 is in fragment 1 and in fragment 2" in panels[1], panels[1])
    check("uncovered atoms are refused", "in no fragment" in panels[2] and
          "must cover the complex" in panels[2], panels[2])
    creates = answers(log, "workflowrecipecreate")
    check("Create boxes is refused while the fragments are wrong",
          creates and creates[0].startswith("error: panel still open"), creates[:1])
    check("the panel picture was taken", os.path.exists(panel_png), panel_png)
    infos = answers(log, "workflowinfo")
    t = steps(infos[-1].replace("ok ", "", 1)) if infos else {}
    cx, f1, f2 = t.get("complex", {}), t.get("fragment-1", {}), t.get("fragment-2", {})
    check("complex and two fragment steps, each from its source",
          cx.get("source") == "co_complex" and f1.get("source") == "complex+fragment-1-field" and
          f2.get("source") == "complex", infos[-1:] if infos else infos)
    check("the complex keeps all atoms and the charge, the fragments their own",
          cx.get("atoms") == str(natoms) and f1.get("atoms") == "1" and
          f2.get("atoms") == str(natoms - 1) and f1.get("charge") == "1" and
          f2.get("charge") == "0" and f1.get("mult") == "1" and f2.get("mult") == "1", t)
    check("all three are ORCA single points with one basis set",
          all(r.get("code") == "ORCA" for r in (cx, f1, f2)) and
          len(set(r.get("basis") for r in (cx, f1, f2))) == 1 and cx.get("basis"), t)
    check("geometry links from the complex to each fragment",
          infos and "complex>fragment-1:geometry" in infos[-1] and
          "complex>fragment-2:geometry" in infos[-1], infos[-1:])
    fld = t.get("fragment-1-field", {})
    check("a one-atom fragment gets a field step: the atom, same charge, geometry from the complex",
          fld.get("atoms") == "1" and fld.get("charge") == "1" and fld.get("source") == "complex" and
          "complex>fragment-1-field:geometry" in infos[-1], (fld, infos[-1:]))
    check("the field step hands its orbitals to the fragment; the other fragment has no field step",
          "fragment-1-field>fragment-1:orbitals" in infos[-1] and "fragment-2-field" not in infos[-1],
          infos[-1:])
    pc = os.path.join(root, "frag", "Workflow", "fragment-1-field", "Inputs", "pointcharges.pc")
    pcl = open(pc).read().split("\n") if os.path.exists(pc) else []
    # Co(NH3)5OSO3: five N and one O bonded to Co, -0.1 e each, hydrogens left out
    check("the field step stores 6 point charges of -0.1 e",
          pcl[:1] == ["6"] and sum(1 for l in pcl[1:] if l.startswith("-0.1 ")) == 6, pcl[:8])
    waiting = answers(log, "workflowfragorb")
    check("the box waits for its steps",
          waiting and "Waiting for complex, fragment-1, fragment-2 to finish." in waiting[0], waiting)

    # The fixtures in the fresh workflow's steps, then the box.
    folder = os.path.join(root, "box", "Workflow")
    for step, fixture in (("complex", "crco6"), ("fragment-1", "cr"), ("fragment-2", "co6")):
        out = os.path.join(folder, step, "Outputs")
        os.makedirs(out, exist_ok=True)
        shutil.copy(os.path.join(FIXTURES, fixture, "results.h5"), out)
    box_png = os.path.join(shots, "fragment-orbitals-box.png")
    details_png = os.path.join(shots, "fragment-orbitals-details.png")
    csv_file = os.path.join(work, "fragorb.csv")
    log = organizer(display, work, "box", [
        "setstate %s/%s completed" % (bwf, s) for s in ("complex", "fragment-1", "fragment-2")] + [
        "workflowfragorb %s png=%s details=%s csv=%s" % (bwf, box_png, details_png, csv_file)])
    if log is None:
        return
    got = answers(log, "workflowfragorb")
    if not check("the box answered", got and got[0].startswith("ok Fragment orbitals"), got):
        return
    paths = [os.path.join(folder, s) for s in ("complex", "fragment-1", "fragment-2")]
    ref = cli(["analyse"] + paths)
    dos = cli(["dos"] + paths)
    if not check("ecce-fragorb runs on the steps' folders", ref is not None and dos is not None):
        return
    text = got[0][3:].split(" png ")[0].split(" details: ")[0]
    lines = text.split("|")
    rows = [tuple(l.split(": ", 1)) for l in lines if ": " in l and l.startswith("MO ")]
    want = expected_rows(ref, ["fragment-1", "fragment-2"])
    check("box rows equal ecce-fragorb's compositions (frontier levels, FO shares)",
          rows == want, "box %r\nCLI %r" % (rows, want))
    check("box names the fragments and the method", "fragment-1 | fragment-2, Mulliken" in text, lines[:3])
    check("box PNG saved", "png saved" in got[0] and os.path.getsize(box_png) > 5000, got[0][-300:])
    m = re.search(r"details: (.*?)( details-png| csv|$)", got[0])
    rep = m.group(1) if m else ""
    nmo = sum(len(c["mo"]) for c in ref["channels"])
    check("Details: one table line per MO, PDOS (total + 2) and one OPDOS curve on the CLI's grid",
          "pdos-curves=3" in rep and "opdos-curves=1" in rep and
          "points=%d" % len(dos["grid"]) in rep and
          int(re.search(r"table-lines=(\d+)", rep).group(1)) > nmo, rep)
    check("Details: the occupied overlap population is the CLI's",
          ("occupied overlap population %.3f" % dos["occupied_overlap"][0]) in rep, rep)
    check("Details PNG saved", "details-png saved" in got[0] and os.path.getsize(details_png) > 10000,
          got[0][-200:])
    try:
        table = list(csv.reader(open(csv_file)))
        cols = len(ref["channels"][0]["fo"])
        ok = (len(table) == 1 + nmo and len(table[0]) == 5 + cols and
              max(abs(float(table[1 + i][5 + k]) - mo["composition"][k])
                  for i, mo in enumerate(ref["channels"][0]["mo"]) for k in range(cols)) < 1e-7)
    except Exception as e:  # noqa
        ok, table = False, str(e)
    check("Details CSV equals the CLI's composition matrix", ok, str(table)[:300])


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="fragorb-gui-", dir=os.environ.get("ECCE_REALUSERHOME"))
    shots = os.environ.get("ECCE_FRAGORB_SHOTS") or os.path.join(work, "shots")
    os.makedirs(shots, exist_ok=True)
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work, shots)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    if not failures:
        shutil.rmtree(work, ignore_errors=True)
    else:
        print("kept " + work)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
