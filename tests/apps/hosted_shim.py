#!/usr/bin/env python3
"""hosted_shim.py TOOL WRAPPER [ARGS]: the tool as a tab of the Organizer.

Run in place of a tool's wrapper under ECCE_TEST_HOSTED=1 (see
hosted/sitecustomize.py).  Starts the ecce-organizer wrapper beside
WRAPPER with ECCE_ONE_PROCESS=1, asks it (ECCE_TEST_ORGANIZER hook
`start TOOL URL`) for the tool on the URL the standalone tool was given
(-context URL, or a structure file as file://), copies the Organizer's
output to this process's stdout, and ends the Organizer once the hosted
tool's tab has closed, so a test that waits for the tool to exit still
sees an exit.  An Organizer crash is passed on as this process dying of
the same signal.
"""
import os
import signal
import subprocess
import sys
import tempfile
import threading
import time

tool, wrapper, argv = sys.argv[1], sys.argv[2], sys.argv[3:]
url = ""
rest = []
i = 0
while i < len(argv):
    a = argv[i]
    if a == "-context" and i + 1 < len(argv):
        url = argv[i + 1]
        i += 2
    elif a == "-pipe" and i + 1 < len(argv):
        rest += argv[i:i + 2]       # stdin authentication: not used hosted
        i += 2
    elif a.startswith("-"):
        rest.append(a)
        i += 1
    else:
        url = a if "://" in a else "file://" + os.path.abspath(a)
        i += 1

work = tempfile.mkdtemp(prefix="ecce-hosted-")
cmds = os.path.join(work, "commands")
open(cmds, "w").close()
env = dict(os.environ, ECCE_ONE_PROCESS="1", ECCE_TEST_ORGANIZER=cmds)
env.pop("ECCE_TEST_HOSTED", None)
organizer = os.path.join(os.path.dirname(wrapper), "ecce-organizer")
sys.stdout.write("HOSTED: %s %s (dropped: %s) via %s\n"
                 % (tool, url or "-", " ".join(rest) or "-", organizer))
sys.stdout.flush()
proc = subprocess.Popen([organizer], env=env, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT)

state = {"ready": False, "seen": False, "gone": False}
KEY = tool + "="


def pump():
    out = sys.stdout.buffer
    for line in iter(proc.stdout.readline, b""):
        out.write(line)
        out.flush()
        text = line.decode("utf-8", "replace")
        if "ECCE_TEST_ORGANIZER: inprocess" in text:
            state["ready"] = True
            n = text.split(KEY, 1)[1].split()[0] if KEY in text else ""
            if n.isdigit() and int(n) > 0:
                state["seen"] = True
            elif n == "0" and state["seen"]:
                state["gone"] = True


def append(line):
    with open(cmds, "a") as handle:
        handle.write(line + "\n")


def forward(sig, _frame):
    try:
        proc.send_signal(sig)
    except ProcessLookupError:
        pass


signal.signal(signal.SIGTERM, forward)
signal.signal(signal.SIGINT, forward)
threading.Thread(target=pump, daemon=True).start()

append("inprocess")
started = False
while proc.poll() is None:
    time.sleep(1)
    if state["ready"] and not started:
        append("start %s %s" % (tool, url))
        started = True
    elif started:
        append("inprocess")
    if state["gone"]:
        time.sleep(1)
        proc.terminate()
        try:
            proc.wait(timeout=15)
        except subprocess.TimeoutExpired:
            proc.kill()
        break

code = proc.wait()
time.sleep(0.3)
try:
    os.unlink(cmds)
    os.rmdir(work)
except OSError:
    pass
if state["gone"]:
    sys.exit(0)
if code < 0:
    signal.signal(-code, signal.SIG_DFL)
    os.kill(os.getpid(), -code)
sys.exit(code)
