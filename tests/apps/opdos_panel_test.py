#!/usr/bin/env python3
"""
The Density of States (OPDOS) panel (#171) on a stored ORCA RHF/def2-SVP
Cr(CO)6, headless: Builder scene commands (opdos*), no input events.
Checked: the panel is ready and passed the c^T S c = 1 check; the curves
integrate to the number of MOs in the window (TDOS) and to 1 per MO times the
gross population (PDOS A + PDOS B = TDOS); the integrated OPDOS at the HOMO
equals the A-B overlap population and that equals `ecce-fragorb dos`
(the CLI of the same library, Cr | the six CO); A/B given by the viewer
selection; Occupied only; a click lists MOs and a double-click on the first
draws its isosurface through the MOs panel; Lorentzian line shape; CSV and PNG
exports.  Stage 2 (#171): the UKS CH2 triplet (6-31G*, d functions) draws
alpha up and beta down, per-spin overlap populations agree with
`ecce-fragorb dos --spin`, a beta MO under a clicked peak is drawn as the
beta orbital; the same calculation with one MO's coefficients scaled by 1.05
is refused with the worst MO named in the panel's message area.

    ECCE_OPDOS_PNG=<dir>   also copy the pictures there

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

CALC = os.path.join(REPO, "tests", "modiagram", "fixtures", "orca-crco6")
META = os.path.join(REPO, "tests", "apps", "fixtures", "calc-orca-water-ccpvdz")
H5 = os.path.join(REPO, "tests", "fragorb_h5", "fixtures", "crco6", "results.h5")
CH2 = os.path.join(REPO, "data", "client", "examples", "genchem", "lewis-structures-and-resonance",
                   "ecceqm-CH2_triplet", "stored", "CH2_triplet")

SCENE = """opdosopen
opdosstate
opdosint 1
opdosatoms 1 2-13
opdosstate
opdosclick -9.6
opdosstate
opdosshow 0
mogrid dosmo
opdoscsv {t}
opdosoccupied 1
opdosstate
opdosoccupied 0
opdossticks 1
opdoswidth 0.2
opdosselect A 1
opdosselect B 2 3 4 5 6 7 8 9 10 11 12 13
opdosstate
opdosatoms 1 2-13
opdosshape lorentzian
opdosstate
opdosshape gaussian
opdosrange -12 2
opdosstate
opdosreset
opdosstate
opdospng {t} 1200 800
hold 1
"""


def blocks(log):
    """One dict per `OPDOS: <cmd>` state dump; `text` is the last key and spans lines."""
    out = []
    lines = log.splitlines()
    for i, line in enumerate(lines):
        m = re.search(r"OPDOS: (opdos\w+)", line)
        if not m:
            continue
        b = {"cmd": m.group(1)}
        last = None
        for item in lines[i + 1:]:
            if "OPDOS: opdos" in item or item.startswith("PLOTSHOT"):
                break
            if last == "text":
                b["text"] += "\n" + item
                continue
            k, _, v = item.partition(" ")
            b[k] = v
            last = k
        out.append(b)
    return out


OPEN_SCENE = """opdosopen
opdosstate
opdoswidth 0.05
opdosclick {homo}
opdosshow 0
mogrid betamo
opdosstate
hold 1
"""


def corrupt(h5, out, exe, mo=4, factor=1.05):
    """results.h5 with alpha MO `mo`'s coefficients scaled, through a Molden round trip."""
    md = out + ".molden"
    subprocess.run([exe, "export", "molden", h5, md], check=True, capture_output=True)
    lines, k, inblock = open(md).read().split("\n"), 0, False
    for i, line in enumerate(lines):
        t = line.strip()
        if t.startswith("Sym="):
            k += 1
        elif t.startswith("Spin="):
            inblock = k == mo and t.split("=")[1].strip().lower().startswith("a")
        elif inblock and len(t.split()) == 2 and t.split()[0].isdigit():
            n, c = t.split()
            lines[i] = "%s %.14e" % (n, float(c) * factor)
    open(md, "w").write("\n".join(lines))
    subprocess.run([exe, "convert", md, out], check=True, capture_output=True)


def stage2(display, results, exe):
    """Open shell, beta MO drawn, and the norm refusal on the CH2 triplet."""
    bad = []
    resexe = os.path.join(os.path.dirname(exe), "ecce-results")
    beta = subprocess.run([exe, "dos", os.path.join(CH2, "Outputs", "results.h5"), "--set", "A=1", "--set", "B=2,3",
                           "--pair", "A:B", "--spin", "beta", "--occupied-only", "--json"], capture_output=True, text=True)
    homo = json.loads(beta.stdout)["fermi_ev"]
    want = {}
    for spin in ("alpha", "beta"):
        r = subprocess.run([exe, "dos", os.path.join(CH2, "Outputs", "results.h5"), "--set", "A=1", "--set", "B=2,3",
                            "--pair", "A:B", "--spin", spin, "--json"], capture_output=True, text=True)
        want[spin] = json.loads(r.stdout)["occupied_overlap"][0]
    for kind in ("open", "corrupt"):
        folder = os.path.join(fixture.stateDir(), "opdos-" + kind)
        urls = installLocal(folder, {"ch2": CH2})
        calc = urls["ch2"][len("file://"):]
        if kind == "corrupt":
            h5 = os.path.join(calc, "Outputs", "results.h5")
            corrupt(os.path.join(CH2, "Outputs", "results.h5"), h5 + ".new", resexe)
            os.replace(h5 + ".new", h5)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        try:
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(OPEN_SCENE.format(homo="%.4f" % homo) if kind == "open" else "opdosopen\nhold 1\n")
            result = apps.run(display, "builder", args=("-context", urls["ch2"]), windowTimeout=240, settle=300,
                              env={"ECCE_PANEL_FULLSCREEN": "1", "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out})
            log = result.log or ""
            bl = blocks(log)
            if kind == "corrupt":
                st = bl[0] if bl else {}
                if st.get("ready") != "0" or "at MO 4 alpha" not in st.get("error", ""):
                    bad.append("scaled MO 4 not refused by name: ready %s error %s" % (st.get("ready"), st.get("error")))
                elif "at MO 4 alpha" not in st.get("text", ""):
                    bad.append("the refusal is not in the message area: %r" % st.get("text"))
                else:
                    print("   refused: " + st["error"])
                continue
            states = [b for b in bl if b["cmd"] == "opdosstate"]
            shown = [b for b in bl if b["cmd"] == "opdosshow"]
            if len(states) < 2 or not shown:
                bad.append("open shell: %d states, %d shows" % (len(states), len(shown)))
                print("\n".join(l for l in log.splitlines() if re.search("opdos|scene|SCENE|OPDOS", l))[-1500:])
                continue
            first, after = states[0], shown[0]
            if first.get("unrestricted") != "1" or first.get("ready") != "1":
                bad.append("CH2 triplet not ready/unrestricted: %s %s" % (first.get("ready"), first.get("unrestricted")))
            curves = [l.split(" ", 1)[1] for l in log.split("OPDOS: opdosstate", 1)[1].split("OPDOS:")[0].splitlines()
                      if l.startswith("curve ") or l.startswith("intcurve ")]
            ranges = {}
            for c in curves:
                nm, lo, hi = c.rsplit("|", 2)
                ranges[nm] = (float(lo), float(hi))
            for nm in ("TDOS alpha", "TDOS beta", "PDOS(A) alpha", "PDOS(A) beta", "alpha", "beta"):
                if nm not in ranges:
                    bad.append("no curve %r (have %s)" % (nm, sorted(ranges)))
            if "TDOS alpha" in ranges and "TDOS beta" in ranges:
                if not (ranges["TDOS alpha"][0] >= 0 < ranges["TDOS alpha"][1]):
                    bad.append("TDOS alpha not upwards %s" % (ranges["TDOS alpha"],))
                if not (ranges["TDOS beta"][1] <= 0 > ranges["TDOS beta"][0]):
                    bad.append("TDOS beta not downwards %s" % (ranges["TDOS beta"],))
            got = {}
            for c, spin in ((0, "alpha"), (1, "beta")):
                v = first.get("channel%d" % c, "").split()
                got[spin] = float(dict(zip(v[0::2], v[1::2])).get("occupied_op", "nan"))
                if not abs(got[spin] - want[spin]) < 1e-6:
                    bad.append("%s occupied OP %s vs CLI --spin %s %s" % (spin, got[spin], spin, want[spin]))
            print("   CH2 triplet C-H overlap population: alpha %.6f beta %.6f (CLI %.6f %.6f)"
                  % (got["alpha"], got["beta"], want["alpha"], want["beta"]))
            row0 = after.get("list_row0", "").split("|")
            if after.get("drawn", "-") == "-" or len(row0) < 2 or row0[1] != "beta" or \
                    after.get("drawn") != "MO %s beta" % row0[0]:
                bad.append("beta HOMO under the click not drawn as beta: row %s drawn %s text %r"
                           % (row0, after.get("drawn"), after.get("text", "")[-160:]))
            g = os.path.join(out, "betamo.txt")
            if not os.path.exists(g) or "fieldMax" not in open(g).read():
                bad.append("no MO grid after opdosshow (betamo.txt)")
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
    print("%s  opdos stage 2%s" % ("ok  " if not bad else "FAIL", "" if not bad else ": " + "; ".join(bad)))
    if bad:
        results.fail("opdos-stage2", "; ".join(bad))


def main():
    exe = os.path.join(os.environ.get("ECCE_TEST_HOME", ""), "bin", "ecce-fragorb")
    if not os.environ.get("ECCE_TEST_HOME") or not os.path.exists(H5):
        print("SKIP: no install")
        return 77
    pngdir = os.environ.get("ECCE_OPDOS_PNG")

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "opdos-localdata")
        urls = installLocal(folder, {"crco6": CALC})
        calc = urls["crco6"][len("file://"):]
        os.makedirs(os.path.join(calc, "Outputs"))
        shutil.copy(H5, os.path.join(calc, "Outputs", "results.h5"))
        # the modiagram fixture predates .ecce-meta; without one the calculation does not open
        for sub in ("", "Parameters", "Props"):
            shutil.copy(os.path.join(META, sub, ".ecce-meta"), os.path.join(calc, sub, ".ecce-meta"))
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        bad = []
        try:
            t = "opdos"
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE.format(t=t))
            result = apps.run(display, "builder", args=("-context", urls["crco6"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_PANEL_FULLSCREEN": "1", "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out})
            log = result.log or ""
            bl = blocks(log)
            bl_all = bl
            # Pick the dumps by command, not by position: every scene command logs a block.
            bl = [b for b in bl if b["cmd"] == "opdosstate"]
            if len(bl) < 5:
                bad.append("only %d OPDOS states in the log" % len(bl))
                print("\n".join(l for l in log.splitlines() if re.search("opdos|scene|SCENE|OPDOS", l))[-1500:])
            else:
                first, atoms, clicked, occ, sel = bl[0], bl[1], bl[2], bl[3], bl[4]
                if first.get("ready") != "1" or first.get("error") != "-":
                    bad.append("panel not ready: %s" % first.get("error"))
                if float(first.get("norm_error", "1")) > 1e-5:
                    bad.append("norm check %s" % first.get("norm_error"))
                if first.get("A") != "1" or first.get("B") != "2-13":
                    bad.append("default split %s | %s, not Cr | rest" % (first.get("A"), first.get("B")))
                c = atoms["channel0"].split()
                v = dict(zip(c[0::2], c[1::2]))
                nmo = int(v["nmo"])
                # the default window is HOMO-20 .. LUMO+10 eV: count its MOs from TDOS
                tdos = float(v["tdos_int"])
                if abs(tdos - round(tdos)) > 0.05 or tdos < 20:
                    bad.append("TDOS integral %s is not 2 per MO in the window" % tdos)
                if abs(float(v["pdosA_int"]) + float(v["pdosB_int"]) - tdos) > 1e-3:
                    bad.append("PDOS(A)+PDOS(B) %s != TDOS %s" % (float(v["pdosA_int"]) + float(v["pdosB_int"]), tdos))
                cli = subprocess.run([exe, "dos", os.path.join(urls["crco6"][len("file://"):]),
                                      "--set", "A=1", "--set", "B=2-13", "--pair", "A:B", "--json"],
                                     capture_output=True, text=True)
                want = json.loads(cli.stdout)["occupied_overlap"][0] if cli.returncode == 0 else None
                if want is None:
                    bad.append("ecce-fragorb dos failed: " + cli.stderr[:200])
                else:
                    if abs(float(v["occupied_op"]) - want) > 1e-6:
                        bad.append("occupied OP %s vs CLI %s" % (v["occupied_op"], want))
                    if abs(float(v["intopdos_at_homo"]) - want) > 2e-3:
                        bad.append("integrated OPDOS at HOMO %s vs total OP %s" % (v["intopdos_at_homo"], want))
                    print("   total A-B OP %.6f, integrated OPDOS at HOMO %s, TDOS integral %s over %d MOs"
                          % (want, v["intopdos_at_homo"], v["tdos_int"], nmo))
                if "MOs within" not in clicked.get("text", "") or int(clicked.get("list_rows", "0")) < 1 \
                        or clicked.get("list_shown") != "1":
                    bad.append("a click lists no MOs: %s" % clicked.get("text", "")[-120:])
                shows = [b for b in bl_all if b["cmd"] == "opdosshow"]
                row0 = clicked.get("list_row0", "").split("|")
                if not shows or shows[0].get("drawn") != "MO %s" % row0[0]:
                    bad.append("double-click on row 0 (%s) drew %s" % (row0, shows[0].get("drawn") if shows else None))
                if not os.path.exists(os.path.join(out, "dosmo.txt")) or "fieldMax" not in open(os.path.join(out, "dosmo.txt")).read():
                    bad.append("no MO grid after opdosshow (dosmo.txt)")
                lor = bl[5] if len(bl) >= 8 else {}
                cg = dict(zip(atoms["channel0"].split()[0::2], atoms["channel0"].split()[1::2]))
                cl = dict(zip(lor.get("channel0", "").split()[0::2], lor.get("channel0", "").split()[1::2]))
                if lor.get("shape") != "lorentzian" or not (0.8 * float(cg["tdos_int"]) < float(cl.get("tdos_int", 0)) < float(cg["tdos_int"])):
                    bad.append("Lorentzian: shape %s, TDOS integral %s vs Gaussian %s (tails leave the window)"
                               % (lor.get("shape"), cl.get("tdos_int"), cg["tdos_int"]))
                if "Mulliken" not in first.get("text", ""):
                    bad.append("the scheme is not named on screen")
                if int(re.search(r"nmo (\d+)", occ["channel0"]).group(1)) != 54:
                    bad.append("Occupied only keeps %s MOs, not 54" % re.search(r"nmo (\d+)", occ["channel0"]).group(1))
                if sel.get("A") != "1" or sel.get("B") != "2-13":
                    bad.append("Use selection gave %s | %s" % (sel.get("A"), sel.get("B")))
            # #934: layout, labels, energy window (the last three states: window, reset)
            if len(bl) >= 8:
                win, rst, lay = bl[-2], bl[-1], bl[0]
                if "in both" in first.get("text", "") or "in both" in atoms.get("text", ""):
                    bad.append("an 'in both A and B' note for disjoint A=1, B=2-13")
                def box(st, k):
                    return [int(x) for x in st[k].split()]
                ax, ay, aw, ah = box(lay, "atomsA_box")
                ux, uy, uw, uh = box(lay, "useA_button")
                bx, by, bw, bh = box(lay, "atomsB_box")
                vx, vy, vw, vh = box(lay, "useB_button")
                # each Use selection directly below its own text box, not beside it
                if not (uy >= ay + ah and abs((ux + uw) - (ax + aw)) <= 40 and ux + uw > ax):
                    bad.append("Use selection (A) is not under box A: box %s button %s" % ((ax, ay, aw, ah), (ux, uy, uw, uh)))
                if not (vy >= by + bh and vx + vw > bx and vx < bx + bw + 40):
                    bad.append("Use selection (B) is not under box B: box %s button %s" % ((bx, by, bw, bh), (vx, vy, vw, vh)))
                if lay.get("intplot_ylabel", "").strip() != "Integrated OPDOS (electrons)" or lay.get("intplot_xlabel", "").strip() != "Energy (eV)":
                    bad.append("lower plot labels: %r / %r" % (lay.get("intplot_xlabel"), lay.get("intplot_ylabel")))
                if win.get("plot_window") != "1 -12.000 2.000" or win.get("intplot_window") != "1 -12.000 2.000":
                    bad.append("From/To not applied to both plots: %s | %s" % (win.get("plot_window"), win.get("intplot_window")))
                if rst.get("plot_window", "").split()[0] != "0" or rst.get("intplot_window", "").split()[0] != "0":
                    bad.append("Reset left a window: %s | %s" % (rst.get("plot_window"), rst.get("intplot_window")))
            else:
                bad.append("window/layout states missing (%d)" % len(bl))
            for ext in ("csv", "png"):
                f = os.path.join(out, "opdos." + ext)
                if not os.path.exists(f) or os.path.getsize(f) < 1000:
                    bad.append("no " + f)
                elif pngdir:
                    os.makedirs(pngdir, exist_ok=True)
                    shutil.copy(f, pngdir)
            print("%s  opdos panel%s" % ("ok  " if not bad else "FAIL", "" if not bad else ": " + "; ".join(bad)))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if bad:
            results.fail("opdos", "; ".join(bad))
        stage2(display, results, exe)

    run_tests.checkApp = checkApp
    xdisplay.SCREEN = "1700x1000x24"
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    sys.argv = sys.argv[:3] + [a for a in sys.argv[3:] if a not in ("--app", "builder")]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
