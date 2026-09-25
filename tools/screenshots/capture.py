#!/usr/bin/env python3
"""Capture ECCE screenshots headlessly, so they are reproducible.

    python3 tools/screenshots/capture.py --out docs/screenshots

Screenshots taken by hand on a desktop vary in size, carry whatever the
window manager decorates them with, and go stale silently -- the committed
`gateway.png` was a 476x68 sliver of a window that release 8.12 removed.
This runs the real applications against the real data server on a private
Xvfb display, at a fixed size, with a known fixture loaded, and captures
the application window alone.

It reuses `tests/apps`, which already knows how to start an isolated
data server and broker, create the accounts, and install a fixture
calculation.  Nothing here touches the developer's own ~/.ECCE, and it
never draws on a live desktop.

IMPORTANT -- THE 3-D VIEWER IS NOT USABLE FROM HERE YET.

Xvfb has no GPU, so Open Inventor renders through Mesa's software GL,
and under software GL every surface picks up a constant green cast
(issue #83: measured as exactly +0.2 in the green channel, on both
llvmpipe and softpipe).  Any screenshot containing the molecular viewer
is therefore wrong in a way a reader would notice.

So: this captures the windows whose content is ordinary 2-D wx drawing
-- the Organizer, the MO diagram, the periodic table, the editors --
which are unaffected.  Viewer screenshots have to be taken on a machine
with hardware GL until #83 is fixed.  The `--viewer` flag captures them
anyway, for use as evidence in that issue rather than as documentation.
"""

import argparse
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

import apps            # noqa: E402
import fixture         # noqa: E402
import isolate         # noqa: E402
import xdisplay        # noqa: E402

#  Bigger than the suite's 1280x1024: these are for reading on a web page,
#  and the apps lay out more sensibly with room.
xdisplay.SCREEN = "1920x1200x24"

#  How long to let an app settle before capturing.  The property panels
#  populate from the data server after the window appears, so a capture
#  taken as soon as a window exists catches a half-drawn frame.
SETTLE = 25


def xdo(display, *args):
    return subprocess.run(["xdotool"] + list(args), env=display.env(),
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT).stdout.decode().strip()


def realUserAuth(path):
    """Credentials for the account the applications log in as.

    Without this every app stops on its password dialog, which is both
    useless as a screenshot and blocks whatever is behind it.
    """
    user = os.environ.get("ECCE_REALUSER") or os.environ.get("USER") or ""
    root = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users", user)
    base = "http://localhost:%d/" % fixture.dataserverPort()
    name = fixture.realm(root)
    keys = ([base + name] if name else []) + [base]
    with open(path, "w") as handle:
        handle.write("%d\n" % len(keys))
        for key in keys:
            handle.write("%s|%s|%s\n" % (key, user, "ecce"))
    os.chmod(path, 0o600)
    return path


def dismissAuth(display, password="ecce"):
    """Answer the data-server password dialog if it appears.

    The -pipe credentials cache is keyed by realm and does not always
    match what an app asks for, and an unanswered dialog sits on top of
    the window being photographed.  Typing the fixture account's own
    password is simpler and more reliable than reverse-engineering the
    cache key, and it is the same thing a person would do.
    """
    for wid, title in display.windows():
        if "Authentication" not in (title or ""):
            continue
        #  There is no window manager on this display, so there is nothing
        #  to activate a window -- focus has to be set explicitly, and the
        #  keystrokes then go through XTEST to whatever holds focus.
        #  "type --window" is not used: GTK entries ignore synthetic events
        #  delivered straight to a window id.
        xdo(display, "windowfocus", "--sync", wid)
        time.sleep(1)
        xdo(display, "type", "--delay", "60", password)
        time.sleep(1)
        xdo(display, "key", "Return")
        time.sleep(8)
        return True
    return False


def windowId(display, title):
    for wid, name in display.windows():
        if name == title:
            return wid
    return None


def capture(display, path, title=None):
    """Capture one window by title, or the whole screen if it is not found."""
    target = windowId(display, title) if title else None
    argv = ["import", "-display", display.name]
    argv += ["-window", target if target else "root"]
    argv += [path]
    subprocess.run(argv, check=False)
    return os.path.exists(path)


def run(display, wrapper, args, title, out, name, settle=SETTLE,
        size=(1400, 900)):
    command = [os.path.join(apps.WRAPPERS, wrapper)] + list(args)
    proc = subprocess.Popen(command, env=display.env(),
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            start_new_session=True)
    try:
        time.sleep(min(settle, 12))
        dismissAuth(display)
        #  Apps open at whatever size they last remembered, which for a
        #  fresh state directory is a small default.  Give them a size that
        #  suits a documentation screenshot -- the content reflows, so this
        #  is not a crop.
        wid = windowId(display, title)
        if wid and size:
            xdo(display, "windowsize", wid, str(size[0]), str(size[1]))
            time.sleep(4)
        time.sleep(max(settle - 12, 8))
        path = os.path.join(out, name + ".png")
        ok = capture(display, path, title)
        print("  %-28s %s" % (name + ".png", "ok" if ok else "FAILED"))
        return ok
    finally:
        try:
            proc.terminate()
            proc.wait(timeout=10)
        except Exception:
            proc.kill()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=os.path.join(ROOT, "docs", "screenshots"))
    parser.add_argument("--viewer", action="store_true",
                        help="also capture viewer windows, which show issue #83")
    options = parser.parse_args()
    os.makedirs(options.out, exist_ok=True)

    #  ISOLATE FIRST, BEFORE ANYTHING READS THE ENVIRONMENT.
    #
    #  Without this the harness runs against ~/.ECCE and port 8096 -- the
    #  developer's own data server -- and stopping services at the end of a
    #  capture takes down a live session, with running jobs still pointing
    #  at it.  That is not hypothetical; it is what this tool did before
    #  this call was added, and it is the same trap as #136.
    #
    #  isolate.apply() also refuses outright if the state directory
    #  resolves to the real home, so a future edit cannot quietly undo it.
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))

    with xdisplay.Display() as display:
        apps.startServices(display)
        fixture.ensureRealUserAccount()
        url, error = fixture.install()
        if error:
            print("fixture: %s" % error)
            return 1
        auth = realUserAuth(os.path.join(options.out, ".auth"))

        print("capturing into %s" % options.out)
        run(display, "ecce-organizer", ["-pipe", auth],
            "ECCE Organizer", options.out, "organizer")
        if options.viewer:
            run(display, "ecce-builder", ["-pipe", auth, "-context", url],
                "ECCE Builder", options.out, "builder-software-gl")

        try:
            os.unlink(auth)
        except OSError:
            pass
        apps.stopServices(display)
    return 0


if __name__ == "__main__":
    sys.exit(main())
