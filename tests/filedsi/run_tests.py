#!/usr/bin/env python3
"""
FileEDSI, the data-storage backend that needs no Apache (#216).

    ./run_tests.py         build the driver and run it
    ./run_tests.py -v      echo the driver's own output

filedsiTest.C goes through EDSIFactory with a file:// URL against a
throwaway directory and checks metadata (the per-directory .ecce-meta
sidecar), append and ranged reads, and that copy, move and remove carry
the metadata with the resource.  No server of any kind is started.

Links the static libraries of build-cmake (ECCE_TEST_BUILD overrides) and
SKIPs when there is no build tree.  Exit status is 0 only if every check
passed (or the suite was skipped).
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "eccecipc", "ecceutil", "eccecomm", "eccercmd"]


def parse_types(lines):
    """(type, ext) pairs, lower-cased extensions."""
    pairs = set()
    for kind, exts in lines:
        for e in exts.split():
            pairs.add((kind, e.lower()))
    return pairs


def check_mime_table():
    """Every AddType in httpd.conf.ecce is in data/client/config/mimetypes
    and the reverse, so Apache and FileEDSI name a file the same way."""
    conf = open(os.path.join(REPO, "packaging", "dataserver",
                             "httpd.conf.ecce")).read().splitlines()
    apache = parse_types(
        m.groups() for m in (re.match(r"^\s*AddType\s+(\S+)\s+(.*\S)\s*$", l)
                             for l in conf) if m and m.group(2) != ".shtml")
    table = []
    for l in open(os.path.join(REPO, "data", "client", "config",
                               "mimetypes")).read().splitlines():
        if l.strip() and not l.startswith("#"):
            kind, _, exts = l.partition(" ")
            table.append((kind, exts))
    ours = parse_types(table)
    for what, diff in (("in httpd.conf.ecce but not in mimetypes", apache - ours),
                       ("in mimetypes but not in httpd.conf.ecce", ours - apache)):
        if diff:
            print("FAIL mime table: %s: %s" % (what, sorted(diff)))
            return False
    print("mime table: %d (type, extension) pairs agree" % len(ours))
    return True


def compile_url_driver(build, out):
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"), "-o", out,
            os.path.join(HERE, "urlTest.C"), "-L" + build]
           + ["-l" + l for l in LIBS] * 3 + ["-lxerces-c"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        print("could not build urlTest against %s:\n%s" % (build, proc.stderr[-2000:]))
        return False
    return True


def run_url(driver, env, *args, **extra):
    e = dict(env)
    e.pop("ECCE_LOCAL_DATA", None)
    e.update(extra)
    p = subprocess.run([driver] + list(args), capture_output=True, text=True,
                       env=e, timeout=60)
    return p.returncode, p.stdout + p.stderr


def check_urls(state, env):
    """EcceURL and EDSIServerCentral: file:// answers, and http unchanged.

    ECCE_TEST_BASELINE_BUILD names a build made before the file:// work
    (the main checkout's build-cmake will do); with it, every answer for an
    http URL, and every answer at all with ECCE_LOCAL_DATA unset, must equal
    the baseline's."""
    ok = True
    new = os.path.join(state, "urlTest")
    if not compile_url_driver(BUILD, new):
        return False
    root = "/data/local"
    _, unset = run_url(new, env, "url")
    _, local = run_url(new, env, "url", ECCE_LOCAL_DATA=root)

    def line(text, url, fn):
        for l in text.splitlines():
            if l.startswith(url + " " + fn + " "):
                return l.split(" ", 2)[2]
        return None

    expect = [
        ("file:///data/local", "isSystemFolder", "1"),
        ("file:///data/local/", "isSystemFolder", "1"),
        ("file:///data/local/proj", "isSystemFolder", "0"),
        ("file:///data/localx", "isSystemFolder", "0"),
        ("file:///data/local/proj", "getEcceRoot", "[file:///data/local]"),
        ("file:///data/local", "getEcceRoot", "[file:///data/local]"),
        ("file:///data/localx", "getEcceRoot", "[]"),
        ("file:///elsewhere/proj", "getEcceRoot", "[]"),
    ]
    bad = 0
    for url, fn, want in expect:
        got = line(local, url, fn)
        if got != want:
            print("FAIL file:// with ECCE_LOCAL_DATA: %s %s = %r, want %r"
                  % (url, fn, got, want))
            bad += 1
    for l in unset.splitlines():
        if l.startswith("file://") and l.split(" ", 2)[2] not in ("0", "[]"):
            print("FAIL file:// with ECCE_LOCAL_DATA unset changed: " + l)
            bad += 1
    print("urlTest: %d file:// expectations, %d failed" % (len(expect), bad))
    ok = ok and bad == 0

    base = os.environ.get("ECCE_TEST_BASELINE_BUILD")
    if base and all(os.path.exists(os.path.join(base, "lib%s.a" % l)) for l in LIBS):
        old = os.path.join(state, "urlTestOld")
        if compile_url_driver(base, old):
            _, was = run_url(old, env, "url")
            same_unset = was == unset
            http = lambda t: [l for l in t.splitlines() if l.startswith("http")]
            same_http = http(was) == http(local)
            print("urlTest vs baseline %s: unset-env output identical: %s; "
                  "http answers with ECCE_LOCAL_DATA set identical: %s"
                  % (base, same_unset, same_http))
            ok = ok and same_unset and same_http
    else:
        print("urlTest: no ECCE_TEST_BASELINE_BUILD, http-unchanged comparison skipped")

    # EDSIServerCentral: local mode with no siteconfig at all, and the
    # unchanged default path against the repository's own DataServers.
    empty_home = os.path.join(state, "emptyhome")
    os.makedirs(empty_home, exist_ok=True)
    # ECCE_HOME with the data directory but, deliberately, no siteconfig.
    if not os.path.exists(os.path.join(empty_home, "data")):
        os.symlink(os.path.join(REPO, "data"), os.path.join(empty_home, "data"))
    data = os.path.join(state, "localdata")
    rc, out = run_url(new, env, "server", data, ECCE_HOME=empty_home,
                      ECCE_LOCAL_DATA=data)
    print(out.rstrip())
    ok = ok and rc == 0 and os.path.isdir(data)
    rc, out = run_url(new, env, "server-default", ECCE_HOME=REPO)
    print(out.rstrip())
    ok = ok and rc == 0
    return ok


def main():
    verbose = "-v" in sys.argv[1:]
    mime_ok = check_mime_table()
    if not mime_ok:
        return 1
    if not all(os.path.exists(os.path.join(BUILD, "lib%s.a" % l)) for l in LIBS):
        print("SKIP: no built static libraries in %s (set ECCE_TEST_BUILD)"
              % BUILD)
        return 0
    state = tempfile.mkdtemp(prefix="filedsi.")
    try:
        env = dict(os.environ)
        env.setdefault("ECCE_HOME", REPO)
        home = os.path.join(state, "home")
        os.mkdir(home)
        env["ECCE_REALUSERHOME"] = home
        env["HOME"] = home
        env["ECCE_REALUSER"] = "tester"
        rc = 0
        # resourceTest runs twice: create, then re-open in a new process.
        for name, args in (("filedsiTest", []), ("resourceTest", ["create"]),
                           ("resourceTest", ["reopen"]), ("lockTest", [])):
            driver = os.path.join(state, name)
            if not os.path.exists(driver):
                cmd = (["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"),
                        "-o", driver, os.path.join(HERE, name + ".C"),
                        "-L" + BUILD] + ["-l" + l for l in LIBS] * 3
                       + ["-lxerces-c"])
                proc = subprocess.run(cmd, capture_output=True, text=True)
                if proc.returncode != 0:
                    print("could not build %s:\n%s" % (name, proc.stderr[-3000:]))
                    rc = 1
                    continue
            scratch = os.path.join(state, name + ".store")
            if not os.path.isdir(scratch):
                os.mkdir(scratch)
            proc = subprocess.run([driver] + args + [scratch],
                                  capture_output=True, text=True, env=env,
                                  timeout=120)
            lines = proc.stdout.splitlines()
            bad = [l for l in lines if l.startswith("FAIL ")]
            if verbose or bad or proc.returncode != 0:
                print(proc.stdout + proc.stderr[-2000:])
            print("%s %s: %d checks, %d failed" %
                  (name, " ".join(args), sum(1 for l in lines
                             if l.startswith(("PASS ", "FAIL "))), len(bad)))
            if proc.returncode != 0 or bad:
                rc = 1
        if not check_urls(state, env):
            rc = 1
        return rc
    finally:
        shutil.rmtree(state, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
