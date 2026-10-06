#!/usr/bin/env python3
"""Smoke test for the spectrum canvas (#214).

    tests/spectrum/run_tests.py [-k]        -k keeps the PNGs (printed)

Builds tools/spectrum/render, which paints the real SpectrumCanvas onto a
bitmap, and runs it on

  * the numbers the codes themselves printed (tests/spectrum/oracle.py
    reads them from the raw fixtures, not through scripts/parsers):
    Gaussian 16 water and CO, ORCA water (IR and Raman), NWChem water (IR
    only);
  * a synthetic spectrum with an imaginary mode.

It checks that what the canvas holds equals the oracle's numbers, that
high wavenumbers are on the left (and on the right with --forward), that
translations/rotations are not drawn, and that every picture painted.  It
cannot say whether a picture is good; look at the PNGs.

Needs wx, g++ and xvfb-run; exits 77 (skip) without them.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

import oracle  # noqa: E402

failures = 0


def check(ok, what):
    global failures
    if not ok:
        failures += 1
        print("  FAIL  " + what)
    return ok


def build(out):
    flags = subprocess.run(["wx-config", "--cxxflags"], capture_output=True,
                           text=True).stdout.split()
    libs = subprocess.run(["wx-config", "--libs", "core,base"],
                          capture_output=True, text=True).stdout.split()
    result = subprocess.run(
        ["g++", "-std=c++17", "-O1", "-w", "-I", os.path.join(ROOT, "include"),
         "-I", os.path.join(ROOT, "src", "apps", "builder")] + flags +
        ["-o", out, os.path.join(ROOT, "tools", "spectrum", "render.C"),
         os.path.join(ROOT, "src", "tdat", "chemistry", "VibSpectrum.C")] +
        libs, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr[-2000:])
    return result.returncode == 0


def run(render, spec, png, *options):
    command = ["xvfb-run", "-a", render, spec, png, "--dump"] + list(options)
    proc = subprocess.run(command, capture_output=True, text=True)
    sticks = []
    ink = 0
    for line in proc.stdout.splitlines():
        f = line.split()
        if f and f[0] == "stick":
            sticks.append(dict(kind=f[1], mode=int(f[2]), wn=float(f[3]),
                               inten=float(f[4]), irrep=f[5], x=int(f[6]),
                               y=int(f[7]), imaginary="imaginary" in f))
        m = re.search(r": (\d+) ink", line)
        if m:
            ink = int(m.group(1))
    return proc.returncode, sticks, ink, proc.stderr


def near(a, b):
    return abs(a - b) <= 1e-6 * max(1.0, abs(b))


def main():
    keep = "-k" in sys.argv
    if shutil.which("wx-config") is None or shutil.which("xvfb-run") is None \
       or shutil.which("g++") is None:
        print("SKIP: needs wx-config, g++ and xvfb-run")
        return 77

    work = tempfile.mkdtemp(prefix="spectrum-test-")
    try:
        render = os.path.join(work, "render")
        if not build(render):
            print("FAIL: tools/spectrum/render does not build")
            return 1

        fixtures = os.path.join(ROOT, "tests", "parsers", "fixtures")
        for name, reader, rel in oracle.CASES:
            spec = reader(os.path.join(fixtures, rel))
            specPath = os.path.join(work, name + ".spec")
            oracle.write(spec, specPath)
            png = os.path.join(work, name + ".png")
            rc, sticks, ink, err = run(render, specPath, png)
            print("%s" % name)
            if not check(rc == 0, "render exited %d: %s" % (rc, err[-300:])):
                continue
            check(ink > 5000, "picture is nearly blank (%d ink)" % ink)

            #  Every real vibration the code printed is a stick with the
            #  code's number; the near-zero modes are not drawn.
            for kind in ("ir", "raman"):
                want = spec[kind]
                if not want:
                    check(not [s for s in sticks if s["kind"] == kind],
                          "%s sticks drawn but the code printed none" % kind)
                    continue
                got = dict((s["mode"] - 1, s) for s in sticks
                           if s["kind"] == kind)
                expect = [i for i, f in enumerate(spec["freq"]) if abs(f) >= 10]
                check(sorted(got) == expect,
                      "%s modes drawn %s, expected %s"
                      % (kind, sorted(got), expect))
                for i in expect:
                    if i in got:
                        check(near(got[i]["wn"], spec["freq"][i]) and
                              near(got[i]["inten"], want[i]),
                              "%s mode %d: canvas %s / %s, output file %s / %s"
                              % (kind, i + 1, got[i]["wn"], got[i]["inten"],
                                 spec["freq"][i], want[i]))
                #  High wavenumber on the left.
                ordered = sorted(got.values(), key=lambda s: s["wn"])
                xs = [s["x"] for s in ordered]
                check(xs == sorted(xs, reverse=True) and len(set(xs)) == len(xs),
                      "%s: x positions not strictly decreasing with "
                      "wavenumber: %s" % (kind, xs))
            if keep:
                shutil.copy(png, os.path.join("/tmp", name + ".png"))
                print("  kept /tmp/%s.png" % name)

        #  Axis direction toggle, zoom, themes, hover/selection all paint.
        water = os.path.join(work, "g16-h2o-optfreq.spec")
        rc, fwd, ink, err = run(render, water, os.path.join(work, "f.png"),
                                "--forward")
        xs = [s["x"] for s in sorted((s for s in fwd if s["kind"] == "ir"),
                                     key=lambda s: s["wn"])]
        check(rc == 0 and xs == sorted(xs), "--forward: low wavenumber is "
              "not on the left: %s" % xs)
        for label, opts in (("dark", ["--dark"]),
                            ("zoom", ["--zoom", "4000", "4500"]),
                            ("sticks only", ["--fwhm", "0"]),
                            ("envelope only", ["--no-sticks"]),
                            ("lorentzian", ["--lorentzian", "--fwhm", "30"]),
                            ("scaled", ["--scale", "0.96"]),
                            ("hover+select", ["--hover", "2", "--select", "3"]),
                            ("tiny window", ["--size", "120", "90"])):
            rc, st, ink, err = run(render, water,
                                   os.path.join(work, "x.png"), *opts)
            check(rc == 0 and ink > 500, "%s: rc %d, %d ink %s"
                  % (label, rc, ink, err[-200:]))
            if label == "zoom":
                #  Only the band asked for is on screen.
                check(all((4000 <= s["wn"] <= 4500) == (s["x"] >= 0)
                          for s in st), "zoom: drawn sticks %s"
                      % [(s["wn"], s["x"]) for s in st])
            if label == "scaled":
                got = [s["wn"] for s in st if s["kind"] == "ir"]
                check(near(got[1], 4141.5426 * 0.96), "scaling not applied")

        #  Imaginary mode: kept, flagged, on the negative side of zero.
        imag = os.path.join(HERE, "data", "imaginary.spec")
        rc, st, ink, err = run(render, imag, os.path.join(work, "i.png"))
        flagged = [s for s in st if s["imaginary"]]
        print("imaginary")
        check(rc == 0 and len(flagged) == 2 and flagged[0]["wn"] < 0,
              "imaginary mode not kept and flagged: %s %s" % (rc, flagged))
        if keep:
            shutil.copy(os.path.join(work, "i.png"), "/tmp/imaginary.png")
    finally:
        shutil.rmtree(work, ignore_errors=True)

    print("FAILED %d" % failures if failures else "spectrum canvas: PASS")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
