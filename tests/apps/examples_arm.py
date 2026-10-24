#!/usr/bin/env python3
"""One first start of the Organizer in a fresh state, and what it imported.

    examples_arm.py OUTDIR [SET PROJECT COUNT]

Starts the Organizer in its own isolated state and Xvfb, lets its first-start
import of the `general` example set (#302) finish (any other SET: the Organizer
is told `examples SET` instead), then copies the resulting project folder
(PROJECT, default Examples) to OUTDIR/PROJECT and prints "ARM <seconds> <copied> <parsed>":
the import time the Organizer logged, and how many examples were copied from
a pre-parsed copy and how many were parsed.  ECCE_NO_PREPARSED=1 in the
environment forces the parse.  Used by examples_preparsed_test.py and by
tools/examples/preparse.py.  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77


def propsOf(project):
    found = {}
    for calc in sorted(os.listdir(project)) if os.path.isdir(project) else []:
        props = os.path.join(project, calc, "Props")
        if os.path.isdir(props):
            found[calc] = sorted(os.listdir(props))
    return found


def waitStable(project, count, seconds=300):
    """The property files of all `count` calculations stopped changing."""
    end = time.time() + seconds
    last, stable, changed = None, 0, time.time()
    while time.time() < end:
        now = propsOf(project)
        sizes = {c: [os.path.getsize(os.path.join(project, c, "Props", p))
                     for p in now[c]] for c in now}
        state = (now, sizes)
        if len(now) == count and state == last:
            stable += 1
            if stable >= 20:
                return changed
        else:
            stable = 0
            changed = time.time()
        last = state
        time.sleep(0.5)
    return None


def main():
    out = sys.argv[1]
    setId, projectName, count = (sys.argv[2:5] if len(sys.argv) >= 5
                                 else ("general", "Examples", "5"))
    count = int(count)
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    settings = isolate.apply(apps.INSTALL)
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="examples-arm-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    project = os.path.join(data, "users", "local", projectName)
    ok = {}
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        # Polled from the start, so the time to the last property file is
        # not rounded up to the settle period.
        watcher = threading.Thread(
            target=lambda: ok.update(done=waitStable(project, count)))
        t0 = time.time()
        watcher.start()
        try:
            env = {"ECCE_NO_EXAMPLE_IMPORT": ""}
            if setId != "general":
                script = os.path.join(work, "commands")
                with open(script, "w") as handle:
                    handle.write("examples %s\n" % setId)
                env = {"ECCE_TEST_ORGANIZER": script}
            result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                              env=env)
            watcher.join()
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    log = result.log or ""
    if result.crashed or not ok.get("done"):
        print("FAIL: the first-start import did not finish\n" + log[-2000:])
        return 1
    took = re.findall(r"Organizer: examples %s imported in ([0-9.]+) s" % setId, log)
    copied = len(re.findall(r"Organizer: example copied:", log))
    for line in re.findall(r"Organizer: example copy failed.*", log):
        print(line)
    parsed = count - copied
    # What the user waits for: import until the last property file is written.
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    shutil.copytree(project, os.path.join(out, projectName), symlinks=True)
    print("ARM %s %.1f %d %d" % (took[-1] if took else "?",
                                 ok["done"] - t0, copied, parsed))
    shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
