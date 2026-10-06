#!/usr/bin/env python3
"""The central-server deployment as it really runs, in rootless podman (#213).

One container is the server (data server + broker with the auth plugin and
ACL, set up as GETTING_STARTED does), two are clients, each a separate Unix
account on its own Xvfb reaching the server over a container network. A
fourth runs ecce-broker.service under systemd, which nothing else has done.

  tests/containers/central_server_test.py [--debs DIR] [--logdir DIR]
                                          [--keep] [--only part,...]

parts: setup clients isolation session-end systemd tls
Exit 0: all checks passed; 1: a check failed; 77: podman or the .deb
packages are not available (build them with `cpack -G DEB`).
"""
import argparse
import glob
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PW = {"alice": "alicepw1", "bob": "bobpw2"}
BASE = "http://srv:8096/Ecce/users"

results = []


def check(ok, what, detail=""):
    results.append((bool(ok), what))
    print("%s  %s%s" % ("PASS" if ok else "FAIL", what,
                        "" if ok or not detail else "\n      " +
                        str(detail).strip().replace("\n", "\n      ")),
          flush=True)
    return bool(ok)


class Podman:
    def __init__(self, tag):
        self.tag, self.net = tag, "ecce-central-" + tag
        self.names = []

    def p(self, *args, check=True, timeout=600, inp=None):
        r = subprocess.run(["podman"] + list(args), capture_output=True,
                           text=True, timeout=timeout, input=inp)
        if check and r.returncode:
            raise RuntimeError("podman %s: %s" % (" ".join(args[:3]),
                                                  r.stderr.strip()))
        return r

    def run(self, name, image, *extra, cmd=()):
        self.p("run", "-d", "--name", self.tag + name, "--hostname", name,
               "--network", self.net, "--network-alias", name,
               "-v", HERE + ":/harness:ro", *extra, image, *cmd)
        self.names.append(self.tag + name)

    def sh(self, name, script, user="root", timeout=300, detach=False,
           inp=None):
        """Run a shell script in a container as a Unix account."""
        home = "/root" if user == "root" else "/home/" + user
        args = ["exec", "-u", user, "-w", home, "-e", "HOME=" + home]
        if detach:
            args.append("-d")
        if inp is not None:
            args.append("-i")
        args += [self.tag + name, "bash", "-c",
                 "export PATH=/opt/ecce/bin:$PATH; " + script]
        return self.p(*args, check=False, timeout=timeout, inp=inp)

    def out(self, name, script, user="root", **kw):
        r = self.sh(name, script, user, **kw)
        return r.stdout + r.stderr, r.returncode

    def cleanup(self):
        for n in self.names:
            self.p("rm", "-f", "-t", "2", n, check=False)
        self.p("network", "rm", self.net, check=False)


def find_debs(d):
    pk = {}
    for kind in ("client", "server"):
        found = sorted(glob.glob(os.path.join(d, "ecce-%s_*.deb" % kind)),
                       key=os.path.getmtime)
        if not found:
            return None
        pk[kind] = found[-1]
    return pk


def build_image(pk, logdir):
    h = hashlib.sha256()
    h.update(open(os.path.join(HERE, "Containerfile"), "rb").read())
    for f in pk.values():
        s = os.stat(f)
        h.update(("%s %d %d" % (os.path.basename(f), s.st_size,
                                int(s.st_mtime))).encode())
    image = "localhost/ecce-central-test:" + h.hexdigest()[:12]
    if subprocess.run(["podman", "image", "exists", image]).returncode == 0:
        return image
    ctx = os.path.join(logdir, "context")
    shutil.rmtree(ctx, ignore_errors=True)
    os.makedirs(os.path.join(ctx, "debs"))
    for f in pk.values():
        shutil.copy(f, os.path.join(ctx, "debs"))
    shutil.copy(os.path.join(HERE, "Containerfile"), ctx)
    print("building %s (first run takes minutes)" % image, flush=True)
    r = subprocess.run(["podman", "build", "-q", "-t", image, ctx],
                       capture_output=True, text=True)
    shutil.rmtree(ctx, ignore_errors=True)
    if r.returncode:
        print(r.stderr)
        raise RuntimeError("image build failed")
    return image


# ---- broker helpers (mosquitto clients are the independent oracle) --------

TLS_PORT = 8883


def mosq(host, port, user, pw):
    a = "-h %s -p %d" % (host, port)
    if port == TLS_PORT:    # the pinned certificate; its name need not match
        a += " --cafile /tmp/server.pem --insecure"
    if user:
        a += " -u %s -P %s" % (user, pw)
    return a


def watch(pm, cont, host, port, user, pw, topics, tag, secs=600):
    """Background subscribers, one per filter (a refused filter must not hide
    the others); output in /tmp/cap_<tag>_<n> as 'topic payload'."""
    for i, t in enumerate(topics):
        pm.sh(cont, "timeout %d mosquitto_sub %s -t '%s' -F '%%t %%p' "
              ">/tmp/cap_%s_%d 2>/tmp/cap_%s_%d.err" %
              (secs, mosq(host, port, user, pw), t, tag, i, tag, i),
              detach=True)


def captured(pm, cont, tag):
    return pm.out(cont, "cat /tmp/cap_%s_[0-9]* 2>/dev/null" % tag)[0]


def pub(pm, cont, host, port, user, pw, topic, msg):
    # mosquitto_pub has no -W; -q 1 returns once the broker acknowledged.
    return pm.sh(cont, "timeout 10 mosquitto_pub %s -t '%s' -m '%s' -q 1" %
                 (mosq(host, port, user, pw), topic, msg))


def delivery(pm, host, port, label, key="k0", ca="alice", cb="bob"):
    """Cross-user delivery between alice (in 'alice') and bob (in 'bob')."""
    bob_topics = ["#", "ecce/#", "ecce/+/#", "ecce/alice/#",
                  "ecce/+/session/#"]
    watch(pm, cb, host, port, "bob", PW["bob"], bob_topics, "b" + label, 40)
    watch(pm, ca, host, port, "alice", PW["alice"],
          ["ecce/alice/#", "ecce/bob/#"], "a" + label, 40)
    time.sleep(2)
    A = "ecce/alice/session/%s/ecce_test" % key
    pub(pm, ca, host, port, "alice", PW["alice"], A, "A-session-" + label)
    pub(pm, ca, host, port, "alice", PW["alice"],
        "ecce/alice/job/j1/status", "A-job-" + label)
    pub(pm, ca, host, port, "alice", PW["alice"],
        "ecce/alice/ecce_machreg_changed", "A-machreg-" + label)
    pub(pm, cb, host, port, "bob", PW["bob"], A, "B-intrusion-" + label)
    pub(pm, cb, host, port, "bob", PW["bob"],
        "ecce/alice/ecce_machreg_changed", "B-spoof-" + label)
    pub(pm, cb, host, port, "bob", PW["bob"],
        "ecce/bob/session/k1/ecce_test", "B-own-" + label)
    time.sleep(3)
    b, a = captured(pm, cb, "b" + label), captured(pm, ca, "a" + label)
    check("A-machreg-" + label in a and "A-session-" + label in a
          and "A-job-" + label in a,
          "%s: alice's own subscriber gets her session, job and machreg "
          "messages (control)" % label, a)
    check("B-own-" + label in b,
          "%s: bob's own subscriber gets bob's message (control)" % label, b)
    check("A-session-" + label not in b and "A-job-" + label not in b,
          "%s: alice's session and job messages are delivered to no bob "
          "subscription (#, ecce/#, ecce/+/#, ecce/alice/#, ecce/+/session/#)"
          % label, b)
    check("A-machreg-" + label in b,
          "%s: the site-wide machreg message does reach bob" % label, b)
    check("B-intrusion-" + label not in a and "B-spoof-" + label not in a,
          "%s: bob cannot publish into alice's topics (nothing delivered)"
          % label, a)
    check("B-own-" + label not in a,
          "%s: alice's subscriptions (incl. ecce/bob/#) receive none of "
          "bob's own messages" % label, a)
    # Wrong passwords, unknown accounts and anonymous clients are refused.
    for who, user, pw in (("bob with alice's password", "bob", PW["alice"]),
                          ("unknown account", "mallory", "x"),
                          ("anonymous", None, None)):
        r = pub(pm, cb, host, port, user, pw, "ecce/bob/x", "no")
        check(r.returncode not in (0, 124), "%s: %s is refused" % (label, who),
              r.stdout + r.stderr)


# ---- parts -----------------------------------------------------------------

def setup(pm, image):
    for n in ("srv", "alice", "bob"):
        pm.run(n, image)
    for c, u in (("srv", "ecce"), ("alice", "alice"), ("bob", "bob")):
        pm.sh(c, "useradd -m -s /bin/bash %s" % u)
    o, rc = pm.out("srv", "ecce-remote-setup --server all && "
                   "ecce-dataserver-start && ecce-gateway-start", "ecce")
    check(rc == 0, "server: ecce-remote-setup --server, data server and broker "
         "started as the server account", o)
    for u, pw in PW.items():
        o, rc = pm.out("srv", "ecce-dataserver-adduser -b %s %s %s Test" %
                       (u, pw, u.title()), "ecce")
        check(rc == 0, "server: data server account %s created" % u, o)
    for c in ("alice", "bob"):
        o, rc = pm.out(c, "curl -s -o /dev/null -w %{http_code} "
                       "http://srv:8096/Ecce/system/siteconfig/MANIFEST")
        check(o.strip() == "200", "%s's machine reaches the data server over "
              "the network (MANIFEST %s)" % (c, o.strip()))
        o, rc = pm.out(c, "timeout 5 bash -c 'echo > /dev/tcp/srv/8088'")
        check(rc == 0, "%s's machine reaches the broker port 8088" % c, o)
        o, rc = pm.out(c, "ecce-remote-setup srv")
        check(rc == 0, "%s: ecce-remote-setup srv" % c, o)


def clients(pm):
    state = {}
    for u in ("alice", "bob"):
        o, rc = pm.out(u, "python3 /harness/guest.py start %s %s srv" %
                       (u, PW[u]), u, timeout=240)
        try:
            state[u] = json.loads(o.strip().splitlines()[-1])
        except Exception:
            state[u] = {}
        s = state[u]
        check(s.get("organizer") and " on srv" in s["organizer"],
              "%s logs in: the Organizer opens and names the central server "
              "(%s)" % (u, s.get("organizer")), o)
        check("gateway" in s.get("procs", {}) and s.get("broker", 0) >= 1,
              "%s's gateway holds a connection to the server's broker "
              "(%s)" % (u, s.get("broker")), json.dumps(s))
        log, _ = pm.out("srv", "cat ~/.ECCE/dataserver/logs/access_log",
                        "ecce")
        check(any(" %s " % u in l and "PROPFIND" in l and " 207 " in l
                  for l in log.splitlines()),
              "%s: the server's access log shows her Organizer's PROPFIND "
              "answered 207 under her own login" % u)
        # Create and read back her own data over the same WebDAV endpoint.
        pm.sh(u, "echo 'data of %s' >/tmp/f.txt" % u)
        o, rc = pm.out(u, "curl -s -u %s:%s -X MKCOL -o /dev/null -w %%{http_code} %s/%s/stage5; echo; "
                       "curl -s -u %s:%s -T /tmp/f.txt -o /dev/null -w %%{http_code} %s/%s/stage5/f.txt; echo; "
                       "curl -s -u %s:%s %s/%s/stage5/f.txt" %
                       ((u, PW[u], BASE, u) * 3))
        codes = o.split("\n")
        check(codes[0] in ("201", "405") and codes[1] in ("201", "204")
              and ("data of %s" % u) in o,
              "%s creates a collection and file in her own data and reads it "
              "back (%s)" % (u, codes[:2]), o)
    return state


def isolation(pm, state):
    for me, other in (("bob", "alice"), ("alice", "bob")):
        U = "%s/%s/stage5/f.txt" % (BASE, other)
        def code(extra):
            return pm.out(me, "curl -s -o /dev/null -w %%{http_code} %s %s" %
                          (extra, U))[0].strip()
        check(code("-u %s:%s" % (me, PW[me])) in ("401", "403"),
              "DAV: %s cannot read %s's file with her own login" % (me, other))
        check(code("") in ("401", "403"),
              "DAV: an anonymous client cannot read %s's file" % other)
        check(code("-u %s:%s -T /tmp/f.txt" % (me, PW[me])) in
              ("401", "403", "405"),
              "DAV: %s cannot overwrite %s's file" % (me, other))
        check(code("-u %s:wrong" % other) == "401",
              "DAV: a wrong password for %s is refused" % other)
        r = pm.out(me, "curl -s -u %s:%s -X MKCOL -o /dev/null -w %%{http_code} "
                   "%s/%s/intruder" % (me, PW[me], BASE, other))[0].strip()
        check(r in ("401", "403"), "DAV: %s cannot create under %s's "
              "home (%s)" % (me, other, r))
    o, _ = pm.out("alice", "curl -s -u alice:%s %s/alice/stage5/f.txt" %
                  (PW["alice"], BASE))
    check("data of alice" in o, "alice's file is unchanged after the attempts")
    key = (state.get("alice", {}).get("sessions") or ["broker_x_k0"])[0][7:]
    delivery(pm, "srv", 8088, "central", key)


def session_end(pm, state):
    pm.sh("bob", "mosquitto_sub -h srv -p 8088 -u bob -P %s -t 'ecce/bob/#' "
          "-F '%%t %%p' -W 60 >/tmp/cap_end 2>&1" % PW["bob"], detach=True)
    time.sleep(2)
    o, rc = pm.out("alice", "python3 /harness/guest.py close", "alice",
                   timeout=120)
    try:
        s = json.loads(o.strip().splitlines()[-1])
    except Exception:
        s = {}
    check(s and not s["procs"].get("gateway") and
          not [t for t in s["titles"] if "Organizer" in t],
          "alice quits: her Organizer, gateway and apps are gone", o)
    o, _ = pm.out("alice", "pgrep -u alice -a 'ecce|gateway|organizer' || true")
    check(not o.strip(), "no ECCE process of alice's is left", o)
    o, _ = pm.out("bob", "python3 /harness/guest.py state", "bob")
    s = json.loads(o.strip().splitlines()[-1])
    check(any("Organizer" in t for t in s["titles"])
          and s["procs"].get("gateway") and s["broker"] >= 1,
          "bob's session is untouched: Organizer open, gateway still "
          "connected to the broker", o)
    r = pub(pm, "alice", "srv", 8088, "alice", PW["alice"],
            "ecce/alice/x", "after")
    check(r.returncode == 0,
          "the server's broker still accepts logins after alice's quit")
    pub(pm, "bob", "srv", 8088, "bob", PW["bob"], "ecce/bob/after", "B-after")
    time.sleep(2)
    o, _ = pm.out("bob", "cat /tmp/cap_end")
    check("B-after" in o, "bob's messages are still delivered", o)
    o, _ = pm.out("bob", "curl -s -o /dev/null -w %%{http_code} -u bob:%s "
                  "%s/bob/stage5/f.txt" % (PW["bob"], BASE))
    check(o.strip() == "200", "bob still reads her data from the server")
    o, rc = pm.out("srv", "ecce-dataserver-status; ecce-gateway-status", "ecce")
    check("not running" not in o.lower(),
          "the server's data server and broker are still running", o)
    o, rc = pm.out("bob", "python3 /harness/guest.py close", "bob", timeout=120)
    o, rc = pm.out("srv", "timeout 5 bash -c 'echo > /dev/tcp/localhost/8088' "
                   "&& timeout 5 bash -c 'echo > /dev/tcp/localhost/8096'",
                   "ecce")
    check(rc == 0, "after both clients quit the server's ports 8096 and 8088 "
          "still answer", o)


def systemd(pm, image):
    # SYS_ADMIN: without it systemd cannot set up the unit's mount namespace
    # (226/NAMESPACE with DynamicUser and ExecStartPre) or drop to User=.
    pm.run("sysb", image, "--systemd=always", "--cap-add", "SYS_ADMIN",
           cmd=("/sbin/init",))
    for _ in range(30):
        o, _ = pm.out("sysb", "systemctl is-system-running")
        if o.strip() in ("running", "degraded"):
            break
        time.sleep(1)
    sh = lambda s, **k: pm.out("sysb", s, **k)
    o, rc = sh("ecce-broker-setup sysb:8088 && "
               "echo %s | ecce-broker-setup --user alice && "
               "echo %s | ecce-broker-setup --user bob && "
               "systemctl link /opt/ecce/server/systemd/ecce-broker.service && "
               "systemctl enable --now ecce-broker" % (PW["alice"], PW["bob"]))
    check(rc == 0, "ecce-broker-setup and `systemctl enable --now "
          "ecce-broker` as GETTING_STARTED gives them", o)
    time.sleep(3)
    o, _ = sh("systemctl is-active ecce-broker")
    check(o.strip() == "active", "ecce-broker.service is active (%s)" %
          o.strip(), sh("systemctl status ecce-broker --no-pager -l; "
                        "journalctl -u ecce-broker --no-pager | tail -20")[0])
    o, _ = sh("ss -ltnpH sport = :8088; ps -o user:20,args -C mosquitto")
    check(":8088" in o and "ecce-broker" in o,
          "it listens on 8088 as its own dynamic account ecce-broker", o)
    r = pub(pm, "alice", "sysb", 8088, "alice", PW["alice"], "ecce/alice/x", "hi")
    check(r.returncode == 0, "a client in another container connects to it "
          "and publishes with her account", r.stdout + r.stderr)
    delivery(pm, "sysb", 8088, "systemd")
    o, rc = sh("echo carol1 | ecce-broker-setup --user carol")
    r = pub(pm, "bob", "sysb", 8088, "carol", "carol1", "ecce/carol/x", "hi")
    check(r.returncode == 0, "an account added by ecce-broker-setup --user "
          "works at once (the script reloads the running service)",
          o + r.stdout + r.stderr)
    r = pub(pm, "bob", "sysb", 8088, "carol", "carol2", "ecce/carol/x", "hi")
    check(r.returncode != 0, "...and a wrong password for it is refused")
    pid, _ = sh("systemctl show -p MainPID --value ecce-broker")
    sh("kill -9 %s" % pid.strip())
    time.sleep(9)
    o, _ = sh("systemctl is-active ecce-broker; systemctl show -p MainPID "
              "--value ecce-broker")
    check(o.split()[0] == "active" and o.split()[1] != pid.strip(),
          "killed (-9), the unit restarts the broker itself (Restart=on-"
          "failure; pid %s -> %s)" % (pid.strip(), o.split()[1:]), o)
    sh("systemctl stop ecce-broker")
    time.sleep(2)
    o, _ = sh("systemctl is-active ecce-broker; ss -ltnH sport = :8088; "
              "pgrep -a mosquitto || true")
    check("inactive" in o and ":8088" not in o and "mosquitto" not in o,
          "systemctl stop: inactive, port 8088 closed, no mosquitto left", o)
    r = pub(pm, "alice", "sysb", 8088, "alice", PW["alice"], "ecce/alice/x", "hi")
    check(r.returncode != 0, "a client now gets no connection")
    o, rc = sh("systemctl start ecce-broker; sleep 3; systemctl is-active "
               "ecce-broker")
    check("active" in o.split()[-1:] and "inactive" not in o,
          "it starts again after a stop", o)
    sh("systemctl stop ecce-broker")


def tls(pm, image):
    """The server over TLS (#236): its own containers, so the plain parts
    above keep their plain server. alice pins the certificate she fetched,
    bob the file copied from the server, eve a different one."""
    for n in ("tsrv", "talice", "tbob", "teve"):
        pm.run(n, image)
    for c, u in (("tsrv", "ecce"), ("talice", "alice"), ("tbob", "bob"),
                 ("teve", "eve")):
        pm.sh(c, "useradd -m -s /bin/bash %s" % u)
    o, rc = pm.out("tsrv", "ecce-remote-setup --server all --tls && "
                   "ecce-dataserver-start && ecce-gateway-start", "ecce")
    check(rc == 0 and "TLS is on" in o, "tls: server set up with "
          "`ecce-remote-setup --server all --tls` (generated certificate), "
          "data server and broker started", o)
    for u, pw in PW.items():
        o, rc = pm.out("tsrv", "ecce-dataserver-adduser -b %s %s %s Test" %
                       (u, pw, u.title()), "ecce")
        check(rc == 0, "tls: data server account %s created" % u, o)
    fp = "openssl x509 -noout -fingerprint -sha256 -in "
    srvfp, _ = pm.out("tsrv", fp + "~/.ECCE/tls/server.pem", "ecce")
    pem = pm.out("tsrv", "cat ~/.ECCE/tls/server.pem", "ecce")[0]

    # Plain ports are closed to the network, the TLS ports are open.
    o, _ = pm.out("tsrv", "ss -ltnH", "ecce")
    def lis(port, loop):
        ls = [l.split()[3] for l in o.splitlines()
              if l.split()[3].endswith(":%d" % port)]
        return ls and all(a.startswith("127.0.0.1") or a.startswith("[::1]")
                          for a in ls) == loop
    check(lis(8096, True) and lis(8088, True),
          "tls: on the server 8096 and 8088 listen on loopback only", o)
    check(lis(8443, False) and lis(8883, False),
          "tls: 8443 (https data server) and 8883 (broker) listen on the "
          "network", o)
    for c in ("talice", "tbob", "teve"):
        o, rc = pm.out(c, "timeout 5 bash -c 'echo > /dev/tcp/tsrv/8096'")
        check(rc != 0, "tls: from %s's machine plain 8096 on the server's "
              "address is closed" % c, o)
        o, rc = pm.out(c, "timeout 5 bash -c 'echo > /dev/tcp/tsrv/8088'")
        check(rc != 0, "tls: from %s's machine plain 8088 is closed" % c, o)
        o, rc = pm.out(c, "timeout 5 bash -c 'echo > /dev/tcp/tsrv/8443' && "
                       "timeout 5 bash -c 'echo > /dev/tcp/tsrv/8883'")
        check(rc == 0, "tls: %s's machine reaches 8443 and 8883" % c, o)

    o, rc = pm.out("talice", "ecce-remote-setup tsrv --tls --fetch-pin")
    check(rc == 0 and srvfp.split("=")[-1].strip() in o,
          "tls: alice `ecce-remote-setup tsrv --tls --fetch-pin` prints the "
          "server certificate's fingerprint", o + "\nserver: " + srvfp)
    pm.sh("talice", "cp /opt/ecce/siteconfig/RemoteServer/server.pem "
          "/tmp/server.pem")
    a, _ = pm.out("talice", "cat /tmp/server.pem")
    check(a == pem, "tls: alice's fetched pin is the server's certificate")
    pm.sh("tbob", "cat > /tmp/server.pem", inp=pem)
    o, rc = pm.out("tbob", "ecce-remote-setup tsrv --tls --pin /tmp/server.pem")
    check(rc == 0, "tls: bob `ecce-remote-setup tsrv --tls --pin <file "
          "copied from the server>`", o)
    ds, _ = pm.out("talice", "cat /opt/ecce/siteconfig/RemoteServer/"
                   "DataServers")
    check("https://tsrv:8443" in ds, "tls: the client's DataServers names "
          "https://tsrv:8443", ds)

    watch(pm, "tbob", "tsrv", 8883, "bob", PW["bob"], ["ecce/#"], "tpb")
    watch(pm, "talice", "tsrv", 8883, "alice", PW["alice"], ["ecce/#"], "tpa")
    state = {}
    for c, u in (("talice", "alice"), ("tbob", "bob")):
        o, rc = pm.out(c, "python3 /harness/guest.py start %s %s tsrv" %
                       (u, PW[u]), u, timeout=240)
        try:
            state[u] = json.loads(o.strip().splitlines()[-1])
        except Exception:
            state[u] = {}
        s = state[u]
        check(s.get("organizer") and " on tsrv" in s["organizer"],
              "tls: %s logs in with a real `ecce -remote`: the Organizer "
              "opens and names the server (%s)" % (u, s.get("organizer")), o)
        check("gateway" in s.get("procs", {}) and s.get("broker", 0) >= 1,
              "tls: %s's gateway holds a connection to the broker on 8883 "
              "(%s)" % (u, s.get("broker")), json.dumps(s))
    log, _ = pm.out("tsrv", "cat ~/.ECCE/dataserver/logs/access_log", "ecce")
    for c, u in (("talice", "alice"), ("tbob", "bob")):
        ip, _ = pm.out(c, "hostname -i")
        ip = ip.split()[0]
        check(any(l.startswith(ip + " ") and " %s " % u in l
                  and "PROPFIND" in l and " 207 " in l
                  for l in log.splitlines()),
              "tls: the server's access log has %s's Organizer PROPFIND "
              "answered 207 from her machine's address %s, which can only "
              "have come in on 8443" % (u, ip))
    check("alice" in " ".join(state.get("alice", {}).get("sessions", ["alice"])),
          "tls: alice's session state exists")
    # Her own data over https, then the DAV isolation checks over https.
    B = "https://tsrv:8443/Ecce/users"
    cu = lambda u, extra, url: "curl -s --cacert /tmp/server.pem %s -o " \
        "/dev/null -w %%{http_code} %s" % (extra, url)
    for c, u in (("talice", "alice"), ("tbob", "bob")):
        pm.sh(c, "echo 'data of %s' >/tmp/f.txt" % u)
        o, rc = pm.out(c, "curl -s --cacert /tmp/server.pem -u %s:%s -X MKCOL "
                       "-o /dev/null -w %%{http_code} %s/%s/stage5; echo; "
                       "curl -s --cacert /tmp/server.pem -u %s:%s -T /tmp/f.txt "
                       "-o /dev/null -w %%{http_code} %s/%s/stage5/f.txt; echo; "
                       "curl -s --cacert /tmp/server.pem -u %s:%s %s/%s/stage5/f.txt"
                       % ((u, PW[u], B, u) * 3))
        codes = o.split("\n")
        check(codes[0] in ("201", "405") and codes[1] in ("201", "204")
              and ("data of %s" % u) in o,
              "tls: %s creates and reads back her own file over https (%s)"
              % (u, codes[:2]), o)
    o, _ = pm.out("talice", "curl -s -o /dev/null -w %%{http_code} "
                  "-u alice:%s %s/alice/stage5/f.txt" % (PW["alice"], B))
    check(o.strip() != "200", "tls: https without the pinned certificate "
          "is not accepted by curl (%s)" % o.strip())
    for me, mc, other in (("bob", "tbob", "alice"), ("alice", "talice", "bob")):
        U = "%s/%s/stage5/f.txt" % (B, other)
        code = lambda extra: pm.out(mc, cu(me, extra, U))[0].strip()
        check(code("-u %s:%s" % (me, PW[me])) in ("401", "403"),
              "tls DAV: %s cannot read %s's file with her own login" %
              (me, other))
        check(code("") in ("401", "403"),
              "tls DAV: an anonymous client cannot read %s's file" % other)
        check(code("-u %s:%s -T /tmp/f.txt" % (me, PW[me])) in
              ("401", "403", "405"),
              "tls DAV: %s cannot overwrite %s's file" % (me, other))
        check(code("-u %s:wrong" % other) == "401",
              "tls DAV: a wrong password for %s is refused" % other)
        r = pm.out(mc, cu(me, "-u %s:%s -X MKCOL" % (me, PW[me]),
                          "%s/%s/intruder" % (B, other)))[0].strip()
        check(r in ("401", "403"), "tls DAV: %s cannot create under %s's "
              "home (%s)" % (me, other, r))
    for c in ("talice",):
        o, rc = pm.out(c, "timeout 10 mosquitto_pub -h tsrv -p 8088 -u alice "
                       "-P %s -t ecce/alice/x -m plain -q 1" % PW["alice"])
        check(rc != 0, "tls: a plain MQTT client gets no answer on 8088", o)
    o, rc = pm.out("talice", "timeout 10 mosquitto_pub -h tsrv -p 8883 -u "
                   "alice -P %s -t ecce/alice/x -m x -q 1 --insecure" %
                   PW["alice"])
    check(rc != 0, "tls: MQTT on 8883 without a trusted certificate is "
          "refused (control for the pin)", o)
    key = (state.get("alice", {}).get("sessions") or ["broker_x_k0"])[0][7:]
    delivery(pm, "tsrv", 8883, "tls", key, "talice", "tbob")

    # eve pinned a certificate that is not the server's.
    o, rc = pm.out("teve", "ecce-remote-setup tsrv --tls --fetch-pin")
    check(rc == 0, "tls: eve is set up with --fetch-pin first", o)
    pm.sh("teve", "openssl req -x509 -newkey rsa:2048 -nodes -days 3 "
          "-subj /CN=other -keyout /tmp/o.key -out /tmp/o.pem 2>/dev/null && "
          "cp /tmp/o.pem /opt/ecce/siteconfig/RemoteServer/server.pem")
    r = pm.out("teve", "curl -s -o /dev/null -w %{http_code} "
               "--cacert /opt/ecce/siteconfig/RemoteServer/server.pem "
               "https://tsrv:8443/Ecce/system/siteconfig/MANIFEST")
    check(r[0].strip() == "000", "tls: eve's pin is another certificate; "
          "the server's is not accepted by it")
    o, rc = pm.out("teve", "ecce-remote-setup tsrv --tls --pin /tmp/o.pem")
    check(rc != 0, "tls: ecce-remote-setup with that wrong pin is refused", o)
    o, rc = pm.out("teve", "python3 /harness/guest.py start-refused eve "
                   "%s tsrv" % PW["alice"], "eve", timeout=240)
    try:
        s = json.loads(o.strip().splitlines()[-1])
    except Exception:
        s = {}
    msg = (s.get("log") or "") + " " + " ".join(s.get("titles") or [])
    check(s and not s.get("organizer"),
          "tls: eve's `ecce -remote` does not open an Organizer", json.dumps(s))
    check("certificate" in msg.lower(),
          "tls: what eve sees names the certificate (titles %s)" %
          s.get("titles"), s.get("log"))
    log, _ = pm.out("tsrv", "cat ~/.ECCE/dataserver/logs/access_log", "ecce")
    ip = pm.out("teve", "hostname -i")[0].split()[0]
    check(not any(l.startswith(ip + " ") and " 207 " in l
                  for l in log.splitlines()),
          "tls: nothing of eve's was answered by the server")
    for c, u in (("talice", "alice"), ("tbob", "bob")):
        pm.out(c, "python3 /harness/guest.py close", u, timeout=120)
    b = captured(pm, "tbob", "tpb")
    seen = [l for l in b.splitlines() if l.startswith("ecce/alice/")
            and "ecce_machreg_changed" not in l and "A-" not in l
            and "B-" not in l]
    check(not seen, "tls real traffic: of everything alice's programs "
          "published, bob's ecce/# subscription received none", "\n".join(seen[:5]))


PARTS = ("setup", "clients", "isolation", "session-end", "systemd", "tls")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--debs", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--logdir", default=os.path.join(HERE, "logs"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--only", default=",".join(PARTS))
    a = ap.parse_args()
    if not shutil.which("podman") or subprocess.run(
            ["podman", "info"], capture_output=True).returncode:
        print("SKIP: podman is not available")
        return 77
    pk = find_debs(a.debs)
    if not pk:
        print("SKIP: no ecce-client/ecce-server .deb in %s (cpack -G DEB)"
              % a.debs)
        return 77
    only = a.only.split(",")
    os.makedirs(a.logdir, exist_ok=True)
    pm = Podman(str(os.getpid()) + "-")
    try:
        image = build_image(pk, a.logdir)
        print("image", image, "from", ", ".join(map(os.path.basename,
                                                     pk.values())))
        pm.p("network", "create", pm.net)
        plain = [x for x in only if x != "tls"]
        if plain:
            setup(pm, image)
        # Passive capture from before any session: whatever alice's real
        # programs publish must never reach bob.
        if plain:
            watch(pm, "bob", "srv", 8088, "bob", PW["bob"], ["ecce/#"], "pb")
            watch(pm, "alice", "srv", 8088, "alice", PW["alice"],
                  ["ecce/#"], "pa")
        state = {}
        if "clients" in only or "isolation" in only or "session-end" in only:
            state = clients(pm)
        if "isolation" in only:
            isolation(pm, state)
        if "session-end" in only:
            session_end(pm, state)
            b, al = captured(pm, "bob", "pb"), captured(pm, "alice", "pa")
            seen = [l for l in b.splitlines() if l.startswith("ecce/alice/")
                    and "ecce_machreg_changed" not in l and "A-" not in l
                    and "B-" not in l]
            check(not seen, "real traffic: of everything alice's programs "
                  "published in this run, bob's ecce/# subscription "
                  "received none (alice's own saw %d messages)" %
                  len(al.splitlines()), "\n".join(seen[:5]))
        if "systemd" in only:
            systemd(pm, image)
        if "tls" in only:
            tls(pm, image)
    except Exception as e:
        check(False, "harness error: %s" % e)
    finally:
        for n in pm.names:
            r = pm.p("logs", n, check=False)
            open(os.path.join(a.logdir, n + ".log"), "w").write(
                r.stdout + r.stderr)
        for c, f in (("srv", "~/.ECCE/mosquitto.log"),
                     ("srv", "~/.ECCE/dataserver/logs/*_log"),
                     ("alice", "~/ecce-session.log"),
                     ("bob", "~/ecce-session.log"),
                     ("tsrv", "~/.ECCE/mosquitto.log"),
                     ("tsrv", "~/.ECCE/dataserver/logs/*_log"),
                     ("talice", "~/ecce-session.log"),
                     ("tbob", "~/ecce-session.log"),
                     ("teve", "~/ecce-session.log")):
            try:
                o, _ = pm.out(c, "tail -n 200 %s" % f,
                              "ecce" if c.endswith("srv") else c.lstrip("t"))
                open(os.path.join(a.logdir, "%s-%s.txt" % (
                    c, os.path.basename(f.replace("*", "all")))), "w").write(o)
            except Exception:
                pass
        try:
            o, _ = pm.out("sysb", "journalctl -u ecce-broker --no-pager")
            open(os.path.join(a.logdir, "ecce-broker-journal.txt"),
                 "w").write(o)
        except Exception:
            pass
        if not a.keep:
            pm.cleanup()
    bad = [w for ok, w in results if not ok]
    print("\n%d checks, %d failed" % (len(results), len(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
