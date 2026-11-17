#!/usr/bin/env python3
"""NMR panel: real ORCA parser fixture, references, grouping, picks and exports."""
import csv
import io
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "launch"))
sys.path.insert(0, os.path.join(ROOT, "tests", "spectrum"))
import builder_overlay_test as bo
import harness
import makecalc
import xdisplay


def table(text, header):
    lines = text.splitlines()
    start = lines.index(header)
    end = next((i for i in range(start + 1, len(lines)) if not lines[i]), len(lines))
    return list(csv.DictReader(io.StringIO("\n".join(lines[start:end]))))


def main():
    build = os.path.join(ROOT, "build-cmake")
    harness.prerequisites(build, ())
    s = harness.Session(build, "nmr-spectrum", {}, local=True)
    display = None
    try:
        for exe in bo.GUI:
            harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
        wrappers = os.path.join(s.state, "wrappers")
        shutil.copytree(os.path.join(build, "wrappers"), wrappers)
        for entry in os.listdir(wrappers):
            os.chmod(os.path.join(wrappers, entry), 0o755)
        if not s.services(True):
            bo.check(False, "services started")
            return 1
        display = xdisplay.Display().__enter__()
        env = s.env({"HOME": s.state, "DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "nmr")
        os.makedirs(project)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        props = makecalc.parse("orca-h2o-nmr")
        # The existing stored water scaffold has the same O,H,H atom order.
        for name in ("sample", "reference"):
            calc = os.path.join(project, name)
            shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-vib"), calc)
            shutil.copy(os.path.join(HERE, "fixtures", "calc-water-freq-g16", ".ecce-meta"), calc)
            meta = os.path.join(calc, ".ecce-meta")
            with open(meta) as f:
                text = f.read().replace("Gaussian-16", "ORCA")
            with open(meta, "w") as f:
                f.write(text)
            for key in ("ISOSHIELD", "SHIELDTENSOR"):
                with open(os.path.join(calc, "Props", key), "w") as f:
                    f.write(makecalc.vector(key, props[key], "Atom"))
        empty, entered, reference, oxygen, again = [os.path.join(s.state, n + ".csv")
                                                    for n in ("empty", "entered", "reference", "oxygen", "again")]
        png = os.path.join(s.state, "nmr.png")
        lines = ["wait 2000", "background 1 1 1", "property NMR spectrum", "nmr status",
                 "nmr csv " + empty, "nmr element H", "nmr reference 34.679",
                 "nmr width 0.05", "nmr csv " + entered, "nmr click 1", "nmr png " + png,
                 "nmr calculation reference", "nmr csv " + reference,
                 "nmr element O", "nmr status", "nmr reference 370", "nmr csv " + oxygen,
                 "nmr element H", "nmr csv " + again, "nmr reference empty", "nmr status", "quit"]
        text, _ = bo.runScript(s, display, env, wrappers,
                              ["-context", "file://" + os.path.join(project, "sample") + "/"],
                              lines, s.state, "nmr", {"ECCE_PANEL_MODE": "detail"})
        bo.check(not os.path.exists(empty), "no export without a reference")
        bo.check("element=H groups=0" in text, "initial and cleared references give no spectrum")
        bo.check("NMR selected: 2 3" in text, "clicking the grouped stick selects both hydrogens")
        bo.check("reversed=1 width=0.05" in text, "reversed axis and ppm broadening")
        bo.check("element=O groups=0" in text, "references are independent per element")
        bo.check(os.path.exists(png) and os.path.getsize(png) > 1000, "PNG export")
        for path in (entered, reference, oxygen, again):
            bo.check(os.path.exists(path), "CSV export " + os.path.basename(path))
        if not all(os.path.exists(p) for p in (entered, reference, oxygen, again)):
            print(text[-6000:])
            return 1
        with open(entered) as f:
            data = f.read()
        gs = table(data, "group,element,shift_ppm,count,atoms_1based")
        bo.check(len(gs) == 1 and gs[0]["count"] == "2" and gs[0]["atoms_1based"] == "2 3"
                 and abs(float(gs[0]["shift_ppm"]) - 1) < 1e-9,
                 "entered reference minus parsed shielding, grouped count and atom IDs")
        curve = table(data, "shift_ppm,envelope_nuclei_per_ppm")
        peak = max(curve, key=lambda r: float(r["envelope_nuclei_per_ppm"]))
        bo.check(abs(float(peak["shift_ppm"]) - 1) < 0.002, "broadened peak at the chemical shift")
        bo.check(float(curve[0]["shift_ppm"]) > float(curve[-1]["shift_ppm"]), "CSV curve descends in ppm")
        with open(reference) as f:
            ref = f.read()
        with open(again) as f:
            repeated = f.read()
        rgs = table(ref, "group,element,shift_ppm,count,atoms_1based")
        bo.check(abs(float(rgs[0]["shift_ppm"])) < 1e-9 and "Mean shielding: reference" in ref,
                 "reference calculation averages matching element, with provenance")
        bo.check(ref == repeated, "changing elements preserves references")
        with open(oxygen) as f:
            ogs = table(f.read(), "group,element,shift_ppm,count,atoms_1based")
        bo.check(len(ogs) == 1 and abs(float(ogs[0]["shift_ppm"]) - 4.306) < 1e-9,
                 "other element allowed with an explicit reference")
        bo.check("Segmentation fault" not in text and "exception:" not in text, "no Builder exception")
        if os.environ.get("ECCE_NMR_TEST_ARTIFACTS"):
            dest = os.environ["ECCE_NMR_TEST_ARTIFACTS"]
            os.makedirs(dest, exist_ok=True)
            shutil.copy(png, dest)
            with open(os.path.join(dest, "builder.log"), "w") as f:
                f.write(text)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
