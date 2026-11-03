#!/usr/bin/env python3
"""A user of ECCE 9.4 upgrades to this version (9.5 and later): what they keep.

    ECCE_TEST_OLD_HOME=<9.4 $ECCE_HOME>  ECCE_TEST_OLD_WRAPPERS=<9.4 wrapper dir> \
        tests/apps/upgrade_94_test.py [A] [B] [C]

Each scenario first runs the 9.4 install on a private state directory to make
the user's state (the services, the Organizer, `ecce-first-start --apply`
as 9.4's start window answered it), then runs THIS tree's install on the same
state, and checks what the user keeps (#767: local by default; a configured
install is unchanged).

  A  local data: the calculations are still in Local data, nothing asks the
     first-start question, the preference stays on, the user's own
     calculation is untouched and the example sets are not duplicated.
  B  the per-user data server of 9.4 (the "here" choice): still the server,
     the preference stays off, no local data folder appears, the server's
     calculations are not touched or duplicated.
  C  a server of the user's own (~/.ECCE/RemoteServer, as 9.4's start window
     wrote it): still a -remote session, no question.  State level only: no
     second machine; tests/containers/central_server_test.py has the real
     central server.

Exit 77 (skip) when ECCE_TEST_OLD_HOME is not set, or there is no Xvfb or
installed tree.  Never touches a real ~/.ECCE: every run has its own state
directory as HOME and ECCE_REALUSERHOME, removed at the end (ECCE_TEST_KEEP=1
keeps it).
"""
import getpass
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps        # noqa: E402
import isolate     # noqa: E402
import xdisplay    # noqa: E402

SKIP = 77
FIXTURE = os.path.join(HERE, "fixtures", "calc-water-opt")
META = ".\thttp://www.emsl.pnl.gov/ecce:%s\t\t%s\n"
OLD = {"home": os.environ.get("ECCE_TEST_OLD_HOME"),
       "wrappers": os.environ.get("ECCE_TEST_OLD_WRAPPERS")}
REALHOME = os.environ["HOME"]
NEW = {"home": apps.INSTALL, "wrappers": apps.WRAPPERS}


class Scenario:
    def __init__(self, name):
        self.name = name
        self.problems = []
        self.notes = []
        cache = os.path.join(REALHOME, ".cache")
        os.makedirs(cache, exist_ok=True)
        self.state = tempfile.mkdtemp(prefix="ecce-upg-%s-" % name, dir=cache)
        self.ports = (isolate.freePort(), isolate.freePort())
        self.display = None

    def check(self, ok, text):
        if not ok:
            self.problems.append(text)
        self.notes.append(("ok   " if ok else "FAIL ") + text)
        return ok

    # -- running one version on the state -----------------------------------
    def use(self, which):
        """Point the environment and the harness at one install."""
        for key in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
            os.environ.pop(key, None)
        os.environ["ECCE_DATASERVER_PORT"] = str(self.ports[0])
        os.environ["ECCE_BROKER_PORT"] = str(self.ports[1])
        for key in ("ECCE_LOCAL_DATA", "ECCE_LOCAL", "ECCE_REMOTE_SERVER",
                    "ECCE_REMOTE_DIR", "ECCE_SESSION_ID"):
            os.environ.pop(key, None)
        os.environ["HOME"] = REALHOME      # apply() refuses a state that is HOME
        isolate.apply(which["home"], self.state)
        os.environ["HOME"] = self.state
        os.environ["ECCE_NO_REAP"] = "1"
        os.environ["GTK_THEME"] = "Adwaita"
        apps.INSTALL = which["home"]
        apps.WRAPPERS = which["wrappers"]

    def tool(self, name, *args, **kw):
        env = dict(self.display.env())
        env.pop("ECCE_NO_FIRST_START", None)
        env.update(kw.get("env", {}))
        r = subprocess.run([os.path.join(apps.INSTALL, "bin", name)] + list(args),
                           env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=240)
        return r.returncode, r.stdout.decode(errors="replace").strip()

    def startServices(self, dataserver):
        if dataserver:
            apps.startServices(self.display)
        else:
            self.tool("ecce-gateway-start")

    def stopServices(self):
        apps.stopServices(self.display)
        waitFor(lambda: not any(isolate._portFree(p) is False for p in self.ports), 30)

    def organizer(self, importExamples, wait=None, settle=10, login=None):
        env = {"ECCE_NO_EXAMPLE_IMPORT": ""} if importExamples else {}
        if login:
            env["ECCE_TEST_AUTH_ANSWER"] = "login:%s:%s" % login
        r = apps.run(self.display, "organizer", windowTimeout=90, settle=settle,
                     env=env, inspect=wait)
        self.check(r.sawWindow and not r.crashed,
                   "the Organizer of %s starts (%s)" %
                   (os.path.basename(apps.INSTALL), r.note or "window shown"))
        return r

    # -- looking at the state ------------------------------------------------
    def prefs(self, name="Organizer"):
        try:
            return open(os.path.join(self.state, ".ECCE", name)).read()
        except OSError:
            return ""

    def exampleSet(self):
        m = re.search(r"EXAMPLESET:\s*(\d+)", self.prefs(), re.I)
        return m.group(1) if m else None

    def close(self):
        if self.display is None:
            pass
        isolate.killLeftovers(self.state)
        if os.environ.get("ECCE_TEST_KEEP"):
            print("kept " + self.state)
        else:
            shutil.rmtree(self.state, ignore_errors=True)


def tree(root, depth=3):
    found = []
    for base, dirs, files in os.walk(root):
        rel = os.path.relpath(base, root)
        level = 0 if rel == "." else rel.count(os.sep) + 1
        dirs[:] = sorted(d for d in dirs if not d.startswith("."))
        if level >= depth:
            dirs[:] = []
        if level and level <= depth:
            found.append(rel)
    return found


def projectMeta(folder, kind="ecceProject"):
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, ".ecce-meta"), "w") as f:
        f.write(META % ("contenttype", kind))


def addOwnCalculation(root):
    """The user's own calculation: a project holding a copy of a stored run."""
    projectMeta(os.path.join(root, "MyWork"))
    shutil.copytree(FIXTURE, os.path.join(root, "MyWork", "mywater"),
                    symlinks=True)


def waitFor(predicate, seconds):
    end = time.time() + seconds
    while time.time() < end:
        if predicate():
            return True
        time.sleep(2)
    return False


def localRoot(state):
    return os.path.join(state, ".ECCE-local", "users", "local")


def exampleNames(root):
    base = os.path.join(root, "Examples")
    out = []
    for dirpath, dirs, _ in os.walk(base):
        dirs[:] = [d for d in dirs if not d.startswith(".")]
        if os.path.exists(os.path.join(dirpath, ".ecce-meta")):
            out.append(os.path.relpath(dirpath, root))
    return sorted(out)


# -- scenario A: local data -----------------------------------------------------
def scenarioA(s):
    s.use(OLD)
    code, text = s.tool("ecce-first-start", "--apply", "local")
    s.check(code == 0, "9.4: ecce-first-start --apply local (%s)" % text[:80])
    root = localRoot(s.state)
    os.makedirs(root, exist_ok=True)
    addOwnCalculation(root)
    s.startServices(False)
    seen = {}

    def waitOld(_d):
        seen["old"] = waitFor(lambda: len(exampleNames(root)) >= 3, 150)
    s.organizer(True, wait=waitOld)
    s.stopServices()
    before = exampleNames(root)
    versionBefore = s.exampleSet()
    s.check(seen.get("old"), "9.4 imported its example set (%d calculations, set %s)"
            % (len(before), versionBefore))
    s.check(os.path.isdir(os.path.join(root, "MyWork", "mywater")),
            "9.4 keeps the user's own calculation")
    prefBefore = s.tool("ecce-localdata", "pref-state")[1]
    s.check(prefBefore.strip().startswith("on") or "on" in prefBefore,
            "9.4: the Local data preference is on (%r)" % prefBefore)
    answerBefore = open(os.path.join(s.state, ".ECCE", "first-start-answer")).read() \
        if os.path.exists(os.path.join(s.state, ".ECCE", "first-start-answer")) else ""

    s.use(NEW)
    #  "local" is the silent default (#767), "skip: ..." a configured start;
    #  only "ask" would be the old window.
    why = s.tool("ecce-first-start", "--check")[1]
    s.check(why in ("local",) or why.startswith("skip"),
            "9.5: the first-start question is not asked (%s)" % why)
    s.check(s.tool("ecce-localdata", "pref-state")[1] == prefBefore,
            "9.5: the Local data preference is as 9.4 left it")
    s.startServices(False)
    seen2 = {}

    def waitNew(_d):
        seen2["names"] = None
        waitFor(lambda: s.exampleSet() not in (None, versionBefore), 120)
        waitFor(lambda: len(exampleNames(root)) >= len(before), 120)
        time.sleep(30)
    s.organizer(True, wait=waitNew, settle=10)
    s.stopServices()
    after = exampleNames(root)
    s.notes.append("     example set %s -> %s; %d -> %d example folders"
                   % (versionBefore, s.exampleSet(), len(before), len(after)))
    s.check(s.exampleSet() is not None and s.exampleSet() != versionBefore,
            "9.5: the example set marker moved on (%s -> %s)" %
            (versionBefore, s.exampleSet()))
    s.check(os.path.isdir(os.path.join(root, "MyWork", "mywater")) and
            os.path.exists(os.path.join(root, "MyWork", "mywater", ".ecce-meta")),
            "9.5: the user's own calculation is untouched")
    tops = sorted(d for d in os.listdir(root) if not d.startswith("."))
    s.check(not [t for t in tops if t.startswith("Examples") and t != "Examples"],
            "9.5: no second copy of the examples at the top (%s)" % tops)
    lost = [n for n in before if n not in after and
            not n.startswith(("Examples/Getting_Started", "Examples/Organic"))]
    s.check(not lost, "9.5: nothing 9.4 imported is missing (%s)" % lost[:3])
    dup = [n for n in after if after.count(n) > 1]
    s.check(not dup, "9.5: no example appears twice")
    s.check(not os.path.exists(os.path.join(s.state, ".ECCE", "RemoteServer")),
            "9.5: no server was configured behind the user's back")
    answerAfter = open(os.path.join(s.state, ".ECCE", "first-start-answer")).read() \
        if os.path.exists(os.path.join(s.state, ".ECCE", "first-start-answer")) else ""
    s.check(answerAfter == answerBefore, "9.5: the recorded first-start answer is unchanged")
    s.check(not os.path.isdir(os.path.join(s.state, ".ECCE", "dataserver")),
            "9.5: local mode starts no per-user data server")


# -- scenario B: the per-user data server ----------------------------------------
def scenarioB(s):
    s.use(OLD)
    code, text = s.tool("ecce-first-start", "--apply", "server:localhost")
    s.check(code == 0, "9.4: ecce-first-start --apply server:localhost (%s)" % text[:80])
    s.startServices(True)
    docroot = os.path.join(s.state, ".ECCE", "dataserver", "htdocs", "Ecce", "users")
    login = (getpass.getuser(), "upgpass1")   # the folder is named for the login
    #  The gateway (9.4) and the Organizer (9.5) both ask for the login.
    os.environ["ECCE_TEST_AUTH_ANSWER"] = "login:%s:%s" % login
    s.check(os.path.isdir(docroot), "9.4: the per-user data server has its folder")
    s.check(not os.path.isdir(os.path.join(s.state, ".ECCE-local", "users")),
            "9.4: no local data folder is used")

    def found():
        return [p for p in tree(docroot, 8) if "Examples/" in p + "/"
                and p.count(os.sep) >= 3]
    seen = {}

    def waitOld(_d):
        seen["ok"] = waitFor(lambda: len(found()) >= 10, 200)
        #  9.4 marks the set once its import is through; stopping it earlier
        #  would test a half-imported tree, not an upgrade.
        waitFor(lambda: s.exampleSet() == "17", 200)
        time.sleep(20)
    s.organizer(True, wait=waitOld, login=login)
    before = sorted(tree(docroot, 8))
    s.check(s.exampleSet() == "17", "9.4 marked its example set 17 (%s)" % s.exampleSet())
    s.check(seen.get("ok"), "9.4 stored its examples on the data server")
    #  9.4 asks at every start (the recorded answer only preselects), so
    #  there is nothing to check here; 9.5 must not ask.
    prefBefore = s.tool("ecce-localdata", "pref-state")[1]
    s.stopServices()

    s.use(NEW)
    why = s.tool("ecce-first-start", "--check")[1]
    s.check(why.startswith("skip"), "9.5: a data-server user is left as they were (%s)" % why)
    s.check(s.tool("ecce-localdata", "pref-state")[1] == prefBefore,
            "9.5: the Local data preference is as 9.4 left it (%r)" % prefBefore)
    s.startServices(True)
    st = s.tool("ecce-dataserver-status")[1]
    s.check("not running" not in st, "9.5: the per-user data server is up (%s)" % st[:80])
    versionBefore = s.exampleSet()

    def waitNew(_d):
        waitFor(lambda: s.exampleSet() not in (None, versionBefore), 120)
        time.sleep(40)
    s.organizer(True, wait=waitNew, login=login)
    after = sorted(tree(docroot, 8))
    s.check(s.exampleSet() not in (None, versionBefore),
            "9.5: the example set marker moved on (%s -> %s)" % (versionBefore, s.exampleSet()))
    s.check(not os.path.isdir(os.path.join(s.state, ".ECCE-local", "users")),
            "9.5: still no local data folder")
    gone = [p for p in before if p not in after]
    s.check(not gone, "9.5: the server's calculations are all still there (%s)" % gone[:3])
    extra = [p for p in after if p not in before]
    s.notes.append("     server folders %d -> %d (new: %s)" % (len(before), len(after), extra[:3]))
    outside = [p for p in extra if not p.startswith("%s/Examples/" % getpass.getuser())]
    s.check(not outside, "9.5: nothing new outside the Examples project (%s)" % outside[:3])
    s.stopServices()


# -- scenario C: a user's own server (state level) -------------------------------
def scenarioC(s):
    s.use(OLD)
    mine = os.path.join(s.state, ".ECCE", "RemoteServer")
    os.makedirs(mine)
    with open(os.path.join(mine, "DataServers"), "w") as f:
        f.write("<DataServers><Server><Url>https://central.example.org:8443/Ecce"
                "</Url></Server></DataServers>\n")
    lib = "%s/bin/ecce-session-lib.sh"
    probe = ('. %s; ecce_user_server_mode; '
             'echo "REMOTE=${ECCE_REMOTE_SERVER:-} DIR=${ECCE_REMOTE_DIR:-}"')
    got = {}
    for name, which in (("9.4", OLD), ("9.5", NEW)):
        s.use(which)
        #  9.4 asks at every start, so only 9.5 has a "skip" to check.
        if which is NEW:
            s.check(s.tool("ecce-first-start", "--check")[1].startswith("skip"),
                    "9.5: --check skips with ~/.ECCE/RemoteServer")
        r = subprocess.run(["bash", "-c", probe % (lib % apps.INSTALL)],
                           env=dict(os.environ), stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=30)
        got[name] = r.stdout.decode().strip().splitlines()[-1]
    s.check(got["9.4"] == got["9.5"] and "REMOTE=1" in got["9.5"],
            "9.5 makes the same -remote session as 9.4 (%s | %s)" % (got["9.4"], got["9.5"]))
    # Preferences > Data lists it as an earlier start's server (read-only).
    code, text = s.tool("ecce-localdata", "servers", "list")
    s.notes.append("     9.5 servers list: %r" % text[:120])


def main():
    if not OLD["home"] or not OLD["wrappers"]:
        print("SKIP: ECCE_TEST_OLD_HOME / ECCE_TEST_OLD_WRAPPERS not set (the 9.4 install)")
        return SKIP
    for who in (OLD, NEW):
        if not os.path.exists(os.path.join(who["wrappers"], "ecce-organizer")):
            print("SKIP: no ecce-organizer under %s" % who["wrappers"])
            return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    wanted = [a.upper() for a in sys.argv[1:]] or ["A", "B", "C"]
    cases = {"A": scenarioA, "B": scenarioB, "C": scenarioC}
    failed = False
    for name in wanted:
        s = Scenario(name)
        try:
            with xdisplay.Display() as display:
                s.display = display
                try:
                    cases[name](s)
                finally:
                    try:
                        s.stopServices()
                    except Exception:
                        pass
        except Exception as exc:        # a harness failure is a failure
            s.check(False, "scenario %s raised %r" % (name, exc))
        finally:
            for line in s.notes:
                print("%s: %s" % (name, line))
            s.close()
        failed = failed or bool(s.problems)
    print("FAILED" if failed else "PASSED")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
