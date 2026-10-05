#!/usr/bin/env python3
"""Compile the viewer as a non-X11 wx port (macOS, Windows) would see it (#232).

    tools/nonx11/check.py [BUILD]        # default: build-cmake (a Coin build)

There is no wxOSX on Linux, so this approximates one: every source of
eccewxinv, eccemoiv, eccevizsg and eccewxviz is compiled with -fsyntax-only,
ECCE_GL_X11=0, X11/GLX/EGL headers replaced by #error stubs, and wxGTK's GLX
canvas header replaced by the portable wxGLCanvas API (glcanvas_nonx11.h).
The rest of wx stays wxGTK, so this checks our code, not wx's.  Exit 1 if a
file fails.
"""
import os, re, shlex, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LIBS = ("eccewxinv", "eccemoiv", "eccevizsg", "eccewxviz")
POISON = ["X11/X.h", "X11/Xlib.h", "X11/Xutil.h", "X11/Intrinsic.h",
          "X11/keysym.h", "X11/extensions/SGIStereo.h", "GL/glx.h",
          "GL/glxext.h", "EGL/egl.h", "EGL/eglext.h", "EGL/eglplatform.h"]


def stubs(d, wxinc):
    for h in POISON:
        p = os.path.join(d, h)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        open(p, "w").write('#error "non-X11 build reached <%s>"\n' % h)
    src = open(os.path.join(wxinc, "wx", "glcanvas.h")).read()
    port = '#include "wx/gtk/glcanvas.h"'
    if port not in src:
        sys.exit("wx/glcanvas.h has no %s; update this script" % port)
    os.makedirs(os.path.join(d, "wx"), exist_ok=True)
    open(os.path.join(d, "wx", "glcanvas.h"), "w").write(
        src.replace(port, '#include "glcanvas_nonx11.h"'))
    open(os.path.join(d, "wx", "glcanvas_nonx11.h"), "w").write(
        open(os.path.join(HERE, "glcanvas_nonx11.h")).read())


def main():
    build = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build-cmake"))
    cmds = subprocess.run(["ninja", "-C", build, "-t", "commands"] + list(LIBS),
                          capture_output=True, text=True, check=True).stdout
    jobs = []
    for l in cmds.splitlines():
        if not re.search(r"CMakeFiles/(%s)\.dir/" % "|".join(LIBS), l) or " -c " not in l:
            continue
        a = shlex.split(l)
        out = []
        skip = False
        for x in a:
            if skip: skip = False; continue
            if x in ("-o", "-MT", "-MF"): skip = True; continue
            if x in ("-MD", "-g"): continue
            out.append(x)
        jobs.append(out)
    wxinc = None   # the directory that holds wx/glcanvas.h
    for j in jobs:
        for i, x in enumerate(j[:-1]):
            if x == "-isystem" and os.path.exists(os.path.join(j[i + 1], "wx", "glcanvas.h")):
                wxinc = j[i + 1]
    if not jobs or not wxinc:
        sys.exit("no viewer sources or wx headers found in %s" % build)
    bad = 0
    with tempfile.TemporaryDirectory(prefix="nonx11-") as d:
        stubs(d, wxinc)
        for a in jobs:
            a = list(a)
            a[a.index("-c")] = "-fsyntax-only"
            a[1:1] = ["-I" + d, "-DECCE_GL_X11=0", "-w"]
            r = subprocess.run(a, cwd=build, capture_output=True, text=True)
            if r.returncode:
                bad += 1
                print("FAILED", os.path.relpath(a[-1], ROOT))
                for e in [l for l in r.stderr.splitlines() if "error" in l][:5]:
                    print("   ", e)
    print("%d of %d files compile without X11/GLX/EGL" % (len(jobs) - bad, len(jobs)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
