#!/usr/bin/env python3
"""Take Coordinates From... with a stored calculation as the source (#620).

Two stored calculations with the Na+(H2O)6 fixture of ensemble_patterns_test:
na-take1 has a GEOMTRACE whose last frame differs from its input structure.
With na-take2 open, taking atoms 1-3 from "na-take1 (final geometry)" must give
the last GEOMTRACE frame, and from "(input structure)" the stored input.
"""
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ensemble_patterns_test as ep  # noqa: E402

fails = []


def check(name, ok, detail=""):
    print(("PASS  " if ok else "FAIL  ") + name + ("" if ok else "  " + str(detail)))
    if not ok:
        fails.append(name)


def dumps(log):
    out, cur = [], None
    for m in re.finditer(r"GTSTRESS: (gttake ok=(\d)|take (\d+) (\S+) (\S+) (\S+) (\S+))", log):
        if m.group(2) is not None:
            cur = {"ok": m.group(2) == "1", "atoms": []}
            out.append(cur)
        else:
            cur["atoms"].append((m.group(4),) + tuple(float(m.group(k)) for k in (5, 6, 7)))
    return out


def run(tmp):
    import apps
    import fixture as fx
    import isolate
    import xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fx.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    one, two = os.path.join(tmp, "na-take1"), os.path.join(tmp, "na-take2")
    ep.fixture(one, dav=True)
    ep.fixture(two, dav=True)
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                check("services came up", False, log)
                return
            fx.ensureRealUserAccount()
            restore = fx.settleUpgradeNotices()
            try:
                _u, error = fx.install("na-take1", source_dir=one)
                url, error2 = fx.install("na-take2", source_dir=two)
                if error or error2:
                    check("install the fixtures", False, error or error2)
                    return
                scene = os.path.join(tmp, "scene")
                open(scene, "w").write(
                    "gttake na-take1_(final_geometry) 1-3\n"
                    "gttake na-take1_(input_structure) 1-3\n"
                    "gttake na-take1_(final_geometry) 1-3 align\nhold 1\n")
                args = ("-context", url)
                if os.path.realpath(state) != os.path.realpath(os.path.expanduser("~")):
                    auth = fx.authFile(os.path.join(state, ".ECCE", "auth.pipe"), user=fx.USER)
                    args = ("-pipe", auth) + args
                result = apps.run(display, "builder", args=args, windowTimeout=120,
                                  settle=120,
                                  env={"ECCE_VIEWER_SCENE": scene,
                                       "ECCE_VIEWER_SCENE_OUT": tmp,
                                       "ECCE_REALUSER": fx.USER})
                log = result.log or ""
                d = dumps(log)
                if len(d) < 3:
                    check("the hooks ran", False, log[-3000:])
                    return
                final, inp, aligned = d[:3]
                last = ep.atoms(ep.FRAMES[-1][0])
                first = ep.atoms("6_0")
                near = lambda got, want: all(
                    abs(g - float(x)) < 1e-6 for g, x in zip(got[1:], want[1:]))
                check("final geometry: atoms 1-3 are the last GEOMTRACE frame",
                      final["ok"] and all(near(final["atoms"][i], last[i]) for i in range(3)),
                      (final["atoms"][:3], last[:3]))
                check("the last frame differs from the input",
                      not all(near(final["atoms"][i], first[i]) for i in range(3)))
                check("input structure: atoms 1-3 are the stored input",
                      inp["ok"] and all(near(inp["atoms"][i], first[i]) for i in range(3)),
                      (inp["atoms"][:3], first[:3]))
                check("atoms 4.. are untouched by either take",
                      all(near(final["atoms"][i], inp["atoms"][i])
                          for i in range(3, len(first))))
                check("aligned take ran", aligned["ok"])
            finally:
                restore()
                fx.remove("na-take1")
                fx.remove("na-take2")
        finally:
            apps.stopServices(display)


def main():
    tmp = tempfile.mkdtemp(prefix="take-calc-")
    try:
        run(tmp)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("FAILED: " + "; ".join(fails) if fails else "PASSED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
