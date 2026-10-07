#!/usr/bin/env python3
"""Screenshot the login dialog and compare focused/unfocused windows (#210).

    tests/look/login_backdrop.py login    OUTDIR [--tag before|after]
    tests/look/login_backdrop.py backdrop OUTDIR [--tag before|after]

Builds authdialog.C / backdrop.C against a build tree and runs them on a
private Xvfb (no window manager; there is no window manager,
so the program sets the :backdrop state on one window itself).  `login` writes one PNG per mode and theme.  `backdrop` shows two
windows, writes a PNG and measures how far the
text in the unfocused window falls short of the same text when focused:
exit 1 when a normal label or entry loses more than 15 % of its contrast,
or a disabled one is no dimmer than a normal one.  `--tag after` builds
with -DHAVE_NEW (the library has the new calls) and, for `backdrop`,
applies the fix; `--tag before` leaves the fix off.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "apps"))
import xdisplay  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LIBS = ["eccewxgui", "eccewxplotctrl", "eccewxthings", "eccewxgui",
        "eccecomm", "eccercmd", "eccedsi", "eccedav", "eccecipc",
        "eccefaces", "eccexml", "eccetdat", "ecceutil"]
THEMES = ("Adwaita", "Adwaita:dark")


def out(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def build(src, work, o):
    binary = os.path.join(work, os.path.splitext(src)[0])
    cxx = (out(["wx-config", "--cxxflags"]).stdout.split()
           + out(["pkg-config", "--cflags", "gtk+-3.0"]).stdout.split())
    libs = out(["wx-config", "--libs", "core,base,adv,html"]).stdout.split()
    cmd = (["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
            os.path.join(HERE, src), "-I" + o.include]
           + (["-DHAVE_NEW"] if o.tag == "after" else []) + cxx
           + ["-L" + o.libdir, "-Wl,-rpath," + o.libdir]
           + ["-l" + lib for lib in LIBS]
           + libs + ["-lxerces-c", "-lssh", "-lmosquitto", "-lssl",
                     "-lcrypto", "-lgtk-3", "-lgdk-3", "-lgobject-2.0", "-lglib-2.0"])
    b = out(cmd)
    if b.returncode:
        print(b.stderr[-3000:])
        sys.exit(1)
    return binary


def window(env, name):
    for _ in range(40):
        r = out(["xdotool", "search", "--name", name], env=env)
        ids = r.stdout.split()
        if ids:
            return ids[-1]
        time.sleep(0.25)
    return None


def shot(env, wid, png):
    subprocess.run(["import", "-window", wid, png], env=env, check=False)


def lum(c):
    def f(v):
        v /= 255.0
        return v / 12.92 if v <= 0.03928 else ((v + 0.055) / 1.055) ** 2.4
    return 0.2126 * f(c[0]) + 0.7152 * f(c[1]) + 0.0722 * f(c[2])


def ratio(a, b):
    la, lb = sorted((lum(a), lum(b)), reverse=True)
    return (la + 0.05) / (lb + 0.05)


def ink(img, rect):
    """Contrast of the strongest pixel in rect against the region's background."""
    x, y, w, h = rect
    crop = img.convert("RGB").crop((x + 2, y + 1, x + w - 2, y + h - 1))
    px = list(crop.getdata())
    bg = max(set(px), key=px.count)
    return max(ratio(p, bg) for p in px)


def login(o, work):
    binary = build("authdialog.C", work, o)
    modes = ["plain", "retry", "change"] + (["tls-pinned", "tls-ca"]
                                            if o.tag == "after" else [])
    for theme in THEMES:
        suffix = "dark" if "dark" in theme else "light"
        for mode in modes + ([] if o.tag == "after" else ["tls-pinned"]):
            with xdisplay.Display(screen="900x500x24") as xvfb:
                env = dict(os.environ, DISPLAY=xvfb.name, HOME=work,
                           ECCE_REALUSERHOME=work, ECCE_HOME=ROOT,
                           GTK_THEME=theme)
                p = subprocess.Popen([binary, mode, "4000"], env=env,
                                     stdout=subprocess.PIPE, text=True)
                wid = window(env, "ECCE Authentication")
                time.sleep(1.5)
                png = os.path.join(o.outdir, "%s-login-%s-%s.png"
                                   % (o.tag, mode, suffix))
                if wid:
                    subprocess.run(["xdotool", "windowfocus", wid], env=env)
                    time.sleep(0.5)
                    shot(env, wid, png)
                print(png if wid else "NO WINDOW for " + mode)
                p.kill()
                print(p.communicate()[0].strip())
    return 0


def backdrop(o, work):
    try:
        from PIL import Image
    except ImportError:
        print("SKIP: PIL not installed")
        return 77
    binary = build("backdrop.C", work, o)
    status = 0
    for theme in THEMES:
        suffix = "dark" if "dark" in theme else "light"
        with xdisplay.Display(screen="940x360x24") as xvfb:
            env = dict(os.environ, DISPLAY=xvfb.name, HOME=work,
                       ECCE_REALUSERHOME=work, ECCE_HOME=ROOT,
                       GTK_THEME=theme)
            p = subprocess.Popen([binary, "1" if o.tag == "after" else "0",
                                  "12000"], env=env, stdout=subprocess.PIPE,
                                 text=True)
            window(env, "^Right$")
            time.sleep(2)
            png = os.path.join(o.outdir, "%s-backdrop-%s.png"
                               % (o.tag, suffix))
            subprocess.run(["import", "-window", "root", png], env=env)
            # Rects come from the program's stdout after the windows exist.
            time.sleep(0.5)
            p.kill()
            text = p.communicate()[0]
            rects = {}
            for m in re.finditer(r"RECT (\w+) (\S+) (-?\d+) (-?\d+) (\d+) (\d+)", text):
                rects[(m.group(1), m.group(2))] = tuple(map(int, m.groups()[2:]))
            # Left is active, Right is in :backdrop (set by the program).
            focused = backed = Image.open(png)
            print("== %s (%s)" % (suffix, o.tag))
            res = {}
            for kind in ("label", "disabled-label", "entry", "disabled-entry"):
                f = ink(focused, rects[("Left", kind)])
                b = ink(backed, rects[("Right", kind)])
                res[kind] = (f, b)
                print("  %-15s focused %.2f  unfocused %.2f  (%.0f%%)"
                      % (kind, f, b, 100 * b / f))
            if o.tag == "after":
                for kind in ("label", "entry"):
                    if res[kind][1] < 0.85 * res[kind][0]:
                        print("FAIL: %s fades when unfocused" % kind)
                        status = 1
                for kind, normal in (("disabled-label", "label"),
                                     ("disabled-entry", "entry")):
                    if res[kind][1] > 0.9 * res[normal][1]:
                        print("FAIL: %s no longer looks disabled" % kind)
                        status = 1
    return status


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("what", choices=["login", "backdrop"])
    ap.add_argument("outdir")
    ap.add_argument("--include", default=os.path.join(ROOT, "include"))
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--tag", default="after", choices=["before", "after"])
    o = ap.parse_args()
    os.makedirs(o.outdir, exist_ok=True)
    work = tempfile.mkdtemp(prefix="login-backdrop")
    try:
        return (login if o.what == "login" else backdrop)(o, work)
    except xdisplay.DisplayUnavailable as e:
        print("SKIP:", e)
        return 77
    finally:
        shutil.rmtree(work, True)


if __name__ == "__main__":
    sys.exit(main())
