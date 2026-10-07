#!/usr/bin/env python3
"""Install the CI packages on each distro from its official base image and
smoke-test every deployment mode (#193).

  tests/packaging/install_matrix.py [--pkgs DIR | --run ID] [--distro a,b]
                                    [--only install,local,central,tls,broker]
                                    [--logdir DIR] [--keep] [--reuse]

DIR holds one directory per CI artifact, as `gh run download <id>` lays them
out: ecce-debian-trixie-deb, ecce-ubuntu-latest-deb, ecce-rockylinux9-rpm,
ecce-fedora-latest-rpm, each with the ecce-client and ecce-server package.
--run ID downloads them into DIR first.

Per distro, in a fresh container: (1) client and server installed from the
local files by the distro's package manager (dependencies from its
repositories; on RPM distros a first try without EPEL, then as
GETTING_STARTED says with it), every failure kept verbatim; (2) test tools
added on top (Xvfb, xdotool: not ECCE dependencies); (3) local mode: `ecce`
on Xvfb, login, Organizer, clean quit; central server: server container and
client container, `ecce -remote`; the same with --tls and --fetch-pin; shared
broker: ecce-broker.service under systemd.

Writes matrix.md and results.json to --logdir. Exit 0 all passed, 1 a check
failed, 77 podman or the packages missing.
"""
import argparse
import concurrent.futures as cf
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "containers"))
import central_server_test as cst   # noqa: E402  (Podman helper, harness dir)

PW = {"alice": "alicepw1", "bob": "bobpw2"}
DEB_TOOLS = ("systemd systemd-sysv dbus iproute2 procps psmisc curl "
             "ca-certificates xvfb xauth xdotool x11-utils python3 "
             "python3-xlib libgl1-mesa-dri mosquitto-clients openssl")
RPM_TOOLS = ("systemd iproute procps-ng psmisc curl-minimal xorg-x11-server-Xvfb "
             "xorg-x11-xauth xdotool xorg-x11-utils xwininfo python3 python3-xlib "
             "mesa-dri-drivers openssl")
DISTROS = [
    dict(key="trixie", name="Debian 13 (trixie)",
         image="docker.io/library/debian:trixie",
         art="ecce-debian-trixie-deb", fam="deb"),
    dict(key="noble", name="Ubuntu 24.04",
         image="docker.io/library/ubuntu:24.04",
         art="ecce-ubuntu-latest-deb", fam="deb"),
    dict(key="rocky9", name="Rocky Linux 9",
         image="docker.io/library/rockylinux:9",
         art="ecce-rockylinux9-rpm", fam="rpm"),
    dict(key="fedora", name="Fedora (latest)",
         image="docker.io/library/fedora:latest",
         art="ecce-fedora-latest-rpm", fam="rpm"),
]
PARTS = ["install", "local", "central", "tls", "broker"]
ERR = re.compile(r"nothing provides|Unable to find a match|No match for "
                 r"argument|Depends:|not installable|^E: |^Error|Problem \d|"
                 r"has no installation candidate|conflicting|"
                 r"Failed to download|Cannot find", re.M)


TRANSIENT = re.compile(r"All mirrors were tried|Cannot download|Failed to "
                       r"fetch|Temporary failure|Could not resolve|"
                       r"Connection timed out|Hash Sum mismatch")


class Distro:
    """One distro's run: its own podman network and result records."""

    def __init__(self, d, pkgdir, logdir, tag):
        self.d, self.key = d, d["key"]
        self.pkgdir = os.path.realpath(os.path.join(pkgdir, d["art"]))
        self.logdir = os.path.join(logdir, self.key)
        os.makedirs(self.logdir, exist_ok=True)
        self.pm = cst.Podman("%s-%s-" % (tag, self.key))
        self.checks = []        # (part, ok, what, detail)
        self.facts = {}
        self.attempts = []      # (label, rc, first error lines)
        self.image = None
        self.overlay = []       # (local file, path in the image)
        self.t = {}
        self.part = "install"

    def check(self, ok, what, detail=""):
        self.checks.append((self.part, bool(ok), what, str(detail).strip()))
        print("[%s] %s %s/%s" % (self.key, "PASS" if ok else "FAIL",
                                 self.part, what), flush=True)
        return bool(ok)

    def status(self, part):
        c = [x for x in self.checks if x[0] == part]
        if not c:
            return "SKIP"
        return "PASS" if all(x[1] for x in c) else "FAIL"

    # -- phase 1: the install, exactly as a user does it --------------------
    def pkgs(self):
        ext = "deb" if self.d["fam"] == "deb" else "rpm"
        return sorted(glob.glob(os.path.join(self.pkgdir, "ecce-*.%s" % ext)))

    def install_cmds(self):
        if self.d["fam"] == "deb":
            inst = ("DEBIAN_FRONTEND=noninteractive apt-get install -y "
                    "/pkg/ecce-client_*.deb /pkg/ecce-server_*.deb")
            return [("apt-get install ./ecce-client ./ecce-server",
                     "apt-get update -q && " + inst)]
        inst = "dnf -y install /pkg/ecce-client-*.rpm /pkg/ecce-server-*.rpm"
        out = [("dnf install (no extra repositories)", inst)]
        if self.key == "rocky9":
            out.append(("dnf install epel-release, then the packages (as "
                        "GETTING_STARTED)", "dnf -y install epel-release && "
                        + inst))
            out.append(("... also with CRB enabled",
                        "dnf -y install epel-release && /usr/bin/crb enable "
                        "&& " + inst))
        return out

    def install(self, reuse):
        t0 = time.time()
        tag = "localhost/ecce-im-%s:installed" % self.key
        if reuse and subprocess.run(["podman", "image", "exists", tag]
                                    ).returncode == 0:
            self.image = tag
            self.check(True, "install: reusing image %s" % tag)
            return True
        for label, cmd in self.install_cmds():
            name = "%sa" % self.pm.tag
            for attempt in (1, 2):
                r = subprocess.run(
                    ["podman", "run", "--name", name, "-v",
                     self.pkgdir + ":/pkg:ro,Z", self.d["image"], "bash",
                     "-c", cmd], capture_output=True, text=True,
                    timeout=3600)
                log = r.stdout + r.stderr
                # A mirror that is down is not a packaging result: once more.
                if r.returncode and attempt == 1 and TRANSIENT.search(log):
                    subprocess.run(["podman", "rm", "-f", name],
                                   capture_output=True)
                    continue
                break
            fn = re.sub(r"\W+", "_", label)[:40]
            open(os.path.join(self.logdir, "install-%s.log" % fn),
                 "w").write(log)
            errs = "\n".join(l.strip() for l in log.splitlines()
                             if ERR.search(l))[:6000]
            self.attempts.append((label, r.returncode, errs))
            if r.returncode == 0:
                subprocess.run(["podman", "commit", "-q", name, tag],
                               capture_output=True)
                subprocess.run(["podman", "rm", "-f", name],
                               capture_output=True)
                self.image = tag
                break
            subprocess.run(["podman", "rm", "-f", name], capture_output=True)
        # Earlier failed attempts are kept in self.attempts (the matrix
        # prints them); only the end state is a check.
        label, rc, errs = self.attempts[-1]
        self.check(rc == 0, "install: %s" % label, errs)
        self.t["install"] = time.time() - t0
        return self.image is not None

    # -- phase 2: test tools on top of the installed image -------------------
    def tools(self):
        tag = "localhost/ecce-im-%s:ready" % self.key
        if self.reuse and subprocess.run(["podman", "image", "exists", tag]
                                         ).returncode == 0:
            self.image = tag
            return True
        if self.d["fam"] == "deb":
            cmd = ("apt-get update -q >/dev/null && DEBIAN_FRONTEND="
                   "noninteractive apt-get install -y --no-install-recommends "
                   + DEB_TOOLS + " >/dev/null")
        else:
            cmd = ("dnf -y -q --setopt=strict=0 install " + RPM_TOOLS + " >/dev/null 2>&1; "
                   "python3 -c 'import Xlib' 2>/dev/null || "
                   "{ dnf -y -q install python3-pip >/dev/null 2>&1 && "
                   "pip3 -q install python-xlib; }; command -v xdotool "
                   "Xvfb xwininfo >/dev/null")
        cmd += (" && cat >/usr/local/bin/reaper.py <<'EOF'\n"
                "import os, time\nwhile True:\n    try:\n        os.wait()\n"
                "    except ChildProcessError:\n        time.sleep(0.5)\n"
                "EOF\n")
        name = "%st" % self.pm.tag
        mounts = []
        for i, (src, dst) in enumerate(self.overlay):
            mounts += ["-v", "%s:/overlay/%d:ro,Z" % (os.path.abspath(src), i)]
            cmd += "cp /overlay/%d %s || exit 1\n" % (i, dst)
        r = subprocess.run(["podman", "run", "--name", name] + mounts +
                           [self.image, "bash", "-c", cmd], capture_output=True,
                           text=True, timeout=3600)
        open(os.path.join(self.logdir, "tools.log"), "w").write(
            r.stdout + r.stderr)
        if r.returncode == 0:
            subprocess.run(["podman", "commit", "-q", name, tag],
                           capture_output=True)
        subprocess.run(["podman", "rm", "-f", name], capture_output=True)
        if r.returncode:
            self.check(False, "test tools (not ECCE dependencies) could not "
                       "be installed", (r.stdout + r.stderr)[-1500:])
            return False
        self.image = tag
        return True

    # -- package facts ---------------------------------------------------------
    def collect_facts(self):
        deb = self.d["fam"] == "deb"
        own = "dpkg -S" if deb else "rpm -qf"
        script = r"""
. /etc/os-release; echo "os=$PRETTY_NAME"
for c in ecce ecce-remote-setup ecce-dataserver-start ecce-dataserver-adduser \
         ecce-gateway-start ecce-broker-setup ecce-diagnose; do
  p=$(command -v $c) || { echo "cmd $c MISSING"; continue; }
  echo "cmd $c $p $(%(own)s $(readlink -f $p) 2>&1 | head -1 | sed 's/:.*//')"
done
for f in /opt/ecce/server/systemd/ecce-broker.service \
         /opt/ecce/server/ecce_users_auth.so /opt/ecce/server/mosquitto.acl; do
  [ -e $f ] && echo "file $f $(%(own)s $f 2>&1 | head -1 | sed 's/:.*//')" \
            || echo "file $f MISSING"
done
if [ -x /usr/bin/systemd-analyze ]; then
  v=$(systemd-analyze verify /opt/ecce/server/systemd/ecce-broker.service 2>&1 | head -5 | tr '\n' ' ')
  echo "unit-verify: ${v:-ok (no complaints)}"
fi
""" % dict(own=own)
        o, _ = self.pm.out_img(self.image, script)
        self.facts["raw"] = o
        if deb:
            q = ("for p in ecce-client ecce-server; do echo \"== $p\"; "
                 "dpkg-query -W -f='Depends: ${Depends}\\nRecommends: "
                 "${Recommends}\\nSuggests: ${Suggests}\\n' $p; done")
        else:
            q = ("for p in ecce-client ecce-server; do echo \"== $p\"; "
                 "rpm -qR $p | grep -v -E '^(lib|rpmlib)' | sort -u | "
                 "tr '\\n' ','; echo; rpm -q --suggests $p | tr '\\n' ','; "
                 "echo; done")
        self.facts["deps"], _ = self.pm.out_img(self.image, q)
        open(os.path.join(self.logdir, "facts.txt"), "w").write(
            o + "\n" + self.facts["deps"])
        return o

    # -- helpers ---------------------------------------------------------------
    def start_container(self, name, *extra, cmd=()):
        self.pm.run(name, self.image, *extra, cmd=cmd or
                    ("python3", "/usr/local/bin/reaper.py"))

    def login(self, c, user, pw, server, what):
        o, rc = self.pm.out(c, "python3 /harness/guest.py start %s %s %s" %
                            (user, pw, server), user, timeout=300)
        try:
            s = json.loads(o.strip().splitlines()[-1])
        except Exception:
            s = {}
        org = s.get("organizer")
        self.check(org and (server == "local" or " on %s" % server in org),
                   "%s: login, the Organizer opens (%s)" % (what, org), o[-1500:])
        return s

    def quit(self, c, user, what):
        o, rc = self.pm.out(c, "python3 /harness/guest.py close", user,
                            timeout=200)
        try:
            s = json.loads(o.strip().splitlines()[-1])
        except Exception:
            s = {}
        self.check(s and not s.get("titles") and not s.get("procs"),
                   "%s: quit closes the Organizer and ends every ECCE "
                   "process" % what, o[-1500:] + self.session_log(c, user))
        return s

    def session_log(self, c, user):
        return "\n--- ~/ecce-session.log\n" + self.pm.out(
            c, "tail -n 25 ~/ecce-session.log", user)[0]

    # -- modes -------------------------------------------------------------------
    def local(self):
        self.part = "local"
        t0 = time.time()
        self.start_container("loc")
        self.pm.sh("loc", "useradd -m -s /bin/bash tester")
        o, rc = self.pm.out("loc", "ecce-dataserver-start && "
                            "ecce-dataserver-adduser -b tester %s Test User" %
                            PW["alice"], "tester")
        self.check(rc == 0, "ecce-dataserver-start and "
                   "ecce-dataserver-adduser as an ordinary user", o)
        if rc == 0:
            self.login("loc", "tester", PW["alice"], "local", "local mode")
            self.quit("loc", "tester", "local mode")
        self.t["local"] = time.time() - t0

    def server_and_client(self, tls, srv, cli, what):
        pm = self.pm
        for n in (srv, cli):
            self.start_container(n)
        pm.sh(srv, "useradd -m -s /bin/bash ecce")
        pm.sh(cli, "useradd -m -s /bin/bash alice")
        flag = " --tls" if tls else ""
        o, rc = pm.out(srv, "ecce-remote-setup --server all%s && "
                       "ecce-dataserver-start && ecce-gateway-start" % flag,
                       "ecce")
        if not self.check(rc == 0 and (not tls or "TLS is on" in o),
                          "%s: server: ecce-remote-setup --server all%s, "
                          "data server and broker started" % (what, flag), o):
            return False
        o, rc = pm.out(srv, "ecce-dataserver-adduser -b alice %s Alice Test"
                       % PW["alice"], "ecce")
        self.check(rc == 0, "%s: ecce-dataserver-adduser" % what, o)
        port = 8443 if tls else 8096
        o, rc = pm.out(cli, "timeout 5 bash -c 'echo > /dev/tcp/%s/%d' && "
                       "timeout 5 bash -c 'echo > /dev/tcp/%s/%d'" %
                       (srv, port, srv, 8883 if tls else 8088))
        self.check(rc == 0, "%s: the client machine reaches the server's "
                   "data server and broker ports" % what, o)
        if tls:
            fp, _ = pm.out(srv, "openssl x509 -noout -fingerprint -sha256 "
                           "-in ~/.ECCE/tls/server.pem", "ecce")
            o, rc = pm.out(cli, "ecce-remote-setup %s --tls --fetch-pin" % srv)
            self.check(rc == 0 and fp.split("=")[-1].strip() in o,
                       "%s: ecce-remote-setup %s --tls --fetch-pin prints "
                       "the server's fingerprint" % (what, srv),
                       o + "\nserver: " + fp)
        else:
            o, rc = pm.out(cli, "ecce-remote-setup %s" % srv)
            self.check(rc == 0, "%s: ecce-remote-setup %s" % (what, srv), o)
        s = self.login(cli, "alice", PW["alice"], srv, what)
        log, _ = pm.out(srv, "cat ~/.ECCE/dataserver/logs/access_log "
                        "~/.ECCE/dataserver/logs/ssl_access_log 2>/dev/null",
                        "ecce")
        self.check(any(" alice " in l and "PROPFIND" in l and " 207 " in l
                       for l in log.splitlines()),
                   "%s: the server's access log shows alice's PROPFIND "
                   "answered 207" % what, log[-800:])
        self.check("gateway" in s.get("procs", {}) and s.get("broker", 0) >= 1,
                   "%s: alice's gateway holds a connection to the server's "
                   "broker (%s)" % (what, s.get("broker")), json.dumps(s))
        self.quit(cli, "alice", what)
        return True

    def central(self):
        self.part = "central"
        t0 = time.time()
        self.server_and_client(False, "srv", "cli", "central")
        self.t["central"] = time.time() - t0

    def tls(self):
        self.part = "tls"
        t0 = time.time()
        self.server_and_client(True, "tsrv", "tcli", "tls")
        self.t["tls"] = time.time() - t0

    def broker(self):
        self.part = "broker"
        t0 = time.time()
        pm = self.pm
        # SYS_ADMIN: see tests/containers/README.md (sysb).
        pm.run("sysb", self.image, "--systemd=always", "--cap-add",
               "SYS_ADMIN", cmd=("/sbin/init",))
        for _ in range(40):
            o, _ = pm.out("sysb", "systemctl is-system-running")
            if o.strip() in ("running", "degraded"):
                break
            time.sleep(1)
        sh = lambda s, **k: pm.out("sysb", s, **k)
        o, rc = sh("ecce-broker-setup localhost:8088 && echo %s | "
                   "ecce-broker-setup --user alice && echo %s | "
                   "ecce-broker-setup --user bob && systemctl link "
                   "/opt/ecce/server/systemd/ecce-broker.service && "
                   "systemctl enable --now ecce-broker" % (PW["alice"], PW["bob"]))
        self.check(rc == 0, "ecce-broker-setup and systemctl link/enable "
                   "--now ecce-broker as GETTING_STARTED gives them", o)
        time.sleep(4)
        o, _ = sh("systemctl is-active ecce-broker")
        self.check(o.strip() == "active", "ecce-broker.service is active (%s)"
                   % o.strip(), sh("systemctl status ecce-broker --no-pager "
                                   "-l; journalctl -u ecce-broker --no-pager"
                                   " | tail -20")[0])
        o, _ = sh("ss -ltnpH sport = :8088")
        self.check(":8088" in o, "it listens on 8088", o)
        a = "-h 127.0.0.1 -p 8088 -u alice -P %s" % PW["alice"]
        o, rc = sh("timeout 10 mosquitto_pub %s -t ecce/alice/x -m hi -q 1"
                   % a)
        self.check(rc == 0, "alice publishes under her own topic", o)
        # Delivery decides, as in tests/containers: a refused publish can
        # still return 0 to the publisher.
        b = "-h 127.0.0.1 -p 8088 -u bob -P %s" % PW["bob"]
        sh("timeout 12 mosquitto_sub %s -t 'ecce/alice/#' -F '%%t %%p' "
           ">/tmp/cap_a 2>&1 &" % a)
        time.sleep(2)
        sh("timeout 10 mosquitto_pub %s -t ecce/alice/session/k/x -m "
           "A-own -q 1; timeout 10 mosquitto_pub %s -t "
           "ecce/alice/session/k/x -m B-intrusion -q 1" % (a, b))
        time.sleep(3)
        o, _ = sh("cat /tmp/cap_a")
        self.check("A-own" in o and "B-intrusion" not in o,
                   "alice's subscription gets her own message and not bob's "
                   "publish into her topic", o)
        o, rc = sh("timeout 10 mosquitto_pub -h 127.0.0.1 -p 8088 -t "
                   "ecce/alice/x -m hi -q 1")
        self.check(rc != 0, "anonymous clients are refused", o)
        sh("systemctl stop ecce-broker")
        self.t["broker"] = time.time() - t0

    def run(self, only, reuse):
        self.reuse = reuse
        try:
            self.pm.p("network", "create", self.pm.net)
            if "install" in only or reuse:
                if not self.install(reuse):
                    return
            else:
                self.image = "localhost/ecce-im-%s:installed" % self.key
            if not self.tools():
                return
            self.part = "install"
            self.collect_facts()
            for p in ("local", "central", "tls", "broker"):
                if p in only:
                    try:
                        getattr(self, p)()
                    except Exception as e:
                        self.part = p
                        self.check(False, "harness error: %s" % e)
        except Exception as e:
            self.check(False, "harness error: %s" % e)
        finally:
            for n in self.pm.names:
                r = self.pm.p("logs", n, check=False)
                open(os.path.join(self.logdir, n + ".log"), "w").write(
                    r.stdout + r.stderr)
            for c, u, f in (("loc", "tester", "~/ecce-session.log"),
                            ("cli", "alice", "~/ecce-session.log"),
                            ("tcli", "alice", "~/ecce-session.log"),
                            ("srv", "ecce", "~/.ECCE/mosquitto.log"),
                            ("srv", "ecce", "~/.ECCE/dataserver/logs/*_log"),
                            ("tsrv", "ecce", "~/.ECCE/mosquitto.log"),
                            ("tsrv", "ecce", "~/.ECCE/dataserver/logs/*_log")):
                try:
                    o, _ = self.pm.out(c, "tail -n 120 %s" % f, u)
                    open(os.path.join(self.logdir, "%s-%s.txt" % (
                        c, os.path.basename(f).replace("*", "all"))),
                        "w").write(o)
                except Exception:
                    pass
            try:
                o, _ = self.pm.out("sysb", "journalctl -u ecce-broker "
                                   "--no-pager")
                open(os.path.join(self.logdir, "ecce-broker-journal.txt"),
                     "w").write(o)
            except Exception:
                pass


def out_img(self, image, script):
    r = subprocess.run(["podman", "run", "--rm", image, "bash", "-c", script],
                       capture_output=True, text=True, timeout=300)
    return r.stdout + r.stderr, r.returncode


cst.Podman.out_img = lambda self, image, script: out_img(self, image, script)


def short(s, n=160):
    s = " ".join(s.split())
    return s if len(s) <= n else s[:n - 3] + "..."


def matrix(runs, commit):
    cols = ["install", "local", "central", "tls", "broker"]
    head = ["Distro", "Install client + server", "Local", "Central server",
            "TLS", "Shared broker"]
    L = ["Packages: %s." % commit, "",
         "| " + " | ".join(head) + " |", "|" + "---|" * len(head)]
    for r in runs:
        row = [r.d["name"]]
        for c in cols:
            st = r.status(c)
            if c == "install" and len(r.attempts) > 1 and st == "PASS":
                st = "PASS only after: %s" % r.attempts[-1][0].split(",")[0]
            row.append(st)
        L.append("| " + " | ".join(row) + " |")
    L += ["", "## Failures", ""]
    n = 0
    for r in runs:
        for part, ok, what, detail in r.checks:
            if not ok:
                n += 1
                L.append("- **%s / %s**: %s" % (r.d["name"], part, what))
                if detail:
                    L.append("  ```\n  " + "\n  ".join(
                        detail.splitlines()[:12]) + "\n  ```")
    if not n:
        L.append("None.")
    L += ["", "## Install attempts", ""]
    for r in runs:
        for label, rc, errs in r.attempts:
            L.append("- %s: %s: exit %d" % (r.d["name"], label, rc))
            if errs:
                L.append("  ```\n  " + "\n  ".join(
                    errs.splitlines()[:14]) + "\n  ```")
    L += ["", "## Package facts", ""]
    for r in runs:
        L.append("### %s\n\n```\n%s\n%s\n```\n" % (
            r.d["name"], r.facts.get("raw", "").strip(),
            r.facts.get("deps", "").strip()))
    L += ["## Timing (seconds, in parallel across distros)", ""]
    for r in runs:
        L.append("- %s: %s" % (r.d["name"], ", ".join(
            "%s %d" % (k, v) for k, v in r.t.items())))
    return "\n".join(L) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pkgs", default=os.path.join(REPO, "build-cmake",
                                                   "ci-artifacts"))
    ap.add_argument("--run", help="gh run id to download into --pkgs")
    ap.add_argument("--distro", default=",".join(d["key"] for d in DISTROS))
    ap.add_argument("--only", default=",".join(PARTS))
    ap.add_argument("--logdir", default=os.path.join(HERE, "logs"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--overlay", action="append", default=[],
                    metavar="FILE:/PATH", help="copy FILE over /PATH in the "
                    "installed image (try a fix without rebuilding the "
                    "package; the matrix header says so)")
    ap.add_argument("--label", default="", help="text for the matrix header "
                    "(which CI run or commit the packages come from)")
    ap.add_argument("--jobs", type=int, default=1,
                    help="distros in parallel (default 1: one at a time)")
    ap.add_argument("--reuse", action="store_true",
                    help="reuse images from an earlier run (skips the install)")
    a = ap.parse_args()
    if not shutil.which("podman") or subprocess.run(
            ["podman", "info"], capture_output=True).returncode:
        print("SKIP: podman is not available")
        return 77
    commit = a.label or a.run or "?"
    if a.run:
        os.makedirs(a.pkgs, exist_ok=True)
        r = subprocess.run(["gh", "run", "download", a.run, "-D", a.pkgs],
                           capture_output=True, text=True)
        if r.returncode:
            print("SKIP: gh run download failed: " + r.stderr)
            return 77
        r = subprocess.run(["gh", "run", "view", a.run, "--json", "headSha",
                            "-q", ".headSha"], capture_output=True, text=True)
        commit = "%s (%s)" % (a.run, r.stdout.strip()[:8])
    want = a.distro.split(",")
    ds = [d for d in DISTROS if d["key"] in want]
    ds = [d for d in ds if glob.glob(os.path.join(a.pkgs, d["art"], "ecce-*"))]
    if not ds:
        print("SKIP: no CI artifacts in %s (gh run download <id> -D DIR, or "
              "--run ID)" % a.pkgs)
        return 77
    os.makedirs(a.logdir, exist_ok=True)
    only = a.only.split(",")
    tag = str(os.getpid())
    runs = [Distro(d, a.pkgs, a.logdir, tag) for d in ds]
    for r in runs:
        r.overlay = [tuple(o.split(":", 1)) for o in a.overlay]
    t0 = time.time()
    try:
        with cf.ThreadPoolExecutor(max(1, a.jobs)) as ex:
            list(ex.map(lambda r: r.run(only, a.reuse), runs))
    finally:
        if not a.keep:
            for r in runs:
                r.pm.cleanup()
    md = matrix(runs, commit) + "\nTotal wall time %d s.\n" % (time.time() - t0)
    open(os.path.join(a.logdir, "matrix.md"), "w").write(md)
    json.dump({r.key: dict(checks=r.checks, attempts=r.attempts, t=r.t)
               for r in runs}, open(os.path.join(a.logdir, "results.json"),
                                    "w"), indent=1)
    print(md)
    bad = [1 for r in runs for c in r.checks if not c[1]]
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
