#!/usr/bin/env python3
"""
What a central server publishes of its siteconfig (#188, #192).

    publish_test.py [--build <build dir>]

The publishing step, packaging/dataserver/ecce-site-publish (run by
ecce-dataserver-start): the files, the MANIFEST, and the INDEX of sha256
sums that is written last and checked here against hashes made with hashlib.
Then, with the real Apache that ecce-dataserver-start starts: the folder is
refused without a login (401), readable with one (200), and not writable
over DAV.  The Apache half needs apache2 or httpd, htpasswd, curl and
<build dir>/ecce-flock, and is skipped with a note without them.

Exit status 77 (CTest SKIP) without bash.
"""

import argparse
import hashlib
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PK = os.path.join(REPO, "packaging")
START = os.path.join(PK, "dataserver", "ecce-dataserver-start")
PUBLISH = os.path.join(PK, "dataserver", "ecce-site-publish")

failures = []


def check(ok, what):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as h:
        h.write(text)


def read(path):
    try:
        with open(path) as h:
            return h.read()
    except OSError:
        return None


def sha(path):
    with open(path, "rb") as h:
        return hashlib.sha256(h.read()).hexdigest()


def parseIndex(path):
    out = {}
    for line in (read(path) or "").splitlines():
        if line and not line.startswith("#"):
            h, name = line.split("  ", 1)
            out[name] = h
    return out


def publish(home, dest):
    return subprocess.run([PUBLISH, dest], env=dict(os.environ, ECCE_HOME=home),
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)


def files(tmp):
    """The server's siteconfig, and what is published from it."""
    home = os.path.join(tmp, "server-home")
    for name, text in (("Machines", "m1\tm1.example.org\n"), ("Queues", "Queues: \n"),
                       ("submit.site", "SERVER_SUBMIT_SITE\n"),
                       ("QueueManagers", "SERVER_QUEUE_MANAGERS\n"),
                       ("StartupMessage", "Maintenance on Friday\n"),
                       ("NewUserMessage", "Welcome\n"),
                       ("CONFIG.m1", "NWChem: /srv/nwchem\n"), ("m1.Q", "Queues: q\n"),
                       ("CONFIG.bad name", "x\n"), (".hidden", "x\n"),
                       ("PublishDir", "/nowhere\n"), ("site_runtime", "x\n"),
                       ("DataServers", "<x/>\n")):
        write(os.path.join(home, "siteconfig", name), text)
    write(os.path.join(home, "siteconfig", "RemoteServer", "DataServers"), "<x/>\n")
    return home


def contents(home, dest):
    r = publish(home, dest)
    check(r.returncode == 0, "the publishing step ran: %s" % r.stdout[-200:])
    check("ecce-site-publish" in read(START), "ecce-dataserver-start runs the same step")
    manifest = (read(os.path.join(dest, "MANIFEST")) or "").split()
    idx = parseIndex(os.path.join(dest, "INDEX"))
    want = {"Machines", "Queues", "submit.site", "QueueManagers", "StartupMessage",
            "NewUserMessage", "CONFIG.m1", "m1.Q"}
    check(set(manifest) == want, "the MANIFEST lists exactly the site files and "
          "messages: %r" % sorted(manifest))
    check(set(idx) == set(manifest) and all(
        idx[n] == sha(os.path.join(dest, n)) for n in idx),
        "every INDEX line is the sha256 of the published file, and no file is missing")
    check(sorted(os.listdir(dest)) == sorted(manifest + ["MANIFEST", "INDEX"]),
          "nothing else is published (no PublishDir, DataServers, site_runtime, "
          "odd names, temporary files): %r" % sorted(os.listdir(dest)))
    mt = {n: os.stat(os.path.join(dest, n)).st_mtime_ns for n in os.listdir(dest)}
    check(mt["INDEX"] >= max(mt.values()), "INDEX is written last")
    check(all(os.stat(os.path.join(dest, n)).st_mode & 0o044 == 0o044
              for n in os.listdir(dest)), "published files are readable by the web server")

    first = read(os.path.join(dest, "INDEX"))
    time.sleep(1.1)
    publish(home, dest)
    check(read(os.path.join(dest, "INDEX")) == first,
          "publishing the same files again leaves the INDEX as it was")
    write(os.path.join(home, "siteconfig", "CONFIG.m1"), "NWChem: /srv/other\n")
    os.remove(os.path.join(home, "siteconfig", "m1.Q"))
    publish(home, dest)
    idx = parseIndex(os.path.join(dest, "INDEX"))
    check(idx.get("CONFIG.m1") == hashlib.sha256(b"NWChem: /srv/other\n").hexdigest()
          and "m1.Q" not in idx and not os.path.exists(os.path.join(dest, "m1.Q")),
          "a changed file and a removed one are both in the new INDEX")
    check(read(os.path.join(dest, "INDEX")) != first, "and the INDEX itself changed")
    write(os.path.join(home, "siteconfig", "m1.Q"), "Queues: q\n")
    publish(home, dest)


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def apache(tmp, build):
    """The real Apache of ecce-dataserver-start, with one account."""
    flock = os.path.join(build, "ecce-flock")
    have = any(shutil.which(c) or os.path.exists("/usr/sbin/" + c)
               for c in ("apache2", "httpd"))
    if not (have and shutil.which("curl") and (shutil.which("htpasswd") or
            os.path.exists("/usr/bin/htpasswd")) and os.path.exists(flock)):
        print("NOTE  no apache2/httpd, htpasswd, curl or ecce-flock: the login checks "
              "are skipped")
        return
    home = os.path.join(tmp, "apache-home")
    os.makedirs(home + "/bin")
    os.makedirs(home + "/server/httpd-conf")
    for d in ("dataserver", "gateway"):
        for f in os.listdir(os.path.join(PK, d)):
            if f.startswith("ecce-") or f.endswith(".sh"):
                os.symlink(os.path.join(PK, d, f), os.path.join(home, "bin", f))
    os.symlink(flock, home + "/bin/ecce-flock")
    os.symlink(os.path.join(REPO, "data"), home + "/data")
    shutil.copytree(os.path.join(REPO, "siteconfig"), home + "/siteconfig")
    shutil.copy(PK + "/dataserver/httpd.conf.ecce", home + "/server/httpd-conf/")
    acct = os.path.join(tmp, "apache-account")
    os.makedirs(acct)
    port = free_port()
    env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=acct, ECCE_SESSION_ID="s1",
               ECCE_REALUSER="pubuser", HOST="pubhost", ECCE_DATASERVER_PORT=str(port))
    for k in ("ECCE_DATASERVER_LISTEN", "ECCE_DATASERVER_TLS", "ECCE_REMOTE_SERVER"):
        env.pop(k, None)

    def run(cmd):
        return subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=120)

    try:
        r = run([home + "/bin/ecce-dataserver-start"])
        if r.returncode != 0:
            check(False, "ecce-dataserver-start: " + r.stderr[-300:])
            return
        run([home + "/bin/ecce-dataserver-adduser", "-b", "pubuser", "pubpw", "Pub", "User"])
        base = "http://127.0.0.1:%d/Ecce/system/siteconfig/" % port

        def curl(name, *a):
            r = subprocess.run(["curl", "-s", "-o", "/dev/null", "-w", "%{http_code}"]
                               + list(a) + [base + name], capture_output=True, text=True)
            return r.stdout.strip()

        check(os.path.exists(acct + "/.ECCE/dataserver/htdocs/Ecce/system/siteconfig/INDEX"),
              "ecce-dataserver-start published")
        for name in ("INDEX", "MANIFEST", "Machines"):
            check(curl(name) == "401", "%s without a login: 401" % name)
            check(curl(name, "-u", "pubuser:pubpw") == "200", "%s with a login: 200" % name)
        check(curl("INDEX", "-u", "pubuser:wrong") == "401", "a wrong password: 401")
        out = subprocess.run(["curl", "-s", "-u", "pubuser:pubpw", base + "INDEX"],
                             capture_output=True, text=True).stdout
        check(out == read(acct + "/.ECCE/dataserver/htdocs/Ecce/system/siteconfig/INDEX"),
              "the INDEX served is the INDEX published")
        code = curl("evil", "-u", "pubuser:pubpw", "-X", "PUT", "--data", "x")
        check(code in ("403", "405"), "a login cannot write there over DAV (%s)" % code)
        check(not os.path.exists(acct + "/.ECCE/dataserver/htdocs/Ecce/system/siteconfig/evil"),
              "and no file appeared")
        code = curl("Machines", "-u", "pubuser:pubpw", "-X", "DELETE")
        check(code in ("403", "405"), "nor delete (%s)" % code)
        anon = subprocess.run(["curl", "-s", "-o", "/dev/null", "-w", "%{http_code}",
                               "http://127.0.0.1:%d/Ecce/system/" % port],
                              capture_output=True, text=True).stdout.strip()
        # the client's copy (ecce-remote-setup) needs a login
        chome = os.path.join(tmp, "client-home")
        write(os.path.join(chome, "siteconfig", "DataServers"), "http://x:1/Ecce/system\n")
        write(os.path.join(chome, "siteconfig", "Machines"), "")
        cenv = dict(os.environ, ECCE_HOME=chome)
        r = subprocess.run(["bash", os.path.join(PK, "dataserver", "ecce-remote-setup"),
                            "127.0.0.1", str(port)], env=cenv, capture_output=True, text=True)
        check(not os.path.exists(os.path.join(chome, "siteconfig", "RemoteServer", "MANIFEST")),
              "ecce-remote-setup without a login copies nothing")
        cenv.update(ECCE_SETUP_PASSWORD="pubpw")
        r = subprocess.run(["bash", os.path.join(PK, "dataserver", "ecce-remote-setup"),
                            "127.0.0.1", str(port), "--login", "pubuser"], env=cenv,
                           capture_output=True, text=True)
        want = read(acct + "/.ECCE/dataserver/htdocs/Ecce/system/siteconfig/Machines")
        check(r.returncode == 0 and want and
              read(os.path.join(chome, "siteconfig", "Machines")) == want,
              "with --login it copies the server's Machines: " + r.stdout[-200:])
        check(anon in ("200", "301", "404"), "the rest of /Ecce/system is not behind a login "
              "(%s)" % anon)
    finally:
        run([home + "/bin/ecce-dataserver-stop"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    args = ap.parse_args()
    if not shutil.which("bash"):
        print("SKIP  needs bash")
        return 77
    tmp = tempfile.mkdtemp(prefix="ecce-publish-")
    try:
        home = files(tmp)
        dest = os.path.join(tmp, "dataroot", "Ecce", "system", "siteconfig")
        contents(home, dest)
        apache(tmp, args.build)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("")
    print("FAILED: %d" % len(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
