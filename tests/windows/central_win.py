#!/usr/bin/env python3
"""A Windows client against a central server, through the apps' own test
hooks, no clicks: the first-start question answered "Connect to a server"
(ECCE_FIRST_START_ANSWER), the session started as ecce.cmd starts it, the
Organizer logged in (an -pipe auth file instead of the login window), a
project and a MOPAC calculation made on the server, the Builder draws water
and saves, the Calculation Editor saves the input; then the input is read
back from the server over WebDAV.  Run on a desktop session:

    python central_win.py <install tree> <state dir> <host:port> <user> <password>

The state dir is this run's own ECCE user state (ECCE_REALUSERHOME); the
user's real ~/.ECCE is not read or changed.
"""
import base64
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.request

tree, state = (os.path.abspath(a).replace("\\", "/") for a in sys.argv[1:3])
server, user, password = sys.argv[3:6]
BIN = tree + "/bin"
failures = []
sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def say(t):
    print(t, flush=True)


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


shutil.rmtree(state, ignore_errors=True)
home, out = state + "/home", state + "/out"
for d in (home + "/.ECCE", out, state + "/tmp"):
    os.makedirs(d)
sysdir = os.environ.get("SystemRoot", r"C:\Windows")
env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
env.update({
    "ECCE_HOME": tree, "ECCE_REALUSERHOME": home,
    "ECCE_REALUSER": os.environ.get("USERNAME", "user"),
    "HOST": os.environ.get("COMPUTERNAME", "localhost"),
    "ECCE_TMPDIR": state + "/tmp", "ECCE_SESSION_LIVENESS": "lease",
    "PATH": os.pathsep.join([BIN, tree + "/scripts", tree + "/scripts/parsers", tree + "/usr/bin",
                             tree + "/python", tree + "/strawberry/perl/site/bin",
                             tree + "/strawberry/perl/bin", tree + "/strawberry/c/bin",
                             sysdir + r"\System32", sysdir, os.environ.get("PATH", "")]),
})

# 1. The first-start question, answered as its "Connect to a server" button.
r = subprocess.run([tree + "/python/python3.exe", BIN + "/ecce-first-start"],
                   env=dict(env, ECCE_FIRST_START_ANSWER="server:" + server),
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
say("first start: rc=%d %s" % (r.returncode, r.stdout.decode(errors="replace").strip()))
mine = home + "/.ECCE/RemoteServer"
check(os.path.exists(mine + "/DataServers"), "the answer wrote %s/DataServers" % mine)
if os.path.exists(mine + "/DataServers"):
    for line in open(mine + "/DataServers", errors="replace"):
        if "<Url>" in line:
            say("  " + line.strip())

# 2. The session, as ecce.cmd starts a server session.
env.update({"ECCE_REMOTE_SERVER": "1", "ECCE_REMOTE_DIR": mine,
            "ECCE_SESSION_ID": os.urandom(8).hex()})
r = subprocess.run([tree + "/usr/bin/bash.exe", BIN + "/ecce-gateway-start"], env=env,
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
say("gateway: rc=%d %s" % (r.returncode, r.stdout.decode(errors="replace").strip()))
check(r.returncode == 0, "ecce-gateway-start reached the server")

# The login is keyed by the server's URL as the answer wrote it (https on
# 8443 when the server has TLS).
base = ""
if os.path.exists(mine + "/DataServers"):
    m = re.search(r"<Url>\s*(https?://[^/<]+)/", open(mine + "/DataServers", errors="replace").read())
    base = m.group(1) + "/" if m else ""


def authPipe(name):
    """An -pipe auth file: the login the window would have collected."""
    path = os.path.join(state, name + ".auth")
    with open(path, "w", newline="\n") as h:
        h.write("1\n%s|%s|%s\n" % (base, user, password))
    return path


cmds = os.path.join(state, "org.cmds")
open(cmds, "w").close()
orglog = open(os.path.join(out, "organizer.log"), "w")
org = subprocess.Popen([BIN + "/organizer.exe", "-pipe", authPipe("org")],
                       env=dict(env, ECCE_TEST_ORGANIZER=cmds, ECCE_FEEDBACK_STDERR="1"),
                       stdout=orglog, stderr=subprocess.STDOUT)


def ask(cmd, wait=120):
    with open(cmds, "a", newline="\n") as h:
        h.write(cmd + "\n")
    for _ in range(wait):
        time.sleep(1)
        text = open(os.path.join(out, "organizer.log"), errors="replace").read()
        hits = [l for l in text.splitlines() if l.startswith("ECCE_TEST_ORGANIZER: %s: " % cmd)]
        if hits:
            return hits[-1].split(": ", 2)[2]
        if org.poll() is not None:
            return "organizer exited %s" % org.returncode
    return "no answer"


def app(name, var, script, args, timeout=240):
    path = os.path.join(state, name + ".script")
    with open(path, "w", newline="\n") as h:
        h.write(script)
    log = os.path.join(out, name + ".log")
    with open(log, "w") as h:
        try:
            rc = subprocess.run([BIN + "/" + name + ".exe", "-pipe", authPipe(name)] + list(args),
                                env=dict(env, **{var: path, "ECCE_FEEDBACK_STDERR": "1"}),
                                stdout=h, stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            rc = "timeout"
    return rc, open(log, encoding="utf-8", errors="replace").read()


def look(proc, name):
    """The windows a process has up, listed and photographed (no input)."""
    here = os.path.dirname(os.path.abspath(__file__))
    ps = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File"]
    r = subprocess.run(ps + [os.path.join(here, "titles.ps1"), "-ProcId", str(proc.pid)],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    say("  %s windows: %s" % (name, " | ".join(r.stdout.decode(errors="replace").split("\n")).strip()))
    subprocess.run(ps + [os.path.join(here, "winshot.ps1"), "-ProcId", str(proc.pid), "-Png",
                         os.path.join(out, name + ".png")], stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, timeout=60)


time.sleep(10)
a = ask("newproject wincentral")
if not a.startswith("ok"):
    look(org, "organizer")
check(a.startswith("ok"), "project made on the server (%s)" % a)
a = ask("newcalc wincentral water MOPAC")
check(a.startswith("ok"), "calculation made on the server (%s)" % a)
url = a.split(" ", 1)[1].strip() if a.startswith("ok ") else ""
say("  calculation: " + url)
if url:
    rc, text = app("builder", "ECCE_BUILDER_SCRIPT",
                   "wait 4000\nadd O Bent 0 0 0\nwait 1500\ncmd addh\nwait 1500\n"
                   "expect atoms 3\nsave\nwait 4000\nquit\n", ("-context", url))
    check("expect atoms 3: ok" in text, "the Builder drew water and saved")
    rc, text = app("calced", "ECCE_CALCED_SCRIPT",
                   "wait 3000\nready\nbutton save\nwait 5000\ninfo\nquit\n", ("-context", url))
    for l in text.splitlines():
        if l.startswith(("FEEDBACK: ", "CALCED: formula")) and l.strip() != "FEEDBACK:":
            say("  " + l)
    check("ready: ok" in text, "the Theory and Runtype dialogs started")
    # The stored input, read back from the server.
    req = urllib.request.Request(url.rstrip("/") + "/Inputs/", method="PROPFIND",
                                 headers={"Depth": "1", "Authorization": "Basic " +
                                          base64.b64encode(("%s:%s" % (user, password)).encode()).decode()})
    ctx = None
    if url.startswith("https:"):
        import ssl
        ctx = ssl.create_default_context(cafile=mine + "/server.pem") \
            if os.path.exists(mine + "/server.pem") else ssl.create_default_context()
        ctx.check_hostname = False
    try:
        listing = urllib.request.urlopen(req, timeout=30, context=ctx).read().decode(errors="replace")
    except Exception as e:
        listing = "error %s" % e
    check(".mop" in listing, "the input is stored on the server (Inputs/*.mop)")

org.kill()
subprocess.run([tree + "/usr/bin/bash.exe", BIN + "/ecce-gateway-stop"], env=env,
               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=60)
say("")
say("FAILED: " + "; ".join(failures) if failures else "PASSED")
sys.exit(1 if failures else 0)
