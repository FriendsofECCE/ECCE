#!/usr/bin/env python3
"""
Register Machines driven through its real window under Xvfb.

    gui_test.py --build <build dir> [--snapshots <dir>]

The window is driven by its test hook (ECCE_MACHREG_SCRIPT, see
src/apps/machregister/WxMachineRegisterScript.H): the commands call the same
handlers the buttons call, no synthetic X input.  Each scenario gets a private
ECCE_HOME (a copy of siteconfig/ plus a few fixtures) and a private home, runs
a script, and then the files are read back here, independently of the app.

--snapshots writes one PNG per tab of a fixture machine and exits.
SKIPs (77) without Xvfb or the machregister binary.
"""

import argparse
import hashlib
import os
import shutil
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

passed = failed = 0


def check(cond, what):
    global passed, failed
    if cond:
        passed += 1
        print("  ok    " + what)
    else:
        failed += 1
        print("  FAIL  " + what)


def write(path, text, mode=None):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)
    if mode is not None:
        os.chmod(path, mode)


def read(path):
    with open(path) as f:
        return f.read()


def digest(root):
    h = hashlib.sha256()
    for d, _, files in sorted(os.walk(root)):
        for n in sorted(files):
            p = os.path.join(d, n)
            h.update(p.encode())
            h.update(open(p, "rb").read())
    return h.hexdigest()


def registration(ue):
    """What registration writes under ~/.ECCE (the app's own startup files,
    EcceGlobal and MyAppColors, are not part of it)."""
    h = hashlib.sha256()
    for n in sorted(os.listdir(ue)):
        if n in ("MyMachines", "Queues") or n.startswith("CONFIG.") \
                or n.endswith(".Q"):
            h.update(n.encode() + read(os.path.join(ue, n)).encode())
    return h.hexdigest()


def keys(path):
    """key -> value of a one-line-per-key CONFIG file, lower-cased keys."""
    out = {}
    for line in read(path).splitlines():
        if line.strip() and not line.lstrip().startswith("#"):
            k, _, v = line.partition(":")
            out[k.strip().lower()] = v.strip()
    return out


def machines(path):
    """name -> fields of a Machines/MyMachines file."""
    out = {}
    if os.path.exists(path):
        for line in read(path).splitlines():
            if line.strip() and not line.startswith("#"):
                f = line.split("\t")
                out[f[0]] = f
    return out


SITE_CLUSTER = ("cluster\tcluster.example.org\tLinux\tx86_64\tZen4\t256:4\tssh\t"
                ":NWChem\tMN:RD:SD:UN:PW:Q\n")
MINE = ("mine\tmine.example.org\tLinux\tx86_64\tZen4\t4:1\tssh\t:NWChem\t"
        "MN:RD:SD:UN:PW\n")


class Env:
    """A private ECCE_HOME and home directory with the fixtures."""

    def __init__(self, tmp, name, remote=False):
        self.root = os.path.join(tmp, name)
        self.home = os.path.join(self.root, "home")
        self.user = os.path.join(self.root, "user")
        self.sc = os.path.join(self.home, "siteconfig")
        self.ue = os.path.join(self.user, ".ECCE")
        os.makedirs(self.ue)
        os.makedirs(os.path.join(self.home, "data"))
        for d in os.listdir(REPO):
            if d in ("scripts", "etc", "share"):
                os.symlink(os.path.join(REPO, d), os.path.join(self.home, d))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(self.home, "data", "client"))
        shutil.copytree(os.path.join(REPO, "siteconfig"), self.sc)
        write(os.path.join(self.sc, "Machines"),
              read(os.path.join(self.sc, "Machines")) + SITE_CLUSTER)
        write(os.path.join(self.sc, "Queues"),
              read(os.path.join(self.sc, "Queues")).replace(
                  "Queues: localhost dummy", "Queues: localhost dummy cluster")
              + "\ncluster|queueMgrName: Slurm\ncluster|prefFile: cluster.Q\n")
        write(os.path.join(self.sc, "cluster.Q"),
              "Queues: short long\n\n"
              "short|minProcessors: 1\nshort|maxProcessors: 64\n"
              "short|runLimit: 60\nshort|memLimit: 128000\n"
              "long|minProcessors: 1\nlong|maxProcessors: 256\n"
              "long|runLimit: 2880\nlong|memLimit: 512000\n")
        write(os.path.join(self.sc, "CONFIG.cluster"),
              "# the site's cluster\nNWChem: /site/nwchem\nperlPath: /site/perl\n"
              "shell: tcsh\n", mode=0o644)
        if remote:
            # a -remote client's published copy of the server's DataServers
            write(os.path.join(self.sc, "RemoteServer", "DataServers"),
                  read(os.path.join(self.sc, "DataServers")))
        write(os.path.join(self.ue, "MyMachines"), MINE)
        write(os.path.join(self.ue, "CONFIG.mine"),
              "nwchem: /opt/nwchem\n# written by hand\nfoo: bar\n")

    def env(self, display, extra=None):
        e = display.env()
        e.update(ECCE_HOME=self.home, ECCE_REALUSERHOME=self.user,
                 HOME=self.user, ECCE_MACHREG_SCRIPT=os.path.join(
                     self.root, "script"))
        e.pop("ECCE_REMOTE_SERVER", None)
        e.update(extra or {})
        return e


def run(display, build, env, script, args=(), extra=None, timeout=120):
    write(os.path.join(env.root, "script"), script)
    p = subprocess.run([os.path.join(build, "machregister")] + list(args),
                       env=env.env(display, extra), cwd=env.root,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, timeout=timeout)
    return p


def clean(p, what, done=True):
    """No FAIL lines from the hook, and (normally) the script ran to the end."""
    fails = [l for l in p.stdout.splitlines() if "FAIL" in l]
    check(not fails and (not done or "script done failures=0" in p.stdout),
          what + (": " + "; ".join(fails[:3]) if fails else
                  "" if "script done" in p.stdout or not done else
                  ": did not finish\n" + p.stdout[-800:]))


def user_mode(tmp, display, build):
    print("user mode, own machine")
    e = Env(tmp, "own")
    p = run(display, build, e, """
select mine
expect field name mine
expect field code:nwchem /opt/nwchem
expect dirty 0
expect save-enabled 0
expect delete-enabled 1
expect list mine yours
expect list cluster site
set code:orca /opt/orca
expect dirty 1
expect save-enabled 1
expect title-star 1
set code:orca ''
expect dirty 0
expect save-enabled 0
expect title-star 0
set code:orca /opt/orca
set perlpath '/p+a&t=h%x y'
set vendor Acme
save
expect dirty 0
expect save-enabled 0
expect title-star 0
expect field perlpath '/p+a&t=h%x y'
quit
""")
    clean(p, "own machine: edit, revert, save")
    check("binary=%s" % os.path.join(build, "machregister") in p.stdout,
          "the hook reports the binary under test")
    cfg = keys(os.path.join(e.ue, "CONFIG.mine"))
    check(cfg.get("orca") == "/opt/orca" and cfg.get("nwchem") == "/opt/nwchem"
          and cfg.get("perlpath") == "/p+a&t=h%x y" and cfg.get("foo") == "bar",
          "CONFIG.mine has the new keys, the old ones and the hand-written one: %r"
          % cfg)
    check("# written by hand" in read(os.path.join(e.ue, "CONFIG.mine")),
          "comments in CONFIG.mine are kept")
    m = machines(os.path.join(e.ue, "MyMachines")).get("mine", [])
    check(len(m) > 7 and m[2] == "Acme" and ":ORCA" in m[7] and ":NWChem" in m[7],
          "MyMachines line has the vendor and both codes: %r" % m)
    check(os.stat(os.path.join(e.ue, "CONFIG.mine")).st_mode & stat.S_IWUSR,
          "the user's CONFIG file stays writable")

    # Close with unsaved changes: cancel keeps the window, discard writes nothing
    before = digest(e.ue)
    p = run(display, build, e, """
select mine
set vendor Other
expect dirty 1
answer cancel
close
wait 300
expect dirty 1
expect field vendor Other
answer no
close
wait 2000
""")
    clean(p, "close with unsaved changes: cancel stays, discard leaves", False)
    check("Save changes to 'mine'" in p.stdout, "the prompt names the machine")
    check(digest(e.ue) == before, "discarding writes nothing")

    # switching machine asks too, and a queue form edit counts as unsaved
    p = run(display, build, e, """
select mine
set vendor Other
answer cancel
select cluster
expect field name mine
answer no
select cluster
expect field name cluster
expect dirty 0
set vendor Linux
set vendor Linux
quit
""")
    clean(p, "switching machine asks about unsaved changes")

    # a new machine, with queues; Q9
    e = Env(tmp, "new")
    p = run(display, build, e, """
select mine
new
expect dirty 0
set fullname newhost.example.org
expect field name newhost
expect save-enabled 1
set qmgr Slurm
tab queues
set q-name short
set q-maxprocs 64
set q-maxwall 0.5
expect field q-maxwall 0.5
set q-maxmem 8
expect label queue-apply 'Add Queue'
queue-apply
expect label queue-apply 'Update Queue'
set q-name long
set q-maxprocs 128
answer yes
save
expect dirty 0
expect list newhost yours
quit
""")
    clean(p, "new machine with two queues, the second applied at Save")
    q = read(os.path.join(e.ue, "newhost.Q"))
    check("short" in q and "long" in q and "maxProcessors:" in q,
          "newhost.Q lists both queues")
    check("short|runLimit:" in q and "30" in q.split("short|runLimit:")[1].split("\n")[0],
          "0.5 h of wall time is stored as 30 minutes")
    check("newhost|queueMgrName:" in read(os.path.join(e.ue, "Queues")),
          "the user's Queues file names the queue manager")
    check("newhost" in machines(os.path.join(e.ue, "MyMachines")),
          "newhost is in MyMachines")

    # Save with the queue form discarded
    e = Env(tmp, "q9")
    p = run(display, build, e, """
select cluster
tab queues
set q-maxprocs 99
expect dirty 1
set vendor Linux2
answer no
save
quit
""")
    clean(p, "an unapplied queue form is discarded on request")
    m = machines(os.path.join(e.ue, "MyMachines")).get("cluster", [])
    q = os.path.join(e.ue, "cluster.Q")
    check(len(m) > 2 and m[2] == "Linux2" and ("99" not in read(q)
          if os.path.exists(q) else True), "the discarded queue edit is not written")


def site_machine(tmp, display, build, remote=False):
    label = "-remote" if remote else "user mode"
    print(label + ", site machine")
    e = Env(tmp, "site-remote" if remote else "site", remote)
    extra = {"ECCE_REMOTE_SERVER": "server.example.org"} if remote else None
    site_before = digest(e.sc)
    p = run(display, build, e, """
select cluster
expect banner 1
expect delete-enabled 0
expect list cluster %s
expect field code:nwchem /site/nwchem
expect field perlpath /site/perl
expect field qmgr Slurm
expect save-enabled 0
set code:orca /user/orca
save
expect list cluster yours
expect banner 0
expect delete-enabled 1
expect field code:nwchem /site/nwchem
quit
""" % ("server" if remote else "site"), extra=extra)
    clean(p, "site machine: shown, saved as your own copy")
    cfg = keys(os.path.join(e.ue, "CONFIG.cluster"))
    check(cfg == {"orca": "/user/orca"},
          "the user's CONFIG.cluster holds only the change: %r" % cfg)
    m = machines(os.path.join(e.ue, "MyMachines")).get("cluster", [])
    check(len(m) > 7 and ":NWChem" in m[7] and ":ORCA" in m[7],
          "the Machines line lists the inherited and the new code: %r" % m)
    check(digest(e.sc) == site_before, "siteconfig is untouched")

    p = run(display, build, e, """
select cluster
set code:nwchem ''
save
expect field code:nwchem ''
select mine
select cluster
expect field code:nwchem ''
answer yes
delete
expect list cluster %s
quit
""" % ("server" if remote else "site"), extra=extra)
    clean(p, "an emptied inherited path is 'no value', delete brings the site back")
    check(digest(e.sc) == site_before, "siteconfig is still untouched")
    check(not os.path.exists(os.path.join(e.ue, "CONFIG.cluster")),
          "delete removed the user's CONFIG.cluster")
    check("cluster" not in machines(os.path.join(e.ue, "MyMachines")),
          "delete removed the MyMachines line")
    check("CONFIG.cluster" in p.stdout or "Delete the registration" in p.stdout,
          "the delete confirmation was shown")


def delete_prompt_lists_files(tmp, display, build):
    print("delete confirmation")
    e = Env(tmp, "del")
    write(os.path.join(e.ue, "mine.Q"), "Queues: a\na|maxProcessors: 4\n")
    p = run(display, build, e, """
select mine
answer cancel
delete
expect message CONFIG.mine
expect message mine.Q
expect message MyMachines
quit
""")
    clean(p, "the confirmation lists CONFIG, .Q and the Machines file")
    check(os.path.exists(os.path.join(e.ue, "CONFIG.mine")),
          "cancel deletes nothing")


def admin_mode(tmp, display, build):
    print("admin mode")
    e = Env(tmp, "admin")
    site_cfg = os.path.join(e.sc, "CONFIG.cluster")
    os.chmod(site_cfg, 0o444)
    user_before = registration(e.ue)
    p = run(display, build, e, """
select cluster
expect banner 0
expect list cluster site
expect list mine <absent>
expect delete-enabled 1
set code:orca /admin/orca
set perlpath /admin/perl
save
expect dirty 0
close
""", args=["-admin"])
    clean(p, "admin: edit a site machine, close without a quit prompt", False)
    cfg = keys(site_cfg)
    check(cfg.get("orca") == "/admin/orca" and cfg.get("perlpath") == "/admin/perl"
          and cfg.get("nwchem") == "/site/nwchem" and cfg.get("shell") == "tcsh",
          "siteconfig/CONFIG.cluster was updated: %r" % cfg)
    check("# the site's cluster" in read(site_cfg), "its comment is kept")
    mode = os.stat(site_cfg).st_mode
    check(not mode & stat.S_IWUSR, "a read-only site file is locked again")
    m = machines(os.path.join(e.sc, "Machines")).get("cluster", [])
    check(len(m) > 7 and ":ORCA" in m[7], "siteconfig/Machines lists ORCA: %r" % m)
    check(registration(e.ue) == user_before,
          "the user's registration files are untouched")
    check("Do you really want to quit" not in p.stdout and "prompt" not in
          p.stdout, "no quit prompt")


def snapshots(tmp, display, build, out):
    print("snapshots")
    os.makedirs(out, exist_ok=True)
    e = Env(tmp, "snap")
    write(os.path.join(e.ue, "CONFIG.cluster"), "ORCA: /user/orca\nnwchem: -\n")
    write(os.path.join(e.ue, "MyMachines"), MINE)
    p = run(display, build, e, """
select cluster
wait 500
snapshot %s
quit
""" % out)
    clean(p, "a PNG per tab")
    for n in sorted(os.listdir(out)):
        print("        " + os.path.join(out, n))
    check(len([n for n in os.listdir(out) if n.endswith(".png")]) >= 5,
          "five PNGs written")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    ap.add_argument("--snapshots")
    a = ap.parse_args()
    build = os.path.abspath(a.build)
    if not os.access(os.path.join(build, "machregister"), os.X_OK):
        print("SKIP  no machregister in " + build)
        return 77
    try:
        from xdisplay import Display, DisplayUnavailable
        disp = Display()
        disp.__enter__()
    except Exception as exc:   # no Xvfb
        print("SKIP  " + str(exc))
        return 77
    tmp = tempfile.mkdtemp(prefix="ecce-machreg-")
    try:
        if a.snapshots:
            snapshots(tmp, disp, build, os.path.abspath(a.snapshots))
        else:
            user_mode(tmp, disp, build)
            site_machine(tmp, disp, build)
            site_machine(tmp, disp, build, remote=True)
            delete_prompt_lists_files(tmp, disp, build)
            admin_mode(tmp, disp, build)
    finally:
        disp.__exit__(None, None, None)
        shutil.rmtree(tmp, ignore_errors=True)
    print("%d passed, %d failed" % (passed, failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
