#!/usr/bin/env python3
"""
The CONFIG.<machine> writer (ConfigFile), the Register Machines draft
(MachineConfigDraft) and the in-tree JSON reader (MiniJson), through
build/configedit.

    configedit_test.py --build <build dir>

* JSON: the reader's output equals Python's json on real GENSUB_EXPLAIN
  output and on hand-made hard cases; malformed text is rejected.
* Round trip: load + save of real files (siteconfig/, the examples, files
  with sentinels, blocks, comments, CRLF, no final newline) is byte-identical.
* Edits: set / remove / clear in every key form changes only that key's
  lines, in place, and refuses what the grammar cannot hold.
* Three ways: for each edit, the draft's prediction (made from explain
  output before the write), gensub's explain after the write and the C++
  merged view (configdump) all agree.

Exit status 77 (CTest SKIP) without perl or the test programs.
"""

import argparse
import glob
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SKIP = 77
CPP_KEYS = ["shell", "sourcefile", "frontendmachine", "frontendbypass",
            "perlpath", "qmgrpath", "libpath", "xappspath", "noremoteaccess",
            "usersubmit", "singleconnect", "checkscratch"]

failures = []


def check(ok, what):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def write(path, text, mode=None):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.exists(path):
        os.chmod(path, 0o644)
    with open(path, "w", newline="") as h:
        h.write(text)
    if mode is not None:
        os.chmod(path, mode)


def readb(path):
    try:
        with open(path, "rb") as h:
            return h.read()
    except OSError:
        return None


def read(path):
    b = readb(path)
    return None if b is None else b.decode()


class Tools:
    def __init__(self, build, perl):
        self.build, self.perl = build, perl
        self.edit = os.path.join(build, "configedit")
        self.dump = os.path.join(build, "configdump")

    def run(self, *args, **kw):
        return subprocess.run([self.edit] + list(args), stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, **kw)

    def file(self, path, *ops):
        return self.run("file", path, *ops)


# ----------------------------------------------------------------- JSON

def json_tests(t, explain_text):
    lines = [l for l in explain_text.splitlines() if l.strip()]
    lines += [
        '{"a":"x\\ny\\t\\"q\\" \\\\ \\/","b":null,"c":[1,-2.5e3,true,false,{}],'
        '"d":{"e":[[],[null]]}}',
        '"caf\\u00e9 \\ud83d\\ude00 \\u0000 \\u007f"',
        '"raw utf-8: café \U0001f600"',
        '  [ ]  ', '0', '-0.5E+2', 'true', 'null',
        '{"k":"v","k":"last"}',
    ]
    proc = subprocess.run([t.edit, "json"], input="\n".join(lines) + "\n",
                          stdout=subprocess.PIPE, text=True)
    out = proc.stdout.splitlines()
    check(len(out) == len(lines), "the reader answers every line")
    same = True
    for src, got in zip(lines, out):
        try:
            same &= json.loads(got) == json.loads(src)
        except ValueError:
            same = False
            print("  bad output for %r: %r" % (src, got))
    check(same, "MiniJson equals python json on %d values (%d real explain rows)"
          % (len(lines), len(explain_text.splitlines())))
    bad = ['{"a":}', '{"a":1,}', '[1 2]', '"abc', '"\\x"', '"\\ud800"',
           '{"a":1} x', '', '{', '[1,', '"tab\there"', '01x', '{1:2}',
           "[" * 100 + "]" * 100]
    proc = subprocess.run([t.edit, "json"],
                          input="\n".join(b for b in bad if b) + "\n",
                          stdout=subprocess.PIPE, text=True)
    errs = proc.stdout.splitlines()
    check(all(l.startswith("ERROR") for l in errs) and len(errs) == len(
        [b for b in bad if b]), "malformed JSON is rejected")


# ----------------------------------------------------------- round trip

SENTINELS = """# header comment
// other comment style

shell: -
Setup {
  module load a
  echo "}" inside
}
wrapup { - }
nwchemCommand:   -
perlPath /usr/bin/perl
Mixed:   Value  with   spaces
emptykey:
loneword
sourceFile: /a/b:c:d
    indented: value
tail {
}
"""


def roundtrip(t, tmp):
    files = [os.path.join(REPO, "siteconfig", "CONFIG.dummy"),
             os.path.join(REPO, "siteconfig", "submit.site")]
    files += sorted(glob.glob(os.path.join(REPO, "siteconfig",
                                           "CONFIG-Examples", "CONFIG.*")))
    n = 0
    for src in files:
        dst = os.path.join(tmp, "rt", os.path.basename(src))
        write(dst, read(src))
        r = t.file(dst)
        check(r.returncode == 0 and readb(dst) == readb(src),
              "load+save is byte-identical: %s" % os.path.relpath(src, REPO))
        n += 1
    variants = {
        "sentinels": SENTINELS,
        "crlf": SENTINELS.replace("\n", "\r\n"),
        "no-final-newline": SENTINELS.rstrip("\n"),
        "unclosed": "a: 1\nblock {\n  text\n",
        "comments-only": "# nothing\n\n",
        "empty": "",
    }
    for name, text in variants.items():
        dst = os.path.join(tmp, "rt", name)
        write(dst, text)
        r = t.file(dst)
        check(r.returncode == 0 and read(dst) == text,
              "load+save is byte-identical: %s" % name)
    miss = os.path.join(tmp, "rt", "missing")
    r = t.file(miss)
    check(r.returncode == 0 and not os.path.exists(miss),
          "no edit on a missing file creates nothing")
    w = t.run("check", os.path.join(tmp, "rt", "unclosed")).stdout
    check("not closed" in w, "an unclosed block is reported")
    w = t.run("check", os.path.join(tmp, "rt", "sentinels")).stdout
    check("'emptykey' has no value" in w and "'loneword' has no value" in w,
          "keys without a value are reported")


# ---------------------------------------------------------------- edits

def edit_case(t, tmp, name, before, ops, after, rc=0):
    p = os.path.join(tmp, "ed", name)
    write(p, before)
    r = t.file(p, *ops)
    got = read(p)
    ok = r.returncode == rc and got == after
    check(ok, "edit: %s" % name)
    if not ok:
        print("  rc=%d out=%r\n  got  %r\n  want %r" % (r.returncode, r.stdout,
                                                       got, after))


def edits(t, tmp):
    c = lambda *a, **k: edit_case(t, tmp, *a, **k)
    base = "# top\n\nShell: tcsh\nsetup {\n  module load x\n}\nkeep: 1\n"
    c("set colon, case and place kept", base, ["set", "shell", "bash"],
      base.replace("Shell: tcsh", "Shell: bash"))
    c("set space form", "perlPath /usr/bin\nz: 1\n",
      ["set", "PERLPATH", "/opt/perl"], "perlPath: /opt/perl\nz: 1\n")
    c("set one-line block as colon", "setup { a }\nz: 1\n",
      ["set", "setup", "b"], "setup: b\nz: 1\n")
    c("set multi-line replaces block", base,
      ["set", "setup", "one\n  two"],
      base.replace("setup {\n  module load x\n}", "setup {\none\n  two\n}"))
    c("set block over single line", "a: 1\nsetup: x\nb: 2\n",
      ["set", "setup", "l1\nl2"], "a: 1\nsetup {\nl1\nl2\n}\nb: 2\n")
    c("duplicates collapse to the first place",
      "k: 1\nother: x\nK: 2\nk {\n  3\n}\nlast: y\n",
      ["set", "k", "new"], "k: new\nother: x\nlast: y\n")
    c("new key after the leading comments", "# a\n# b\nx: 1\n",
      ["set", "newKey", "v"], "# a\n# b\nnewKey: v\nx: 1\n")
    c("new key in an empty file", "", ["set", "k", "v"], "k: v\n")
    c("value with a brace becomes a block", "a: 1\n",
      ["set", "k", "x{y}"], "k {\nx{y}\n}\na: 1\n")
    c("remove every occurrence", "a: 1\nK: 2\nb {\n x\n}\nk { 3 }\nc: 3\n",
      ["remove", "k"], "a: 1\nb {\n x\n}\nc: 3\n")
    c("remove an absent key is a no-op", "a: 1\n", ["remove", "zz"], "a: 1\n")
    c("clear writes the sentinel", "a: 1\nshell: bash\n",
      ["clear", "Shell"], "a: 1\nshell: -\n")
    c("clear keeps the block form", "setup {\n  x\n}\na: 1\n",
      ["clear", "setup"], "setup { - }\na: 1\n")
    c("clear a new key", "a: 1\n", ["clear", "k"], "k: -\na: 1\n")
    c("hand-written keys, comments and order survive", SENTINELS,
      ["set", "sourceFile", "/new"],
      SENTINELS.replace("sourceFile: /a/b:c:d", "sourceFile: /new"))
    c("crlf lines are kept", "a: 1\r\nb: 2\r\n", ["set", "b", "3"],
      "a: 1\r\nb: 3\n")
    c("no final newline gains one only when edited", "a: 1",
      ["set", "b", "2"], "b: 2\na: 1\n")
    nasty = "+ & = % \" $ \\ ' `x` $(id)"
    c("hostile value is written verbatim", "a: 1\n", ["set", "k", nasty],
      "k: %s\na: 1\n" % nasty)
    # refusals leave the file alone
    for what, val in (("'-'", "-"), ("empty", ""), ("leading blank", " x"),
                      ("trailing blank", "x "), ("closing brace line",
                                                  "a\n}\nb")):
        c("refuses %s" % what, "a: 1\n", ["set", "k", val], "a: 1\n", rc=3)
    c("refuses a key with a colon", "a: 1\n", ["set", "k:x", "v"], "a: 1\n",
      rc=3)

    # a file left holding nothing but comments is deleted; modes
    p = os.path.join(tmp, "ed", "deleted")
    write(p, "k: 1\nK: 2\n")
    t.file(p, "remove", "k")
    check(not os.path.exists(p), "removing the last key deletes the file")
    p = os.path.join(tmp, "ed", "keepcomments")
    write(p, "# c\nk: 1\n")
    t.file(p, "remove", "k")
    check(not os.path.exists(p), "a file left with only comments is deleted")
    p = os.path.join(tmp, "ed", "locked")
    write(p, "k: 1\n", mode=0o444)
    t.file(p, "set", "k", "2")
    m = os.stat(p).st_mode
    check(read(p) == "k: 2\n" and m & stat.S_IWUSR and m & stat.S_IRGRP,
          "a read-only file is rewritten and left writable, group bits kept")
    p = os.path.join(tmp, "ed", "locksite")
    write(p, "k: 1\n", mode=0o444)
    t.file(p, "--site-file", "set", "k", "2")
    check(read(p) == "k: 2\n" and not os.stat(p).st_mode & stat.S_IWUSR,
          "a read-only site file is locked again after the write")
    p = os.path.join(tmp, "ed", "opensite")
    write(p, "k: 1\n", mode=0o644)
    t.file(p, "--site-file", "set", "k", "2")
    check(os.stat(p).st_mode & stat.S_IWUSR, "a writable site file stays writable")
    p = os.path.join(tmp, "ed", "newfile")
    t.file(p, "set", "k", "v")
    check(read(p) == "k: v\n" and stat.S_IMODE(os.stat(p).st_mode) == 0o644 &
          ~current_umask(), "a new file is created, mode 0644 under umask")
    check(not [f for f in os.listdir(os.path.join(tmp, "ed"))
               if ".tmp." in f], "no temporary file is left behind")
    link = os.path.join(tmp, "ed", "link")
    write(os.path.join(tmp, "ed", "target"), "k: 1\n")
    os.symlink("target", link)
    t.file(link, "set", "k", "2")
    check(os.path.islink(link) and read(os.path.join(tmp, "ed", "target"))
          == "k: 2\n", "a symlink is written through, not replaced")


def current_umask():
    m = os.umask(0)
    os.umask(m)
    return m


# ---------------------------------------------------------- three ways

MACHINES = ("testhost\ttesthost.example.org\tAcme\tUnspecified\t"
            "Unspecified\t1:1\tssh\t:NWChem\tMN:RD:SD:UN:PW\n")

SITE = """# site
Shell: tcsh
sourceFile: /site/env
frontendMachine: login.site
perlPath /site/perl
noRemoteAccess: true
setup {
  module load site
}
cleared: site
siteonly: yes
bothset: site
nwchemenvironment {
  A=1
  B=2
}
"""

USER = """# user
shell: bash
frontendMachine: -
libPath: /user/lib
setup { module load user }
cleared: -
useronly: 1
bothset: user
"""

SUBMIT = "shell: sh\nsetup: echo SUBMITSITE\nonlysub: s\nqmgrPath: /sub/q\n"
VENDOR = "onlysub: vendor\nvendoronly: v\n"


class World:
    def __init__(self, tmp, tools):
        self.t = tools
        self.tmp = tmp
        self.home = os.path.join(tmp, "home")
        self.user = os.path.join(tmp, "user")
        self.empty = os.path.join(tmp, "nouser")
        os.makedirs(os.path.join(self.user, ".ECCE"))
        os.makedirs(os.path.join(self.empty, ".ECCE"))
        os.makedirs(os.path.join(self.home, "data"))
        os.symlink(os.path.join(REPO, "scripts"),
                   os.path.join(self.home, "scripts"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(self.home, "data", "client"))
        sc = os.path.join(self.home, "siteconfig")
        self.siteF = os.path.join(sc, "CONFIG.testhost")
        self.userF = os.path.join(self.user, ".ECCE", "CONFIG.testhost")
        write(os.path.join(sc, "Machines"), MACHINES)
        write(os.path.join(sc, "submit.site"), SUBMIT)
        write(os.path.join(sc, "CONFIG.ACME"), VENDOR)
        write(self.siteF, SITE)
        write(self.userF, USER)
        self.params = os.path.join(tmp, "params")
        write(self.params,
              " -H testhost\n -Q Shell\n -c NWChem\n -d localhost\n"
              " -n 1\n -N 1\n -r %s\n -i a.nw\n -o a.out\n -f %s\n"
              % (tmp, os.path.join(tmp, "submit__x")))

    def env(self, admin=False):
        return dict(os.environ, ECCE_HOME=self.home,
                    ECCE_REALUSERHOME=self.empty if admin else self.user)

    def explain(self, admin=False):
        r = subprocess.run(
            [self.t.perl, os.path.join(REPO, "scripts", "gensub"), "-p",
             self.params], env=dict(self.env(admin), GENSUB_EXPLAIN="1"),
            cwd=self.tmp, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True)
        assert r.returncode == 0, r.stdout
        return r.stdout

    def rows(self, admin=False):
        return {j["key"]: j for j in map(json.loads, self.explain(admin)
                                         .splitlines())}

    def cpp(self, admin=False):
        out = subprocess.run([self.t.dump, "testhost"], env=self.env(admin),
                             stdout=subprocess.PIPE, text=True).stdout
        d = {}
        for line in out.splitlines():
            k, v = line.split(": ", 1)
            d[k] = v.replace("\\n", "\n")
        return d

    def draft(self, mode, *ops, write_=False, merged=True):
        admin = mode == "admin"
        ef = os.path.join(self.tmp, "explain.json")
        write(ef, self.explain(admin))
        args = ["draft", "--mode", mode, "--site", self.siteF, "--user",
                self.userF, "--explain", ef]
        if merged:
            args += ["--merged", "testhost"]
        if write_:
            args += ["--write"]
        r = subprocess.run([self.t.edit] + args + list(ops),
                           env=self.env(admin), stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, text=True)
        out = {"lines": [], "keys": {}, "rc": r.returncode}
        for line in r.stdout.splitlines():
            if line.startswith("{"):
                j = json.loads(line)
                out["keys"][j["key"]] = j
            else:
                out["lines"].append(line)
        return out


def three_way(t, tmp, name, mode, ops, expect_text=None):
    w = World(os.path.join(tmp, name.replace(" ", "_")), t)
    admin = mode == "admin"
    target = w.siteF if admin else w.userF
    other = w.userF if admin else w.siteF
    other_before = readb(other)
    pred = w.draft(mode, *ops, write_=True)
    ok = pred["rc"] == 0 and not [l for l in pred["lines"]
                                  if l.startswith(("MISMATCH", "REFUSED",
                                                   "WRITE"))]
    check(ok, "%s: draft loaded, matches the C++ view, wrote (%s)"
          % (name, pred["lines"]))
    check(readb(other) == other_before, "%s: the other layer is untouched"
          % name)
    if expect_text is not None:
        check(read(target) == expect_text, "%s: file text" % name)
        if read(target) != expect_text:
            print("  got  %r\n  want %r" % (read(target), expect_text))
    rows = w.rows(admin)
    bad = []
    for k, p in pred["keys"].items():
        got = rows[k]["value"] if k in rows else None
        if got != p["effective"]:
            bad.append((k, "explain", got, "draft", p["effective"]))
    for k, r in rows.items():
        if k not in pred["keys"] and r["value"] is not None:
            bad.append((k, "explain has a key the draft lacks", r["value"]))
    check(not bad, "%s: draft prediction == explain after the write %s"
          % (name, bad))
    cpp = w.cpp(admin)
    bad = []
    for k in CPP_KEYS:
        p = pred["keys"].get(k, {}).get("cpp")
        if cpp.get(k) != p:
            bad.append((k, "configdump", cpp.get(k), "draft", p))
    check(not bad, "%s: draft prediction == C++ merged view %s" % (name, bad))
    for k in CPP_KEYS:
        if k in rows and rows[k]["source"] != "cleared":
            pass
    # a fresh draft from the written files agrees with itself: not dirty
    again = w.draft(mode)
    check(again["rc"] == 0 and "dirty 0" in again["lines"] and not [
        l for l in again["lines"] if l.startswith("MISMATCH")],
        "%s: a draft reloaded after the write is clean" % name)
    return w


def tags_and_modes(t, tmp):
    w = World(os.path.join(tmp, "tags"), t)
    d = w.draft("user")
    want = {"shell": "yours", "setup": "yours", "useronly": "yours",
            "bothset": "yours", "siteonly": "site", "nwchemenvironment": "site",
            "onlysub": "site defaults", "vendoronly": "site defaults",
            "cleared": "no value", "frontendmachine": "no value",
            "sourcefile": "site", "perlpath": "site", "libpath": "yours",
            "qmgrpath": "site defaults"}
    got = {k: v["tag"] for k, v in d["keys"].items()}
    check(all(got.get(k) == v for k, v in want.items()),
          "user mode tags %s" % {k: got.get(k) for k in want
                                 if got.get(k) != want[k]})
    d = w.draft("remote")
    got = {k: v["tag"] for k, v in d["keys"].items()}
    want = {"shell": "yours", "siteonly": "server", "onlysub": "server",
            "qmgrpath": "server", "cleared": "no value"}
    check(all(got.get(k) == v for k, v in want.items()),
          "remote mode tags %s" % {k: got.get(k) for k in want
                                   if got.get(k) != want[k]})
    d = w.draft("admin")
    got = {k: v["tag"] for k, v in d["keys"].items()}
    want = {"shell": "site (editing)", "siteonly": "site (editing)",
            "setup": "site (editing)", "onlysub": "site defaults",
            "vendoronly": "site defaults", "qmgrpath": "site defaults",
            "useronly": None, "libpath": None}
    ok = all(got.get(k) == v for k, v in want.items() if v)
    check(ok and "useronly" not in got and "libpath" not in got,
          "admin mode tags, no user layer %s" % {k: got.get(k) for k in want
                                                if got.get(k) != want[k]})
    d = w.draft("user", "set", "shell", "zsh")
    check("dirty 1" in d["lines"] and d["keys"]["shell"]["effective"] == "zsh"
          and d["keys"]["shell"]["tag"] == "yours", "an edit marks the draft dirty")
    d = w.draft("user", "set", "siteonly", "zsh", "set", "siteonly", "")
    check("dirty 0" in d["lines"], "an edit reverted is not dirty")
    d = w.draft("user", "set", "shell", "bash")
    check("dirty 0" in d["lines"], "typing the loaded value again is not dirty")
    d = w.draft("user", "clear", "nosuchkey", "clear", "onlysub")
    check("REFUSED clear nosuchkey" in d["lines"]
          and d["keys"]["onlysub"]["tag"] == "no value",
          "clear needs an inherited value")
    d = w.draft("user", "remove", "libpath", "clear", "perlpath")
    check(d["keys"]["libpath"]["effective"] is None
          and d["keys"]["libpath"]["tag"] == "default"
          and d["keys"]["perlpath"]["effective"] is None
          and d["keys"]["perlpath"]["tag"] == "no value",
          "remove and clear change the prediction and the tag")


HOSTILE = "+ & = % \" $ \\ ' `x` $(id)"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    args = ap.parse_args()
    perl = shutil.which("perl")
    t = Tools(args.build, perl)
    if not perl or not os.path.exists(t.edit) or not os.path.exists(t.dump):
        print("SKIP  needs perl, %s and %s" % (t.edit, t.dump))
        return SKIP
    tmp = tempfile.mkdtemp(prefix="ecce-cfgedit-")
    try:
        w = World(os.path.join(tmp, "json"), t)
        json_tests(t, w.explain())
        roundtrip(t, tmp)
        edits(t, tmp)
        tags_and_modes(t, tmp)

        three_way(t, tmp, "set existing keys", "user",
                  ["set", "shell", "zsh", "set", "sourcefile", "/u/env",
                   "set", "bothset", "mine"])
        three_way(t, tmp, "hostile values", "user",
                  ["set", "NewKey", HOSTILE, "set", "perlPath", HOSTILE,
                   "set", "brace", "a{b}", "set", "close", "}",
                   "set", "hash", "# not a comment",
                   "set", "multi", "l1 {\n  l2\nl3"])
        three_way(t, tmp, "use site value", "user",
                  ["remove", "shell", "remove", "setup", "remove", "bothset",
                   "remove", "cleared"])
        three_way(t, tmp, "use no value", "user",
                  ["clear", "perlpath", "clear", "sourcefile", "clear",
                   "siteonly", "clear", "onlysub", "clear",
                   "nwchemenvironment", "clear", "setup"])
        three_way(t, tmp, "set after clear", "user",
                  ["set", "frontendmachine", "login.user", "set", "cleared",
                   "back"])
        three_way(t, tmp, "block over one-liner", "user",
                  ["set", "setup", "first\nsecond", "set",
                   "nwchemenvironment", "A=9\nB=8"])
        three_way(t, tmp, "remote client edits user file", "remote",
                  ["set", "sourcefile", "/r/env", "clear", "frontendmachine"])
        w2 = three_way(t, tmp, "admin edits the site file", "admin",
                       ["set", "shell", "ksh", "set", "libpath", "/s/lib",
                        "clear", "qmgrpath", "remove", "siteonly",
                        "set", "setup", "site1\nsite2"])
        check(read(w2.userF) == USER, "admin: the user file is untouched")
        e = w2.rows(True)
        check(e["shell"]["value"] == "ksh" and e["shell"]["source"] == "site"
              and e.get("siteonly", {}).get("source") != "site",
              "admin: gensub sees the new site values (no user layer)")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("")
    print("FAILED: %d" % len(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
