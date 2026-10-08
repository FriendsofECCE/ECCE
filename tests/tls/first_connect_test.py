#!/usr/bin/env python3
"""Connecting to a server from the first-start window (#240), no GUI.

    first_connect_test.py <scratch dir>

`ecce-first-start --apply server:HOST:PORT` (the window's own connection step) against throwaway loopback https
servers: a certificate the system trusts (a private CA, given to curl as
SSL_CERT_FILE) writes no pin; a self-signed one is pinned as first presented;
a different certificate later is not accepted by that pin, and connecting
again pins the new one; plain http is the last resort; nothing answering
fails and writes nothing.  Exit 77 without openssl or curl.
"""
import http.server
import os
import shutil
import socket
import ssl
import subprocess
import sys
import threading

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SETUP = os.path.join(REPO, "packaging", "gateway", "ecce-first-start")
scratch = sys.argv[1]
if not shutil.which("openssl") or not shutil.which("curl"):
    sys.exit(77)
shutil.rmtree(scratch, ignore_errors=True)
os.makedirs(scratch)
failures = []


def check(ok, what):
    print(("PASS  " if ok else "FAIL  ") + what, flush=True)
    if not ok:
        failures.append(what)


def sh(*cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, timeout=60, **kw)


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


class Quiet(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, *a):
        pass


class Server(http.server.HTTPServer):
    def handle_error(self, request, client_address):
        pass


def serve(port, cert=None, key=None):
    srv = Server(("127.0.0.1", port), Quiet)
    if cert:
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_cert_chain(cert, key)
        srv.socket = ctx.wrap_socket(srv.socket, server_side=True)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


def cert(name, ca=None):
    key, crt = f"{scratch}/{name}.key", f"{scratch}/{name}.pem"
    san = "subjectAltName=DNS:localhost,IP:127.0.0.1"
    if ca is None:
        sh("openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "3",
           "-keyout", key, "-out", crt, "-subj", "/CN=localhost", "-addext", san)
    else:
        csr = f"{scratch}/{name}.csr"
        ext = f"{scratch}/{name}.ext"
        open(ext, "w").write(san + "\n")
        sh("openssl", "req", "-newkey", "rsa:2048", "-nodes", "-keyout", key,
           "-out", csr, "-subj", "/CN=localhost")
        sh("openssl", "x509", "-req", "-in", csr, "-CA", ca + ".pem", "-CAkey",
           ca + ".key", "-CAcreateserial", "-out", crt, "-days", "3", "-extfile", ext)
    return crt, key


ehome = scratch + "/ecce"
os.makedirs(ehome + "/siteconfig")
open(ehome + "/siteconfig/DataServers", "w").write(
    "<EcceData><EcceServer><Url>http://localhost:8096/Ecce</Url>"
    "<Desc>Local</Desc></EcceServer></EcceData>\n")


def connect(name, port, **extra):
    user = f"{scratch}/{name}"
    os.makedirs(user, exist_ok=True)
    env = dict(os.environ, ECCE_HOME=ehome, ECCE_REALUSERHOME=user, **extra)
    for v in ("SSL_CERT_FILE", "SSL_CERT_DIR"):
        if v not in extra:
            env.pop(v, None)
    r = sh(sys.executable, SETUP, "--apply", f"server:localhost:{port}", env=env)
    return r, user + "/.ECCE/RemoteServer"


# a private CA the "system" trusts, and a self-signed server
sh("openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "3",
   "-keyout", scratch + "/ca.key", "-out", scratch + "/ca.pem", "-subj", "/CN=Test CA")

p = free_port()
good = serve(p, *cert("signed", ca=scratch + "/ca"))
r, d = connect("trusted", p, SSL_CERT_FILE=scratch + "/ca.pem")
check(r.returncode == 0 and r.stdout.strip() == "trusted", "CA-signed: " + r.stdout + r.stderr)
check("https://localhost:%d/Ecce" % p in open(d + "/DataServers").read(),
      "CA-signed: DataServers names the https URL")
check(not os.path.exists(d + "/server.pem"), "CA-signed: no pin is written")
good.shutdown()

p = free_port()
first = serve(p, *cert("self1"))
r, d = connect("pinned", p)
check(r.returncode == 0 and r.stdout.strip() == "pinned", "self-signed: " + r.stdout + r.stderr)
check(os.path.exists(d + "/server.pem")
      and open(d + "/server.pem").read() == open(scratch + "/self1.pem").read(),
      "self-signed: the certificate as first presented is pinned")
first.shutdown()
first.server_close()

# the server now presents another certificate on the same port
second = serve(p, *cert("self2"))
pin = d + "/server.pem"
h = sh("bash", "-c", "openssl x509 -in %s -noout -pubkey | openssl pkey -pubin "
       "-outform der | openssl dgst -sha256 -binary | openssl enc -base64" % pin).stdout.strip()
r = sh("curl", "-s", "-o", "/dev/null", "--insecure", "--pinnedpubkey", "sha256//" + h,
       "https://localhost:%d/" % p)
check(r.returncode != 0, "a changed certificate is refused by the old pin (curl exit %d)"
      % r.returncode)
r, d2 = connect("pinned", p)           # "Change server" again: pins the new one
check(r.returncode == 0 and open(d2 + "/server.pem").read() == open(scratch + "/self2.pem").read(),
      "connecting again pins the new certificate")
second.shutdown()

p = free_port()
plain = serve(p)
r, d = connect("plain", p)
check(r.returncode == 0 and r.stdout.strip() == "plain", "plain http: " + r.stdout + r.stderr)
check("http://localhost:%d/Ecce" % p in open(d + "/DataServers").read(),
      "plain http: DataServers names the http URL")
check(not os.path.exists(d + "/server.pem"), "plain http: no pin")
plain.shutdown()
plain.server_close()

p = free_port()
r, d = connect("nothing", p)
check(r.returncode != 0 and not os.path.exists(d + "/DataServers"),
      "nothing answering: fails and writes nothing")

sys.exit(1 if failures else 0)
