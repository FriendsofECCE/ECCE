#!/usr/bin/env python3
"""Final stress: wx-free arithmetic and isolated Builder arrows/screenshots.

Run with ECCE_TEST_HOME and ECCE_TEST_WRAPPERS; --unit-only needs only C++.
--png DIR retains screenshots and Builder logs for visual review.
"""
import argparse
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time

REPO = Path(__file__).resolve().parents[2]


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def unit(work):
    exe = str(work / "stress-unit")
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-I", str(REPO / "include"),
                    str(REPO / "tests/stress/stress_tensor_test.C"),
                    str(REPO / "src/util/chemutil/Diag3.C"), "-o", exe], check=True)
    subprocess.run([exe], check=True)
    qe=(REPO / "data/client/examples/genchem/packing-in-solids/qe-al_fcc/al_fcc.pwout").read_text()
    qe=qe[qe.rindex("total   stress"):]
    mlp="energy: -1.0\nstress (kbar):\n-20.0 -10.0 0.0\n-10.0 -20.0 0.0\n0.0 0.0 20.0\n"
    for script, block, pressure in (("qe.stress",qe,-4.84),("mlp.final",mlp,-20/3)):
        parsed=subprocess.run(["perl",str(REPO / "scripts/parsers" / script),"STRESS"],
                              input=block,text=True,capture_output=True,check=True).stdout
        p=re.search(r"key: PRESSURE\nsize:\n1\nvalues:\n([^\n]+)",parsed)
        s=re.search(r"key: STRESS.*?values:\n(.*?)\nunits:\nkbar",parsed,re.S)
        require(p is not None and s is not None,script+": stress/pressure records")
        values=[float(x) for x in s[1].split()]
        require(len(values)==9 and abs(float(p[1])-pressure)<1e-7 and
                abs(sum(values[::4])/3-float(p[1]))<1e-7,
                script+": compression-positive tensor and pressure in kbar")
    print("PASS QE example and MLP parser stress/pressure signs and kbar units",flush=True)


def builder(display, url, work, png):
    import apps
    work.mkdir()
    script = work / "script"
    lines = ["wait 3000", "background 1 1 1", "list Stress Tensor", "wait 1000",
             "viewall", "rotate 25", "stressinfo", "vector STRESS", "overlay",
             "shot arrows", "viewer Stress Tensor", "wait 500", "overlay",
             "shot hidden", "viewer Stress Tensor", "wait 500", "overlay",
             "shot restored", "list Energies", "wait 500", "overlay", "quit"]
    script.write_text("\n".join(lines)+"\n")
    env = display.env()
    env.update(ECCE_BUILDER_SCRIPT=str(script), ECCE_TRANSPARENCY_FALLBACK_MS="0")
    logpath = work / "builder.log"
    with logpath.open("w") as log:
        proc = subprocess.Popen([str(Path(apps.WRAPPERS) / "ecce-builder"),
                                 "-context", url], env=env, stdout=log,
                                stderr=subprocess.STDOUT, start_new_session=True)
        deadline = time.monotonic()+180
        try:
            while proc.poll() is None and time.monotonic()<deadline:
                for tag in ("arrows", "hidden", "restored"):
                    ready = work / (tag+".ready")
                    if ready.exists():
                        time.sleep(0.5)
                        subprocess.run(["import", "-display", display.name,
                                        "-window", "root", str(png / (work.name+"-"+tag+".png"))],
                                       check=True)
                        ready.unlink()
                        (work / (tag+".go")).touch()
                time.sleep(0.2)
            require(proc.poll() == 0, "Builder failed or timed out")
        finally:
            if proc.poll() is None:
                apps._terminate(proc)
    text = logpath.read_text(errors="replace")
    shutil.copy(logpath, png / (work.name+".log"))
    require("FAIL" not in text and "unknown command" not in text, text[-3000:])
    require("vector STRESS arrows=6" in text, "three pairs of arrows")
    require("Tension positive; Quantum ESPRESSO's own output has the opposite sign." in " ".join(text.split()),
            "sign convention in summary")
    states=re.findall(r"\[Stress Tensor shown=(\d) focus=(\d) box=([01-])\]",text)
    require(states==[("1","1","1"),("1","0","0"),("1","1","1"),("0","0","0")],
            "panel focus follows checkbox and panel replacement: "+repr(states))
    return " ".join(text.split())


def gui(work, png):
    import apps
    import isolate
    import orca_mo_test as base
    import xdisplay
    from PIL import Image, ImageChops

    os.environ["XDG_CACHE_HOME"] = str(work / "cache")
    settings = isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    data = work / "data"
    (data / "users/local").mkdir(parents=True)
    os.environ["ECCE_LOCAL_DATA"] = str(data)
    source = REPO / "data/client/examples/genchem/packing-in-solids/qe-al_fcc/stored/al_fcc"
    xdisplay.SCREEN = "1700x1100x24"
    with xdisplay.Display() as display:
        try:
            # The existing QE example has a tensile final cell (P=-4.84 kbar).
            url = base.makeCalc(str(data), "qe-tension", source=str(source))
            text = builder(display, url, work / "qe", png)
            require("Pressure p = -tr(sigma)/3: -0.4840 GPa (-4.8400 kbar)" in text,
                    "QE pressure matches the stored PRESSURE")
            require(text.count("+0.4840 GPa (+4.8400 kbar)")==3,
                    "QE principal values are tensile")
            require("shrink along a and b and c" in text, "QE relaxation sense")
            # A known mixed-sign tensor on the same periodic fixture exercises
            # inward arrows, off-axis directions and unequal lengths.
            url = base.makeCalc(str(data), "mixed", source=str(source))
            calc = Path(url.removeprefix("file://"))
            meta=calc / ".ecce-meta"
            meta.write_text(meta.read_text().replace("\tQuantumESPRESSO\n","\tMLP\n"))
            (calc / "Props/STRESS").write_text(
                '<vector name="STRESS" rows="9" units="kbar">'
                '-20 -10 0 -10 -20 0 0 0 20</vector>\n')
            text = builder(display, url, work / "mixed", png)
            require("-0.6667 GPa (-6.6667 kbar)" in text, "mixed pressure")
            require("Von Mises: 4.3589 GPa; max shear: 2.5000 GPa" in text,
                    "mixed shear")
            pattern = r"vector STRESS (\d+) start=(\S+) dir=(\S+) length=(\S+)"
            rows = re.findall(pattern, text)
            require(len(rows)==6, "six measured arrow geometries")
            lengths = [float(row[3]) for row in rows]
            require(abs(lengths[0]/lengths[2]-3)<0.002 and
                    abs(lengths[4]/lengths[2]-2)<0.002, "length ratios 3:1:2")
            center = [2.025]*3
            for index, start, direction, length in rows:
                start = [float(x) for x in start.split(",")]
                direction = [float(x) for x in direction.split(",")]
                tip = [s+float(length)*d for s,d in zip(start,direction)]
                end = start if int(index)<4 else tip
                require(math.dist(center,end)<0.003,
                        "tension starts and compression ends at cell centre")
                axis = int(index)//2
                expected = [(2**-0.5,2**-0.5,0), (2**-0.5,-2**-0.5,0), (0,0,1)][axis]
                require(abs(sum(a*b for a,b in zip(direction,expected)))>0.999,
                        "principal direction matches hand calculation")
            for name in ("qe", "mixed"):
                a=Image.open(png / (name+"-arrows.png")).convert("RGB")
                b=Image.open(png / (name+"-hidden.png")).convert("RGB")
                # Restrict to the viewer, excluding changes in the panel's box.
                crop=(50,160,950,800)
                require(ImageChops.difference(a.crop(crop),b.crop(crop)).getbbox() is not None,
                        "Show in viewer changes rendered pixels")
            print("PASS Builder QE tensile cell, synthetic MLP mixed signs, principal directions, length ratios, summary, viewer toggle, screenshots")
        finally:
            apps.stopServices(display)
            isolate.killLeftovers(settings["ECCE_REALUSERHOME"])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unit-only", action="store_true")
    parser.add_argument("--png", type=Path)
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix=".stress-test-",dir=REPO) as temp:
        work=Path(temp)
        unit(work)
        if not args.unit_only:
            png=args.png.resolve() if args.png else work / "png"
            png.mkdir(parents=True,exist_ok=True)
            gui(work,png)
    return 0


if __name__=="__main__":
    raise SystemExit(main())
