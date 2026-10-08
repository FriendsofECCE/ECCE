#!/usr/bin/env python3
"""
The server half of "ecce -admin" on a -remote client (#234): ecce-site-admin
applies a request to a central server's siteconfig and publishes it, and
the INDEX follows it, and "ecce-remote-setup --refresh" brings the client's copy up to date.

    siteadmin_test.py --build <build dir>

The server is simulated on this machine (central_server.py).  The request
files are written here from the format, not by the C++ encoder, and the
results are read back with gensub's GENSUB_EXPLAIN, which neither side of
the transfer uses.  The window's side (the same steps driven through
Register Machines) is in tests/machregister/gui_test.py.

Exit status 77 (CTest SKIP) without perl, curl or the ecce-site-admin binary.
"""

import argparse
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from central_server import (CentralServer, digest, form, make_client,  # noqa
                            request, write, REPO)

failures = []


def check(ok, what):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def sha(text):
    import hashlib
    return hashlib.sha256(text.encode()).hexdigest()


def index(published):
    out = {}
    for line in (read(os.path.join(published, "INDEX")) or "").splitlines():
        if line and not line.startswith("#"):
            h, name = line.split(None, 1)
            out[name] = h
    return out


def read(path):
    try:
        with open(path) as h:
            return h.read()
    except OSError:
        return None


BASE = {"type": "accept", "siteconfig": "true", "config": "external",
        "machine": "cluster.example.org", "name": "cluster",
        "vendor": "Unspecified", "model": "Unspecified",
        "processor": "Unspecified", "procs": "8", "nodes": "1", "ssh": "true",
        "registeredcodes": "NWChem", "NWChem": "/srv/nwchem",
        "AA": "false", "qmgr": "None", "numQueues": "0"}


def hostile(canary):
    return {
        "perlPath": "/p+a&t=h%%20 $(touch %s) `touch %s`" % (canary, canary),
        "sourceFile": "/x'y\"z;touch %s;\\" % canary,
        "NWChemCommand": "nwchem 'in put' \"$HOME\" > out; touch %s" % canary,
        "setup": "export A='1 2'\n  echo \"$(touch %s)\"\n  # %%XX + & =" % canary,
    }


def apply(server, req, env):
    path = os.path.join(server.root, "request")
    write(path, req)
    return subprocess.run(["ecce-site-admin", "apply", path], env=env,
                          cwd=server.root, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True)


def explain(server, name):
    params = os.path.join(server.root, "params")
    write(params, " -H %s\n -Q Shell\n -c NWChem\n -d localhost\n -n 1\n"
                  " -N 1\n -r %s\n -i a\n -o a\n -f %s/submit__x\n"
                  % (name, server.root, server.root))
    nobody = os.path.join(server.root, "nouser")
    os.makedirs(nobody, exist_ok=True)
    r = subprocess.run(["perl", os.path.join(REPO, "scripts", "gensub"), "-p",
                        params], env=dict(os.environ, GENSUB_EXPLAIN="1",
                                          ECCE_HOME=server.home,
                                          ECCE_REALUSERHOME=nobody),
                       cwd=server.root, stdout=subprocess.PIPE, text=True)
    return {row["key"]: row["value"] for row in
            map(json.loads, r.stdout.splitlines())}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    a = ap.parse_args()
    build = os.path.abspath(a.build)
    if not os.access(os.path.join(build, "ecce-site-admin"), os.X_OK):
        print("SKIP  no ecce-site-admin in " + build)
        return 77
    if not shutil.which("perl") or not shutil.which("curl"):
        print("SKIP  needs perl and curl")
        return 77
    if os.geteuid() == 0:
        print("SKIP  as root every directory is writable")
        return 77

    tmp = tempfile.mkdtemp(prefix="ecce-siteadmin-")
    try:
        s = CentralServer(tmp, build)
        canary = os.path.join(tmp, "PWNED")
        env = s.client_env(os.environ)
        check(s.publish().returncode == 0, "the server's start publishes")
        write(os.path.join(s.sc, "CONFIG.gone"), "NWChem: /old\n")
        s.publish()
        check(os.path.exists(os.path.join(s.published, "CONFIG.gone")),
              "CONFIG.gone is published")

        # the account that runs the data server publishes in place, so a
        # group-writable directory keeps its mode across restarts
        os.chmod(s.published, 0o2775)

        # -- the client's first copy
        port = s.serve()
        client = os.path.join(tmp, "client-home")
        csc = make_client(client, port)
        r = subprocess.run([os.path.join(client, "bin", "ecce-remote-setup"),
                            "--refresh"], env=dict(os.environ, ECCE_HOME=client),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True)
        check(r.returncode == 0 and read(os.path.join(csc, "CONFIG.gone")) ==
              "NWChem: /old\n", "--refresh copies the server's list: " +
              r.stdout[-300:])
        check(index(s.published).get("CONFIG.gone") == sha("NWChem: /old\n"),
              "the INDEX lists CONFIG.gone with its hash")

        # -- an administrator's save, with values a shell would act on
        values = hostile(canary)
        edits = [("set", k, v) for k, v in values.items()] + \
                [("clear", "shell"), ("remove", "frontendBypass")]
        write(os.path.join(s.sc, "CONFIG.cluster"),
              "# the site's cluster\nshell: tcsh\nfrontendBypass: .site.org\n",
              0o664)
        os.remove(os.path.join(s.sc, "CONFIG.gone"))
        r = apply(s, request("cluster", form(BASE), edits), env)
        check(r.returncode == 0 and r.stdout.rstrip().endswith(
              "ecce-site-admin: ok"), "an admin's save is applied: " + r.stdout)
        check(not os.path.exists(canary), "no value was run by a shell")
        got = explain(s, "cluster")
        bad = {k: (v, got.get(k.lower())) for k, v in values.items()
               if got.get(k.lower()) != v}
        check(not bad, "every hostile value reads back unchanged: %r" % bad)
        check(got.get("shell") is None and "frontendbypass" not in got,
              "clear and remove reach the server's file")
        cfg = read(os.path.join(s.sc, "CONFIG.cluster")) or ""
        check(cfg.startswith("# the site's cluster\n") and "shell: -" in cfg,
              "the comment is kept and the cleared key reads '-'")
        check("cluster\tcluster.example.org" in read(os.path.join(s.sc, "Machines")),
              "processmachine wrote the Machines line")
        check(read(os.path.join(s.published, "CONFIG.cluster")) == cfg,
              "the new CONFIG.cluster is published")
        check(not os.path.exists(os.path.join(s.published, "CONFIG.gone")),
              "a removed file is no longer published")
        check(stat.S_IMODE(os.stat(s.published).st_mode) == 0o2775,
              "the published directory keeps its group mode")
        mode = stat.S_IMODE(os.stat(os.path.join(s.published, "CONFIG.cluster")).st_mode)
        check(mode & 0o444 == 0o444, "published files are readable by the web server")
        check(stat.S_IMODE(os.stat(os.path.join(s.sc, "CONFIG.cluster")).st_mode)
              & 0o020, "the site file stays writable by the group")

        r = subprocess.run([os.path.join(client, "bin", "ecce-remote-setup"),
                            "--refresh"], env=dict(os.environ, ECCE_HOME=client),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True)
        check(r.returncode == 0 and read(os.path.join(csc, "CONFIG.cluster")) == cfg,
              "--refresh brings the client's copy up to date")
        check(not os.path.exists(os.path.join(csc, "CONFIG.gone")),
              "--refresh removes a CONFIG the server no longer publishes")
        idx = index(s.published)
        check(idx.get("CONFIG.cluster") == sha(cfg) and "CONFIG.gone" not in idx,
              "the INDEX follows the admin's save and the removal")

        # -- a raw edit: applied only if the server's file is the base
        r = apply(s, request("cluster", text="NWChem: /raw\n", base="stale\n"), env)
        check(r.returncode != 0 and "was changed on the server" in r.stdout and
              read(os.path.join(s.sc, "CONFIG.cluster")) == cfg,
              "a raw edit from a stale copy is refused")
        r = apply(s, request("cluster", text="NWChem: /raw $(x)\n", base=cfg), env)
        check(r.returncode == 0 and read(os.path.join(s.sc, "CONFIG.cluster")) ==
              "NWChem: /raw $(x)\n", "a raw edit from the current copy is written")

        # -- refused requests change nothing
        before = (digest(s.sc), digest(s.published))
        for label, req in (
                ("a name with a slash", request("../x", form(dict(BASE, name="../x")))),
                ("a form for another machine", request("cluster", form(dict(BASE, name="other")))),
                ("a user-mode form", request("cluster", form(dict(BASE, siteconfig="false")))),
                ("a key that is not a key", request("cluster", None, [("set", "a\nb", "v")])),
                ("a truncated request", request("cluster", form(BASE))[:-4])):
            r = apply(s, req, env)
            check(r.returncode != 0 and "ecce-site-admin: error:" in r.stdout,
                  "%s is refused: %s" % (label, r.stdout.strip()[-120:]))
        check((digest(s.sc), digest(s.published)) == before,
              "refused requests change nothing")

        # -- not an administrator: siteconfig not writable for this login
        os.chmod(s.sc, 0o555)
        try:
            r = apply(s, request("cluster", form(BASE), [("set", "NWChem", "/no")]), env)
            check(r.returncode != 0 and "cannot change the site settings" in r.stdout
                  and "GETTING_STARTED" in r.stdout,
                  "a login that may not write siteconfig is told why: " +
                  r.stdout.strip())
            c = subprocess.run(["ecce-site-admin", "check"], env=env,
                               stdout=subprocess.PIPE, text=True)
            check(c.returncode != 0 and "not writable by its group" in c.stdout,
                  "check says the same: " + c.stdout.strip())
        finally:
            os.chmod(s.sc, 0o755)
        check((digest(s.sc), digest(s.published)) == before,
              "nothing changed for the non-administrator")
        os.chmod(os.path.join(s.sc, "Machines"), 0o444)
        r = apply(s, request("cluster", form(BASE)), env)
        check(r.returncode != 0 and "Machines" in r.stdout,
              "a site file that is not writable is named: " + r.stdout.strip())
        os.chmod(os.path.join(s.sc, "Machines"), 0o664)
        c = subprocess.run(["ecce-site-admin", "check"], env=env,
                           stdout=subprocess.PIPE, text=True)
        check(c.returncode == 0, "check passes for an administrator")

        # -- an administrator who is not the data server's account
        own = os.path.join(tmp, "other-admin")
        os.makedirs(own)
        r = subprocess.run([os.path.join(s.home, "bin", "ecce-site-publish")],
                           env=dict(os.environ, ECCE_HOME=s.home,
                                    ECCE_REALUSERHOME=own),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True)
        check(r.returncode != 0 and "PublishDir" in r.stdout,
              "without a data server of its own, publishing asks for PublishDir")
        write(os.path.join(s.sc, "PublishDir"), "# the ecce account's\n%s\n"
              % s.published)
        write(os.path.join(s.sc, "CONFIG.cluster"), "NWChem: /via-publishdir\n")
        r = subprocess.run([os.path.join(s.home, "bin", "ecce-site-publish")],
                           env=dict(os.environ, ECCE_HOME=s.home,
                                    ECCE_REALUSERHOME=own),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True)
        check(r.returncode == 0 and read(os.path.join(s.published, "CONFIG.cluster"))
              == "NWChem: /via-publishdir\n", "PublishDir names where to publish")
        check("PublishDir" not in read(os.path.join(s.published, "MANIFEST")),
              "PublishDir itself is not published")
    finally:
        try:
            s.stop()
        except Exception:
            pass
        for d, dirs, _ in os.walk(tmp):
            for n in dirs:
                try:
                    os.chmod(os.path.join(d, n), 0o755)
                except OSError:
                    pass
        shutil.rmtree(tmp, ignore_errors=True)
    print("")
    print("FAILED: %d" % len(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
