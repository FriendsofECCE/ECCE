#!/usr/bin/env python3
"""The Organizer draws first while remembered ssh servers log in (#1272).

Two remembered SSH folders on one host connect at start.  `ssh` on PATH is
a stand-in that takes 15 s to fail a login and logs each attempt, so no
host is contacted.  The host is a .invalid name: localhost would be taken
as this machine and run without ssh.  Checks:
  - "Organizer: window shown" comes before either login returns;
  - while the login is pending a hook command is answered within ~2 s and
    both roots say they are connecting;
  - the two servers share one login attempt (one prompt, not two);
  - afterwards the roots say "(not connected)" and `desk` reports a pane
    of real size, shown on screen.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402

LOGIN_SECONDS = 15
failures = []

FAKE_SSH = r"""#!/bin/sh
# Stand-in for ssh: answers -G/-O at once, any login fails after a delay.
log=%(log)s
echo "call $*" >> "$log"
for a in "$@"; do
  case "$a" in
    -G) printf 'controlmaster false\ncontrolpath none\n'; exit 0 ;;
    -O) exit 255 ;;
  esac
done
echo "start $(date +%%s.%%N)" >> "$log"
sleep %(secs)d
echo "end $(date +%%s.%%N)" >> "$log"
echo "ssh: connect to host localhost port 22: Connection timed out" >&2
exit 255
"""


def check(ok, what):
    print(("ok    " if ok else "FAIL  ") + what, flush=True)
    if not ok:
        failures.append(what)


def run(display):
    work = tempfile.mkdtemp(prefix="ecce-slowssh-")
    data = tempfile.mkdtemp(prefix="ecce-slowssh-data-")
    try:
        bindir = os.path.join(work, "bin")
        os.makedirs(bindir)
        sshlog = os.path.join(work, "ssh.log")
        open(sshlog, "w").close()
        with open(os.path.join(bindir, "ssh"), "w") as f:
            f.write(FAKE_SSH % {"log": sshlog, "secs": LOGIN_SECONDS})
        os.chmod(os.path.join(bindir, "ssh"), 0o755)

        state = os.environ["ECCE_REALUSERHOME"]
        os.makedirs(os.path.join(state, ".ECCE"), exist_ok=True)
        servers = os.path.join(state, ".ECCE", "AddedServers")
        saved = open(servers).read() if os.path.exists(servers) else None
        user = os.environ.get("USER", "ecce")
        with open(servers, "w") as f:
            f.write("ssh\tSlow one\tsftp://%s@slow-login.invalid/tmp/ecce-slow-one\t1\n"
                    "ssh\tSlow two\tsftp://%s@slow-login.invalid/tmp/ecce-slow-two\t1\n"
                    % (user, user))

        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()
        live = []
        original = apps._drain

        def drain(proc, chunks):
            live.append(chunks)
            original(proc, chunks)
        apps._drain = drain

        def logText():
            return b"".join(live[0]).decode("utf-8", "replace") if live else ""

        def sshEvents(kind):
            return [l for l in open(sshlog).read().splitlines() if l.startswith(kind)]

        answers = {}

        def ask(command, timeout):
            n = len(answers)
            with open(cmds, "a") as f:
                f.write(command + "\n")
            t0 = time.time()
            pat = re.compile(r"ECCE_TEST_ORGANIZER: %s: (.*)" % re.escape(command))
            while time.time() - t0 < timeout:
                found = pat.findall(logText())
                if len(found) > answers.get(command, 0):
                    answers[command] = len(found)
                    return found[-1], time.time() - t0
                time.sleep(0.1)
            return None, time.time() - t0

        done = threading.Event()

        def drive():
            try:
                t0 = time.time()
                while time.time() - t0 < 120 and "Organizer: window shown" not in logText():
                    time.sleep(0.1)
                shown = "Organizer: window shown" in logText()
                ended = sshEvents("end")
                check(shown and not ended,
                      "window shown (%.1f s) before any login returned (%d returned)"
                      % (time.time() - t0, len(ended)))
                labels, waited = ask("server roots", 10)
                pending = not sshEvents("end")
                print("      roots while pending: %r (%.1f s)" % (labels, waited))
                check(labels is not None and waited < 2.5,
                      "a hook answers within 2 s while the login is pending (%.1f s)" % waited)
                check(pending and labels is not None and labels.count("connecting") == 2,
                      "both roots say they are connecting")
                t1 = time.time()
                while time.time() - t1 < 3 * LOGIN_SECONDS + 30:
                    starts, ends = sshEvents("start"), sshEvents("end")
                    if starts and len(ends) == len(starts):
                        break
                    time.sleep(0.5)
                time.sleep(3)
                check(len(sshEvents("start")) == 1,
                      "the two servers on one host share one login (%d attempts)"
                      % len(sshEvents("start")))
                labels, _ = ask("server roots", 10)
                print("      roots after: %r" % labels)
                check(labels is not None and labels.count("(not connected)") == 2
                      and "connecting" not in labels,
                      "both roots end up not connected")
                desk, _ = ask("desk", 10)
                print("      desk: %r" % desk)
                m = re.match(r"desk (\d+)x(\d+) shown=(\d)", desk or "")
                check(m is not None and int(m.group(1)) >= 100 and int(m.group(2)) >= 100
                      and m.group(3) == "1", "the desk has a real size and is shown")
            finally:
                done.set()

        driver = threading.Thread(target=drive, daemon=True)
        driver.start()
        env = {"ECCE_TEST_ORGANIZER": cmds, "ECCE_LOCAL_DATA": data,
               "ECCE_SSH_BACKEND": "openssh",
               "PATH": bindir + ":" + os.environ.get("PATH", "")}
        result = apps.run(display, "organizer", windowTimeout=120, settle=240,
                          env=env, ready=done.is_set)
        apps._drain = original
        driver.join(5)
        if not done.is_set():
            check(False, "the driver finished (window: %s)" % result.sawWindow)
        if failures:
            print((result.log or "")[-3000:])
        if saved is None:
            os.remove(servers)
        else:
            with open(servers, "w") as f:
                f.write(saved)
    finally:
        shutil.rmtree(work, ignore_errors=True)
        shutil.rmtree(data, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("slowssh", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
