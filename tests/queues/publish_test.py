#!/usr/bin/env python3
"""
What a central server publishes of its siteconfig, and what a -remote client
takes from it.

    publish_test.py

The publishing step, packaging/dataserver/ecce-site-publish (run by
ecce-dataserver-start, so Apache is not needed here), then the client
side, packaging/dataserver/ecce-remote-setup, runs against a plain HTTP
server holding that output: once with a server that publishes submit.site and
QueueManagers, once with an older server that does not, which must leave the
client's own copies in place.

Exit status 77 (CTest SKIP) without curl or bash.
"""

import functools
import http.server
import os
import shutil
import subprocess
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
START = os.path.join(REPO, "packaging", "dataserver", "ecce-dataserver-start")
SETUP = os.path.join(REPO, "packaging", "dataserver", "ecce-remote-setup")
PUBLISH = os.path.join(REPO, "packaging", "dataserver", "ecce-site-publish")

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


def publishLoop(tmp):
    """Run the server's publishing step (ecce-site-publish, which
    ecce-dataserver-start runs on every start); return its directory."""
    home = os.path.join(tmp, "server-home")
    dataroot = os.path.join(tmp, "dataroot")
    for name, text in (("Machines", "m1\tm1.example.org\n"), ("Queues", "Queues: \n"),
                       ("submit.site", "SERVER_SUBMIT_SITE\n"),
                       ("QueueManagers", "SERVER_QUEUE_MANAGERS\n"),
                       ("CONFIG.m1", "NWChem: /srv/nwchem\n"), ("m1.Q", "Queues: q\n"),
                       ("DataServers", "<x/>\n")):
        write(os.path.join(home, "siteconfig", name), text)
    dest = os.path.join(dataroot, "Ecce", "system", "siteconfig")
    r = subprocess.run([PUBLISH, dest], env=dict(os.environ, ECCE_HOME=home),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    check(r.returncode == 0, "the publishing step ran: %s" % r.stdout[-200:])
    check("ecce-site-publish" in read(START),
          "ecce-dataserver-start runs the same step")
    return dest


def client(tmp, published, label, manifest=None):
    """Run ecce-remote-setup against a server serving `published`."""
    root = os.path.join(tmp, "www-" + label)
    served = os.path.join(root, "Ecce", "system", "siteconfig")
    shutil.copytree(published, served)
    if manifest is not None:
        write(os.path.join(served, "MANIFEST"), manifest)
    class Quiet(http.server.SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    handler = functools.partial(Quiet, directory=root)
    httpd = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    try:
        home = os.path.join(tmp, "client-" + label)
        for name, text in (("DataServers", "http://x:1/Ecce/system\n"),
                           ("submit.site", "CLIENT_SUBMIT_SITE\n"),
                           ("QueueManagers", "CLIENT_QUEUE_MANAGERS\n"),
                           ("Machines", "")):
            write(os.path.join(home, "siteconfig", name), text)
        r = subprocess.run(
            ["bash", SETUP, "127.0.0.1", str(httpd.server_address[1])],
            env=dict(os.environ, ECCE_HOME=home), stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True)
        check(r.returncode == 0, "%s: ecce-remote-setup ran%s"
              % (label, "" if r.returncode == 0 else ": " + r.stdout[-300:]))
        return os.path.join(home, "siteconfig")
    finally:
        httpd.shutdown()


def main():
    if not shutil.which("curl") or not shutil.which("bash"):
        print("SKIP  needs curl and bash")
        return 77
    tmp = tempfile.mkdtemp(prefix="ecce-publish-")
    try:
        published = publishLoop(tmp)
        manifest = (read(os.path.join(published, "MANIFEST")) or "").split()
        check("submit.site" in manifest and "QueueManagers" in manifest,
              "the server publishes submit.site and QueueManagers")
        check("Machines" in manifest and "CONFIG.m1" in manifest
              and "m1.Q" in manifest, "the server still publishes the rest")

        site = client(tmp, published, "new")
        check(read(os.path.join(site, "submit.site")) == "SERVER_SUBMIT_SITE\n",
              "the client takes the server's submit.site")
        check(read(os.path.join(site, "QueueManagers")) == "SERVER_QUEUE_MANAGERS\n",
              "the client takes the server's QueueManagers")
        check(read(os.path.join(site, "CONFIG.m1")) == "NWChem: /srv/nwchem\n",
              "the client takes the server's CONFIG.m1")
        backup = os.path.join(site, "local-machines.orig")
        check(read(os.path.join(backup, "submit.site")) == "CLIENT_SUBMIT_SITE\n",
              "the client's own submit.site is backed up")

        # An older server: same files, but a MANIFEST that predates the two.
        old = "".join(n + "\n" for n in manifest
                      if n not in ("submit.site", "QueueManagers"))
        site = client(tmp, published, "old", manifest=old)
        check(read(os.path.join(site, "submit.site")) == "CLIENT_SUBMIT_SITE\n",
              "an older server leaves the client's submit.site alone")
        check(read(os.path.join(site, "QueueManagers")) == "CLIENT_QUEUE_MANAGERS\n",
              "an older server leaves the client's QueueManagers alone")
        check(read(os.path.join(site, "CONFIG.m1")) == "NWChem: /srv/nwchem\n",
              "an older server's other files are still copied")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("")
    print("FAILED: %d" % len(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
