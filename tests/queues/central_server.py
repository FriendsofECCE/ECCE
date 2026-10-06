"""
A central ECCE server simulated on this machine, for "ecce -admin" on a
-remote client (#234).

The server is a second ECCE_HOME (its siteconfig) and a second
ECCE_REALUSERHOME (the account that runs the data server, whose
~/.ECCE/dataserver/htdocs is what Apache would serve).  A plain HTTP server
serves that htdocs tree to the client's ecce-remote-setup, as Apache does.

The client reaches the server through the same RCommand path as over ssh,
but ssh to localhost needs a key login, so the server is named 127.0.0.1 with
no login: RCommand then runs the commands directly (DirectTransport), and
`ecce-site-admin` on PATH is a wrapper that switches to the server's
ECCE_HOME and ECCE_REALUSERHOME, which an ssh login would get from the
installed wrapper and the server account's home.
"""

import functools
import hashlib
import http.server
import os
import shutil
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PACKAGING = os.path.join(REPO, "packaging", "dataserver")

UNRESERVED = set(b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                 b"0123456789-_.~")


def pct(text):
    """ProcessMachine::encode, written again here."""
    return "".join(chr(b) if b in UNRESERVED else "%%%02X" % b
                   for b in text.encode())


def request(machine, form=None, edits=(), text=None, base=None):
    """A SiteRequest file, written from the format, not by the C++ encoder.
    edits: (op, key[, value]) with op set/clear/remove."""
    out = ["ecce-site-request 1", "machine " + pct(machine)]
    if form is not None:
        out.append("form " + pct(form))
    for e in edits:
        out.append(" ".join([e[0]] + [pct(x) for x in e[1:]]))
    if text is not None:
        out.append("text " + pct(text))
        out.append("base " + pct(base or ""))
    out.append("end")
    return "\n".join(out) + "\n"


def form(fields):
    return "&".join(pct(k) + "=" + pct(v) for k, v in fields.items())


def digest(root):
    h = hashlib.sha256()
    for d, _, files in sorted(os.walk(root)):
        for n in sorted(files):
            p = os.path.join(d, n)
            h.update(p.encode())
            with open(p, "rb") as f:
                h.update(f.read())
    return h.hexdigest()


def write(path, text, mode=None):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)
    if mode is not None:
        os.chmod(path, mode)


class CentralServer:
    def __init__(self, root, build, siteconfig=None):
        self.root = root
        self.home = os.path.join(root, "server-home")       # its ECCE_HOME
        self.account = os.path.join(root, "server-account")  # runs the data server
        self.sc = os.path.join(self.home, "siteconfig")
        self.htdocs = os.path.join(self.account, ".ECCE", "dataserver", "htdocs")
        self.published = os.path.join(self.htdocs, "Ecce", "system", "siteconfig")
        self.path = os.path.join(root, "server-path")
        os.makedirs(os.path.join(self.home, "bin"))
        os.makedirs(os.path.join(self.htdocs, "Ecce", "system"))
        os.makedirs(self.path)
        os.symlink(os.path.join(REPO, "scripts"), os.path.join(self.home, "scripts"))
        os.symlink(os.path.join(PACKAGING, "ecce-site-publish"),
                   os.path.join(self.home, "bin", "ecce-site-publish"))
        shutil.copytree(siteconfig or os.path.join(REPO, "siteconfig"), self.sc)
        # what an ssh login runs: the installed wrapper sets ECCE_HOME
        write(os.path.join(self.path, "ecce-site-admin"),
              "#!/bin/sh\nECCE_HOME='%s' ECCE_REALUSERHOME='%s' exec '%s' \"$@\"\n"
              % (self.home, self.account, os.path.join(build, "ecce-site-admin")),
              0o755)
        self.httpd = None

    def publish(self):
        """What ecce-dataserver-start does on every start."""
        import subprocess
        return subprocess.run(
            [os.path.join(PACKAGING, "ecce-site-publish"), self.published],
            env=dict(os.environ, ECCE_HOME=self.home,
                     ECCE_REALUSERHOME=self.account),
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    def serve(self):
        class Quiet(http.server.SimpleHTTPRequestHandler):
            def log_message(self, *args):
                pass
        handler = functools.partial(Quiet, directory=self.htdocs)
        self.httpd = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
        threading.Thread(target=self.httpd.serve_forever, daemon=True).start()
        return self.httpd.server_address[1]

    def stop(self):
        if self.httpd:
            self.httpd.shutdown()
            self.httpd.server_close()
            self.httpd = None

    def client_env(self, env):
        """PATH with the server's ecce-site-admin first."""
        env = dict(env)
        env["PATH"] = self.path + os.pathsep + env.get("PATH", "")
        return env


def make_client(home, port, repo_siteconfig=None):
    """A -remote client's ECCE_HOME pointed at the server on 127.0.0.1:port,
    with ecce-remote-setup in bin/ as the package installs it."""
    os.makedirs(os.path.join(home, "bin"), exist_ok=True)
    link = os.path.join(home, "bin", "ecce-remote-setup")
    if not os.path.lexists(link):
        os.symlink(os.path.join(PACKAGING, "ecce-remote-setup"), link)
    sc = os.path.join(home, "siteconfig")
    if not os.path.isdir(sc):
        shutil.copytree(repo_siteconfig or os.path.join(REPO, "siteconfig"), sc)
    ds = open(os.path.join(REPO, "siteconfig", "DataServers")).read()
    import re
    ds = re.sub(r"http://[^:/]*:[0-9]*/Ecce", "http://127.0.0.1:%d/Ecce" % port, ds)
    write(os.path.join(sc, "RemoteServer", "DataServers"), ds)
    return sc
