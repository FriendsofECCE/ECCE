#!/usr/bin/env python3
"""The local UDP link between ECCE apps and their relay.

* The relay and every app subscriber listen on 127.0.0.1 only, and a packet
  without the session's token is dropped by both ends (#194).
* The port file holding the token is private to the user.

Runs an isolated broker and relay (tests/launch/harness.py) and the tree's
own `jmsprobe`, which is the real JMSSubscriber/JMSPublisher/AuthCache.

    tests/jms/run_tests.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import os
import re
import socket
import stat
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import harness  # noqa: E402
from harness import say  # noqa: E402

LOCAL_TOPIC = "ecce_check_child"      # FILTER BY: USER:HOSTNAME:DISPLAY


class Reader(threading.Thread):
    """Collects a child process's stdout lines."""

    def __init__(self, proc):
        super().__init__(daemon=True)
        self.proc, self.lines = proc, []
        self.start()

    def run(self):
        for line in self.proc.stdout:
            self.lines.append(line.rstrip("\n"))

    def waitFor(self, text, seconds=30):
        end = time.time() + seconds
        while time.time() < end:
            if any(text in l for l in self.lines):
                return True
            time.sleep(0.1)
        return False

    def grep(self, text):
        return [l for l in self.lines if text in l]


def udpBinding(port):
    """The local addresses the kernel has a UDP socket bound to on `port`."""
    found = []
    for path in ("/proc/net/udp", "/proc/net/udp6"):
        try:
            rows = open(path).read().splitlines()[1:]
        except OSError:
            continue
        for row in rows:
            addr = row.split()[1]
            host, p = addr.rsplit(":", 1)
            if int(p, 16) == port:
                found.append(host)
    return found


def packet(token, method, **items):
    text = "TOKENSTART%sTOKENEND" % token if token else ""
    text += "METHODSTART%sMETHODEND" % method
    for key, value in items.items():
        text += "%sSTART%s%sEND" % (key.upper(), value, key.upper())
    return text.encode()


def publishItems(topic, url):
    body = "NAMESTARTurlNAMEENDVALUESTART%sVALUEEND" % url
    return dict(topic=topic, body=body, sender="forger\n1", target="\n",
                replytopic="")


def send(port, data):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(data, ("127.0.0.1", port))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args()
    build = os.path.abspath(args.build)

    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "jmsprobe"), os.X_OK):
        harness.skip("jmsprobe is not built in %s (ninja jmsprobe)" % build)
    jar = os.path.join(REPO, "java", "lib", "ecce_jms.jar")
    if not os.path.exists(jar):
        harness.skip("java/lib jars are not built")

    s = harness.Session(build, "jms", {}, (8693, 8685), keep=args.keep)
    probe = os.path.join(s.home, "bin", "jmsprobe")
    harness.link(os.path.join(build, "jmsprobe"),
                 os.path.join(os.environ["ECCE_TEST_HOME"], "bin", "jmsprobe"))
    env = s.env()
    children = []

    def spawn(argv):
        proc = subprocess.Popen(argv, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True,
                                cwd=os.path.join(s.home, "bin"))
        children.append(proc)
        return proc, Reader(proc)

    try:
        rc, out = s.run([os.path.join(s.home, "bin", "ecce-gateway-start")],
                        timeout=240)
        say("  ecce-gateway-start: rc=%d %s" % (rc, out.strip().replace("\n", " | ")[:200]))
        if not s.check(rc == 0, "relay and broker started"):
            return finish(s)

        # --- the port file ------------------------------------------------
        portfile = os.path.join(s.state, ".ECCE", "%s_%s" % (env["HOST"], env["DISPLAY"]))
        mode = stat.S_IMODE(os.stat(portfile).st_mode)
        s.check(mode == 0o600, "port file is mode 0600 (is %o)" % mode)
        fields = open(portfile).read().split()
        s.check(len(fields) == 2 and fields[0].isdigit()
                and re.fullmatch(r"[0-9a-f]{32,}", fields[1]) is not None,
                "port file holds a port and a token of at least 128 bits")
        port, token = int(fields[0]), fields[1]

        # --- who may reach the relay ---------------------------------------
        bound = udpBinding(port)
        # Java's dual-stack socket shows 127.0.0.1 as ::ffff:127.0.0.1.
        s.check(bound in (["0100007F"], ["0000000000000000FFFF00000100007F"]),
                "relay socket is bound to 127.0.0.1 only (%s)" % bound)

        # --- an app's subscriber, with the real C++ classes ------------------
        lp, listener = spawn([probe, "listen", LOCAL_TOPIC, "25"])
        s.check(listener.waitFor("READY"), "probe subscribed")
        appPort = int(listener.grep("PORT ")[0].split()[1])
        bound = udpBinding(appPort)
        s.check(bound == ["0100007F"],
                "app subscriber socket is bound to 127.0.0.1 only (%s)" % bound)

        # Forgeries first, so a late delivery would show up.
        send(port, packet(None, "publish", **publishItems(LOCAL_TOPIC, "forged-relay-none")))
        send(port, packet("0" * 32, "publish", **publishItems(LOCAL_TOPIC, "forged-relay-wrong")))
        listenerBody = ("BODYSTART%sBODYEND" % "NAMESTARTurlNAMEENDVALUESTARTforged-appVALUEEND"
                        + "SENDERSTARTforger\n1SENDEREND"
                        + "TARGETSTART\nTARGETEND"
                        + "TOPICSTART%sTOPICEND" % LOCAL_TOPIC).encode()
        send(appPort, listenerBody)
        send(appPort, b"TOKENSTART" + b"0" * 32 + b"TOKENEND" + listenerBody)
        time.sleep(1.5)
        s.check(not listener.grep("GOT"), "forged packets (no/wrong token) delivered nothing")

        # Controls: the same packets WITH the token do get through, so the
        # silence above is the token's doing and not a malformed packet.
        send(port, packet(token, "publish", **publishItems(LOCAL_TOPIC, "control-relay")))
        send(appPort, b"TOKENSTART" + token.encode() + b"TOKENEND" + listenerBody)
        s.check(listener.waitFor("url=control-relay", 10),
                "a packet with the token is accepted by the relay")
        s.check(listener.waitFor("url=forged-app", 10),
                "a packet with the token is accepted by the app")
        s.check(not listener.grep("forged-relay") and
                len(listener.grep("GOT")) == 2, "nothing else got through")

        # Normal operation: an app publishing through the real classes.
        rc, out = s.run([probe, "publish", LOCAL_TOPIC, "url=real-publish"])
        s.check(rc == 0 and listener.waitFor("url=real-publish", 10),
                "apps still talk to each other through the relay")

        # --- the relay logs a bad packet once ------------------------------
        log = open(os.path.join(s.state, ".ECCE", "jmsdispatcher.log")).read()
        s.check(log.count("Dropped a packet without") == 1,
                "relay reported dropped packets once")

        return finish(s)
    finally:
        for proc in children:
            if proc.poll() is None:
                proc.terminate()
        for proc in children:
            try:
                proc.wait(5)
            except subprocess.TimeoutExpired:
                proc.kill()
        s.stop()


def finish(s):
    if s.failures:
        say("FAILED: %d check(s)" % len(s.failures))
        return 1
    say("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
