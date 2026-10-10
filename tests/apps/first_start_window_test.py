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
import socket
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

# A listener standing in for the broker (the window checks it is reachable).
brk = socket.socket()
brk.bind(("127.0.0.1", 0))
brk.listen(5)
broker_port = brk.getsockname()[1]
dead = socket.socket()          # a port nothing answers on
dead.bind(("127.0.0.1", 0))
dead_port = dead.getsockname()[1]
dead.close()


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
    env.update(ECCE_HOME=ehome, ECCE_REALUSERHOME=user, ECCE_BROKER_PORT=str(broker_port),
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

# The next start asks again with the server preselected; Continue keeps
# it as it is rather than setting it up again.
before = os.stat(ds).st_mtime_ns if os.path.exists(ds) else 0
r = window(env, "continue")
check(r.returncode == 0 and "preselected server" in r.stderr
      and os.path.exists(ds) and os.stat(ds).st_mtime_ns == before,
      "next start, server preselected, Continue leaves it untouched (rc=%d %s)"
      % (r.returncode, r.stderr.strip()))

# Edit > Change Server...: from the server back to this computer, then away again.
r = window(env, "local", "--change")
check(r.returncode == 0 and not os.path.exists(ds) and pref(env) == "on",
      "Change Server, to this computer (rc=%d %s)" % (r.returncode, r.stderr.strip()))
r = window(env, "server:127.0.0.1:%d" % port, "--change")
check(r.returncode == 0 and os.path.exists(ds) and pref(env) == "off",
      "Change Server, to the server (rc=%d %s)" % (r.returncode, r.stderr.strip()))

# A Linux user who already works on the per-user data server, with the
# server package installed: asked, "Connect to a server" preselected with
# localhost, and Continue keeps that data server (no local folder,
# preference untouched).
env, user = fresh("perusersrv")
os.makedirs(os.path.join(user, ".ECCE", "dataserver"))
open(os.path.join(env["ECCE_HOME"], "bin", "ecce-dataserver-start"), "w").close()
r = subprocess.run([sys.executable, SCRIPT, "--check"], env=env, capture_output=True,
                   text=True, timeout=60)
check(r.stdout.strip() == "ask", "per-user data server user is asked (%s)" % r.stdout.strip())
r = window(env, "continue")
ans = os.path.join(user, ".ECCE", "first-start-answer")
check(r.returncode == 0 and "preselected server" in r.stderr
      and not os.path.isdir(os.path.join(user, local_dir)) and pref(env) == "unset"
      and os.path.exists(ans),
      "per-user data server: server (localhost) preselected, Continue keeps the "
      "data server (rc=%d %s)" % (r.returncode, r.stderr.strip()))

# The same user choosing "Store data on this computer" gets the folder,
# not the data server they had.
env, user = fresh("perusersrv-local")
os.makedirs(os.path.join(user, ".ECCE", "dataserver"))
open(os.path.join(env["ECCE_HOME"], "bin", "ecce-dataserver-start"), "w").close()
r = window(env, "local")
check(r.returncode == 0 and os.path.isdir(os.path.join(user, local_dir)) and pref(env) == "on",
      "per-user data server user choosing this computer gets the folder "
      "(rc=%d %s)" % (r.returncode, r.stderr.strip()))

env, user = fresh("refused")
r = window(env, "server:127.0.0.1:1")
check(r.returncode == 4 and not os.path.exists(os.path.join(user, ".ECCE", "RemoteServer", "DataServers")),
      "window, a server that does not answer is refused and writes nothing (rc=%d)" % r.returncode)

# The address field: the last server after switching to this computer; with
# a server package and nothing set up, this computer.
env2, user2 = fresh("prefill")
r = window(env2, "server:127.0.0.1:%d" % port)
r = window(env2, "local")
r = window(env2, "continue")
check("address 127.0.0.1\n" in r.stderr,
      "address prefilled with the last server after switching to local (%s)" % r.stderr.strip())
env2, user2 = fresh("prefill-srv")
open(os.path.join(env2["ECCE_HOME"], "bin", "ecce-dataserver-start"), "w").close()
r = window(env2, "continue")
check("address localhost\n" in r.stderr,
      "address prefilled with localhost when the server package is installed (%s)" % r.stderr.strip())

# This computer with the server package and no broker answering: the
# per-user data server, as for a user who never saw the window.
env2, user2 = fresh("thiscomputer")
open(os.path.join(env2["ECCE_HOME"], "bin", "ecce-dataserver-start"), "w").close()
env2["ECCE_BROKER_PORT"] = str(dead_port)
r = window(env2, "server:localhost")
check(r.returncode == 0 and not os.path.exists(os.path.join(user2, ".ECCE", "RemoteServer"))
      and pref(env2) == "off" and os.path.exists(os.path.join(user2, ".ECCE", "first-start-answer")),
      "server:localhost with a server package and no broker: per-user data server (rc=%d %s)"
      % (r.returncode, r.stderr.strip()))
# ... but a broker answering there means a central server: connect as a client.
env2, user2 = fresh("thiscomputer-central")
open(os.path.join(env2["ECCE_HOME"], "bin", "ecce-dataserver-start"), "w").close()
r = window(env2, "server:localhost:%d" % port)
check(r.returncode == 0 and os.path.exists(os.path.join(user2, ".ECCE", "RemoteServer", "DataServers")),
      "server:localhost with a broker answering: connected as a client (rc=%d %s)"
      % (r.returncode, r.stderr.strip()))

# A data server that answers on a host whose broker does not: refused, nothing written.
env2, user2 = fresh("nobroker")
env2["ECCE_BROKER_PORT"] = str(dead_port)
r = window(env2, "server:127.0.0.1:%d" % port)
check(r.returncode == 4 and "message broker" in r.stderr
      and not os.path.exists(os.path.join(user2, ".ECCE", "RemoteServer"))
      and pref(env2) == "unset",
      "a server whose broker does not answer is refused, nothing written (rc=%d %s)"
      % (r.returncode, r.stderr.strip()))

httpd.shutdown()
sys.exit(1 if failures else 0)
