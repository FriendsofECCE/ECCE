#!/usr/bin/env python3
"""The first-start window itself (#240), answered by its test hook.

    first_start_window_test.py <ecce-localdata binary> <scratch dir> [<ecce-first-start>]

Runs the real window (wxPython, a display) with ECCE_FIRST_START_ANSWER, as
ecce, ecce.cmd and Edit > Change Server... start it, for both answers and
checks what is left on disk: local = the data folder and the preference on;
server = ~/.ECCE/RemoteServer written against a stub data server (an HTTP
listener).  Linux, macOS and Windows; exit 77 without wxPython.
"""
import http.server
import os
import shutil
import subprocess
import sys
import threading

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
localdata, scratch = os.path.abspath(sys.argv[1]), sys.argv[2].replace("\\", "/")
SCRIPT = sys.argv[3] if len(sys.argv) > 3 else os.path.join(REPO, "packaging", "gateway", "ecce-first-start")
WIN = sys.platform == "win32"
failures = []

if subprocess.run([sys.executable, "-c", "import wx"], capture_output=True).returncode:
    print("no wxPython")
    sys.exit(77)


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


httpd = http.server.HTTPServer(("127.0.0.1", 0), Stub)
threading.Thread(target=httpd.serve_forever, daemon=True).start()
port = httpd.server_address[1]


def fresh(name):
    base = os.path.join(scratch, name)
    shutil.rmtree(base, ignore_errors=True)
    ehome, user = base + "/ecce", base + "/user"
    os.makedirs(ehome + "/bin")
    os.makedirs(ehome + "/siteconfig")
    os.makedirs(user)
    with open(ehome + "/siteconfig/DataServers", "w") as f:
        f.write("<DataServers><Server><Url>http://localhost:8096/Ecce</Url>"
                "<Desc>local</Desc></Server></DataServers>\n")
    env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
    env.update(ECCE_HOME=ehome, ECCE_REALUSERHOME=user,
               PATH=os.path.dirname(localdata) + os.pathsep + os.environ.get("PATH", ""))
    return env, user


def window(env, answer, *args):
    env = dict(env, ECCE_FIRST_START_ANSWER=answer)
    return subprocess.run([sys.executable, SCRIPT, *args], env=env,
                          capture_output=True, text=True, timeout=180)


def pref(env):
    return subprocess.run([localdata, "pref-state"], env=env,
                          capture_output=True, text=True).stdout.strip()


local_dir = "ecce-local" if WIN else ".ECCE-local"

env, user = fresh("local")
r = window(env, "local")
check(r.returncode == 0 and os.path.isdir(os.path.join(user, local_dir)) and pref(env) == "on",
      "window, Store data on this computer: the folder and the preference (rc=%d %s)"
      % (r.returncode, r.stderr.strip()))

env, user = fresh("server")
r = window(env, "server:127.0.0.1:%d" % port)
ds = os.path.join(user, ".ECCE", "RemoteServer", "DataServers")
text = open(ds).read() if os.path.exists(ds) else ""
check(r.returncode == 0 and ("http://127.0.0.1:%d/Ecce" % port) in text and pref(env) == "off",
      "window, Connect to a server: RemoteServer written, preference off (rc=%d %s)"
      % (r.returncode, r.stderr.strip()))

# Edit > Change Server...: from the server back to this computer, then away again.
r = window(env, "local", "--change")
check(r.returncode == 0 and not os.path.exists(ds) and pref(env) == "on",
      "Change Server, to this computer (rc=%d %s)" % (r.returncode, r.stderr.strip()))
r = window(env, "server:127.0.0.1:%d" % port, "--change")
check(r.returncode == 0 and os.path.exists(ds) and pref(env) == "off",
      "Change Server, to the server (rc=%d %s)" % (r.returncode, r.stderr.strip()))

env, user = fresh("refused")
r = window(env, "server:127.0.0.1:1")
check(r.returncode == 4 and not os.path.exists(os.path.join(user, ".ECCE", "RemoteServer", "DataServers")),
      "window, a server that does not answer is refused and writes nothing (rc=%d)" % r.returncode)

httpd.shutdown()
sys.exit(1 if failures else 0)
