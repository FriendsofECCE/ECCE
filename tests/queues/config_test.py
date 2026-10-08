#!/usr/bin/env python3
"""
CONFIG.<machine> precedence, and processmachine's treatment of hand-written
keys.

    config_test.py --build <build dir>

Precedence: a site file and a user file go through RefMachine::config()
(build/configdump) and through gensub (GENSUB_DUMP_CONFIG), and the two must
give the same effective values; the expected values are also written out
here, so agreement is not the only check.  The sentinel `key: -` must clear a
key and must never reach a generated job script.

processmachine: it is fed the form Machine Registration posts and the files
it leaves behind are checked for the keys and blocks it does not manage.

Exit status 77 (CTest SKIP) without perl or configdump.
"""

import argparse
import json
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SKIP = 77

failures = []


def check(ok, what):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.exists(path):
        os.chmod(path, 0o644)
    with open(path, "w") as h:
        h.write(text)


def read(path):
    try:
        with open(path) as h:
            return h.read()
    except OSError:
        return None


def parseDump(text):
    out = {}
    for line in text.splitlines():
        if ": " in line:
            k, v = line.split(": ", 1)
            out[k] = v
    return out


MACHINES = ("testhost\ttesthost.example.org\tUnspecified\tUnspecified\t"
            "Unspecified\t1:1\tssh\t:NWChem\tMN:RD:SD:UN:PW\n")

SITE = """# site file
NWChem: /site/nwchem
Shell: tcsh
perlPath /site/perl
dupkey: first
dupkey: second
siteonly: yes
frontendMachine: login.site
noRemoteAccess: true
clearedonlyhere: -
setup {
  module load site
}
sourceFile: /site/env
Mixed: keepme
"""

USER = """# user file
nwchem: /user/nwchem
SHELL: bash
frontendMachine: -
useronly: 1
noremoteaccess:
setup { module load user }
sourcefile value:with:colons
emptykey:
userdup: 1
userdup: 2
mixed   spaced value
"""

EXPECTED = {
    "nwchem": "/user/nwchem",          # case-insensitive override
    "shell": "bash",                    # override with another case
    "perlpath": "/site/perl",           # "key value" form, site only
    "dupkey": "second",                 # last duplicate wins
    "siteonly": "yes",
    "noremoteaccess": "true",           # empty user value does not override
    "setup": "module load user",        # one-line block replaces the site block
    "sourcefile": "value:with:colons",  # space form, colons in the value
    "useronly": "1",
    "userdup": "2",
    "mixed": "spaced value",
    # frontendMachine: cleared by "-"; clearedonlyhere and emptykey: absent
}


def precedence(build, perl):
    tmp = tempfile.mkdtemp(prefix="ecce-config-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        os.makedirs(os.path.join(user, ".ECCE"))
        os.makedirs(os.path.join(home, "data"))
        os.symlink(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(home, "data", "client"))
        write(os.path.join(home, "siteconfig", "Machines"), MACHINES)
        write(os.path.join(home, "siteconfig", "CONFIG.testhost"), SITE)
        write(os.path.join(user, ".ECCE", "CONFIG.testhost"), USER)
        env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user)

        cpp = subprocess.run([os.path.join(build, "configdump"), "testhost"],
                             env=env, stdout=subprocess.PIPE, text=True)
        cppCfg = parseDump(cpp.stdout)

        params = os.path.join(tmp, "params")
        write(params, " -H testhost\n -Q Shell\n -c NWChem\n -d localhost\n"
                      " -n 1\n -N 1\n -r %s\n -i a.nw\n -o a.out\n -f %s\n"
                      % (tmp, os.path.join(tmp, "submit__x")))
        gen = subprocess.run(
            [perl, os.path.join(REPO, "scripts", "gensub"), "-p", params],
            env=dict(env, GENSUB_DUMP_CONFIG="1"), cwd=tmp,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        genCfg = parseDump(gen.stdout)

        check(cpp.returncode == 0, "configdump ran")
        check(gen.returncode == 0, "gensub dumped its config")
        check(cppCfg == genCfg,
              "C++ and gensub give the same effective values"
              + ("" if cppCfg == genCfg else
                 "\n  C++:    %r\n  gensub: %r" % (cppCfg, genCfg)))
        check(cppCfg == EXPECTED, "the effective values are the expected ones"
              + ("" if cppCfg == EXPECTED else
                 "\n  got:      %r\n  expected: %r" % (cppCfg, EXPECTED)))
        check("frontendmachine" not in cppCfg,
              "the sentinel cleared a site key")

        acc = subprocess.run(
            [os.path.join(build, "configdump"), "-accessors", "testhost"],
            env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True)
        a = parseDump(acc.stdout)
        want = {"shell": "bash", "sourceFile": "value:with:colons",
                "frontendMachine": "", "noRemoteAccess": "1",
                "exePath NWChem": "/user/nwchem"}
        got = {k: a.get(k) for k in want}
        check(got == want, "RefMachine accessors read the merged view: %r" % got)
        check(a.get("shellPath", "").startswith("/site/perl:"),
              "shellPath uses the site perlPath: %s" % a.get("shellPath"))

        # -remote: the client's siteconfig holds the server's published file;
        # the same merge applies to it.
        write(os.path.join(user, ".ECCE", "CONFIG.testhost"), "shell: csh\n")
        acc = subprocess.run(
            [os.path.join(build, "configdump"), "-accessors", "testhost"],
            env=env, stdout=subprocess.PIPE, text=True)
        check(parseDump(acc.stdout).get("shell") == "csh",
              "a user key overrides the published site file")
        os.unlink(os.path.join(user, ".ECCE", "CONFIG.testhost"))
        acc = subprocess.run(
            [os.path.join(build, "configdump"), "-accessors", "testhost"],
            env=env, stdout=subprocess.PIPE, text=True)
        check(parseDump(acc.stdout).get("shell") == "tcsh",
              "without a user file the site file applies")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def threeLayers(build, perl):
    """-remote with the server's files cached (#192): [the user's copy of
    their server registrations, the server's site files, the install's].  The
    site CONFIG.<m> comes whole from one layer, the user's merges over it, and
    localhost is the client's alone.  C++ and gensub must agree."""
    tmp = tempfile.mkdtemp(prefix="ecce-config-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        key = "srv.example.org_8096"
        cache = os.path.join(user, ".ECCE", "server", key)
        srv = os.path.join(cache, "site")
        mine = os.path.join(cache, "user-alice")
        os.makedirs(os.path.join(home, "data"))
        os.symlink(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(home, "data", "client"))
        local = MACHINES + MACHINES.replace("testhost\ttesthost.example.org",
                                            "localhost\tlocalhost")
        write(os.path.join(home, "siteconfig", "RemoteServer", "DataServers"),
              "<EcceData><EcceServer><Url>http://srv.example.org:8096/Ecce"
              "</Url></EcceServer></EcceData>\n")
        write(os.path.join(home, "siteconfig", "Machines"), local)
        write(os.path.join(home, "siteconfig", "CONFIG.testhost"),
              "shell: inst\ninstkey: i\n")
        write(os.path.join(home, "siteconfig", "CONFIG.localhost"),
              "shell: instlocal\n")
        write(os.path.join(srv, "Machines"), local)
        write(os.path.join(srv, "CONFIG.testhost"), "shell: srv\nsrvkey: s\n")
        write(os.path.join(srv, "CONFIG.localhost"),
              "shell: srvlocal\nsrvlocalkey: x\n")
        write(os.path.join(mine, "CONFIG.testhost"), "shell: usr\n")
        write(os.path.join(user, ".ECCE", "CONFIG.localhost"), "useronly: l\n")
        env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user,
                   ECCE_REMOTE_SERVER="1", ECCE_SERVER_LOGIN="alice")

        def cpp(m):
            return parseDump(subprocess.run(
                [os.path.join(build, "configdump"), m], env=env,
                stdout=subprocess.PIPE, text=True).stdout)

        def dirs(m):
            return subprocess.run(
                [os.path.join(build, "configdump"), "-dirs", m], env=env,
                stdout=subprocess.PIPE, text=True).stdout.strip()

        def gensub(m, dirList):
            params = os.path.join(tmp, "params")
            write(params, " -H %s\n -Q Shell\n -c NWChem\n -d localhost\n"
                          " -n 1\n -N 1\n -r %s\n -i a.nw\n -o a.out\n"
                          " -f %s\n" % (m, tmp, os.path.join(tmp, "submit__x")))
            return parseDump(subprocess.run(
                [perl, os.path.join(REPO, "scripts", "gensub"), "-p", params],
                env=dict(env, GENSUB_DUMP_CONFIG="1",
                         ECCE_SITECONFIG_DIRS=dirList),
                cwd=tmp, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True).stdout)

        inst = os.path.join(home, "siteconfig") + "/"
        want = ":".join([mine + "/", srv + "/", inst])
        check(dirs("testhost") == want, "layers for a machine: user, server, "
              "install" + ("" if dirs("testhost") == want else
                           "\n  got %r\n  want %r" % (dirs("testhost"), want)))
        wantLocal = ":".join([os.path.join(user, ".ECCE") + "/", inst])
        check(dirs("localhost") == wantLocal,
              "layers for localhost: ~/.ECCE and the install only")

        t = cpp("testhost")
        check(t == {"shell": "usr", "srvkey": "s"},
              "testhost: server file, never merged with the install's, "
              "user key on top: %r" % t)
        check(gensub("testhost", dirs("testhost")) == t,
              "testhost: gensub agrees with C++")
        l = cpp("localhost")
        check(l == {"shell": "instlocal", "useronly": "l"},
              "localhost: install and ~/.ECCE only: %r" % l)
        check(gensub("localhost", dirs("localhost")) == l,
              "localhost: gensub agrees with C++")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def explain(build, perl):
    """GENSUB_EXPLAIN agrees with GENSUB_DUMP_CONFIG and configdump, and its
    provenance matches layers built here, independently of gensub."""
    tmp = tempfile.mkdtemp(prefix="ecce-config-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        os.makedirs(os.path.join(user, ".ECCE"))
        os.makedirs(os.path.join(home, "data"))
        os.symlink(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(home, "data", "client"))
        sc = os.path.join(home, "siteconfig")
        siteF = os.path.join(sc, "CONFIG.testhost")
        userF = os.path.join(user, ".ECCE", "CONFIG.testhost")
        subF = os.path.join(sc, "submit.site")
        venF = os.path.join(sc, "CONFIG.ACME")
        write(os.path.join(sc, "Machines"),
              MACHINES.replace("Unspecified\tUnspecified\tUnspecified",
                               "Acme\tUnspecified\tUnspecified", 1))
        write(subF, "shell: sh\nsetup: echo SUBMITSITE\nonlysub: s\n")
        write(venF, "onlysub: vendor\nvendoronly: v\n")
        write(siteF, "Shell: tcsh\nsetup {\n  module load site\n}\n"
                     "cleared: site\nsiteonly: yes\nbothset: site\n"
                     "nwchemenvironment {\n  A=1\n  B=2\n}\n")
        write(userF, "shell: bash\nsetup { module load user }\ncleared: -\n"
                     "useronly: 1\nbothset: user\nclearthenset: -\n")
        # a key cleared in the site file and set again by the user file
        write(siteF, read(siteF) + "clearthenset: -\n")
        write(userF, read(userF) + "clearthenset: back\n")
        env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user)
        params = os.path.join(tmp, "params")
        out = os.path.join(tmp, "submit__x")
        write(params, " -H testhost\n -Q Shell\n -c NWChem\n -d localhost\n"
                      " -n 1\n -N 1\n -r %s\n -i a.nw\n -o a.out\n -f %s\n"
                      % (tmp, out))

        def gensub(extra):
            return subprocess.run(
                [perl, os.path.join(REPO, "scripts", "gensub"), "-p", params],
                env=dict(env, **extra), cwd=tmp, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True)

        ex = gensub({"GENSUB_EXPLAIN": "1"})
        check(ex.returncode == 0, "GENSUB_EXPLAIN ran"
              + ("" if ex.returncode == 0 else ": " + ex.stdout[-200:]))
        check(not os.path.exists(out), "explain writes no job script")
        rows = {}
        for line in ex.stdout.splitlines():
            r = json.loads(line)
            rows[r["key"]] = r
        # the dumps escape newlines; blocks are trimmed on read
        eff = {k: r["value"].replace("\n", "\\n") for k, r in rows.items()
               if r["value"] is not None}
        dump = parseDump(gensub({"GENSUB_DUMP_CONFIG": "1"}).stdout)
        check(eff == dump, "explain's effective values equal GENSUB_DUMP_CONFIG"
              + ("" if eff == dump else "\n  %r\n  %r" % (eff, dump)))
        cpp = parseDump(subprocess.run(
            [os.path.join(build, "configdump"), "testhost"], env=env,
            stdout=subprocess.PIPE, text=True).stdout)
        # configdump reads only the machine files, so compare on its keys
        check(all(eff.get(k) == v for k, v in cpp.items()) and
              all(k in cpp for k in ("shell", "setup", "siteonly", "useronly",
                                     "bothset", "nwchemenvironment")),
              "explain's effective values equal configdump's")

        want = {  # key: (value, source, file, [(overridden source, value)])
            "shell": ("bash", "user", userF,
                      [("submit.site", "sh"), ("site", "tcsh")]),
            "setup": ("module load user", "user", userF,
                      [("submit.site", "echo SUBMITSITE"),
                       ("site", "module load site")]),
            "onlysub": ("vendor", "vendor", venF, [("submit.site", "s")]),
            "vendoronly": ("v", "vendor", venF, []),
            "siteonly": ("yes", "site", siteF, []),
            "useronly": ("1", "user", userF, []),
            "bothset": ("user", "user", userF, [("site", "site")]),
            "cleared": (None, "cleared", userF, [("site", "site")]),
            "clearthenset": ("back", "user", userF, [("site", None),
                                                    ("user", None)]),
            "nwchemenvironment": ("A=1\n  B=2", "site", siteF, []),
        }
        for k, (val, src, f, ov) in want.items():
            r = rows.get(k)
            got = r and (r["value"], r["source"], r["file"],
                         [(o["source"], o["value"]) for o in r["overridden"]])
            check(got == (val, src, f, ov), "provenance of %s: %r" % (k, got))
        check("nosuchkey" not in rows, "a key in no layer is absent")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def sentinelScript(perl):
    """The sentinel is a clear, never a value, in a real job script."""
    tmp = tempfile.mkdtemp(prefix="ecce-config-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        os.makedirs(os.path.join(user, ".ECCE"))
        os.makedirs(os.path.join(home, "data"))
        os.symlink(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(home, "data", "client"))
        os.makedirs(os.path.join(home, "siteconfig"))
        for name in ("QueueManagers", "submit.site"):
            shutil.copy(os.path.join(REPO, "siteconfig", name),
                        os.path.join(home, "siteconfig", name))
        write(os.path.join(home, "siteconfig", "Machines"), MACHINES)
        write(os.path.join(home, "siteconfig", "CONFIG.testhost"),
              "NWChem: /opt/nwchem\nsetup: echo SITEMARKER\n"
              "wrapup: echo SITEWRAPUP\n")
        write(os.path.join(user, ".ECCE", "CONFIG.testhost"),
              "setup: -\nnwchemCommand: -\nneverset: -\nwrapup {\n  -\n}\n")
        env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user)
        params = os.path.join(tmp, "params")
        out = os.path.join(tmp, "submit__x")
        write(params, " -H testhost\n -Q Shell\n -c NWChem\n -d localhost\n"
                      " -n 1\n -N 1\n -r %s\n -i a.nw\n -o a.out\n -f %s\n"
                      % (tmp, out))
        gen = subprocess.run(
            [perl, os.path.join(REPO, "scripts", "gensub"), "-p", params],
            env=env, cwd=tmp, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True)
        script = read(out)
        check(gen.returncode == 0 and script is not None,
              "gensub generated a script" + ("" if script else ": " + gen.stdout))
        if script is None:
            return
        check("SITEMARKER" not in script, "setup: - removed the site setup")
        check("SITEWRAPUP" not in script, "a one-line block holding - clears too")
        check(not [l for l in script.splitlines() if l.strip() == "-"],
              "no sentinel line in the generated script")
        check("/opt/nwchem" in script, "an untouched site key still applies")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def moduleScript(perl):
    """A before-command using `module` gets one sh prologue that defines it."""
    tmp = tempfile.mkdtemp(prefix="ecce-module-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        os.makedirs(os.path.join(user, ".ECCE"))
        os.makedirs(os.path.join(home, "data"))
        os.symlink(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(home, "data", "client"))
        os.makedirs(os.path.join(home, "siteconfig"))
        for name in ("QueueManagers", "submit.site"):
            shutil.copy(os.path.join(REPO, "siteconfig", name),
                        os.path.join(home, "siteconfig", name))
        write(os.path.join(home, "siteconfig", "Machines"), MACHINES)
        env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user)
        params = os.path.join(tmp, "params")
        out = os.path.join(tmp, "submit__x")

        def gen(config):
            write(os.path.join(home, "siteconfig", "CONFIG.testhost"), config)
            write(params, " -H testhost\n -Q Shell\n -c NWChem\n -d localhost\n"
                          " -n 1\n -N 1\n -r %s\n -i a.nw\n -o a.out\n -f %s\n"
                          % (tmp, out))
            subprocess.run(
                [perl, os.path.join(REPO, "scripts", "gensub"), "-p", params],
                env=env, cwd=tmp, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True)
            return read(out)

        plain = gen("NWChem: /opt/nwchem\nsetup: echo hello\n")
        check(plain is not None and "command -v module" not in plain,
              "no module prologue when the script does not use module")

        script = gen("NWChem: /opt/nwchem\nsetup: module load x\n"
                     "wrapup: module unload x\n")
        check(script is not None, "gensub generated a script using module")
        if script is None:
            return
        pro = "command -v module"
        i = script.find(pro)
        j = script.find("module load x")
        check(script.count("# Make the module command available") == 1,
              "the module prologue appears once")
        check(0 <= i < j, "the prologue comes before the first module use")
        dash = shutil.which("dash")
        if not dash:
            print("SKIP  dash not installed: module checks not run")
            return
        r = subprocess.run([dash, "-n", out], capture_output=True, text=True)
        check(r.returncode == 0, "the script with the prologue passes dash -n: "
              + r.stderr)

        # Run the prologue and the module line under dash against fake inits.
        start = script.find("# Make the module command available")
        end = script.find("module load x")
        frag = script[start:end] + "module load x\n"
        frag = frag.replace(os.path.join(tmp, "ecce.submit.log"),
                            os.path.join(tmp, "frag.log"))
        init = 'module() { echo "MODULE-CALLED $*"; }\n'
        for label, layout, var in (
                ("LMOD_PKG", "hpc2n/eb/software/lmod/lmod", "LMOD_PKG"),
                ("MODULESHOME", "usr/share/Modules", "MODULESHOME")):
            pkg = os.path.join(tmp, layout)
            write(os.path.join(pkg, "init", "sh"), init)
            e = {k: v for k, v in os.environ.items()
                 if k not in ("LMOD_PKG", "MODULESHOME", "LMOD_CMD")}
            e[var] = pkg
            r = subprocess.run([dash, "-c", frag], env=e, capture_output=True,
                               text=True)
            check(r.stdout.strip() == "MODULE-CALLED load x",
                  "module load works under dash via %s: %r %r"
                  % (label, r.stdout, r.stderr))
        pkg = os.path.join(tmp, "lmodcmd", "lmod")
        write(os.path.join(pkg, "init", "sh"), init)
        e = {k: v for k, v in os.environ.items()
             if k not in ("LMOD_PKG", "MODULESHOME")}
        e["LMOD_CMD"] = os.path.join(pkg, "libexec", "lmod")
        r = subprocess.run([dash, "-c", frag], env=e, capture_output=True,
                           text=True)
        check(r.stdout.strip() == "MODULE-CALLED load x",
              "module load works under dash via LMOD_CMD: %r" % r.stdout)
        if not (glob.glob("/etc/profile.d/*lmod*") or
                glob.glob("/etc/profile.d/modules.sh") or
                glob.glob("/usr/share/*mod*/*/init/sh")):
            e = {k: v for k, v in os.environ.items()
                 if k not in ("LMOD_PKG", "MODULESHOME", "LMOD_CMD")}
            r = subprocess.run([dash, "-c", frag], env=e, capture_output=True,
                               text=True)
            check("module command not available in sh" in r.stderr
                  and "module command not available in sh" in
                  (read(os.path.join(tmp, "frag.log")) or ""),
                  "a missing module system is reported, not silent")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def post(home, user, fields, site=False, script=None, encoder=None,
         cwd=None):
    """encoder: build/pmform, the GUI's own encoder; urlencode otherwise."""
    fields = dict(fields)
    fields["siteconfig"] = "true" if site else "false"
    if encoder:
        args = [x for kv in fields.items() for x in kv]
        body = subprocess.run([encoder] + args, stdout=subprocess.PIPE,
                              check=True, text=True).stdout
    else:
        from urllib.parse import urlencode
        body = urlencode(fields)
    env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user,
               CONTENT_LENGTH=str(len(body)))
    return subprocess.run(["perl", script or os.path.join(REPO, "scripts",
                                                         "processmachine")],
                          input=body, env=env, text=True, cwd=cwd,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


BASE = {"type": "accept", "machine": "testhost.example.org", "name": "testhost",
        "vendor": "Unspecified", "model": "Unspecified",
        "processor": "Unspecified", "procs": "1", "nodes": "1", "ssh": "1",
        "registeredcodes": "NWChem,Gaussian-16", "NWChem": "", "Gaussian-16": "",
        "perlPath": "", "qmgrPath": "", "AA": "false", "qmgr": "None",
        "numQueues": "0"}


# Characters the shell or the form encoding treat specially.
NASTY = "+ & = % \" $ \\ ' `x` $(id)"


def processmachine(script=None, encoder=None):
    tmp = tempfile.mkdtemp(prefix="ecce-pm-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        os.makedirs(os.path.join(home, "siteconfig"))
        os.makedirs(os.path.join(user, ".ECCE"))
        ue = os.path.join(user, ".ECCE")
        cfg = os.path.join(ue, "CONFIG.testhost")

        # 1. hand-written keys survive a save that sets paths
        write(cfg, "# my machine\nnwchem: /old/lowercase\nShell: csh\n"
                   "sourceFile: /home/me/env\nnoRemoteAccess: true\n"
                   "setup {\n  module load x\n  perlPath not-a-key\n}\n"
                   "perlPath: /old/perl\n")
        r = post(home, user, dict(BASE, NWChem="/new/nwchem",
                                  perlPath="/new/perl"), script=script)
        text = read(cfg) or ""
        check(r.returncode == 0, "processmachine ran: %s" % r.stdout[-200:])
        for keep in ("Shell: csh", "sourceFile: /home/me/env",
                     "noRemoteAccess: true", "module load x",
                     "perlPath not-a-key"):
            check(keep in text, "a save keeps hand-written %r" % keep)
        check("/old/lowercase" not in text and "/old/perl" not in text,
              "GUI-owned keys are replaced, whatever their case")
        check("NWChem: /new/nwchem" in text and "perlPath: /new/perl" in text,
              "the GUI values are written")
        check(text.startswith("# my machine\n"), "the header comment stays first")

        # 2. a save with no paths does not delete a file with other keys
        post(home, user, BASE, script=script)
        text = read(cfg)
        check(text is not None and "Shell: csh" in text and
              "NWChem" not in text and "/new/perl" not in text,
              "an empty-path save keeps the file, minus the GUI's keys")

        # 3. ... but deletes one that holds only GUI keys
        write(cfg, "NWChem: /only/gui\nperlPath: /only/perl\n")
        post(home, user, BASE, script=script)
        check(read(cfg) is None, "a file of only GUI keys is removed")

        # 4. qmgrPath alone is a reason to write the file
        post(home, user, dict(BASE, qmgrPath="/opt/slurm/bin"), script=script)
        check("qmgrPath: /opt/slurm/bin" in (read(cfg) or ""),
              "qmgrPath alone is written")
        check(os.access(cfg, os.W_OK),
              "a file processmachine creates stays writable for the user")
        os.chmod(cfg, 0o444)
        post(home, user, dict(BASE, qmgrPath="/opt/pbs/bin"), script=script)
        check("/opt/pbs/bin" in (read(cfg) or "") and
              os.access(cfg, os.W_OK),
              "a read-only user file (older versions) is written and left writable")
        os.chmod(cfg, 0o644)

        # 5. the .Q file keeps what the GUI does not manage
        qfile = os.path.join(ue, "testhost.Q")
        write(qfile, "# Queue details for testhost\n\nQueues: batch gone\n\n"
                     "batch|maxProcessors:   4\nbatch|defProcessors:   2\n"
                     "batch|memUnits:        MB\nbatch|defRun:          1:00\n"
                     "gone|defProcessors:    9\n")
        q = "name|batch,minNodes|1,maxNodes|16,maxCPU|600,maxMemory|2000,minScratch|0,"
        post(home, user, dict(BASE, qmgr="Slurm", numQueues="1", q0=q),
             script=script)
        qtext = read(qfile) or ""
        for keep in ("batch|defProcessors:   2", "batch|memUnits:        MB",
                     "batch|defRun:          1:00"):
            check(keep in qtext, "the .Q keeps %r" % keep)
        check("maxProcessors:       16" in qtext and "maxProcessors:   4" not in qtext,
              "the .Q takes the GUI's maxProcessors")
        check("gone|" not in qtext, "a queue the GUI dropped loses its lines")

        # 6. a queue-manager change on a machine that already has queues
        qs = os.path.join(ue, "Queues")
        write(qs, "Queues: othermachine testhost\n\n"
                  "othermachine|queueMgrName: PBS\n"
                  "testhost|queueMgrName:    PBS\n"
                  "testhost|queueMgrVersion: 2.0~\n"
                  "testhost|prefFile:        testhost.Q\n"
                  "testhost|handKey:         keepme\n"
                  "mytesthost|queueMgrName:  LSF\n")
        post(home, user, dict(BASE, qmgr="Slurm", numQueues="1", q0=q),
             script=script)
        qt = read(qs) or ""
        check("\ntesthost|queueMgrName:    Slurm" in qt
              and "\ntesthost|queueMgrName:    PBS" not in qt,
              "the queue manager change persists")
        check("othermachine|queueMgrName: PBS" in qt,
              "another machine's entry is untouched")
        check("mytesthost|queueMgrName:  LSF" in qt,
              "a machine whose name ends in this one is untouched")
        check("testhost|handKey:         keepme" in qt,
              "a hand-written key of the machine survives")
        check(qt.splitlines()[0].split().count("testhost") == 1,
              "the machine is listed once")

        # 7. config=external: the GUI writes CONFIG.<m> itself, so
        # processmachine neither rewrites nor unlocks it
        write(cfg, "NWChem: /gui/own\nShell: csh\n")
        os.chmod(cfg, 0o444)
        before = read(cfg)
        post(home, user, dict(BASE, NWChem="/other", perlPath="/other/perl",
                              config="external"), script=script)
        check(read(cfg) == before and not os.access(cfg, os.W_OK),
              "config=external leaves CONFIG.<m> byte-identical and locked")
        os.chmod(cfg, 0o644)

        escaping(home, user, tmp, script, encoder)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def escaping(home, user, tmp, script, encoder):
    from urllib.parse import parse_qsl
    ue = os.path.join(user, ".ECCE")
    cfg = os.path.join(ue, "CONFIG.testhost")
    machines = os.path.join(ue, "MyMachines")
    qs = os.path.join(ue, "Queues")
    if encoder:
        # the GUI's encoder against an independent decoder
        fields = [("type", "accept"), ("NWChem", NASTY), ("q0", "name|a,")]
        out = subprocess.run([encoder] + [x for kv in fields for x in kv],
                             stdout=subprocess.PIPE, text=True).stdout
        check(parse_qsl(out, keep_blank_values=True) == fields,
              "the GUI's form encoding decodes back to its fields")
        check(out.count("&") == 2 and out.count("=") == 3,
              "no value can split the form")
    else:
        print("note  no pmform: posting with urlencode, not the GUI encoder")

    # 7. special characters in paths and fields survive a save unchanged
    path = "/opt/nw chem/" + NASTY + "/bin/nwchem"
    qmgr = "/opt/q+m&g=r%41/bin"
    vendor = "A&B=C+D%20"
    q = "name|q.a-1_b,minNodes|1,maxNodes|4,maxCPU|60,maxMemory|0,minScratch|0,"
    r = post(home, user, dict(BASE, NWChem=path, qmgrPath=qmgr, vendor=vendor,
                              qmgr="Slurm", numQueues="1", q0=q),
             script=script, encoder=encoder, cwd=tmp)
    text = read(cfg) or ""
    check(r.returncode == 0, "a save with special characters runs: %s"
          % r.stdout[-200:])
    check(("NWChem: %s\n" % path) in text, "a code path round-trips exactly")
    check(("qmgrPath: %s\n" % qmgr) in text, "qmgrPath round-trips exactly")
    line = [l for l in (read(machines) or "").splitlines()
            if l.startswith("testhost\t")]
    check(bool(line) and line[0].split("\t")[2] == vendor,
          "a Machines field round-trips exactly")
    check("q.a-1_b|maxProcessors:       4" in
          (read(os.path.join(ue, "testhost.Q")) or ""),
          "a queue name round-trips")

    # 8. a queue name the .Q format cannot hold is refused, nothing written
    before = read(cfg)
    bad = "name|a b&c=d+e%,minNodes|1,"
    r = post(home, user, dict(BASE, NWChem="/changed", qmgr="Slurm",
                              numQueues="1", q0=bad),
             script=script, encoder=encoder, cwd=tmp)
    check(r.returncode != 0 and read(cfg) == before,
          "an invalid queue name is refused before any file changes")

    # 9. deleting machines whose names hold quotes and command substitutions
    marker = os.path.join(tmp, "executed")
    slashed = 'q"$(touch %s)' % marker
    odd = 'odd"$(touch executed)"`touch executed`'
    write(machines, "testhost\tt\tU\tU\tU\t1\tssh\t:\tMN\n"
                    "%s\tx\tU\tU\tU\t1\tssh\t:\tMN\n"
                    "%s\ty\tU\tU\tU\t1\tssh\t:\tMN\n" % (slashed, odd))
    write(os.path.join(ue, "CONFIG." + odd), "NWChem: /x\n")
    write(os.path.join(ue, odd + ".Q"), "Queues: a\n")
    write(qs, "Queues: testhost %s\n\n%s|queueMgrName: PBS\n"
              "testhost|queueMgrName: Slurm\n" % (odd, odd))
    for name in (odd, slashed):
        r = post(home, user, {"type": "delete", "name": name},
                 script=script, encoder=encoder, cwd=tmp)
        check(r.returncode == 0,
              "delete of %r runs: %s" % (name, r.stdout[-200:]))
    check(not os.path.exists(marker), "no command in a machine name ran")
    ml = (read(machines) or "").splitlines()
    check(len(ml) == 1 and ml[0].startswith("testhost\t"),
          "both lines leave MyMachines, the other machine's stays")
    check(not os.path.exists(os.path.join(ue, "CONFIG." + odd)) and
          not os.path.exists(os.path.join(ue, odd + ".Q")),
          "the deleted machine's CONFIG and .Q files are removed")
    check(os.path.exists(cfg) and
          os.path.exists(os.path.join(ue, "testhost.Q")),
          "the other machine's files stay")
    qt = read(qs) or ""
    check(odd not in qt and "testhost|queueMgrName: Slurm" in qt,
          "the Queues entry is removed, the other machine's stays")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--processmachine",
                    help="test this processmachine instead (to see old bugs)")
    args = ap.parse_args()
    perl = shutil.which("perl")
    exe = os.path.join(args.build, "configdump")
    if not perl or not os.path.exists(exe):
        print("SKIP  needs perl and %s" % exe)
        return SKIP

    if not args.processmachine:
        precedence(args.build, perl)
        threeLayers(args.build, perl)
        sentinelScript(perl)
        moduleScript(perl)
        explain(args.build, perl)
    encoder = os.path.join(args.build, "pmform")
    processmachine(args.processmachine,
                   encoder if os.path.exists(encoder) else None)
    print("")
    print("FAILED: %d" % len(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
