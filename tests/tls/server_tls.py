#!/usr/bin/env python3
"""The central server over TLS (#236): data server and broker.

    server_tls.py <tls_client_test> <tls_broker_test> <build dir> <scratch>

Runs the real ecce-remote-setup, ecce-dataserver-start and ecce-gateway-start
from packaging/ as one throwaway account with a fresh self-signed certificate,
then drives the clients: https with the right pin, with a wrong one, the plain
ports not on the network address, MQTT over TLS with the right and a wrong
pin.  Exit 77 (SKIP) without apache2, mosquitto, openssl or htpasswd.
"""
import os
import shutil
import socket
import subprocess
import sys

exe_client, exe_broker, build, scratch = sys.argv[1:5]
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PK = os.path.join(REPO, "packaging")
USERS_AUTH = os.path.join(build, "ecce_users_auth.so")

need = ["openssl", "htpasswd", "mosquitto", "curl"]
apache = any(shutil.which(c) or os.path.exists("/usr/sbin/" + c)
             for c in ("apache2", "httpd"))
if (not apache or any(not shutil.which(c) and not os.path.exists("/usr/sbin/" + c)
                      for c in need) or not os.path.exists(USERS_AUTH)
        or not os.path.exists("/usr/lib/apache2/modules/mod_ssl.so")
        and not os.path.exists("/usr/lib64/httpd/modules/mod_ssl.so")):
    print("SKIP  apache2 with mod_ssl, mosquitto, openssl, htpasswd or the "
          "login plugin missing")
    sys.exit(77)

failures = []


def check(ok, what):
    print(("PASS  " if ok else "FAIL  ") + what, flush=True)
    if not ok:
        failures.append(what)


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def lan_address():
    """A non-loopback IPv4 address of this host, or None."""
    try:
        out = subprocess.run(["hostname", "-I"], capture_output=True,
                             text=True).stdout.split()
    except OSError:
        return None
    return next((a for a in out if "." in a and not a.startswith("127.")), None)


def listening(host, port):
    try:
        socket.create_connection((host, port), timeout=3).close()
        return True
    except OSError:
        return False


shutil.rmtree(scratch, ignore_errors=True)
os.makedirs(scratch)
ports = {"ECCE_DATASERVER_PORT": free_port(), "ECCE_DATASERVER_TLS_PORT": free_port(),
         "ECCE_BROKER_PORT": free_port(), "ECCE_BROKER_TLS_PORT": free_port()}
for k in list(ports):
    ports[k] = str(ports[k])
lan = lan_address()


def make_home(name):
    """An $ECCE_HOME with the scripts of this tree and its own siteconfig."""
    home = os.path.join(scratch, name)
    os.makedirs(home + "/bin")
    os.makedirs(home + "/server/httpd-conf")
    for d in ("dataserver", "gateway"):
        for f in os.listdir(os.path.join(PK, d)):
            if f.startswith("ecce-") or f.endswith(".sh"):
                os.symlink(os.path.join(PK, d, f), os.path.join(home, "bin", f))
    os.symlink(os.path.join(build, "ecce-flock"), home + "/bin/ecce-flock")
    os.symlink(os.path.join(REPO, "data"), home + "/data")
    shutil.copytree(os.path.join(REPO, "siteconfig"), home + "/siteconfig")
    shutil.copy(PK + "/dataserver/httpd.conf.ecce", home + "/server/httpd-conf/")
    shutil.copy(PK + "/gateway/ecce-mosquitto.acl", home + "/server/mosquitto.acl")
    os.symlink(USERS_AUTH, home + "/server/ecce_users_auth.so")
    return home


def env_for(home, userhome, session="s1"):
    e = dict(os.environ)
    e.update(ports)
    e.update(ECCE_HOME=home, ECCE_REALUSERHOME=userhome, ECCE_SESSION_ID=session,
             ECCE_REALUSER="tlsuser", HOST="tlshost")
    for k in ("ECCE_DATASERVER_LISTEN", "ECCE_DATASERVER_TLS", "ECCE_REMOTE_SERVER",
              "ECCE_NO_REAP"):
        e.pop(k, None)
    return e


def run(cmd, env, **kw):
    return subprocess.run(cmd, env=env, capture_output=True, text=True,
                          timeout=120, **kw)


server_home = make_home("server-home")
server_user = os.path.join(scratch, "server-user")
os.makedirs(server_user)
senv = env_for(server_home, server_user)
bin_ = server_home + "/bin/"
tlsdir = server_user + "/.ECCE/tls"


def stop_everything():
    run([bin_ + "ecce-dataserver-stop"], senv)
    try:
        pid = int(open(server_user + "/.ECCE/mosquitto.pid").read())
        os.kill(pid, 15)
    except (OSError, ValueError):
        pass


try:
    # ---- the server's setup
    r = run([bin_ + "ecce-remote-setup", "--server", lan or "loopback", "--tls"], senv)
    check(r.returncode == 0, "ecce-remote-setup --server --tls ran: " + r.stderr[-200:])
    check(os.path.exists(tlsdir + "/server.pem") and os.path.exists(tlsdir + "/enabled"),
          "certificate and the TLS mark are in ~/.ECCE/tls")
    check((os.stat(tlsdir + "/server.key").st_mode & 0o077) == 0, "the key is private")
    bad = run([bin_ + "ecce-remote-setup", "--server", "--new-cert"], senv)
    check(bad.returncode != 0, "--new-cert without --tls is refused")

    # ---- services
    r = run([bin_ + "ecce-dataserver-start"], senv)
    check(r.returncode == 0, "ecce-dataserver-start with TLS: " + r.stderr[-300:])
    r = run([bin_ + "ecce-dataserver-adduser", "-b", "tlsuser", "tlspw", "Tls", "User"], senv)
    check(r.returncode == 0, "account created")
    r = run([bin_ + "ecce-gateway-start"], senv)
    check(r.returncode == 0, "ecce-gateway-start (central broker, TLS): " + r.stderr[-300:])

    dport, tport = int(ports["ECCE_DATASERVER_PORT"]), int(ports["ECCE_DATASERVER_TLS_PORT"])
    tport_s = str(tport)
    bport, btport = int(ports["ECCE_BROKER_PORT"]), int(ports["ECCE_BROKER_TLS_PORT"])
    check(listening("127.0.0.1", tport) and listening("127.0.0.1", btport),
          "TLS ports answer on loopback")
    if lan:
        check(listening(lan, tport) and listening(lan, btport),
              "TLS ports answer on " + lan)
        check(not listening(lan, dport), "plain data port is closed on " + lan)
        check(not listening(lan, bport), "plain broker port is closed on " + lan)
    else:
        print("NOTE  no non-loopback address on this host: network checks skipped")
    check(listening("127.0.0.1", dport) and listening("127.0.0.1", bport),
          "plain ports are open on loopback only")

    # ---- a client installation with the right pin, through the real setup
    good_home = make_home("client-home")
    cenv = env_for(good_home, os.path.join(scratch, "client-user"))
    os.makedirs(cenv["ECCE_REALUSERHOME"])
    host = lan or "127.0.0.1"
    cenv["ECCE_SETUP_PASSWORD"] = "tlspw"
    r = run([good_home + "/bin/ecce-remote-setup", host, ports["ECCE_DATASERVER_TLS_PORT"],
             "--tls", "--pin", tlsdir + "/server.pem", "--login", "tlsuser"], cenv)
    check(r.returncode == 0, "client ecce-remote-setup --tls --pin: " + r.stderr[-300:])
    pin = good_home + "/siteconfig/RemoteServer/server.pem"
    check(os.path.exists(pin) and open(pin).read() == open(tlsdir + "/server.pem").read(),
          "the pin is installed")
    check("https://" in open(good_home + "/siteconfig/RemoteServer/DataServers").read(),
          "DataServers names the https URL")
    check(os.path.exists(good_home + "/siteconfig/RemoteServer/MANIFEST"),
          "the machine list was fetched over TLS with the pin and a login")
    # the published site files are behind the data-server login, over TLS too
    pinhash = subprocess.run(
        "openssl x509 -in %s -noout -pubkey | openssl pkey -pubin -outform der | "
        "openssl dgst -sha256 -binary | openssl enc -base64" % (tlsdir + "/server.pem"),
        shell=True, capture_output=True, text=True).stdout.strip()
    siteurl = "https://%s:%s/Ecce/system/siteconfig/INDEX" % (lan or "127.0.0.1", tport_s)
    for creds, want in (([], "401"), (["-u", "tlsuser:tlspw"], "200")):
        r = subprocess.run(["curl", "-s", "-o", "/dev/null", "-w", "%{http_code}",
                            "--insecure", "--pinnedpubkey", "sha256//" + pinhash]
                           + creds + [siteurl], capture_output=True, text=True)
        check(r.stdout.strip() == want, "site INDEX over TLS %s: %s"
              % ("with a login" if creds else "without one", r.stdout.strip()))

    # ---- a wrong pin: another certificate
    other = os.path.join(scratch, "other")
    os.makedirs(other)
    run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "3",
         "-keyout", other + "/o.key", "-out", other + "/o.pem", "-subj", "/CN=other"], senv)
    bad_home = make_home("wrong-home")
    benv = env_for(bad_home, os.path.join(scratch, "wrong-user"))
    os.makedirs(benv["ECCE_REALUSERHOME"])
    r = run([bad_home + "/bin/ecce-remote-setup", host, ports["ECCE_DATASERVER_TLS_PORT"],
             "--tls", "--pin", other + "/o.pem"], benv)
    check(r.returncode != 0 and not os.path.exists(
        bad_home + "/siteconfig/RemoteServer/DataServers"),
        "setup with the wrong pin fails and writes nothing")
    os.makedirs(bad_home + "/siteconfig/RemoteServer", exist_ok=True)
    shutil.copy(other + "/o.pem", bad_home + "/siteconfig/RemoteServer/server.pem")

    # ---- the C++ clients
    url = "https://%s:%s/Ecce/system/" % (host, tport)
    r = run([exe_client, url, "ok"], cenv)
    check(r.returncode == 0, "https DAV request with the right pin: " + r.stdout.strip())
    r = run([exe_client, url, "cert"], benv)
    check(r.returncode == 0, "wrong pin gives CERTIFICATE_REJECTED: " + r.stdout.strip())

    # ---- MQTT: each client's own broker file, written by the real gateway start
    for env, home, name in ((cenv, good_home, "right"), (benv, bad_home, "wrong")):
        env["ECCE_REMOTE_SERVER"] = "1"
        env["ECCE_NO_REAP"] = "1"
        # the client's DataServers URL decides TLS; the wrong-pin home has none
        # of its own yet, so give it the right-pin one
        if name == "wrong":
            shutil.copy(good_home + "/siteconfig/RemoteServer/DataServers",
                        home + "/siteconfig/RemoteServer/DataServers")
        r = run([home + "/bin/ecce-gateway-start"], env)
        check(r.returncode == 0, "client gateway start (%s pin): %s" % (name, r.stderr[-200:]))
    r = run([exe_broker, "tlsuser", "tlspw"], cenv)
    check(r.returncode == 0, "MQTT over TLS with the right pin: " + r.stdout.strip())
    r = run([exe_broker, "tlsuser", "tlspw"], benv)
    check(r.returncode == 3, "MQTT with a wrong pin is refused as a TLS failure (rc %d): %s"
          % (r.returncode, r.stdout.strip()))
finally:
    stop_everything()

if failures:
    print("%d failed" % len(failures))
    sys.exit(1)
print("all passed")
