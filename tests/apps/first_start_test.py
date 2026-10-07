#!/usr/bin/env python3
"""The first-start question (#240): when it is asked and when it is not.

    first_start_test.py <ecce-localdata binary> <scratch dir>

Runs `ecce-first-start --check` (no window) against a private $ECCE_HOME and
user home.  The one fresh client asks; every setup that already exists, in
any of the three deployment modes, does not.  Needs no display and no
services.
"""
import os
import shutil
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SCRIPT = os.path.join(REPO, "packaging", "gateway", "ecce-first-start")
localdata, scratch = sys.argv[1], sys.argv[2]
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
    os.symlink(os.path.abspath(localdata), ehome + "/bin/ecce-localdata")
    if server_install:
        open(ehome + "/bin/ecce-dataserver-start", "w").close()
    env = {k: v for k, v in os.environ.items()
           if not k.startswith("ECCE_") and k not in ("DISPLAY", "WAYLAND_DISPLAY")}
    env.update(ECCE_HOME=ehome, ECCE_REALUSERHOME=user, DISPLAY=":99")
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

cases = []


def case(name, what, setup, server_install=False):
    cases.append((name, what, setup, server_install))


case("siteremote", "siteconfig/RemoteServer is present (admin or ecce-remote-setup)",
     lambda e, u, h: (os.makedirs(h + "/siteconfig/RemoteServer"),
                      open(h + "/siteconfig/RemoteServer/DataServers", "w").close()))
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
case("localdir", "~/.ECCE-local exists",
     lambda e, u, h: os.makedirs(u + "/.ECCE-local"))
case("serverdata", "~/.ECCE/dataserver exists",
     lambda e, u, h: os.makedirs(u + "/.ECCE/dataserver"))
case("chosen", "a server was chosen before (~/.ECCE/RemoteServer)",
     lambda e, u, h: (os.makedirs(u + "/.ECCE/RemoteServer"),
                      open(u + "/.ECCE/RemoteServer/DataServers", "w").close()))
case("pref", "the data folder preference was set",
     lambda e, u, h: subprocess.run([localdata, "pref", "off"], env=e, check=True))
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

sys.exit(1 if failures else 0)
