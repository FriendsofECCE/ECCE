#!/usr/bin/env python3
"""The first-start question (#240): when it is asked and when it is not.

    first_start_test.py <ecce-localdata binary> <scratch dir>

Runs `ecce-first-start --check` (no window) against a private $ECCE_HOME and
user home.  The one fresh client asks, on every platform (the platform's
default data folder is not a choice); every setup that already exists, in
any of the three deployment modes, does not.  Then `--apply` makes each
answer without a window, against a stub server (an HTTP listener).  Needs no
display and no services; runs on Linux, macOS and Windows.
"""
import http.server
import os
import shutil
import socket
import subprocess
import sys
import threading

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SCRIPT = os.path.join(REPO, "packaging", "gateway", "ecce-first-start")
localdata, scratch = sys.argv[1], sys.argv[2].replace("\\", "/")
failures = []


def check(ok, what):
    print(("PASS  " if ok else "FAIL  ") + what, flush=True)
    if not ok:
        failures.append(what)


def fresh(name, server_install=False):
    """(env, home, ecce_home): a client-only install and a user with nothing."""
    base = os.path.join(scratch, name)
    shutil.rmtree(base, ignore_errors=True)
    ehome, user = base + "/ecce", base + "/user"
    os.makedirs(ehome + "/bin")
    os.makedirs(ehome + "/siteconfig")
    os.makedirs(user + "/.ECCE")
    env_path = os.environ.get("PATH", "")
    if os.name == "nt":
        # A copy would lose the DLLs beside the program: find it on PATH.
        env_path = os.path.dirname(os.path.abspath(localdata)) + os.pathsep + env_path
    else:
        os.symlink(os.path.abspath(localdata), ehome + "/bin/ecce-localdata")
        os.symlink(os.path.join(REPO, "data"), ehome + "/data")
    if server_install:
        open(ehome + "/bin/ecce-dataserver-start", "w").close()
    env = {k: v for k, v in os.environ.items()
           if not k.startswith("ECCE_") and k not in ("DISPLAY", "WAYLAND_DISPLAY")}
    env.update(ECCE_HOME=ehome, ECCE_REALUSERHOME=user, DISPLAY=":99", PATH=env_path)
    return env, user, ehome


def verdict(env):
    r = subprocess.run([sys.executable, SCRIPT, "--check"], env=env,
                       capture_output=True, text=True, timeout=30)
    return r.returncode, r.stdout.strip()


shutil.rmtree(scratch, ignore_errors=True)
os.makedirs(scratch)

env, user, ehome = fresh("fresh")
rc, out = verdict(env)
check(rc == 0 and out == "ask", "a fresh client-only user is asked (%s)" % out)

# Data from before the question only preselects a choice: still asked once.
asked = [("localdir", "the local data folder exists",
          lambda e, u, h: os.makedirs(u + ("/ecce-local" if sys.platform == "win32" else "/.ECCE-local"))),
         ("serverdata", "~/.ECCE/dataserver exists",
          lambda e, u, h: os.makedirs(u + "/.ECCE/dataserver")),
         ("chosen", "a server was chosen before (~/.ECCE/RemoteServer)",
          lambda e, u, h: (os.makedirs(u + "/.ECCE/RemoteServer"),
                           open(u + "/.ECCE/RemoteServer/DataServers", "w").close())),
         ("pref", "the data folder preference was set",
          lambda e, u, h: subprocess.run([localdata, "pref", "off"], env=e, check=True))]
for name, what, setup in asked:
    env, user, ehome = fresh(name)
    setup(env, user, ehome)
    rc, out = verdict(env)
    check(rc == 0 and out == "ask", "asked once although " + what + " (%s)" % out)

cases = []


def case(name, what, setup, server_install=False):
    cases.append((name, what, setup, server_install))


case("siteremote", "siteconfig/RemoteServer is present (admin or ecce-remote-setup)",
     lambda e, u, h: (os.makedirs(h + "/siteconfig/RemoteServer"),
                      open(h + "/siteconfig/RemoteServer/DataServers", "w").close()))
if sys.platform.startswith("linux"):   # the other packages carry the script but run no server
    case("serverpkg", "the server package is installed (central server, FastX on it)",
         lambda e, u, h: None, True)
case("serveraccount", "this account runs a central server (~/.ECCE/mosquitto.server)",
     lambda e, u, h: open(u + "/.ECCE/mosquitto.server", "w").close())
case("sharedbroker", "a shared broker is declared (siteconfig/SharedBroker)",
     lambda e, u, h: open(h + "/siteconfig/SharedBroker", "w").close())
case("remoteenv", "ecce -remote / ECCE_REMOTE_SERVER",
     lambda e, u, h: e.update(ECCE_REMOTE_SERVER="1"))
case("localenv", "ECCE_LOCAL_DATA is set",
     lambda e, u, h: e.update(ECCE_LOCAL_DATA=u + "/x"))
case("localenv-empty", "ECCE_LOCAL_DATA is set and empty (a data server)",
     lambda e, u, h: e.update(ECCE_LOCAL_DATA=""))
case("localflag", "ecce --local",
     lambda e, u, h: e.update(ECCE_LOCAL="1"))
case("answered", "the question was answered before (~/.ECCE/first-start-answer)",
     lambda e, u, h: open(u + "/.ECCE/first-start-answer", "w").write("local\n"))
if sys.platform.startswith("linux"):   # macOS and Windows always have a screen
    case("nodisplay", "there is no display",
         lambda e, u, h: e.pop("DISPLAY"))
case("switch", "ECCE_NO_FIRST_START is set (tests, scripts)",
     lambda e, u, h: e.update(ECCE_NO_FIRST_START="1"))

for name, what, setup, srv in cases:
    env, user, ehome = fresh(name, srv)
    setup(env, user, ehome)
    rc, out = verdict(env)
    check(rc == 1 and out.startswith("skip"), "no question when " + what +
          " (%s)" % out)

# The preference alone, as Preferences writes it, is "set"; before it, "unset".
env, user, ehome = fresh("pref-unset")
r = subprocess.run([localdata, "pref-state"], env=env, capture_output=True, text=True)
check(r.stdout.strip() == "unset", "preference starts unset")

# --- the two answers, without a window ----------------------------------
class Stub(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.end_headers()

    def log_message(self, *a):
        pass


httpd = http.server.HTTPServer(("127.0.0.1", 0), Stub)
threading.Thread(target=httpd.serve_forever, daemon=True).start()
port = httpd.server_address[1]


def apply(env, answer):
    r = subprocess.run([sys.executable, SCRIPT, "--apply", answer], env=env,
                       capture_output=True, text=True, timeout=120)
    return r.returncode, (r.stdout + r.stderr).strip()


def prefstate(env):
    r = subprocess.run([localdata, "pref-state"], env=env, capture_output=True, text=True)
    return r.stdout.strip()


env, user, ehome = fresh("apply")
with open(ehome + "/siteconfig/DataServers", "w") as f:
    f.write("<DataServers><Server><Url>http://localhost:8096/Ecce</Url>"
            "<Desc>local</Desc></Server></DataServers>\n")
rc, out = apply(env, "local")
local_dir = user + ("/ecce-local" if sys.platform == "win32" else "/.ECCE-local")
check(rc == 0 and os.path.isdir(local_dir) and prefstate(env) == "on",
      "local: the data folder exists and the preference is on (%s)" % out)
rc, out = verdict(env)
check(rc == 1, "local chosen: not asked again (%s)" % out)

rc, out = apply(env, "server:127.0.0.1:%d" % port)
ds = user + "/.ECCE/RemoteServer/DataServers"
text = open(ds).read() if os.path.exists(ds) else ""
check(rc == 0 and ("http://127.0.0.1:%d/Ecce" % port) in text and
      "ECCE Data Server on 127.0.0.1" in text and prefstate(env) == "off",
      "server: RemoteServer/DataServers names the server, preference off (%s)" % out)
rc, out = verdict(env)
check(rc == 1 and "already answered" in out, "server chosen: not asked again (%s)" % out)

rc, out = apply(env, "local")
check(rc == 0 and not os.path.exists(ds) and os.path.exists(user + "/.ECCE/RemoteServer.off/DataServers")
      and prefstate(env) == "on", "back to local: the server choice is set aside (%s)" % out)

s0 = socket.socket()
s0.bind(("127.0.0.1", 0))
dead = s0.getsockname()[1]
s0.close()
rc, out = apply(env, "server:127.0.0.1:%d" % dead)
check(rc != 0 and not os.path.exists(ds), "an unreachable server writes nothing (%s)" % out)

httpd.shutdown()
sys.exit(1 if failures else 0)
