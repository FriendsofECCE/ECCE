#!/usr/bin/env python3
"""The Properties list groups spectra and diffraction by technique (#1079).

    tests/apps/property_groups_test.py [--build build-cmake] [--png DIR]

Two calculations, headless: calc-water-freq-g16 (frequencies) with NMR shieldings
and excitation energies added to its Props, and the NaCl periodic example
(Diffraction).  The Builder script command "plist" prints one
"BUILDER: entry group/name" line per row of the list.  Checked: Vibrational spectra, Electronic spectra,
Magnetic resonance and Diffraction and scattering hold their rows, the old
group names are gone, and Diffraction is no longer under Geometry.

Exit status 77 (skip) without Xvfb or the built binaries.
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import harness  # noqa: E402
import packing_cells_test as pc  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
OLD = ("Vibrations/", "Excited states/", "Magnetic properties/")


def addProps(calc):
    props = os.path.join(calc, "Props")
    head = '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
    docs = {
        "ISOSHIELD": '<vector name="ISOSHIELD" rowLabel="Atom" rows="3" '
                     'units="PPM">320.1   31.2   31.2</vector>',
        "ANISOSHIELD": '<vector name="ANISOSHIELD" rowLabel="Atom" rows="3" '
                       'units="PPM">45.0   12.5   12.5</vector>',
        "EXCIENERGY": '<vector name="EXCIENERGY" rowLabel="Excited State" '
                      'rows="2" units="NA">0.30   0.45</vector>',
        "EXCIOSC": '<vector name="EXCIOSC" rowLabel="Excited State" '
                   'rows="2" units="NA">0.05   0.10</vector>',
    }
    docs["VIBFREQ"] = ('<vector name="VIBFREQ" rowLabel="Normal Mode" rows="3" '
                       'units="cm-1">1645.0   3866.2   3965.0</vector>')
    src = os.path.join(HERE, "fixtures", "calc-water-vib", "Props")
    for key in ("VIB", "DIPOLE"):
        shutil.copy(os.path.join(src, key), os.path.join(props, key))
        with open(os.path.join(props, ".ecce-meta"), "a") as f:
            f.write(key + "\thttp://www.emsl.pnl.gov/ecce:mime-type\t\ttext/xml\n")
    for key, doc in docs.items():
        with open(os.path.join(props, key), "w") as f:
            f.write(head + doc + "\n")
        with open(os.path.join(props, ".ecce-meta"), "a") as f:
            f.write(key + "\thttp://www.emsl.pnl.gov/ecce:mime-type\t\ttext/xml\n")


def entries(text):
    return [l[len("BUILDER: entry "):].strip() for l in text.splitlines()
            if l.startswith("BUILDER: entry ")]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "propgroups", {}, local=True)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    work = os.path.join(s.state, "fixtures")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-freq-g16"),
                    os.path.join(work, "calc-water-freq-g16"), symlinks=True)
    addProps(os.path.join(work, "calc-water-freq-g16"))
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        extra = {"ECCE_PANEL_MODE": "detail"}

        url = eg.install(s, "calc-water-freq-g16", work)
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", url],
                               ["wait 4000", "plist", "shot list", "quit"], png,
                               "vib", extra)
        got = entries(text)
        say("entries:\n  " + "\n  ".join(got))
        for want in ("Vibrational spectra/Vibrational Frequencies",
                     "Electronic spectra/UV/Vis Spectrum",
                     "Magnetic resonance/Isotropic Shielding",
                     "Electric properties/Dipole Moment",
                     "Magnetic resonance/Magnetic Shielding Anisotropy"):
            check(want in got, "listed: " + want)
        check(not any(e.startswith(OLD) for e in got),
              "no entry under the old group names")
        check(not any(e.startswith("Geometry/Diffraction") for e in got),
              "no Diffraction under Geometry in a non-periodic calculation")
        groups = [e.split("/")[0] for e in got]
        check(all(g in groups for g in ("Electric properties", "Energies")),
              "Electric properties and Energies stay: %s" % sorted(set(groups)))

        example = os.path.join(REPO, "data", "client", "examples", "genchem",
                               "diffraction", "qe-nacl_powder", "nacl_powder.pwout")
        url = pc.importExample(s, env, wrappers, example)
        if not check(bool(url), "NaCl example imports"):
            return 1
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", url],
                               ["wait 4000", "plist", "quit"], png, "nacl", extra)
        got = entries(text)
        say("entries:\n  " + "\n  ".join(got))
        check("Diffraction and scattering/Diffraction" in got,
              "Diffraction is under Diffraction and scattering")
        check(not any(e.startswith("Geometry/Diffraction") for e in got),
              "Diffraction is not under Geometry")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
