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
        return int(((r > 120) & (g > 120) & (b < 60)).sum())
    return int(((b > 120) & (r < 80) & (g < 80)).sum())


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

    say("classic layout, the Properties menu")
    lines = ["wait 4000", "overlay", "shot plain",
             "property Dipole Moment", "wait 1500", "overlay", "shot dipole",
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
    for name, want in (("plain", False), ("dipole", True), ("closed", False)):
        if name in shots:
            n = tinted(shots[name], "yellow")
            check((n > 3) == want, "%s: %d yellow pixels in the viewer"
                  % (name, n))


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


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--only", choices=("overlay", "thumbnail", "new"))
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
        if args.only in (None, "overlay"):
            overlayCase(s, display, env, wrappers, png)
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
