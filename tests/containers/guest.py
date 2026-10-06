#!/usr/bin/env python3
"""Runs INSIDE a test container (mounted at /harness) as a client account.

  guest.py start <user> <password> <server>   Xvfb + `ecce -remote`, logged in
  guest.py start-refused <user> <password> <server>
                                              the same, expecting no Organizer;
                                              reports what the session shows
  guest.py close                              close the Organizer like a WM
  guest.py state                              one JSON line: what is running

Real windows on a real Xvfb, driven the way tests/apps/session_end.py does:
the data server's login dialog is typed into, the Organizer closed with
WM_DELETE_WINDOW. Prints JSON; the harness reads it.
"""
import json
import os
import re
import subprocess
import sys
import time

DISPLAY = ":90"
LOG = os.path.expanduser("~/ecce-session.log")


def env():
    e = dict(os.environ)
    e.update(DISPLAY=DISPLAY, PATH="/opt/ecce/bin:" + e.get("PATH", ""))
    return e


def windows():
    out = subprocess.run(["xwininfo", "-root", "-tree"], env=env(),
                         capture_output=True, text=True).stdout
    found = []
    for line in out.splitlines():
        m = re.match(r'\s+(0x[0-9a-f]+) "([^"]*)"', line)
        if m:
            found.append((m.group(1), m.group(2)))
    return found


def procs():
    """name -> [pids] of ECCE programs of this account."""
    found = {}
    for pid in filter(str.isdigit, os.listdir("/proc")):
        try:
            exe = os.readlink("/proc/%s/exe" % pid)
            uid = os.stat("/proc/%s" % pid).st_uid
        except OSError:
            continue
        if uid == os.getuid() and exe.startswith("/opt/ecce/"):
            found.setdefault(os.path.basename(exe), []).append(int(pid))
    return found


def broker_connections():
    out = subprocess.run(["ss", "-tnpH"], capture_output=True, text=True).stdout
    return [l for l in out.splitlines()
            if (":8088" in l or ":8883" in l) and "gateway" in l]


def start(user, password, server, expect_login=True):
    subprocess.Popen(["Xvfb", DISPLAY, "-screen", "0", "1280x1024x24",
                      "-nolisten", "tcp"], stdout=subprocess.DEVNULL,
                     stderr=subprocess.DEVNULL, start_new_session=True)
    for _ in range(50):
        if subprocess.run(["xwininfo", "-root"], env=env(),
                          capture_output=True).returncode == 0:
            break
        time.sleep(0.2)
    with open(LOG, "w") as log:
        subprocess.Popen(["ecce", "-remote"], env=env(), stdout=log,
                         stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                         start_new_session=True)
    deadline = time.time() + (150 if expect_login else 75)
    typed = 0
    t0 = time.time()
    while time.time() < deadline:
        ws = windows()
        if any("Organizer" in t for _, t in ws):
            break
        # `ecce` has ended without a window: nothing more will appear.
        if not expect_login and time.time() - t0 > 20 and not procs() \
                and not ws:
            break
        auth = next((w for w, t in ws if t == "ECCE Authentication"), None)
        if auth and typed < 4:
            subprocess.run(["xdotool", "windowfocus", str(int(auth, 16))],
                           env=env(), stderr=subprocess.DEVNULL)
            time.sleep(0.3)
            subprocess.run(["xdotool", "type", "--delay", "50", password],
                           env=env())
            subprocess.run(["xdotool", "key", "Return"], env=env())
            typed += 1
            time.sleep(3)
        time.sleep(0.5)
    ws = windows()
    org = next((t for _, t in ws if "Organizer" in t), None)
    time.sleep(3)
    sessions = [f for f in os.listdir(os.path.expanduser("~/.ECCE"))
                if f.startswith("broker_")]
    if not expect_login:
        print(json.dumps({"organizer": org, "titles": [t for _, t in ws if t],
                          "log": open(LOG).read()[-3000:],
                          "procs": procs()}))
        return
    print(json.dumps({"organizer": org, "titles": [t for _, t in ws if t],
                      "sessions": sessions, "procs": procs(),
                      "broker": len(broker_connections())}))


def close():
    """WM_DELETE_WINDOW to the Organizer, Return on the Quit dialog."""
    from Xlib import X, display, protocol
    deadline = time.time() + 60
    lastclose = 0
    while time.time() < deadline:
        ws = windows()
        org = next((w for w, t in ws if "Organizer" in t), None)
        if org is None and not procs().get("gateway"):
            break
        dlg = next((w for w, t in ws if t == "Quit ECCE"), None)
        if dlg:
            subprocess.run(["xdotool", "windowfocus", str(int(dlg, 16))],
                           env=env(), stderr=subprocess.DEVNULL)
            time.sleep(0.5)
            subprocess.run(["xdotool", "key", "Return"], env=env())
        elif org and time.time() - lastclose > 8:
            d = display.Display(DISPLAY)
            w = d.create_resource_object("window", int(org, 16))
            d.send_event(w, protocol.event.ClientMessage(
                window=w, client_type=d.intern_atom("WM_PROTOCOLS"),
                data=(32, [d.intern_atom("WM_DELETE_WINDOW"), X.CurrentTime,
                           0, 0, 0])), event_mask=X.NoEventMask)
            d.flush()
            d.close()
            lastclose = time.time()
        time.sleep(1)
    time.sleep(2)
    print(json.dumps(state()))


def state():
    return {"titles": [t for _, t in windows() if t], "procs": procs(),
            "broker": len(broker_connections())}


if __name__ == "__main__":
    cmd = sys.argv[1]
    if cmd == "start":
        start(*sys.argv[2:5])
    elif cmd == "start-refused":
        start(*sys.argv[2:5], expect_login=False)
    elif cmd == "close":
        close()
    elif cmd == "state":
        print(json.dumps(state()))
