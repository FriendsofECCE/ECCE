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
pipe = []
i = 0
while i < len(argv):
    a = argv[i]
    if a == "-context" and i + 1 < len(argv):
        url = argv[i + 1]
        i += 2
    elif a == "-pipe" and i + 1 < len(argv):
        pipe = argv[i:i + 2]        # the credentials: the Organizer reads them
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


def orphans():
    """Organizers of earlier shims of this HOME whose shim is gone.

    A test that stops the tool with a plain kill of the shim leaves the
    Organizer running, and the next start for the same user and data
    would then only hand over to it ("ECCE is already running").
    """
    home = os.environ.get("HOME", "").encode()
    found = []
    for pid in os.listdir("/proc"):
        if not pid.isdigit():
            continue
        try:
            words = open("/proc/%s/environ" % pid, "rb").read().split(b"\0")
        except OSError:
            continue
        shim = [w for w in words if w.startswith(b"ECCE_HOSTED_SHIM=")]
        if shim and b"HOME=" + home in words:
            owner = int(shim[0].split(b"=", 1)[1])
            if owner != os.getpid() and not os.path.exists("/proc/%d" % owner):
                found.append(int(pid))
    return found


for _ in range(20):
    left = orphans()
    if not left:
        break
    for pid in left:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    time.sleep(0.5)

env = dict(os.environ, ECCE_ONE_PROCESS="1", ECCE_TEST_ORGANIZER=cmds,
           ECCE_HOSTED_SHIM=str(os.getpid()))
env.pop("ECCE_TEST_HOSTED", None)
organizer = os.path.join(os.path.dirname(wrapper), "ecce-organizer")


def dieWithShim():
    try:
        import ctypes
        ctypes.CDLL("libc.so.6").prctl(1, signal.SIGKILL)   # PR_SET_PDEATHSIG
    except OSError:
        pass


proc = subprocess.Popen([organizer] + pipe, env=env, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, start_new_session=True,
                        preexec_fn=dieWithShim)


def killOrganizer(sig):
    try:
        os.killpg(proc.pid, sig)
    except OSError:
        pass

state = {"ready": False, "seen": False, "gone": False}
# A Builder on a calculation with results counts as a Viewer.
KEYS = ("Builder=", "Viewer=") if tool == "Builder" else (tool + "=",)
keep = os.environ.get("ECCE_TEST_HOSTED_LOGDIR")
copy = (open(os.path.join(keep, "%s-%d.log" % (tool, os.getpid())), "wb")
        if keep else None)
# Into the kept log only: some tests count the lines of a tool's output.
if copy:
    copy.write(("HOSTED: %s %s (dropped: %s) via %s %s\n"
                % (tool, url or "-", " ".join(rest) or "-", organizer,
                   " ".join(pipe))).encode())
    copy.flush()


def pump():
    out = sys.stdout.buffer
    for line in iter(proc.stdout.readline, b""):
        if copy:
            copy.write(line)
            copy.flush()
        text = line.decode("utf-8", "replace")
        if "already running" in text:
            out.write(b"HOSTED: HARNESS: handed over to an Organizer still running\n")
        if "ECCE_TEST_ORGANIZER: inprocess" in text:
            # The shim's own polling: not part of the tool's output.
            state["ready"] = True
            counts = [text.split(k, 1)[1].split()[0] for k in KEYS if k in text]
            if any(n.isdigit() and int(n) > 0 for n in counts):
                state["seen"] = True
            elif counts and state["seen"]:
                state["gone"] = True
            continue
        out.write(line)
        out.flush()


def append(line):
    with open(cmds, "a") as handle:
        handle.write(line + "\n")


def forward(sig, _frame):
    killOrganizer(sig)


signal.signal(signal.SIGTERM, forward)
signal.signal(signal.SIGINT, forward)
threading.Thread(target=pump, daemon=True).start()

append("inprocess")
started = False
while proc.poll() is None:
    time.sleep(2)
    if state["ready"] and not started:
        append("start %s %s" % (tool, url))
        started = True
    elif started:
        append("inprocess")
    if state["gone"]:
        time.sleep(1)
        killOrganizer(signal.SIGTERM)
        try:
            proc.wait(timeout=15)
        except subprocess.TimeoutExpired:
            killOrganizer(signal.SIGKILL)
        break

code = proc.wait()
killOrganizer(signal.SIGKILL)       # whatever the wrapper left behind
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
