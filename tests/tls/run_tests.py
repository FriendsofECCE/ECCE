#!/usr/bin/env python3
"""https data-server client test (#236, step 1).

    run_tests.py <tls_client_test binary> <scratch dir>

Starts throwaway loopback servers (python http.server, TLS on its own thread)
with self-signed certificates generated here, and runs tls_client_test against
them.  Exit 77 (SKIP) without the openssl command line.
"""
import http.server
import os
import shutil
import ssl
import subprocess
import sys
import threading

exe, scratch = sys.argv[1], sys.argv[2]
if not shutil.which("openssl"):
    sys.exit(77)

shutil.rmtree(scratch, ignore_errors=True)
os.makedirs(scratch)


def make_cert(name, san):
    key, crt = f"{scratch}/{name}.key", f"{scratch}/{name}.pem"
    subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                    "-keyout", key, "-out", crt, "-days", "3",
                    "-subj", f"/CN={name}", "-addext", f"subjectAltName={san}"],
                   check=True, capture_output=True)
    return key, crt


certA = make_cert("A", "DNS:localhost,IP:127.0.0.1")
certB = make_cert("B", "DNS:localhost,IP:127.0.0.1")
certC = make_cert("C", "DNS:other.example")

hosts = []


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_HEAD(self):
        hosts.append(self.headers.get("Host"))
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.send_header("Connection", "close")
        self.end_headers()

    def log_message(self, *a):
        pass


class Server(http.server.ThreadingHTTPServer):
    def handle_error(self, request, client_address):
        pass


def serve(cert=None):
    srv = Server(("127.0.0.1", 0), Handler)
    if cert:
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_cert_chain(cert[1], cert[0])
        srv.socket = ctx.wrap_socket(srv.socket, server_side=True)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv.server_address[1]


def home(name, pin=None):
    h = f"{scratch}/home-{name}"
    os.makedirs(f"{h}/siteconfig/RemoteServer")
    if pin:
        shutil.copy(pin[1], f"{h}/siteconfig/RemoteServer/server.pem")
    return h


def run(url, expect, ecce_home, extra_env=None):
    env = dict(os.environ, ECCE_HOME=ecce_home)
    env.pop("SSL_CERT_FILE", None)
    env.pop("SSL_CERT_DIR", None)
    env.update(extra_env or {})
    r = subprocess.run([exe, url, expect], env=env, capture_output=True,
                       text=True, timeout=60)
    print(("PASS " if r.returncode == 0 else "FAIL ") + r.stdout.strip(),
          r.stderr.strip())
    return r.returncode == 0


ports = {"A": serve(certA), "B": serve(certB), "C": serve(certC),
         "plain": serve()}
ok = True
nohome = home("none")
a = f"https://127.0.0.1:{ports['A']}/"

# pinned: the exact certificate matches (works for an IP address)
ok &= run(a, "ok", home("pinA", certA))
# pinned: a different certificate is refused
ok &= run(f"https://127.0.0.1:{ports['B']}/", "cert", home("pinA2", certA))
# pinned: a pin file that is not a certificate is refused, no fallback
bad = home("bad")
open(f"{bad}/siteconfig/RemoteServer/server.pem", "w").write("garbage\n")
ok &= run(a, "cert", bad)
# CA mode: a self-signed certificate is not trusted
ok &= run(a, "cert", nohome)
# CA mode with a trusted certificate: host name matches
ok &= run(f"https://localhost:{ports['A']}/", "ok", nohome,
          {"SSL_CERT_FILE": certA[1]})
# CA mode: trusted certificate for another host name is refused
ok &= run(f"https://127.0.0.1:{ports['C']}/", "cert", nohome,
          {"SSL_CERT_FILE": certC[1]})
# https to a plain-http port never falls back to http
ok &= run(f"https://127.0.0.1:{ports['plain']}/", "fail", home("pinA3", certA))
# plain http is unchanged, and the Host header keeps its port
n = len(hosts)
ok &= run(f"http://127.0.0.1:{ports['plain']}/", "ok", nohome)
ok &= hosts[n:] == [f"127.0.0.1:{ports['plain']}"]
if hosts[n:] != [f"127.0.0.1:{ports['plain']}"]:
    print("FAIL plain Host header", hosts[n:])
# https non-default port keeps the port in Host
n = len(hosts)
run(a, "ok", home("pinA4", certA))
if hosts[n:] != [f"127.0.0.1:{ports['A']}"]:
    print("FAIL https Host header", hosts[n:])
    ok = False

sys.exit(0 if ok else 1)
