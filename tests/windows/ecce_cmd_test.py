#!/usr/bin/env python3
"""ecce.cmd on Windows: the first-start question and the two kinds of session (#240).

    ecce_cmd_test.py <unpacked ecce dir> <scratch dir>

Runs the package's own ecce.cmd as a fresh user (USERPROFILE is a scratch
folder), the window answered by ECCE_FIRST_START_ANSWER:
  local   the data folder is made and no server is chosen;
  server  against a stub server (HTTP listener for the data server, a TCP
          listener for the broker), ~/.ECCE/RemoteServer is written and the
          session's broker file names the stub broker, which is what the
          apps read to reach a central server.
organizer.exe is started both times and ended by the test.
"""
import glob
import http.server
import os
import shutil
import subprocess
import sys
import threading
import time

root, scratch = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
failures = []


def check(ok, what):
    print(("PASS  " if ok else "FAIL  ") + what, flush=True)
    if not ok:
        failures.append(what)


class Stub(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.end_headers()

    def log_message(self, *a):
        pass


def listener():
    s = http.server.HTTPServer(("127.0.0.1", 0), Stub)
    threading.Thread(target=s.serve_forever, daemon=True).start()
    return s, s.server_address[1]


data, dport = listener()
broker, bport = listener()


def organizer_running():
    r = subprocess.run(["tasklist", "/FI", "IMAGENAME eq organizer.exe", "/NH"],
                       capture_output=True, text=True)
    return "organizer.exe" in r.stdout


def cmd(env, cwd, log):
    """ecce.cmd to a log file: the Organizer it starts keeps a pipe open for as long as it lives."""
    with open(log, "w") as out:
        r = subprocess.run(["cmd", "/c", os.path.join(root, "ecce.cmd")], env=env,
                           stdout=out, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                           timeout=180, cwd=cwd)
    return r.returncode, open(log, errors="replace").read()


def run(name, answer, **extra):
    profile = os.path.join(scratch, name)
    shutil.rmtree(profile, ignore_errors=True)
    os.makedirs(profile)
    os.makedirs(scratch, exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
    env.update(USERPROFILE=profile, ECCE_FIRST_START_ANSWER=answer, **extra)
    rc, text = cmd(env, profile, os.path.join(scratch, name + ".log"))
    up = False
    for _ in range(30):
        if organizer_running():
            up = True
            break
        time.sleep(1)
    subprocess.run(["taskkill", "/F", "/IM", "organizer.exe"], capture_output=True)
    time.sleep(2)
    print("%s: rc=%d organizer=%s\n%s" % (name, rc, up, text))
    return rc, profile, up


r, profile, up = run("local", "local")
check(r == 0 and up, "local: ecce.cmd starts the Organizer")
check(os.path.isdir(profile + "/ecce-local"), "local: the data folder is made")
check(not os.path.exists(profile + "/.ECCE/RemoteServer/DataServers"), "local: no server is chosen")

r, profile, up = run("server", "server:127.0.0.1:%d" % dport, ECCE_BROKER_PORT=str(bport))
check(os.path.exists(profile + "/.ECCE/RemoteServer/DataServers"),
      "server: ~/.ECCE/RemoteServer is written")
files = glob.glob(profile + "/.ECCE/broker_*")
text = open(files[0]).read() if files else ""
check("host=127.0.0.1" in text and ("port=%d" % bport) in text,
      "server: the session's broker file names the server's broker (%s)" % text.replace("\n", " "))
check(up, "server: ecce.cmd starts the Organizer")

# The server is down: ecce.cmd says so and starts nothing.
data.shutdown()
data.server_close()
profile = os.path.join(scratch, "server")
env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
env.update(USERPROFILE=profile, ECCE_FIRST_START_ANSWER="", ECCE_NO_FIRST_START="1",
           ECCE_BROKER_PORT=str(bport))
for f in glob.glob(profile + "/.ECCE/broker_*"):
    os.remove(f)
rc, text = cmd(env, profile, os.path.join(scratch, "down.log"))
print("down: rc=%d\n%s" % (rc, text))
check(rc != 0 and not organizer_running(), "server down: ecce.cmd stops with an error")
subprocess.run(["taskkill", "/F", "/IM", "organizer.exe"], capture_output=True)
sys.exit(1 if failures else 0)
