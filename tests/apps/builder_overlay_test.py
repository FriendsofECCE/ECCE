#!/usr/bin/env python3
"""Builder property overlays, the calculation thumbnail, and File > New for
MD studies, headless, with the tree's own binaries.

    tests/apps/builder_overlay_test.py [--build build-cmake] [--png DIR]

Overlays (fixtures/calc-water-opt, NWChem water with a dipole, Mulliken
charges and MOs), in the list + detail layout and in the classic one, each
step a user's own action through the handler it reaches (Builder script
commands "list", "property", "viewer"):

  * choosing Dipole Moment in the Properties list (or ticking it in the
    Properties menu) puts its vector in the viewer at once;
  * choosing another panel removes the first one's overlay;
  * the panel's "Show in viewer" box takes the overlay off and puts it back;
  * closing a panel from the menu removes its overlay.

Each step prints the viz focus of every panel ("overlay") and the viewer is
photographed; the overlay must show as pixels that differ from the plain
molecule.

Thumbnail: File > Save of a new structure writes Thumbnail.jpeg, which the
Organizer then shows on the calculation's Builder button.

New: the Organizer's New of a GROMACS MD study and of an Energy task in it,
and of an NWChem MD study and its Prepare task, through CalcMgr's own
createResource (Organizer hook "new"); the GROMACS Energy task then opens
in the MD Energy editor.

Exit status 77 (skip) without Xvfb, import, python3-pil/numpy or the built
binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import gromacs_test  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

GUI = ("builder", "organizer", "mdenergy")
FIXTURE = os.path.join(HERE, "fixtures", "calc-water-opt")
PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data",
                            "project.ecce-meta")

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def stop(proc):
    if proc.poll() is None:
        try:
            os.killpg(proc.pid, 15)
            proc.wait(timeout=20)
        except Exception:
            try:
                os.killpg(proc.pid, 9)
            except Exception:
                pass


def largestWindow(display, env):
    """x, y, w, h of the largest top-level window (the app's frame)."""
    best = None
    for wid, _title in display.windows():
        info = subprocess.run(["xwininfo", "-id", wid], env=env,
                              capture_output=True, text=True).stdout
        geo = {}
        for line in info.splitlines():
            for key in ("Absolute upper-left X", "Absolute upper-left Y",
                        "Width", "Height"):
                if line.strip().startswith(key + ":"):
                    geo[key] = int(line.split(":")[1])
        if len(geo) == 4 and (best is None or
                              geo["Width"] * geo["Height"] > best[2] * best[3]):
            best = (geo["Absolute upper-left X"], geo["Absolute upper-left Y"],
                    geo["Width"], geo["Height"])
    return best


def snap(display, env, path):
    """The screen cut to the largest window: importing a GL frame by id
    fails under Xvfb."""
    argv = ["import", "-window", "root"]
    best = largestWindow(display, env)
    if best:
        argv += ["-crop", "%dx%d+%d+%d" % (best[2], best[3], best[0], best[1]),
                 "+repage"]
    subprocess.run(argv + [path], env=env, timeout=60,
                   stderr=subprocess.DEVNULL)
    return os.path.exists(path)


def viewerBox(img):
    """The viewer in a capture: the rows and columns that are mostly the
    viewer's black background."""
    import numpy
    px = numpy.asarray(img.convert("RGB")).astype(int)
    black = px.sum(axis=2) < 30
    cols = numpy.where(black.mean(axis=0) > 0.3)[0]
    rows = numpy.where(black[:, cols].mean(axis=1) > 0.5)[0] \
        if len(cols) else []
    if not len(rows) or not len(cols):
        return None
    return (int(cols[0]), int(rows[0]), int(cols[-1]) + 1, int(rows[-1]) + 1)


def differs(a, b):
    """Pixels of the viewer that differ between two captures."""
    from PIL import Image
    import numpy
    ia, ib = Image.open(a), Image.open(b)
    box = viewerBox(ia)
    if box is None or ia.size != ib.size:
        return -1
    pa = numpy.asarray(ia.convert("RGB").crop(box)).astype(int)
    pb = numpy.asarray(ib.convert("RGB").crop(box)).astype(int)
    return int((numpy.abs(pa - pb).max(axis=2) > 40).sum())


def tinted(path, which):
    """Viewer pixels of the dipole's yellow or Mulliken's blue (the
    hydrogens' positive charge): neither is in the plain molecule."""
    from PIL import Image
    import numpy
    img = Image.open(path)
    box = viewerBox(img)
    if box is None:
        return -1
    p = numpy.asarray(img.convert("RGB").crop(box)).astype(int)
    r, g, b = p[..., 0], p[..., 1], p[..., 2]
    if which == "yellow":
        # Pure yellow, or the arrow's see-through part blended at 40 %
        # over the atoms (about 113,113,10): red and green alike, no blue.
        return int(((r > 80) & (g > 80) & (b < 60) &
                    (abs(r - g) < 25)).sum())
    # The charge blue is lit and shaded by the viewer, so its green channel
    # is about half its blue (e.g. 60,111,241), not near zero: test for a
    # clearly blue hue. White hydrogens and red oxygen never satisfy it.
    return int(((b > 100) & (b > r + 60) & (b > g + 30)).sum())


def runScript(s, display, env, wrappers, argv, lines, png, prefix, extra=None):
    """Runs the Builder with ECCE_BUILDER_SCRIPT; photographs each "shot"."""
    work = os.path.join(s.state, prefix)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    script = os.path.join(work, "script")
    with open(script, "w") as handle:
        handle.write("\n".join(lines) + "\n")
    log = os.path.join(work, "builder.log")
    benv = dict(env, ECCE_BUILDER_SCRIPT=script, **(extra or {}))
    proc = subprocess.Popen([os.path.join(wrappers, "ecce-builder")] + argv,
                            env=benv, cwd=os.path.join(s.home, "bin"),
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    shots = [l.split()[1] for l in lines if l.startswith("shot ")]
    taken = {}
    deadline = time.time() + 300
    try:
        while shots and time.time() < deadline and proc.poll() is None:
            name = shots[0]
            ready = os.path.join(work, name + ".ready")
            if os.path.exists(ready):
                time.sleep(1.5)
                path = os.path.join(png, "%s-%s.png" % (prefix, name))
                if snap(display, env, path):
                    taken[name] = path
                os.unlink(ready)
                open(os.path.join(work, name + ".go"), "w").close()
                shots.pop(0)
            time.sleep(0.3)
        while proc.poll() is None and time.time() < deadline:
            with open(log) as handle:
                if "ECCE_BUILDER_SCRIPT: quit" in handle.read():
                    break
            time.sleep(0.5)
        time.sleep(2)
    finally:
        stop(proc)
    with open(log, errors="replace") as handle:
        text = handle.read()
    return text, taken


def overlays(text):
    """The "overlay" lines: one dict per line, panel -> focus."""
    result = []
    for line in text.splitlines():
        if line.startswith("BUILDER: overlay"):
            state = {"_tab": line.split("tab=")[1].split()[0]
                     if "tab=" in line else "?"}
            for part in line.split("[")[1:]:
                part = part.rstrip("] ")
                name = part.split(" shown=")[0]
                state[name] = dict(kv.split("=") for kv in
                                   part[len(name):].split())
            result.append(state)
    return result


def focused(state):
    return sorted(n for n, v in state.items()
                  if n != "_tab" and v.get("focus") == "1")


def overlayCase(s, display, env, wrappers, png):
    data = s.localData()
    project = os.path.join(data, "users", "local", "overlay")
    os.makedirs(project, exist_ok=True)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, "water-opt")
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(FIXTURE, calc, symlinks=True)
    url = "file://" + calc + "/"

    say("list + detail layout, choices from the Properties list")
    lines = ["wait 4000", "overlay", "shot plain",
             "list Dipole Moment", "wait 1500", "overlay", "shot dipole",
             "list Mulliken Charges", "wait 1500", "overlay", "shot mulliken",
             "viewer Mulliken Charges", "wait 1000", "overlay", "shot cleared",
             "viewer Mulliken Charges", "wait 1000", "overlay",
             "list MOs", "wait 1500", "overlay", "shot mos",
             "quit"]
    text, shots = runScript(s, display, env, wrappers, ["-context", url],
                            lines, png, "detail",
                            {"ECCE_PANEL_MODE": "detail"})
    states = overlays(text)
    if not check(len(states) == 6, "six overlay reports (%d)" % len(states)):
        say(text[-3000:])
        return
    check(focused(states[0]) == [], "nothing drawn on opening: %s"
          % focused(states[0]))
    check(states[0]["_tab"] == "1", "a finished calculation opens on the "
          "Properties tab (tab=%s)" % states[0]["_tab"])
    check(focused(states[1]) == ["Dipole Moment"],
          "Dipole Moment from the list has the viewer: %s" % focused(states[1]))
    check(focused(states[2]) == ["Mulliken Charges"],
          "Mulliken Charges takes it from Dipole Moment: %s"
          % focused(states[2]))
    check(focused(states[3]) == [] and
          states[3]["Mulliken Charges"].get("box") == "0",
          "Show in viewer unticked clears it: %s" % states[3])
    check(focused(states[4]) == ["Mulliken Charges"],
          "ticked again, it is back: %s" % focused(states[4]))
    check("Mulliken Charges" not in focused(states[5]),
          "opening MOs takes the Mulliken colours off: %s"
          % focused(states[5]))
    for name, colour, want in (("plain", "yellow", False),
                               ("plain", "blue", False),
                               ("dipole", "yellow", True),
                               ("mulliken", "blue", True),
                               ("cleared", "blue", False),
                               ("mos", "blue", False)):
        if name in shots:
            n = tinted(shots[name], colour)
            check((n > 30) == want, "%s: %d %s pixels in the viewer"
                  % (name, n, colour))

    from PIL import Image
    edges = {n: (viewerBox(Image.open(shots[n])) or (0, 0, 0, 0))[2]
             for n in ("plain", "dipole", "mulliken", "mos") if n in shots}
    check(len(set(edges.values())) == 1,
          "the panel column keeps its width across panels: %s" % edges)

    say("classic layout, the Properties menu")
    lines = ["wait 4000", "overlay", "shot plain",
             "property Dipole Moment", "wait 1500", "overlay", "shot dipole",
             "vector DIPOLE",
             "property Dipole Moment", "wait 1500", "overlay", "shot closed",
             "quit"]
    text, shots = runScript(s, display, env, wrappers, ["-context", url],
                            lines, png, "classic",
                            {"ECCE_PANEL_MODE": "classic"})
    states = overlays(text)
    if check(len(states) == 3, "three overlay reports (%d)" % len(states)):
        check(focused(states[1]) == ["Dipole Moment"],
              "Properties > Dipole Moment has the viewer: %s"
              % focused(states[1]))
        check(focused(states[2]) == [],
              "closing it from the menu clears it: %s" % focused(states[2]))
    dipoleArrow(text)
    for name, want in (("plain", False), ("dipole", True), ("closed", False)):
        if name in shots:
            n = tinted(shots[name], "yellow")
            check((n > 3) == want, "%s: %d yellow pixels in the viewer"
                  % (name, n))

    say("Quadrupole Moment draws no arrow (#266), no ellipsoid (#279), "
        "an angular surface (#428)")
    lines = ["wait 4000", "list Quadrapole Moment", "wait 1500", "overlay",
             "ellipsoid QUADPOLE", "surface QUADPOLE", "shot quad", "quit"]
    text, shots = runScript(s, display, env, wrappers, ["-context", url],
                            lines, png, "quad",
                            {"ECCE_PANEL_MODE": "detail"})
    if "quad" in shots:
        n = tinted(shots["quad"], "yellow")
        check(n <= 3, "quad: %d yellow pixels in the viewer" % n)
    check("BUILDER: ellipsoid QUADPOLE count=0" in text,
          "no ellipsoid under the quadrupole")
    check("BUILDER: surface QUADPOLE count=1" in text,
          "the quadrupole is one angular surface (#428)")
    hfCase(s, display, env, wrappers, png)
    polarCase(s, display, env, wrappers, png)


def moleculeCalc(s, name, atoms, bonds, dipole=None):
    """A calculation of the given atoms [(symbol, x, y, z)] and bonds
    [(i, j)] (1-based), made from the water fixture: its chemsys replaced
    and its other properties dropped, a DIPOLE (Debye) written if given."""
    project = os.path.join(s.localData(), "users", "local", "overlay")
    os.makedirs(project, exist_ok=True)
    if not os.path.exists(os.path.join(project, ".ecce-meta")):
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(FIXTURE, calc, symlinks=True)
    text = "title: %s\ntype: molecule\nnum_atoms: %d\n" % (name, len(atoms))
    text += "atom_info: symbol cart\natom_list: \n"
    text += "".join("%s %.5f %.5f %.5f\n" % a for a in atoms)
    text += "attr_list:\n" + " 0.000000   1   0   0   0\n" * len(atoms)
    text += "atom_type_list:\n" + "        6   2\n" * len(atoms)
    text += "num_bonds: %d\nbond_list: \n" % len(bonds)
    text += "".join("%d %d 1.00000 2\n" % b for b in bonds)
    with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as out:
        out.write(text)
    for prop in os.listdir(os.path.join(calc, "Props")):
        if not prop.startswith(".") and not (dipole and prop == "DIPOLE"):
            os.unlink(os.path.join(calc, "Props", prop))
    if dipole:
        with open(os.path.join(calc, "Props", "DIPOLE"), "w") as out:
            out.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                      '<vector name="DIPOLE" rowLabel="Coordinate" '
                      'rowLabels="x y z" rows="3" units="Debye">%s %s %s'
                      '</vector>\n' % dipole)
    return calc


def hfCase(s, display, env, wrappers, png):
    """A diatomic's dipole arrow lies inside the bond; it must still show
    (drawn again with the depth test off)."""
    say("HF, Ball and Stick: the dipole arrow is not hidden by the bond")
    calc = moleculeCalc(s, "hf", [("F", 0, 0, 0), ("H", 0.92, 0, 0)],
                        [(1, 2)], dipole=(1.9, 0, 0))
    lines = ["wait 4000", "list Dipole Moment", "wait 1500", "vector DIPOLE",
             "shot hf", "quit"]
    text, shots = runScript(s, display, env, wrappers,
                            ["-context", "file://" + calc + "/"], lines, png,
                            "hf", {"ECCE_PANEL_MODE": "detail"})
    if "hf" not in shots:
        check(False, "HF photographed")
        return
    from PIL import Image
    import numpy
    img = Image.open(shots["hf"])
    box = viewerBox(img)
    p = numpy.asarray(img.convert("RGB").crop(box)).astype(int)
    r, g, b = p[..., 0], p[..., 1], p[..., 2]
    #  The arrow's own yellow (r and g alike, no blue); the light green of
    #  fluorine is not it.
    n = int(((r > 120) & (abs(r - g) < 35) & (b < 0.5 * r)).sum())
    #  About 85 without the second pass (a sliver at the hydrogen), 222 with
    #  it now that the arrow is thinner (429 at the old 0.06 A shaft).
    check(n > 150, "hf: %d yellow pixels in the viewer" % n)



def coloursCase(s, display, env, wrappers, png):
    """Preferences > Colours: HF and CFClBrI in Jmol and ECCE (classic)
    colours.  The scheme is the saved preference, as the dialog writes it."""
    import numpy
    from PIL import Image
    say("Element colours: Jmol (default) and ECCE classic")
    t = 1.0 / 3 ** 0.5
    halo = [("C", 0, 0, 0)]
    for sym, d, v in (("F", 1.35, (1, 1, 1)), ("Cl", 1.77, (1, -1, -1)),
                      ("Br", 1.94, (-1, 1, -1)), ("I", 2.14, (-1, -1, 1))):
        halo.append((sym, d * t * v[0], d * t * v[1], d * t * v[2]))
    molecules = (("hf", moleculeCalc(s, "hf-col", [("F", 0, 0, 0),
                                                   ("H", 0.92, 0, 0)],
                                     [(1, 2)])),
                 ("halogens", moleculeCalc(s, "halogens", halo,
                                           [(1, 2), (1, 3), (1, 4), (1, 5)])))
    prefs = os.path.join(s.state, ".ECCE", "EcceGlobal")
    found = {}
    for scheme in ("jmol", "ecce"):
        with open(prefs, "w") as out:
            out.write("ElementColours:\t%s\n" % scheme)
        for name, calc in molecules:
            lines = ["wait 4000", "shot x", "quit"]
            text, shots = runScript(
                s, display, env, wrappers, ["-context", "file://" + calc + "/"],
                lines, png, "colours-%s-%s" % (name, scheme),
                {"ECCE_PANEL_MODE": "detail"})
            if "x" not in shots:
                check(False, "%s in %s photographed" % (name, scheme))
                continue
            img = Image.open(shots["x"])
            p = numpy.asarray(img.convert("RGB").crop(viewerBox(img))).astype(int)
            r, g, b = p[..., 0], p[..., 1], p[..., 2]
            found[(name, scheme)] = (
                int(((g > 150) & (r < g - 30) & (b < g - 60)).sum()),  # green
                int(((b > 150) & (r > 90) & (g < 90)).sum()),          # purple
                int(((r > 110) & (g < 70) & (b < 70)).sum()))          # brown-red
    hf = [found.get(("hf", k), (0, 0, 0)) for k in ("jmol", "ecce")]
    check(hf[0][0] > 3000 and hf[0][1] < 800,
          "hf: Jmol fluorine is light green (%d green, %d purple)" % hf[0][:2])
    check(hf[1][1] > 3000 and hf[1][0] < 800,
          "hf: classic fluorine is purple (%d green, %d purple)" % hf[1][:2])
    hal = [found.get(("halogens", k), (0, 0, 0)) for k in ("jmol", "ecce")]
    check(hal[0][2] > 200 and hal[1][2] < 50,
          "halogens: Jmol bromine is brown-red, classic is not (%d, %d)"
          % (hal[0][2], hal[1][2]))


# Real polarizability tensors (HF, a.u.) from Gaussian 16 and ORCA 6.1.1 for
# water, CO2 along (1,2,2)/3 and benzene in the xy plane; the Gaussian ones
# went through gaussian-16.desc/.db, the ORCA ones (no POLARIZ parser) were
# read off the printed raw cartesian tensor.
POLAR_FIXTURES = ("calc-polar-h2o-g16", "calc-polar-co2-g16",
                  "calc-polar-c6h6-g16", "calc-polar-h2o-orca",
                  "calc-polar-co2-orca")


def polarTensor(calc):
    """xx xy xz yy yz zz of the fixture's POLARIZ, as a symmetric matrix."""
    import numpy
    import re
    with open(os.path.join(calc, "Props", "POLARIZ")) as handle:
        v = [float(x) for x in re.search(r"<vector [^>]*>([^<]*)<", handle.read())
             .group(1).split()]
    return numpy.array([[v[0], v[1], v[2]], [v[1], v[3], v[4]],
                        [v[2], v[4], v[5]]])


def polarAtoms(calc):
    import numpy
    atoms = []
    with open(os.path.join(calc, "Parameters", "chemsys.mvm")) as handle:
        lines = handle.read().splitlines()
    start = lines.index("atom_list: ") + 1
    for l in lines[start:]:
        w = l.split()
        if len(w) != 4:
            break
        atoms.append((w[0], numpy.array([float(x) for x in w[1:]])))
    return atoms


def ellipsoids(text):
    """The "ellipsoid POLARIZ k" lines: centre and (length, direction) of
    each axis, as the renderer will transform the unit sphere."""
    import numpy
    found = []
    for line in text.splitlines():
        if line.startswith("BUILDER: ellipsoid POLARIZ ") and "centre=" in line:
            kv = dict(p.split("=") for p in line.split()[4:])
            axes = []
            for i in range(3):
                n = [float(x) for x in kv["axis%d" % i].split(",")]
                axes.append((n[0], numpy.array(n[1:])))
            found.append((numpy.array([float(x) for x in
                                       kv["centre"].split(",")]), axes))
    return found


def polarCase(s, display, env, wrappers, png):
    import numpy
    say("Polarizability: a translucent ellipsoid on the principal axes (#279)")
    data = s.localData()
    project = os.path.join(data, "users", "local", "overlay")
    for name in POLAR_FIXTURES:
        calc = os.path.join(project, name)
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(os.path.join(HERE, "fixtures", name), calc,
                        symlinks=True)
        lines = ["wait 4000", "shot plain", "list Polarizability",
                 "wait 1500", "overlay", "ellipsoid POLARIZ", "shot polar",
                 "quit"]
        text, shots = runScript(s, display, env, wrappers,
                                ["-context", "file://" + calc + "/"], lines,
                                png, name, {"ECCE_PANEL_MODE": "detail"})
        found = ellipsoids(text)
        if not check(len(found) == 1, "%s: one ellipsoid (%d)"
                     % (name, len(found))):
            say(text[-2000:])
            continue
        centre, axes = found[0]
        atoms = polarAtoms(calc)
        z = numpy.array([{"H": 1, "C": 6, "O": 8}[a] for a, _ in atoms], float)
        xyz = numpy.array([p for _, p in atoms])
        want = (z[:, None] * xyz).sum(axis=0) / z.sum()
        check(numpy.abs(centre - want).max() < 0.002,
              "%s: centred on the centre of nuclear charge %s (%s)"
              % (name, numpy.round(want, 4), numpy.round(centre, 4)))
        # numpy's eigh is the oracle for the command's own Diag3.
        w, v = numpy.linalg.eigh(polarTensor(calc))
        order = numpy.argsort(w)[::-1]
        w, v = w[order], v[:, order]
        lengths = numpy.array([a[0] for a in axes])
        scale = lengths / w
        check(scale.std() / scale.mean() < 1e-3,
              "%s: semi-axes %s proportional to the eigenvalues %s"
              % (name, numpy.round(lengths, 3), numpy.round(w, 3)))
        check(abs(lengths.max() - 1.5) < 0.01,
              "%s: longest semi-axis %.3f A at the default scale"
              % (name, lengths.max()))
        for i in range(3):
            # A degenerate pair has no unique axes: compare the plane.
            degenerate = any(abs(w[i] - w[j]) < 1e-3 * w[0]
                             for j in range(3) if j != i)
            if not degenerate:
                dot = abs(float(numpy.dot(axes[i][1], v[:, i])))
                check(dot > 0.999, "%s: axis %d along the eigenvector "
                      "(|cos| %.5f)" % (name, i, dot))
        if "co2" in name:
            along = numpy.array([1, 2, 2]) / 3.0
            dot = abs(float(numpy.dot(axes[0][1], along)))
            check(dot > 0.999 and w[0] > 2 * w[1],
                  "%s: the long axis lies along the molecule (|cos| %.5f)"
                  % (name, dot))
        if "c6h6" in name:
            check(abs(w[0] - w[1]) < 1e-3 * w[0] and w[1] > 1.5 * w[2],
                  "%s: in-plane axes equal and larger than the out-of-plane "
                  "one: %s" % (name, numpy.round(w, 3)))
            check(abs(float(axes[2][1][2])) > 0.999,
                  "%s: the short axis is the ring normal %s"
                  % (name, numpy.round(axes[2][1], 4)))
        if "plain" in shots and "polar" in shots:
            n = differs(shots["plain"], shots["polar"])
            check(n > 200, "%s: the ellipsoid changes %d viewer pixels"
                  % (name, n))



def multipoleCase(s, display, env, wrappers, png):
    """#428: quadrupole, octupole and hexadecapole as angular surfaces, on
    the Gaussian water (all three stored), centred on the output's origin,
    the largest lobe 1.5 times the molecule's radius."""
    import numpy
    say("Multipoles: angular surfaces r = s|f(n)| (#428)")
    project = os.path.join(s.localData(), "users", "local", "overlay")
    # A Gaussian 16 calculation shell (it has the metadata) holding the water
    # structure and the three moments of calc-water-vib.
    name = "calc-multipole-h2o-g16"
    vib = os.path.join(HERE, "fixtures", "calc-water-vib")
    calc = os.path.join(project, name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-polar-h2o-g16"), calc,
                    symlinks=True)
    shutil.copy(os.path.join(vib, "Parameters", "chemsys.mvm"),
                os.path.join(calc, "Parameters", "chemsys.mvm"))
    for prop in ("QUADPOLE", "OCTAPOLE", "HEXPOLE"):
        shutil.copy(os.path.join(vib, "Props", prop),
                    os.path.join(calc, "Props", prop))
    atoms = polarAtoms(calc)
    z = numpy.array([{"H": 1, "O": 8}[a] for a, _ in atoms], float)
    xyz = numpy.array([p for _, p in atoms])
    want = (z[:, None] * xyz).sum(axis=0) / z.sum()
    radius = max(1.0, numpy.linalg.norm(xyz - want, axis=1).max())
    for label, prop, tag in (("Quadrapole Moment", "QUADPOLE", "quadrupole"),
                             ("Octupole Moment", "OCTAPOLE", "octupole"),
                             ("Hexadecapole Moment", "HEXPOLE", "hexadecapole")):
        lines = ["wait 4000", "shot plain", "list " + label, "wait 1500",
                 "overlay", "surface " + prop, "shot " + tag, "quit"]
        text, shots = runScript(s, display, env, wrappers,
                                ["-context", "file://" + calc + "/"], lines,
                                png, "multipole-428-" + tag,
                                {"ECCE_PANEL_MODE": "detail"})
        found = [l for l in text.splitlines()
                 if l.startswith("BUILDER: surface %s 0 " % prop)]
        if not check(len(found) == 1, "%s: one surface" % tag):
            say(text[-2000:])
            continue
        kv = dict(p.split("=") for p in found[0].split()[4:])
        centre = numpy.array([float(x) for x in kv["centre"].split(",")])
        # Gaussian reports the moments about its coordinate origin.
        check(numpy.abs(centre).max() < 0.002,
              "%s: centred on the output's origin (%s)"
              % (tag, numpy.round(centre, 4)))
        check(int(kv["vertices"]) == 10242 and int(kv["triangles"]) == 20480,
              "%s: icosphere mesh %s vertices" % (tag, kv["vertices"]))
        reach = float(kv["farthest"])
        check(abs(reach - 1.5 * radius) < 0.05 * radius,
              "%s: largest lobe %.3f A, 1.5 x radius %.3f A"
              % (tag, reach, radius))
        if "plain" in shots and tag in shots:
            n = differs(shots["plain"], shots[tag])
            check(n > 500, "%s: the surface changes %d viewer pixels"
                  % (tag, n))


def dipoleArrow(text):
    """#578: the arrow runs between the charge poles along mu, with a
    constant shaft and the corner text; the poles are checked in
    dipole_poles_test.py."""
    arrow = value = None
    for line in text.splitlines():
        if line.startswith("BUILDER: vector DIPOLE 0 "):
            arrow = dict(kv.split("=") for kv in line.split()[4:])
        elif line.startswith("BUILDER: vector DIPOLE value="):
            value = line.split("value=")[1].split()[0]
    if not check(arrow is not None and value is not None,
                 "the dipole arrow is measured"):
        return
    mu = [float(v) for v in value.split(",")]
    norm = sum(v * v for v in mu) ** 0.5
    u = [float(v) for v in arrow["dir"].split(",")]
    length = float(arrow["length"])
    check(length > 0.1, "arrow %.3f A between the charge poles" % length)
    check(sum(a * b for a, b in zip(u, mu)) / norm > 0.999,
          "arrow along mu: %s vs %s" % (arrow["dir"], value))
    check(abs(float(arrow["radius"]) - 0.10) < 0.01,
          "head radius %s A" % arrow["radius"])
    check("label=Dipole moment %.2f D (Mulliken charges)" % norm in text,
          "the corner text gives |mu| %.2f D" % norm)


def thumbnailCase(s, display, env, wrappers, png):
    say("thumbnail on File > Save")
    work = os.path.join(s.state, "orgcmd")
    os.makedirs(work, exist_ok=True)
    cmd = os.path.join(work, "thumb.cmd")
    open(cmd, "w").close()
    org, log = organizer(s, display, env, wrappers, cmd, "thumb")
    try:
        out = orgCommand(cmd, log, "newproject thumbs")
        out = orgCommand(cmd, log, "newcalc thumbs water NWChem")
        check(out.startswith("ok "), "an NWChem calculation is made: %s" % out)
        calcUrl = out.split()[1] if out.startswith("ok ") else ""
    finally:
        stop(org)
    if not calcUrl:
        return
    lines = ["wait 3000", "add O Bent 0 0 0", "cmd addh", "wait 500", "save",
             "wait 1500", "quit"]
    text, _ = runScript(s, display, env, wrappers, ["-context", calcUrl],
                        lines, png, "thumb")
    calcDir = calcUrl[len("file://"):].rstrip("/")
    thumbs = []
    for root, _dirs, files in os.walk(calcDir):
        thumbs += [os.path.join(root, f) for f in files
                   if f.lower().startswith("thumbnail")]
    size = os.path.getsize(thumbs[0]) if thumbs else 0
    if not check(size > 500, "Thumbnail stored by File > Save: %s (%d bytes)"
                 % (thumbs, size)):
        say("\n".join(l for l in text.splitlines()
                      if "humbnail" in l or "VizRender" in l or "Coin" in l))
        return
    shutil.copy(thumbs[0], os.path.join(png, "thumbnail.jpeg"))
    cmd = os.path.join(work, "thumb2.cmd")
    open(cmd, "w").close()
    org, log = organizer(s, display, env, wrappers, cmd, "thumb2")
    try:
        out = orgCommand(cmd, log, "summary " + calcUrl)
        out = orgCommand(cmd, log, "snap " + os.path.join(
            png, "organizer-thumbnail.png"))
        check(out == "saved", "the Organizer photographed: %s" % out)
        if out == "saved":
            from PIL import Image
            import numpy
            px = numpy.asarray(Image.open(os.path.join(
                png, "organizer-thumbnail.png")).convert("RGB")).astype(int)
            n = int((px.sum(axis=2) < 30).sum())
            #  The thumbnail's black background; nothing else there is black.
            check(n > 1000, "the Builder button shows the thumbnail (%d "
                  "black pixels)" % n)
    finally:
        stop(org)


def organizer(s, display, env, wrappers, cmd, tag):
    log = os.path.join(s.state, "organizer-%s.log" % tag)
    proc = subprocess.Popen([os.path.join(wrappers, "ecce-organizer")],
                            env=dict(env, ECCE_TEST_ORGANIZER=cmd),
                            cwd=os.path.join(s.home, "bin"),
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    return proc, log


def orgCommand(cmd, log, line, timeout=90):
    """Appends one hook command and waits for its answer in the log."""
    with open(cmd, "a") as handle:
        handle.write(line + "\n")
    tag = "ECCE_TEST_ORGANIZER: %s: " % line
    deadline = time.time() + timeout
    while time.time() < deadline:
        with open(log, errors="replace") as handle:
            for l in handle:
                if l.startswith(tag):
                    return l[len(tag):].strip()
        time.sleep(0.5)
    return "timeout"


def prefsCase(s, display, env, wrappers, png):
    """The Colours tab: the default is Jmol, a choice is saved at once and a
    new Organizer shows the saved one."""
    say("Preferences > Colours: choice saved and kept")
    png = os.path.abspath(png)
    prefs = os.path.join(s.state, ".ECCE", "EcceGlobal")
    if os.path.exists(prefs):
        os.unlink(prefs)

    def saved():
        try:
            with open(prefs) as handle:
                for line in handle:
                    if line.upper().startswith("ELEMENTCOLOURS"):
                        return line.split(":", 1)[1].strip()
        except OSError:
            pass
        return None

    for run, (choose, want) in enumerate(((None, "jmol"), ("ecce", "ecce"),
                                          (None, "ecce"), ("jmol", "jmol"))):
        cmd = os.path.join(s.state, "prefs%d.cmd" % run)
        open(cmd, "w").close()
        org, log = organizer(s, display, env, wrappers, cmd, "prefs%d" % run)
        try:
            out = orgCommand(cmd, log, "prefs state")
            if run == 0:
                check(out == "colours=jmol", "a new user's tab shows Jmol: %s"
                      % out)
            elif run == 2:
                check(out == "colours=ecce",
                      "a new Organizer shows the saved choice: %s" % out)
            out = orgCommand(cmd, log, "prefs snap " + os.path.join(
                png, "prefs-colours-%d.png" % run))
            check(out == "saved", "the tab is photographed: %s" % out)
            if choose:
                out = orgCommand(cmd, log, "prefs colours " + choose)
                check(out == "ok", "choosing %s: %s" % (choose, out))
                check(saved() == want, "saved as %s (file has %s)"
                      % (want, saved()))
                orgCommand(cmd, log, "prefs snap " + os.path.join(
                    png, "prefs-colours-%d-%s.png" % (run, choose)))
        finally:
            stop(org)


def newCase(s, display, env, wrappers, png):
    say("File > New: MD studies and their tasks")
    cmd = os.path.join(s.state, "new.cmd")
    open(cmd, "w").close()
    org, log = organizer(s, display, env, wrappers, cmd, "new")
    try:
        out = orgCommand(cmd, log, "newproject md")
        check(out.startswith("ok "), "a project: %s" % out)
        project = out.split()[1].rstrip("/") if out.startswith("ok ") else ""
        projectDir = project[len("file://"):]
        for study, task in (("gromacs_md_study", "gromacs_md_energy"),
                            ("nwchem_md_study", "nwchem_md_prepare")):
            before = set(os.listdir(projectDir))
            out = orgCommand(cmd, log, "new %s/ %s" % (project, study))
            made = sorted(set(os.listdir(projectDir)) - before)
            if not check(out == "ok" and len(made) == 1,
                         "New %s: %s, made %s" % (study, out, made)):
                continue
            studyUrl = "%s/%s/" % (project, made[0])
            studyDir = os.path.join(projectDir, made[0])
            before = set(os.listdir(studyDir))
            out = orgCommand(cmd, log, "new %s %s" % (studyUrl, task))
            made = sorted(set(os.listdir(studyDir)) - before)
            check(out == "ok" and made, "New %s in it: %s, made %s"
                  % (task, out, made))
            if not made:
                continue
            taskUrl = studyUrl + made[0] + "/"
            if study.startswith("gromacs"):
                duplicateStudyTask(cmd, log, studyDir, studyUrl, made[0])
            if not task.endswith("_energy"):
                continue
            #  What the Organizer's editor button starts, driven through
            #  ECCE_TEST_MDED (the gateway is not running here).
            cmddir = os.path.join(s.state, "mded-" + study)
            os.makedirs(cmddir, exist_ok=True)
            ed = gromacs_test.Editor(s, display, dict(env, ECCE_TEST_MDED=cmddir),
                                     wrappers, cmddir, "mdenergy", "MDEnergy")
            try:
                if ed.up():
                    out = ed.cmd("open " + taskUrl)
                    check(out == "ok", "the Energy task opens in its editor: %s"
                          % out)
                    title = ed.cmd("title")
                    check(("GROMACS" in title) == (study.startswith("gromacs")),
                          "the editor's title: %s" % title)
                    snap(display, env, os.path.join(png, "new-%s.png" % study))
            finally:
                ed.close()
        orgCommand(cmd, log, "snap " + os.path.join(png, "new-organizer.png"))
    finally:
        stop(org)


def duplicateStudyTask(cmd, log, studyDir, studyUrl, first):
    """#514: Duplicate of a GROMACS study task joins the study after the same
    predecessor, with its own copy of the settings."""
    say("Duplicate a GROMACS study task")
    before = set(os.listdir(studyDir))
    out = orgCommand(cmd, log, "new %s gromacs_md_optimize" % studyUrl)
    new = sorted(set(os.listdir(studyDir)) - before)
    if not check(out == "ok" and new, "Optimize after Energy: %s, made %s"
                 % (out, new)):
        return
    made = {"gromacs_md_energy": first, "gromacs_md_optimize": new[0]}
    before = set(os.listdir(studyDir))
    out = orgCommand(cmd, log, "duplicate " + studyUrl + made["gromacs_md_optimize"])
    new = sorted(set(os.listdir(studyDir)) - before)
    if not check(out.startswith("ok") and len(new) == 1,
                 "Duplicate of the Optimize task: %s, made %s" % (out[:80], new)):
        return
    linkbase = open(os.path.join(studyDir, "linkbase.xml")).read()
    check(new[0] in linkbase, "the copy %s is in linkbase.xml" % new[0])
    # linked after the same predecessor as the original
    label = dict(re.findall(r'xlink:label="(\w+)"\s*xlink:href="([^"]+)"',
                            linkbase))
    label = {href: lab for lab, href in label.items()}
    arcs = set(re.findall(r'xlink:title="input_provider"\s*'
                          r'xlink:from="(\w+)"\s*xlink:to="(\w+)"', linkbase))
    for name in (made["gromacs_md_optimize"], new[0]):
        check((label.get(made["gromacs_md_energy"]), label.get(name)) in arcs,
              "%s takes %s as input (arcs %s)"
              % (name, made["gromacs_md_energy"], sorted(arcs)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--only", choices=("overlay", "thumbnail", "new", "hf", "colours",
                                           "prefs", "multipole"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    try:
        import numpy  # noqa: F401
        from PIL import Image  # noqa: F401
    except ImportError:
        harness.skip("python3-pil and python3-numpy are needed")
    harness.prerequisites(build, ())
    for exe in GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "overlay", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    say("screenshots: %s" % png)
    for exe in GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % ", ".join(
        os.path.realpath(os.path.join(s.home, "bin", e)) for e in GUI))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        say("X display %s" % display.name)
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        if args.only == "hf":
            hfCase(s, display, env, wrappers, png)
        if args.only in (None, "prefs"):
            prefsCase(s, display, env, wrappers, png)
        if args.only in (None, "colours"):
            coloursCase(s, display, env, wrappers, png)
        if args.only in (None, "overlay"):
            overlayCase(s, display, env, wrappers, png)
        if args.only in (None, "multipole"):
            multipoleCase(s, display, env, wrappers, png)
        if args.only in (None, "thumbnail"):
            thumbnailCase(s, display, env, wrappers, png)
        if args.only in (None, "new"):
            newCase(s, display, env, wrappers, png)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures.extend(gromacs_test.failures)
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
