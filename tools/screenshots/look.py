#!/usr/bin/env python3
"""Capture every main ECCE window in a light or dark GTK theme (#210).

    ECCE_TEST_HOME=<install>/ecce ECCE_TEST_WRAPPERS=<install>/bin \\
    ECCE_TEST_STATE=~/.cache/look-state \\
    ECCE_DATASERVER_PORT=8396 ECCE_BROKER_PORT=8388 \\
        tools/screenshots/look.py --out shots/after/light --theme light --display 94

    tools/screenshots/look.py --sheets shots    # contact sheets, see below

Window chrome changes (#210) are reviewed as before/after pairs: run this
once against an install of main and once against the branch, in both
themes, then `--sheets` puts each window's four captures on one page
(before | after, light over dark), from DIR/{before,after}/{light,dark}/.

It reuses tests/apps for isolation, services and the fixture
calculation, so nothing touches ~/.ECCE; choose ports and a display
that no other run is using.  The Organizer is captured with the fixture
calculation selected, and Preferences is opened from its Edit menu with
synthetic keys -- on this Xvfb display only, never a live desktop.

The 3-D viewer renders through software GL here and shows a green cast
(#83); the Builder's capture is for its chrome, not its viewer.
Needs Xvfb, xdotool, xwininfo and ImageMagick (import, montage).
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

WINDOWS = ["organizer", "preferences", "calced", "launcher", "builder",
           "basistool", "machregister", "machbrowser", "pertable", "polyed",
           "solvate", "mdprepare", "metadyn", "dirdyed", "mddynamics",
           "mdenergy", "mdoptimize"]

#  Time for an app to fetch from the data server and draw before capture.
SETTLE = 22


def xdo(display, *args):
    return subprocess.run(["xdotool"] + list(args), env=display.env(),
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT
                          ).stdout.decode().strip()


def viewableSize(display, wid):
    out = subprocess.run(["xwininfo", "-display", display.name, "-id", wid],
                         stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                         timeout=20).stdout.decode()
    w = h = 0
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Width:"):
            w = int(line.split()[1])
        elif line.startswith("Height:"):
            h = int(line.split()[1])
        elif line.startswith("Map State:") and "IsViewable" not in line:
            return 0, 0
    return w, h


def newWindows(display, known):
    """Mapped, titled top-level windows not in `known`, largest first."""
    found = []
    for wid, title in display.windows():
        if wid in known or not title:
            continue
        w, h = viewableSize(display, wid)
        if w > 60 and h > 60:
            found.append((w * h, wid, title))
    return sorted(found, reverse=True)


def selectCalc(display, wid):
    #  The tree opens on the user's home with the fixture just below it.
    xdo(display, "windowfocus", "--sync", wid)
    time.sleep(1)
    xdo(display, "key", "Down")
    time.sleep(8)


def openPrefs(display, wid):
    #  Preferences is the last entry of the Edit menu.
    xdo(display, "windowfocus", "--sync", wid)
    time.sleep(1)
    xdo(display, "key", "alt+e")
    time.sleep(2)
    xdo(display, "key", "Up")
    time.sleep(1)
    xdo(display, "key", "Return")
    time.sleep(8)


def shoot(display, out, name, wrapper, args, size=None, after=None,
          settle=SETTLE):
    import apps
    import capture
    known = set(w for w, _ in display.windows())
    proc = subprocess.Popen([os.path.join(apps.WRAPPERS, wrapper)] + list(args),
                            env=display.env(), stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL, start_new_session=True)
    try:
        time.sleep(10)
        capture.dismissAuth(display)
        time.sleep(max(settle - 10, 1))
        wins = newWindows(display, known)
        if not wins:
            print("  %-14s NO WINDOW" % name)
            return False
        _, wid, title = wins[0]
        if size:
            xdo(display, "windowsize", wid, str(size[0]), str(size[1]))
            time.sleep(4)
        if after:
            after(display, wid)
            extra = newWindows(display, known | {wid})
            if extra:
                _, wid, title = extra[0]
        path = os.path.join(out, name + ".png")
        subprocess.run(["import", "-display", display.name, "-window", wid,
                        path], check=False)
        ok = os.path.exists(path)
        print("  %-14s %s  [%s]" % (name, "ok" if ok else "FAILED", title))
        return ok
    finally:
        try:
            os.killpg(proc.pid, 15)
            proc.wait(timeout=10)
        except Exception:
            try:
                os.killpg(proc.pid, 9)
            except Exception:
                pass
        time.sleep(2)


def capture(out, theme, number, only):
    import apps
    import capture as cap
    import fixture
    import isolate
    import xdisplay

    xdisplay.SCREEN = "1600x1100x24"
    os.makedirs(out, exist_ok=True)
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    #  Apps run one at a time; without this the first to exit stops the
    #  gateway for all that follow (see tests/apps/README.md).
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita:dark" if theme == "dark" else "Adwaita"

    failed = 0
    with xdisplay.Display(number=number) as display:
        apps.startServices(display)
        fixture.ensureRealUserAccount()
        url, error = fixture.install()
        if error:
            print("fixture: %s" % error)
            return 1
        #  A copy in the user's own home, so the Organizer shows it at once.
        mine = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users",
                            fixture.realUser(), "calc-water-vib")
        shutil.rmtree(mine, ignore_errors=True)
        shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-vib"),
                        mine, symlinks=True)
        auth = cap.realUserAuth(os.path.join(out, ".auth"))
        pipe = ["-pipe", auth]
        plan = [
            ("organizer", "ecce-organizer", pipe, dict(size=(1300, 850), after=selectCalc)),
            ("preferences", "ecce-organizer", pipe, dict(size=(1300, 850), after=openPrefs)),
            ("calced", "ecce-calced", ["-context", url], {}),
            ("launcher", "ecce-launcher", ["-context", url], {}),
            ("builder", "ecce-builder", pipe + ["-context", url], dict(size=(1300, 900), settle=30)),
            ("basistool", "ecce-basistool", ["-context", url], {}),
            ("machregister", "ecce-machregister", [], {}),
            ("machbrowser", "ecce-machbrowser", [], {}),
            ("pertable", "ecce-pertable", [], {}),
        ] + [(n, "ecce-" + n, [], {}) for n in WINDOWS[9:]]
        print("capturing %s into %s" % (theme, out))
        for name, wrapper, args, extra in plan:
            if only and name not in only:
                continue
            if not apps.installed(wrapper[len("ecce-"):]):
                continue
            if not shoot(display, out, name, wrapper, args, **extra):
                failed += 1
        try:
            os.unlink(auth)
        except OSError:
            pass
        os.environ.pop("ECCE_NO_REAP", None)
        apps.stopServices(display)
    return 1 if failed else 0


def sheets(root):
    out = os.path.join(root, "contact")
    os.makedirs(out, exist_ok=True)
    for name in WINDOWS:
        argv = ["montage"]
        found = False
        for theme in ("light", "dark"):
            for side in ("before", "after"):
                path = os.path.join(root, side, theme, name + ".png")
                found = found or os.path.exists(path)
                argv += ["-label", "%s  %s  %s" % (name, side, theme),
                         path if os.path.exists(path) else "xc:gray50"]
        if not found:
            continue
        target = os.path.join(out, name + ".png")
        argv += ["-tile", "2x2", "-geometry", "+10+10", "-pointsize", "18",
                 "-background", "#808080", target]
        subprocess.run(argv, check=False)
        print(target)
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", help="capture into this directory")
    parser.add_argument("--theme", choices=["light", "dark"], default="light")
    parser.add_argument("--display", type=int, default=94)
    parser.add_argument("--only", action="append", choices=WINDOWS)
    parser.add_argument("--sheets", metavar="DIR",
                        help="make contact sheets from DIR/{before,after}/{light,dark}")
    options = parser.parse_args()
    if options.sheets:
        return sheets(options.sheets)
    if not options.out:
        parser.error("--out or --sheets is required")
    return capture(options.out, options.theme, options.display, options.only)


if __name__ == "__main__":
    sys.exit(main())
