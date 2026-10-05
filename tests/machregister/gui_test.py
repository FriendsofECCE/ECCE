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
              "shell: tcsh\nsourceFile: /site/modules.sh\n"
              "qmgrPath: /site/slurm/bin\nnoRemoteAccess: yes\n"
              "checkScratch: no\nfrontendBypass: .site.org\n", mode=0o644)
        # only gensub reads a vendor file; the C++ view must not show it
        write(os.path.join(self.sc, "CONFIG.Linux"), "libPath: /vendor/lib\n")
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


CPP_KEYS = ["shell", "sourcefile", "frontendmachine", "frontendbypass",
            "perlpath", "qmgrpath", "libpath", "xappspath", "noremoteaccess",
            "usersubmit", "singleconnect", "checkscratch"]
DEFAULTS = {"shell": "bash", "noremoteaccess": "false", "usersubmit": "false",
            "singleconnect": "no", "checkscratch": "true"}
BOOLS = {"noremoteaccess": "t", "usersubmit": "t", "checkscratch": "f"}


def explain(e, name, admin=False):
    """GENSUB_EXPLAIN rows for the machine, key -> row."""
    import json
    tmp = os.path.join(e.root, "explain")
    os.makedirs(tmp, exist_ok=True)
    write(os.path.join(tmp, "params"),
          " -H %s\n -Q Shell\n -c NWChem\n -d localhost\n -n 1\n -N 1\n"
          " -r %s\n -i a\n -o a\n -f %s/submit__x\n" % (name, tmp, tmp))
    user = os.path.join(e.root, "nouser") if admin else e.user
    os.makedirs(user, exist_ok=True)
    env = dict(os.environ, ECCE_HOME=e.home, ECCE_REALUSERHOME=user,
               GENSUB_EXPLAIN="1")
    p = subprocess.run(["perl", os.path.join(REPO, "scripts", "gensub"), "-p",
                        os.path.join(tmp, "params")], env=env, cwd=tmp,
                       stdout=subprocess.PIPE, text=True)
    check(p.returncode == 0, "gensub explain ran")
    return {r["key"]: r for r in map(json.loads, p.stdout.splitlines())}


def cpp_view(e, build, name, admin=False):
    user = os.path.join(e.root, "nouser") if admin else e.user
    env = dict(os.environ, ECCE_HOME=e.home, ECCE_REALUSERHOME=user)
    p = subprocess.run([os.path.join(build, "configdump"), name], env=env,
                       stdout=subprocess.PIPE, text=True)
    out = {}
    for line in p.stdout.splitlines():
        k, _, v = line.partition(": ")
        out[k.lower()] = v
    return out


def oracle(rows, edited, admin, remote):
    """key -> (effective value or None, tag) as C++ sees the key: layers only
    gensub reads (submit.site, vendor) do not count."""
    out = {}
    for k in CPP_KEYS:
        r = rows.get(k)
        layers = []
        if r:
            for o in r["overridden"] + [r]:
                if o["source"] in ("submit.site", "vendor"):
                    continue
                layers.append((o["file"], o["value"]))
        if not layers:
            out[k] = (None, "default")
            continue
        f, v = layers[-1]
        if v is None:
            out[k] = (None, "no value" if f == edited else
                      "server" if remote else "site")
        elif f == edited:
            out[k] = (v, "site (editing)" if admin else "yours")
        else:
            out[k] = (v, "server" if remote else "site")
    return out


PLAIN = {"default": "not set", "site": "from site", "server": "from server",
         "yours": "your value", "no value": "your value",
         "site (editing)": "site value"}


def jobs_tag(want):
    """The group's tag: the user's own value first, then a layer's."""
    a, b = want["noremoteaccess"][1], want["usersubmit"][1]
    own = ("yours", "no value", "site (editing)")
    for t in (a, b):
        if t in own:
            return PLAIN[t]
    return PLAIN[a if a != "default" else b]


def jobs_radio(want):
    nr = shown("noremoteaccess", want["noremoteaccess"][0]) == "1"
    us = shown("usersubmit", want["usersubmit"][0]) == "1"
    return "none" if nr else "user" if us else "copy"


def shown(k, v):
    """What the control shows for an effective value (None: the default)."""
    if v is None:
        v = DEFAULTS.get(k, "")
    low = v.lower()
    if k in BOOLS:
        if BOOLS[k] == "t":
            return "1" if low in ("true", "yes") else "0"
        return "0" if low in ("false", "no") else "1"
    if k == "singleconnect":
        return "yes" if low in ("true", "yes") else \
               "no" if low in ("false", "no") else "auto"
    return v


def connection_tab(tmp, display, build, mode):
    admin, remote = mode == "admin", mode == "remote"
    print("connection tab, " + mode)
    e = Env(tmp, "conn-" + mode, remote)
    extra = {"ECCE_REMOTE_SERVER": "server.example.org"} if remote else None
    args = ["-admin"] if admin else []
    write(os.path.join(e.ue, "CONFIG.cluster"), "foo: bar\n")
    site = "site value" if admin else "from server" if remote else "from site"
    Y = "site value" if admin else "your value"
    edited = os.path.join(e.sc if admin else e.ue, "CONFIG.cluster")
    if admin:
        os.chmod(edited, 0o644)
    site_before = digest(e.sc)
    user_before = registration(e.ue)

    pre = """
select cluster
tab connection
expect field shell tcsh
expect label tag:shell '%(site)s'
expect field sourcefile /site/modules.sh
expect label tag:sourcefile '%(site)s'
expect field frontendbypass .site.org
expect field libpath ''
expect label tag:libpath 'not set'
expect field jobs:none 1
expect field jobs:user 0
expect enabled jobs:user 0
expect label jobs:mode "Files only, not submitted"
expect shown jobs:icon 1
expect advanced 1
expect label tag:jobs '%(site)s'
expect field checkscratch 0
expect label tag:checkscratch '%(site)s'
expect field singleconnect no
expect label tag:singleconnect 'not set'
expect shown xappspath 0
expect dirty 0
expect save-enabled 0
""" % {"site": site, "Y": Y}
    if admin:
        edits = """
set shell sh
set sourcefile ''
set frontendmachine login.example.org
set perlpath /admin/perl
set qmgrpath /admin/slurm
set libpath /admin/lib
set jobs:none 0
set jobs:user 1
set singleconnect auto
set checkscratch 1
set xappspath /admin/x
"""
    else:
        edits = """
set shell bash
expect label tag:shell '%(Y)s'
set sourcefile ''
expect label tag:sourcefile '%(Y)s'
expect field sourcefile ''
set frontendmachine login.example.org
set perlpath /my/perl
expect label tag:perlpath '%(Y)s'
undo perlpath
expect field perlpath /site/perl
expect label tag:perlpath '%(site)s'
set qmgrpath ''
expect label tag:qmgrpath '%(Y)s'
set libpath /my/lib
set jobs:none 0
expect label tag:jobs '%(Y)s'
expect label jobs:mode 'ECCE submits the job'
expect shown jobs:icon 0
set jobs:user 1
expect label jobs:mode 'Interactive submission'
expect shown jobs:icon 1
set singleconnect auto
set checkscratch 1
set xappspath /my/x
expect label tag:xappspath '%(Y)s'
""" % {"site": site, "Y": Y}
    p = run(display, build, e, pre + edits + """
expect dirty 1
expect save-enabled 1
save
expect dirty 0
quit
""", args=args, extra=extra)
    clean(p, "%s: set, clear and save each Connection field" % mode)

    cfg = keys(edited)
    if not admin:
        check(cfg.get("foo") == "bar", "the hand-written key is kept")
        check(cfg.get("shell") == "bash" and cfg.get("sourcefile") == "-" and
              cfg.get("qmgrpath") == "-" and cfg.get("noremoteaccess") == "false"
              and cfg.get("perlpath") is None and
              cfg.get("libpath") == "/my/lib" and
              cfg.get("frontendmachine") == "login.example.org" and
              cfg.get("usersubmit") == "true" and
              cfg.get("singleconnect") == "auto" and
              cfg.get("checkscratch") == "true" and
              cfg.get("xappspath") == "/my/x",
              "user CONFIG.cluster: %r" % cfg)
        check(digest(e.sc) == site_before, "siteconfig is untouched")
    else:
        check(cfg.get("shell") == "sh" and cfg.get("nwchem") == "/site/nwchem"
              and cfg.get("sourcefile") is None and
              cfg.get("noremoteaccess") is None and
              cfg.get("libpath") == "/admin/lib" and
              cfg.get("usersubmit") == "true" and
              cfg.get("singleconnect") == "auto" and
              cfg.get("checkscratch") is None,   # "yes" is the default
              "site CONFIG.cluster: %r" % cfg)
        check(registration(e.ue) == user_before,
              "the user's files are untouched")

    # gensub (explain), the C++ merged view and the window must agree
    rows = explain(e, "cluster", admin)
    want = oracle(rows, edited, admin, remote)
    cpp = cpp_view(e, build, "cluster", admin)
    bad = [(k, want[k][0], cpp.get(k)) for k in CPP_KEYS
           if (want[k][0] or "") != cpp.get(k, "")]
    check(not bad, "%s: gensub explain == configdump for the Connection "
          "keys %s" % (mode, bad))
    lines = ["select cluster", "tab connection"]
    for k in CPP_KEYS:
        v, tag = want[k]
        lines.append("expect field %s '%s'" % (k, shown(k, v)))
        if k not in ("noremoteaccess", "usersubmit"):
            lines.append("expect label tag:%s '%s'" % (k, PLAIN[tag]))
    lines.append("expect field jobs:user %d" % (jobs_radio(want) == "user"))
    lines.append("expect field jobs:none %d" % (jobs_radio(want) == "none"))
    lines.append("expect label tag:jobs '%s'" % jobs_tag(want))
    lines += ["expect shown xappspath 1", "expect dirty 0", "quit"]
    p = run(display, build, e, "\n".join(lines) + "\n", args=args, extra=extra)
    clean(p, "%s: the window shows what gensub and configdump report, "
          "with their tags" % mode)

    if admin:
        return
    p = run(display, build, e, """
select cluster
tab connection
undo shell
undo sourcefile
set frontendbypass ''
set jobs:user 0
set jobs:none 1
expect label tag:shell '%(site)s'
expect label tag:sourcefile '%(site)s'
expect label tag:frontendbypass '%(Y)s'
save
quit
""" % {"site": site, "Y": Y}, args=args, extra=extra)
    clean(p, "%s: undo, emptied field and the job radios, saved" % mode)
    rows = explain(e, "cluster", admin)
    want = oracle(rows, edited, admin, remote)
    cfg = keys(edited)
    check("shell" not in cfg and "sourcefile" not in cfg and
          cfg.get("frontendbypass") == "-" and "noremoteaccess" not in cfg
          and "usersubmit" not in cfg and jobs_radio(want) == "none" and want["shell"] == ("tcsh", "server" if remote else "site") and
          want["frontendbypass"] == (None, "no value"),
          "%s: those three keys read back as site, site, no value: %r"
          % (mode, cfg))


def stage3_pngs(tmp, display, build, out):
    print("connection tab PNGs")
    os.makedirs(out, exist_ok=True)
    e = Env(tmp, "pngs")
    p = run(display, build, e, """
select cluster
tab connection
wait 800
shot %(o)s/connection-site.png
set perlpath /my/perl
wait 500
shot %(o)s/connection-yours.png
set jobs:none 0
set jobs:user 1
wait 500
shot %(o)s/connection-interactive.png
quit
""" % {"o": out})
    clean(p, "Connection PNGs")
    for n in sorted(os.listdir(out)):
        print("        " + os.path.join(out, n))


def cfg_blocks(path):
    """key -> text of a CONFIG file, lower-cased keys; blocks keep their
    inner lines, 'key {' ... '}' and one-line 'key { v }' alike."""
    out, cur, body = {}, None, []
    for line in (read(path) if os.path.exists(path) else "").splitlines():
        if cur is not None:
            if line.startswith("}"):
                out[cur] = "\n".join(body).strip()
                cur = None
            else:
                body.append(line)
            continue
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if "{" in line and ":" not in line.split("{")[0]:
            k, _, rest = line.partition("{")
            if rest.strip().endswith("}"):
                out[k.strip().lower()] = rest.strip()[:-1].strip()
            else:
                cur, body = k.strip().lower(), [rest] if rest.strip() else []
        else:
            k, _, v = line.partition(":")
            out[k.strip().lower()] = v.strip()
    return out


def esc(text):
    """One word for the hook: single-quoted, a quote written as '"'"'."""
    return "'" + text.replace("'", "'\"'\"'").replace("\n", "\\n") + "'"


def job_script(e, user, code="NWChem"):
    """The job script gensub makes for cluster on Slurm."""
    out = os.path.join(e.root, "gen")
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    write(os.path.join(out, "params"),
          " -H cluster\n -Q Slurm\n -q short\n -c %s\n -d localhost\n"
          " -n 4\n -N 1\n -T 1:00:00\n -m 8\n -r %s\n -i a.nw\n -o a.out\n"
          " -f %s/submit__x\n" % (code, out, out))
    env = dict(os.environ, ECCE_HOME=e.home, ECCE_REALUSERHOME=user)
    p = subprocess.run(["perl", os.path.join(REPO, "scripts", "gensub"), "-p",
                        os.path.join(out, "params")], env=env, cwd=out,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True)
    made = os.path.exists(os.path.join(out, "submit__x"))
    check(p.returncode == 0 and made, "gensub made a Slurm job script" +
          ("" if p.returncode == 0 else ": " + p.stdout[-300:]))
    return read(os.path.join(out, "submit__x")) if made else ""


def job_script_tab(tmp, display, build, mode):
    admin, remote = mode == "admin", mode == "remote"
    print("job script tab, " + mode)
    e = Env(tmp, "job-" + mode, remote)
    extra = {"ECCE_REMOTE_SERVER": "server.example.org"} if remote else None
    args = ["-admin"] if admin else []
    site_setup = "module load site"
    write(os.path.join(e.sc, "CONFIG.cluster"),
          read(os.path.join(e.sc, "CONFIG.cluster")) +
          "setup {\n  %s\n}\n" % site_setup, mode=0o644)
    edited = os.path.join(e.sc if admin else e.ue, "CONFIG.cluster")
    userhome = os.path.join(e.root, "nouser") if admin else e.user
    os.makedirs(userhome, exist_ok=True)
    site_before = digest(e.sc)
    user_before = registration(e.ue)

    rows = explain(e, "cluster", admin)
    header = rows["slurm"]["value"].strip()
    check(rows["slurm"]["source"] == "submit.site" and "#SBATCH" in header,
          "the Slurm header comes from submit.site")
    src = "the ECCE server" if remote else "the site's default"
    setup_src = "the ECCE server" if remote else "the site"
    site = "from server" if remote else "from site"
    yours = "site value" if admin else "your value"
    hdr_label = "From %s: submit.site (read-only)" % src

    if admin:
        setup_pre = """expect field blk:setup '%s'
expect label tag:setup 'site value'
expect shown blk:setup:site 0
expect shown undo:setup 0
set blk:setup ''
expect field blk:setup ''
expect label tag:setup 'not set'
expect shown blk:setup:none 0
set blk:setup %s
""" % (site_setup, esc(site_setup + "\nmodule load mine"))
    else:
        setup_pre = """expect field blk:setup ''
expect field blk:setup:site '%s'
expect label tag:setup '%s'
expect label blk:setup:label 'From %s: CONFIG.cluster (read-only)'
click blk:setup:copy
expect field blk:setup '%s'
expect label tag:setup 'your value'
set blk:setup %s
""" % (site_setup, site, setup_src, site_setup,
       esc(site_setup + "\nmodule load mine"))
    p = run(display, build, e, """
select cluster
tab job
expect label tag:header '%(site)s'
expect field blk:header:site %(hdr)s
expect field blk:header ''
expect label blk:header:label %(hdrlabel)s
expect shown blk:header:site 1
expect enabled blk:header:copy 1
expect label tag:wrapup 'not set'
expect shown blk:wrapup:site 0
expect shown blk:wrapup:none 0
expect shown condorallowtmp 0
expect shown undo:header 0
expect dirty 0
expect save-enabled 0
click blk:header:copy
expect field blk:header %(hdr)s
expect label tag:header '%(yours)s'
expect shown undo:header 1
expect dirty 1
set blk:header %(hdr2)s
%(setup)sset blk:wrapup 'echo done'
expect label tag:wrapup '%(yours)s'
wait 900
expect dirty 1
save
expect dirty 0
expect field blk:header %(hdr2)s
expect field blk:wrapup 'echo done'
expect label blk:setup:csh ''
quit
""" % dict(site=site, hdr=esc(header), hdr2=esc(header + "\n#SBATCH --qos=normal"),
           hdrlabel=esc(hdr_label), yours=yours, setup=setup_pre),
        args=args, extra=extra)
    clean(p, "%s: copy the site text, edit three blocks, save" % mode)
    c = cfg_blocks(edited)
    check(c.get("slurm") == header + "\n#SBATCH --qos=normal" and
          c.get("setup") == site_setup + "\nmodule load mine" and
          c.get("wrapup") == "echo done" and
          (c.get("nwchem") == "/site/nwchem") == admin,
          "the file holds the three blocks and the old keys: %r" % c)
    if admin:
        check("# the site's cluster" in read(edited), "the file's comment is kept")
        check(registration(e.ue) == user_before, "the user's files are untouched")
    else:
        check(digest(e.sc) == site_before, "siteconfig is untouched")
    rows = explain(e, "cluster", admin)
    check(rows["slurm"]["value"].strip() == c["slurm"] and
          rows["slurm"]["source"] == ("site" if admin else "user") and
          rows["setup"]["value"].strip() == c["setup"],
          "gensub explain reports the edited blocks")
    script = job_script(e, userhome)
    check("#SBATCH --qos=normal" in script and
          "#SBATCH --partition=short" in script and
          script.count("--partition") == 1,
          "the job script has the replaced header, once")
    check("module load site\nmodule load mine" in script and "echo done" in script,
          "the job script has the setup and wrap-up text")

    # "no value": header and setup cleared, wrap-up changed
    clear_setup = "" if admin else "expect enabled blk:setup 0\n"
    p = run(display, build, e, """
select cluster
tab job
click blk:header:none
expect enabled blk:header 0
expect label tag:header 'your value'
click blk:setup:none
%(clear)sset blk:wrapup 'echo done2'
save
expect dirty 0
expect shown undo:header 1
quit
""" % dict(clear=clear_setup), args=args, extra=extra)
    clean(p, "%s: no text for the header and setup" % mode)
    c = cfg_blocks(edited)
    if admin:
        check(c.get("setup") == "-" or "setup" not in c, "admin setup: %r" % c)
        check(c.get("slurm") == "-" and c.get("wrapup") == "echo done2",
              "admin: header cleared, wrap-up changed: %r" % c)
    else:
        check(c.get("slurm") == "-" and c.get("setup") == "-" and
              c.get("wrapup") == "echo done2",
              "'-' is written for the cleared blocks: %r" % c)
    script = job_script(e, userhome)
    check("#SBATCH" not in script, "no scheduler header in the script")
    check("module load" not in script and "echo done2" in script,
          "no setup, new wrap-up")
    check(not [l for l in script.splitlines() if l.strip() == "-"],
          "no sentinel line in the script")

    # back to the site: undo the header; replace the setup (not add to it)
    setup_cmds = ("set blk:setup 'module load only'\n" if admin else
                  "undo setup\nclick blk:setup:copy\n"
                  "set blk:setup 'module load only'\n")
    p = run(display, build, e, """
select cluster
tab job
expect field blk:header ''
expect label tag:header 'your value'
undo header
expect label tag:header '%(site)s'
expect shown undo:header 1
expect dirty 1
%(setup)sset blk:wrapup ''
save
expect dirty 0
quit
""" % dict(site=site, setup=setup_cmds), args=args, extra=extra)
    clean(p, "%s: undo the header, replace the setup, empty the wrap-up" % mode)
    c = cfg_blocks(edited)
    check("slurm" not in c and c.get("setup") == "module load only" and
          "wrapup" not in c, "the file: header and wrap-up lines gone: %r" % c)
    script = job_script(e, userhome)
    check("#SBATCH --partition=short" in script and
          "module load only" in script and "module load site" not in script,
          "default header is back; the setup replaces the site's")
    return e, edited, userhome, args, extra


def job_script_advanced(tmp, display, build, mode, ctx):
    print("job script tab, advanced edit file, " + mode)
    e, edited, userhome, args, extra = ctx
    good = "# raw\nnwchem: /raw/nwchem\nsetup {\n  module load raw\n}\nfoo: bar\n"
    p = run(display, build, e, """
select cluster
tab job
set blk:wrapup 'echo unsaved'
answer cancel
click edit-file
expect raw-dialog 0
expect dirty 1
answer no
click edit-file
expect raw-dialog 1
expect dirty 0
expect field blk:wrapup ''
expect contains raw:text 'module load only'
set raw:text 'setup {\\nmodule load x'
click raw:check
expect contains raw:report 'Error'
click raw:save
expect raw-dialog 1
expect message 'not closed'
set raw:text 'setup {\\n  setenv A 1\\n  foreach i (a b)\\n  end\\n}\\nbogus: 1\\n'
click raw:check
expect contains raw:report 'Line 2 is csh'
expect contains raw:report 'bogus'
shot-dialog %(shot)s
click raw:cancel
expect raw-dialog 0
click edit-file
set raw:text %(good)s
click raw:save
expect raw-dialog 0
expect dirty 0
expect field blk:setup 'module load raw'
expect field code:nwchem /raw/nwchem
quit
""" % dict(good=esc(good), shot=os.path.join(e.root, "dialog.png")),
        args=args, extra=extra)
    clean(p, "%s: edit the file as text, checks, cancel, save" % mode)
    check(read(edited) == good, "the file is the text that was saved: %r"
          % read(edited))
    check(os.path.exists(os.path.join(e.root, "dialog.png")) and
          os.path.getsize(os.path.join(e.root, "dialog.png")) > 1000,
          "the dialog was captured")
    script = job_script(e, userhome)
    check("module load raw" in script, "the job script follows the file")
    p = run(display, build, e, """
select cluster
tab job
set blk:wrapup 'echo form'
answer yes
click edit-file
expect raw-dialog 1
expect contains raw:text 'echo form'
click raw:cancel
quit
""", args=args, extra=extra)
    clean(p, "%s: unsaved form changes are saved before the file opens" % mode)
    check("echo form" in read(edited), "they were saved")


def job_script_pngs(tmp, display, build, out):
    print("job script tab PNGs")
    os.makedirs(out, exist_ok=True)
    e = Env(tmp, "job-pngs")
    write(os.path.join(e.sc, "CONFIG.cluster"),
          read(os.path.join(e.sc, "CONFIG.cluster")) +
          "setup {\n  module load site\n}\n", mode=0o644)
    hdr = explain(e, "cluster")["slurm"]["value"].strip()
    p = run(display, build, e, """
select cluster
tab job
wait 1000
shot %(o)s/job-script-site.png
click blk:header:copy
set blk:header %(hdr)s
wait 1000
shot %(o)s/job-script-copied.png
words
wait 500
shot-dialog %(o)s/job-script-words.png
click words:close
quit
""" % {"o": out, "hdr": esc("#SBATCH --qos=normal\n" + hdr)})
    clean(p, "job script PNGs")
    write(os.path.join(e.ue, "CONFIG.cluster"),
          "# my settings\nperlPath: /my/perl\nsetup {\n  module load mine\n"
          "  setenv A 1\n}\nbogus: 1\n")
    p = run(display, build, e, """
select cluster
tab job
click edit-file
click raw:check
wait 1000
shot-dialog %(o)s/job-script-edit-file.png
click raw:cancel
quit
""" % {"o": out})
    clean(p, "edit-file PNG")
    for n in sorted(os.listdir(out)):
        print("        " + os.path.join(out, n))


ENV_TEXT = ("g16root /opt\nGAUSS_SCRDIR /scratch\nMYTOOLPATH /my/tools")


def codes_tab(tmp, display, build, mode):
    admin, remote = mode == "admin", mode == "remote"
    print("codes tab, " + mode)
    e = Env(tmp, "codes-" + mode, remote)
    extra = {"ECCE_REMOTE_SERVER": "server.example.org"} if remote else None
    args = ["-admin"] if admin else []
    write(os.path.join(e.sc, "CONFIG.cluster"),
          read(os.path.join(e.sc, "CONFIG.cluster")) +
          "Gaussian-16: /site/g16\nGaussian-16Environment {\n"
          "  g16root /sitedir\n}\n", mode=0o644)
    edited = os.path.join(e.sc if admin else e.ue, "CONFIG.cluster")
    userhome = os.path.join(e.root, "nouser") if admin else e.user
    os.makedirs(userhome, exist_ok=True)
    site_before = digest(e.sc)
    user_before = registration(e.ue)
    site = "from server" if remote else "from site"
    yours = "site value" if admin else "your value"
    cmd = "echo gaussian on $inFile"

    if admin:
        first = """expect field blk:cenv 'g16root /sitedir'
expect shown blk:cenv:site 0
"""
    else:
        first = """expect field blk:cenv ''
expect field blk:cenv:site 'g16root /sitedir'
expect enabled blk:cenv:copy 1
"""
    p = run(display, build, e, """
select cluster
tab codes
expect code-listed Gaussian-16 1
expect code-listed ORCA 1
expect code-listed Gaussian-03 0
expect code-listed Gaussian-98 0
expect code-listed GAMESS-UK 0
expect code-listed Amica 0
code Gaussian-16
expect label code:title Gaussian-16
expect field code:gaussian-16 /site/g16
expect label tag:code '%(csite)s'
expect label tag:cenv '%(csite)s'
expect shown undo:code 0
expect shown undo:cenv 0
%(first)sexpect label tag:ccmd 'not set'
expect label tag:files '%(fsite)s'
expect shown blk:ccmd:site 0
expect label cmd:help "When this is empty, ECCE's built-in command is used."
code ORCA
expect label code:title ORCA
expect field code:orca ''
code Gaussian-16
set blk:cenv %(env)s
set blk:ccmd %(cmd)s
expect label tag:cenv '%(yours)s'
expect shown undo:cenv 1
set code:gaussian-16 /opt/g16/g16
set code:files '*.rwf'
set code:prelim '*.old'
set blk:csetup 'echo before g16'
expect label tag:code '%(yours)s'
expect label tag:files '%(yours)s'
code ORCA
expect field blk:cenv ''
expect field code:files ''
code Gaussian-16
expect field blk:cenv %(env)s
expect field code:files '*.rwf'
expect dirty 1
save
expect dirty 0
quit
""" % dict(site=site, yours=yours, first=first, env=esc(ENV_TEXT), cmd=esc(cmd),
           fsite="from server" if remote else "from site",
           csite=yours if admin else site),
        args=args, extra=extra)
    clean(p, "%s: Gaussian-16 program, environment, command, setup and file "
          "lists; switching code keeps each code's own" % mode)
    c = cfg_blocks(edited)
    check(c.get("gaussian-16environment") == ENV_TEXT and
          c.get("gaussian-16command") == cmd and
          c.get("gaussian-16") == "/opt/g16/g16" and
          c.get("gaussian-16_setup") == "echo before g16" and
          c.get("gaussian-16filestoremove") == "*.rwf" and
          c.get("gaussian-16prelimfilestoremove") == "*.old",
          "the file has the Gaussian-16 keys: %r" % c)
    if admin:
        check(registration(e.ue) == user_before, "the user's files are untouched")
    else:
        check(digest(e.sc) == site_before, "siteconfig is untouched")
    rows = explain(e, "cluster", admin)
    src = "site" if admin else "user"
    check(rows["gaussian-16environment"]["value"].strip() == ENV_TEXT and
          rows["gaussian-16environment"]["source"] == src and
          rows["gaussian-16command"]["value"] == cmd and
          rows["gaussian-16filestoremove"]["value"] == "*.rwf",
          "gensub explain reports the environment and the command")
    script = job_script(e, userhome, "Gaussian-16")
    check('export g16root="/opt"' in script and
          'export GAUSS_SCRDIR="/scratch"' in script and
          'if [ -n "${MYTOOLPATH+set}" ]; then' in script and
          'export MYTOOLPATH="${MYTOOLPATH}:/my/tools"' in script,
          "the Gaussian-16 job script exports the variables, appending the "
          "PATH-like one")
    check("echo gaussian on a.nw" in script and "echo before g16" in script
          and "*.rwf" in script and "*.old" in script,
          "the job script has the command (placeholder replaced), setup and "
          "file lists")

    # undo: an unsaved change goes back to the saved value; a saved own value
    # goes back to the site's, only when there is one
    if admin:
        tail = "expect shown undo:cenv 0\n"
    else:
        tail = """undo cenv
expect field blk:cenv ''
expect label tag:cenv '%s'
expect dirty 1
""" % site
    p = run(display, build, e, """
select cluster
tab codes
code Gaussian-16
set blk:cenv 'g16root /other'
expect shown undo:cenv 1
undo cenv
expect field blk:cenv %(env)s
expect dirty 0
set blk:ccmd ''
expect label tag:ccmd 'not set'
expect shown undo:ccmd 1
undo ccmd
expect field blk:ccmd %(cmd)s
%(tail)ssave
expect dirty 0
quit
""" % dict(env=esc(ENV_TEXT), cmd=esc(cmd), tail=tail), args=args, extra=extra)
    clean(p, "%s: undo goes to the saved value, then to the site's" % mode)
    c = cfg_blocks(edited)
    if admin:
        check(c.get("gaussian-16environment") == ENV_TEXT,
              "admin: the saved environment is unchanged")
    else:
        check("gaussian-16environment" not in c and
              c.get("gaussian-16command") == cmd,
              "user: the environment line is gone, the command stays: %r" % c)
        script = job_script(e, userhome, "Gaussian-16")
        check('export g16root="/sitedir"' in script and
              'GAUSS_SCRDIR="/scratch"' not in script,
              "the job script uses the site's environment again")

        # "Use no text" replaces the site's environment with nothing
        p = run(display, build, e, """
select cluster
tab codes
code Gaussian-16
click blk:cenv:none
expect enabled blk:cenv 0
expect label tag:cenv 'your value'
set blk:ccmd ''
save
expect dirty 0
quit
""", args=args, extra=extra)
        clean(p, "%s: no environment, empty command" % mode)
        c = cfg_blocks(edited)
        check(c.get("gaussian-16environment") == "-" and
              "gaussian-16command" not in c,
              "'-' for the environment, the command line removed: %r" % c)
        script = job_script(e, userhome, "Gaussian-16")
        check("export g16root" not in script and
              "echo gaussian on" not in script,
              "no variables and the built-in command in the job script")

    # placeholders go at the cursor of the box last typed in
    p = run(display, build, e, """
select cluster
tab codes
code Gaussian-16
set blk:ccmd 'ab'
focus blk:ccmd
cursor blk:ccmd 1
words
wait 300
expect enabled words:insert 1
expect contains words:where 'Command line'
words-pick '$queue'
click words:insert
expect field blk:ccmd 'a$queueb'
words-activate '$nodes'
expect field blk:ccmd 'a$queue$nodesb'
click words:close
expect dirty 1
focus blk:cenv
words
words-activate '$ppn'
expect contains blk:ccmd '$ppn'
click words:close
tab job
words
words-activate '$wallTime'
expect contains blk:header '$wallTime'
click words:close
tab machine
words
expect enabled words:insert 0
expect contains words:where 'Nothing to insert'
click words:close
quit
""", args=args, extra=extra)
    clean(p, "%s: a placeholder is inserted at the cursor of the last box; "
          "never into the environment; disabled where there is no box" % mode)


def codes_retired(tmp, display, build):
    print("codes tab, retired codes")
    e = Env(tmp, "codes-retired")
    write(os.path.join(e.ue, "CONFIG.mine"), "nwchem: /opt/nwchem\n"
          "Gaussian-03: /old/g03\n")
    p = run(display, build, e, """
select mine
expect code-listed Gaussian-03 1
expect code-listed Gaussian-98 0
code Gaussian-03
expect field code:gaussian-03 /old/g03
quit
""")
    clean(p, "a retired code is listed when the machine has a key for it")


def codes_pngs(tmp, display, build, out):
    print("codes tab PNGs")
    os.makedirs(out, exist_ok=True)
    e = Env(tmp, "codes-pngs")
    write(os.path.join(e.sc, "CONFIG.cluster"),
          read(os.path.join(e.sc, "CONFIG.cluster")) +
          "NWChemEnvironment {\n  NWCHEM_BASIS_LIBRARY /site/libraries/\n}\n"
          "NWChemCommand {\n  mpirun -np $totalprocs $nwchem $inFile > $outFile\n}\n",
          mode=0o644)
    p = run(display, build, e, """
select cluster
tab codes
code NWChem
wait 1000
shot %(o)s/codes-site.png
code Gaussian-16
set code:gaussian-16 /opt/g16/g16
set blk:cenv 'g16root /opt\\nGAUSS_SCRDIR /scratch'
wait 1000
shot %(o)s/codes-gaussian-environment.png
click code:advanced
wait 800
shot %(o)s/codes-advanced.png
focus blk:ccmd
words
wait 500
shot-dialog %(o)s/placeholders-dialog.png
click words:close
quit
""" % {"o": out})
    clean(p, "Codes PNGs")
    for n in sorted(os.listdir(out)):
        print("        " + os.path.join(out, n))


def fixes(tmp, display, build):
    print("undo, Advanced, words")
    e = Env(tmp, "fixes")
    p = run(display, build, e, """
select mine
tab connection
expect advanced 0
mark-size
click advanced:toggle
expect advanced 1
expect size-kept 1
click advanced:toggle
expect advanced 0
expect size-kept 1
expect shown undo:perlpath 0
set perlpath /saved
expect shown undo:perlpath 1
save
expect shown undo:perlpath 0
set perlpath /changed
expect shown undo:perlpath 1
undo perlpath
expect field perlpath /saved
expect dirty 0
expect shown undo:perlpath 0
set jobs:user 1
save
set jobs:user 0
set jobs:none 1
expect shown undo:jobs 1
undo jobs
expect field jobs:user 1
expect field jobs:none 0
expect dirty 0
tab job
expect shown header:variables 1
expect shown blk:header:site 0
expect shown blk:setup 1
set blk:setup 'echo a'
save
set blk:setup 'echo b'
expect shown undo:setup 1
undo setup
expect field blk:setup 'echo a'
expect dirty 0
expect shown undo:setup 0
quit
""")
    clean(p, "undo restores the saved value on a machine of your own; Advanced "
          "keeps the frame size; the words link needs no queue manager")
    cfg = keys(os.path.join(e.ue, "CONFIG.mine"))
    check(cfg.get("perlpath") == "/saved", "the saved value is in the file")


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
    ap.add_argument("--job-pngs")
    ap.add_argument("--codes-pngs")
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
        if a.codes_pngs:
            codes_pngs(tmp, disp, build, os.path.abspath(a.codes_pngs))
            job_script_pngs(tmp, disp, build, os.path.abspath(a.codes_pngs))
        elif a.job_pngs:
            job_script_pngs(tmp, disp, build, os.path.abspath(a.job_pngs))
        elif a.snapshots:
            snapshots(tmp, disp, build, os.path.abspath(a.snapshots))
            stage3_pngs(tmp, disp, build, os.path.abspath(a.snapshots))
        else:
            user_mode(tmp, disp, build)
            site_machine(tmp, disp, build)
            site_machine(tmp, disp, build, remote=True)
            delete_prompt_lists_files(tmp, disp, build)
            fixes(tmp, disp, build)
            admin_mode(tmp, disp, build)
            for m in ("user", "remote", "admin"):
                connection_tab(tmp, disp, build, m)
            for m in ("user", "remote", "admin"):
                ctx = job_script_tab(tmp, disp, build, m)
                job_script_advanced(tmp, disp, build, m, ctx)
            for m in ("user", "remote", "admin"):
                codes_tab(tmp, disp, build, m)
            codes_retired(tmp, disp, build)
    finally:
        disp.__exit__(None, None, None)
        shutil.rmtree(tmp, ignore_errors=True)
    print("%d passed, %d failed" % (passed, failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
