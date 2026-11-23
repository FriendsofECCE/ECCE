#!/usr/bin/env python3
"""No empty or silently repeated password reaches a password-only host (#1274).

tests/apps/fakessh/ssh plays a host that wants the password "secret": a
connection without a master is denied, a login asks SSH_ASKPASS up to three
times and, as OpenSSH does, sends "" when askpass fails; it logs every prompt
and every answer.  ECCE_ASKPASS is a wrapper that runs ECCE's own
ecce-askpass with the answer from a file (passdialog's ECCE_TEST_AUTH_ANSWER),
so the real askpass and passdialog are under test.  Four SSH folders on the
host pw-login.invalid, one user each so that no login shares another's state:
  - "server connect" with the answer "secret": one prompt, connected;
  - a second folder connected the same way, its master then gone: selecting
    it logs in again in the background, where no prompt is allowed: no
    further prompt and no answer sent;
  - "server connect" answered "cancel": one dialog, no answer sent;
  - "server connect" answered "wrong": each repeated dialog says the password
    was refused.
Exit 77 (skip) without an install or Xvfb.
"""
import os
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402

HOST = "pw-login.invalid"
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + str(detail)) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    work = tempfile.mkdtemp(prefix="ecce-sshpw-")
    try:
        folder = os.path.join(work, "remote")
        os.makedirs(folder)
        users = {"Good": "eccegood", "Quiet": "eccequiet",
                 "Cancel": "eccecancel", "Wrong": "eccewrong"}
        addr = {n: "sftp://%s@%s%s" % (u, HOST, folder) for n, u in users.items()}
        prefs = os.path.join(display.env()["ECCE_REALUSERHOME"], ".ECCE")
        os.makedirs(prefs, exist_ok=True)
        added = os.path.join(prefs, "AddedServers")
        saved = open(added).read() if os.path.exists(added) else None
        with open(added, "w") as h:
            for n in users:
                h.write("ssh\t%s\t%s\t0\n" % (n, addr[n]))
        authlog = os.path.join(work, "auth.log")
        dialogs = os.path.join(work, "dialogs.log")
        answer = os.path.join(work, "answer")
        masters = os.path.join(work, "masters")
        os.makedirs(masters)
        control = os.path.join(work, "cm")
        os.makedirs(control, mode=0o700)
        wrapper = os.path.join(work, "askpass")
        with open(wrapper, "w") as h:
            h.write('#!/bin/sh\nECCE_TEST_AUTH_ANSWER=$(cat %s) '
                    'exec "$ECCE_HOME/scripts/ecce-askpass" "$@"\n' % answer)
        os.chmod(wrapper, 0o755)
        commands, answers = os.path.join(work, "commands"), os.path.join(work, "answers")
        open(commands, "w").close()
        seen = []

        def command(line, timeout=120):
            with open(commands, "a") as h:
                h.write(line + "\n")
            end = time.monotonic() + timeout
            while time.monotonic() < end:
                rows = open(answers).read().splitlines() if os.path.exists(answers) else []
                if len(rows) > len(seen):
                    seen.append(rows[len(seen)])
                    return seen[-1]
                time.sleep(0.05)
            return "no answer"

        def auth(user):
            rows = open(authlog).read().splitlines() if os.path.exists(authlog) else []
            mine = [r for r in rows if (" %s@" % user) in r]
            return ([r for r in mine if r.startswith("prompt")],
                    [r for r in mine if r.startswith("answer")])

        def inspect(_display):
            out = {}
            for name, reply, cmd in (("Good", "secret", "server connect Good"),
                                     ("QuietConnect", "secret", "server connect Quiet"),
                                     ("Quiet", "cancel", "select %s/" % addr["Quiet"]),
                                     ("Cancel", "cancel", "server connect Cancel"),
                                     ("Wrong", "wrong", "server connect Wrong")):
                if name == "Quiet":
                    try:
                        os.remove(os.path.join(masters, "%s@%s" % (users["Quiet"], HOST)))
                    except OSError:
                        pass
                with open(answer, "w") as h:
                    h.write(reply)
                out[name] = command(cmd)
                time.sleep(1)
            return out

        env = {"ECCE_TEST_ORGANIZER": commands, "ECCE_TEST_ORGANIZER_ANSWERS": answers,
               "PATH": os.path.join(HERE, "fakessh") + os.pathsep + display.env()["PATH"],
               "FAKE_SSH_PASSWORD": "secret", "FAKE_SSH_MASTERS": masters,
               "FAKE_SSH_AUTH_LOG": authlog, "ECCE_TEST_AUTH_LOG": dialogs,
               "ECCE_SSH_BACKEND": "openssh", "ECCE_SSH_CONTROL_DIR": control,
               "ECCE_ASKPASS": wrapper, "ECCE_LOCAL_DATA": os.path.join(work, "local")}
        try:
            result = apps.run(display, "organizer", windowTimeout=60, settle=3,
                              env=env, inspect=inspect)
        finally:
            if saved is None:
                os.remove(added)
            else:
                open(added, "w").write(saved)
        if not check(not result.crashed and result.inspected, "the Organizer ran",
                     (result.log or "")[-1500:]):
            return
        out = result.inspected
        log = open(authlog).read() if os.path.exists(authlog) else ""
        shown = open(dialogs).read() if os.path.exists(dialogs) else ""
        print("  hook answers: %r" % out)
        print("  server's auth log:\n    " + "\n    ".join(log.splitlines()))
        print("  dialogs:\n    " + "\n    ".join(shown.splitlines()))
        empty = [r for r in log.splitlines() if r.startswith("answer") and r.endswith("''")]
        check(not empty, "no empty password reached the server", empty)

        p, a = auth(users["Good"])
        check(out.get("Good") == "connected" and len(p) == 1 and len(a) == 1
              and a[0].endswith("'secret'"),
              "connect with the right password: one prompt, connected", (out.get("Good"), p, a))
        p, a = auth(users["Quiet"])
        check(out.get("QuietConnect") == "connected" and len(p) == 1 and len(a) == 1,
              "after its master is gone, a background login asks nothing and sends nothing",
              (out.get("QuietConnect"), p, a))
        p, a = auth(users["Cancel"])
        check(len(p) == 1 and not a, "a cancelled dialog: one prompt, no answer sent", (p, a))
        p, a = auth(users["Wrong"])
        notes = [r for r in shown.splitlines() if ("\t%s\t" % users["Wrong"]) in r]
        check(len(a) == 3 and all(r.endswith("'wrong'") for r in a),
              "a wrong password: each try was typed in a dialog", (p, a))
        check(len(notes) == 3 and "refused" not in notes[0]
              and all("refused" in r for r in notes[1:]),
              "the dialogs after a refusal say the password was refused", notes)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    def check_app(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("ssh password refused", ", ".join(failures))
    run_tests.checkApp = check_app
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
