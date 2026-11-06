#!/usr/bin/env python3
"""The Organizer survives a session-start catch-up whose machine never
answers (#780).

A MOPAC job (a stand-in that waits for a `release` file) runs on localhost;
its store is stopped with SIGTERM, as at logout, which parks it as "waiting
for login".  The machine's host is then pointed at a listener that accepts
TCP and never sends the ssh banner, through a private ssh config (a wrapper
ssh on PATH passes -F; the real ~/.ssh is never read), with an ECCE-owned
master as `ecce` sets it up.  The Organizer starts, its catch-up tries the
login, the login times out ("no answer in time") and the Organizer must still
be running and answer a hook afterwards.

    tests/launch/silent_host_catchup_test.py [--build DIR] [--keep] [--gdb] [--ask]

--gdb runs the Organizer under gdb and prints the stack of a crash; --ask
makes ECCE's own wait (ECCE_SSH_ASK_TIMEOUT) end the login before ssh's
ConnectTimeout does, as when name resolution hangs.
Exit status 77 (CTest SKIP) without Xvfb, ssh, mopac or the built binaries.
"""

import argparse
import glob
import os
import shutil
import signal
import socket
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def procs(needle):
    found = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                cmd = handle.read().replace(b"\0", b" ").decode("utf-8", "replace")
        except OSError:
            continue
        if all(n in cmd for n in needle):
            found.append(int(entry))
    return found


def silentListener():
    """A loopback port that accepts and holds every connection, mute."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", 0))
    s.listen(16)
    held = []

    def serve():
        while True:
            try:
                c, _ = s.accept()
            except OSError:
                return
            held.append(c)
    threading.Thread(target=serve, daemon=True).start()
    return s, s.getsockname()[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--gdb", action="store_true")
    ap.add_argument("--ask", action="store_true",
                    help="let ECCE's own wait end first ('no answer in time')")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    if not os.access("/usr/bin/ssh", os.X_OK):
        harness.skip("no /usr/bin/ssh")
    harness.prerequisites(build, ("mopac",))
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "silentcu", {}, local=True, keep=args.keep)
    wrapper = os.path.join(s.state, "held-mopac")
    with open(wrapper, "w") as h:
        h.write("#!/bin/sh\n"
                "n=0\n"
                "while [ ! -e release ] && [ $n -lt 600 ]; do sleep 1; n=$((n+1)); done\n"
                "exec %s \"$@\"\n" % shutil.which("mopac"))
    os.chmod(wrapper, 0o755)
    harness.link(os.path.join(build, "organizer"),
                 os.path.join(s.home, "bin", "organizer"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    orgWrapper = os.path.join(wrappers, "ecce-organizer")
    if args.gdb:
        text = open(orgWrapper).read().replace(
            '"$ECCE_HOME/bin/organizer" "$@"',
            'gdb -q -batch -ex run -ex "thread apply all bt" '
            '--args "$ECCE_HOME/bin/organizer" "$@"')
        with open(orgWrapper, "w") as h:
            h.write(text)

    s.registerMachine("localhost", "Shell", {"MOPAC": wrapper})
    jobs = os.path.join(s.state, "jobs")
    os.makedirs(jobs, exist_ok=True)
    shutil.rmtree(os.path.join(s.state, ".ECCE", "waiting"), ignore_errors=True)
    #  No "0" in the names: see waiting_test.py.
    stamp = int(str(int(time.time()) % 100000).replace("0", "9"))
    display = org = listener = None
    rundir = ""
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        name = "silent-%d" % stamp
        url, out = s.create(name, "mopac_es", DECK, "mopac.mop", jobs)
        if not check(url is not None, "%s created" % name):
            say(out)
            return 1
        rc, out = s.launch(url)
        if not check(rc == 0, "%s launched" % name):
            say(out[-600:])
            return 1
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
               if l.startswith("run directory:")]
        rundir = ran[-1] if ran else ""
        seen = False
        for _ in range(120):
            logs = glob.glob(os.path.join(s.state, "tmp", "*", "jobs",
                                          name + "__*", "eccejob*.log*"))
            if any("job monitor running on" in open(l, errors="replace").read()
                   for l in logs):
                seen = True
                break
            time.sleep(1)
        check(seen, "%s is monitored" % name)
        for pid in procs(["eccejobstore", name]) + procs(["eccejobmaster", url]):
            try:
                os.kill(pid, signal.SIGTERM)
            except OSError:
                pass
        for _ in range(60):
            if not procs(["eccejobstore", name]):
                break
            time.sleep(0.5)
        st = s.waitState(url, seconds=60, want=("waiting",))
        if not check(st == "waiting", "parked as waiting (%s)" % st):
            return 1

        #  The machine now lives on a host that never answers.
        listener, port = silentListener()
        sshdir = os.path.join(s.state, "ssh")
        sshbin = os.path.join(s.state, "sshbin")
        control = os.path.join(s.state, "cm")
        for d in (sshdir, sshbin, control):
            os.makedirs(d, mode=0o700, exist_ok=True)
            os.chmod(d, 0o700)
        cfg = os.path.join(sshdir, "config")
        with open(cfg, "w") as h:
            h.write("Host silenthost\n  HostName 127.0.0.1\n  Port %d\n"
                    "  UserKnownHostsFile %s/known_hosts\n"
                    "  StrictHostKeyChecking yes\n" % (port, sshdir))
        with open(os.path.join(sshbin, "ssh"), "w") as h:
            h.write('#!/bin/sh\nexec /usr/bin/ssh -F "%s" "$@"\n' % cfg)
        os.chmod(os.path.join(sshbin, "ssh"), 0o755)
        mm = os.path.join(s.state, ".ECCE", "MyMachines")
        lines = open(mm).read().splitlines()
        with open(mm, "w") as h:
            for l in lines:
                f = l.split("\t")
                if f and f[0] == "localhost":
                    f[1] = "silenthost"
                h.write("\t".join(f) + "\n")

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": sshbin + os.pathsep + wrappers + os.pathsep +
                              s.env()["PATH"],
                      "ECCE_SSH_CONTROL_DIR": control,
                      "ECCE_SSH_CONNECT_TIMEOUT": "40" if args.ask else "4",
                      "ECCE_SSH_ASK_TIMEOUT": "6" if args.ask else "120",
                      "ECCE_ASKPASS": "/bin/false"})
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [orgWrapper], env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        #  The catch-up's login gives up after the connect timeout.
        said = False
        for _ in range(180):
            text = open(olog, errors="replace").read()
            if "could not open the ssh connection" in text or org.poll() is not None:
                said = "could not open the ssh connection" in text
                break
            time.sleep(1)
        check(said, "the catch-up's login timed out")
        time.sleep(10)
        text = open(olog, errors="replace").read()
        #  A host that did not answer gets no second login over libssh.
        check("trying the built-in client" not in text,
              "no built-in client retry after the host did not answer")
        if args.ask:
            check("no answer in time" in text, "ECCE's own wait ended the login")

        def send(line, timeout=120):
            seen = len(open(olog, errors="replace").read().splitlines())
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if org.poll() is not None:
                    return "the Organizer ended (%s)" % org.returncode
                for t in open(olog, errors="replace").read().splitlines()[seen:]:
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        ans = send("reconnectselectedlist %s" % url)
        check(org.poll() is None, "the Organizer is still running")
        check(name in ans, "the Organizer answers a hook afterwards (%s)" % ans)
        check(s.state_of(url) == "waiting", "the job is still waiting")
    finally:
        if org and org.poll() is None:
            os.killpg(org.pid, signal.SIGTERM)
        if org and (failures or args.gdb):
            say(open(olog, errors="replace").read()[-6000:])
        if rundir:
            open(os.path.join(rundir, "release"), "w").close()
        if listener:
            listener.close()
        if display:
            display.__exit__(None, None, None)
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
