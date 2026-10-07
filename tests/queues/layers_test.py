#!/usr/bin/env python3
"""
Which layer a machine comes from (#192): the user's, the server's published
site files, the install's.

    layers_test.py --build <build dir>

A -remote client with the server's files cached is set up by hand, and the
RefMachine and QueueManager tables are read through build/configdump.  One
machine comes whole from one layer, the highest that has it; the machine lists
of all layers are joined; localhost is the client's own whatever the server
publishes; with no cache the lookup is the local one.

Exit status 77 (CTest SKIP) without configdump.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SKIP = 77
failures = []


def check(ok, what):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as h:
        h.write(text)


def machine(name, full):
    return ("%s\t%s\tUnspecified\tUnspecified\tUnspecified\t1:1\tssh\t:NWChem"
            "\tMN:RD:SD:UN:PW\n" % (name, full))


def queues(*entries):
    """entries: (machine, manager, prefFile or None)"""
    text = "Queues: " + " ".join(e[0] for e in entries) + "\n"
    for m, mgr, pref in entries:
        text += "%s|queueMgrName: %s\n" % (m, mgr)
        if pref:
            text += "%s|prefFile: %s\n" % (m, pref)
    return text


QFILE = "Queues: normal\nnormal|minProcessors: 1\nnormal|maxProcessors: 8\n" \
        "normal|runLimit: 60\nnormal|memLimit: 0\nnormal|memUnits: MB\n"


def parse(text):
    out = {}
    for line in text.splitlines():
        if ": " in line:
            k, v = line.split(": ", 1)
            out[k] = v
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    args = ap.parse_args()
    exe = os.path.join(args.build, "configdump")
    if not os.path.exists(exe):
        print("SKIP  needs %s" % exe)
        return SKIP

    tmp = tempfile.mkdtemp(prefix="ecce-layers-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        cache = os.path.join(user, ".ECCE", "server", "srv.example.org_8096")
        srv = os.path.join(cache, "site")
        mine = os.path.join(cache, "user-alice")
        inst = os.path.join(home, "siteconfig")
        os.makedirs(os.path.join(home, "data"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(home, "data", "client"))
        os.makedirs(inst)
        shutil.copy(os.path.join(REPO, "siteconfig", "QueueManagers"), inst)
        write(os.path.join(inst, "RemoteServer", "DataServers"),
              "<EcceData><EcceServer><Url>https://srv.example.org:8096/Ecce"
              "</Url></EcceServer></EcceData>\n")

        # install: localhost, shared, instonly
        write(os.path.join(inst, "Machines"),
              machine("localhost", "localhost") + machine("shared", "shared.inst")
              + machine("instonly", "instonly.inst"))
        write(os.path.join(inst, "Queues"),
              queues(("localhost", "Shell", None), ("shared", "PBS", "shared.Q"),
                     ("instonly", "Shell", None)))
        write(os.path.join(inst, "shared.Q"), QFILE + "# inst\n")
        # server: localhost (ignored), shared, srvonly
        write(os.path.join(srv, "Machines"),
              machine("localhost", "localhost.srv") + machine("shared", "shared.srv")
              + machine("srvonly", "srvonly.srv"))
        write(os.path.join(srv, "Queues"),
              queues(("localhost", "PBS", "localhost.Q"), ("shared", "Slurm", "shared.Q"),
                     ("srvonly", "Shell", None)))
        write(os.path.join(srv, "shared.Q"), QFILE + "# srv\n")
        write(os.path.join(srv, "localhost.Q"), QFILE)
        # the user's copy of the server registrations: srvonly overridden, one new
        write(os.path.join(mine, "MyMachines"),
              machine("srvonly", "srvonly.mine") + machine("minemachine", "mine.mine"))
        write(os.path.join(mine, "Queues"),
              queues(("minemachine", "Shell", None)))
        write(os.path.join(mine, "shared.Q"), QFILE + "# mine\n")
        # ~/.ECCE: what the client has of its own; MyMachines here is not read
        # in -remote mode once the server copy exists
        write(os.path.join(user, ".ECCE", "MyMachines"), machine("oldlocal", "old.local"))

        base = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user,
                    ECCE_REALUSER="alice")
        for k in ("ECCE_REMOTE_SERVER", "ECCE_SERVER_LOGIN"):
            base.pop(k, None)
        remote = dict(base, ECCE_REMOTE_SERVER="1", ECCE_SERVER_LOGIN="alice")

        def dump(env, *a):
            r = subprocess.run([exe] + list(a), env=env, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True)
            return r.stdout

        m = parse(dump(remote, "-machines"))
        check(m.get("shared") == "shared.srv",
              "a machine the server and the install both have comes from the server")
        check(m.get("instonly") == "instonly.inst" and m.get("srvonly") == "srvonly.mine",
              "install-only machines stay; the user's copy overrides the server's: %r" % m)
        check(m.get("minemachine") == "mine.mine", "a machine of the user's own is listed")
        check(m.get("localhost") == "localhost",
              "the server's localhost is ignored: %r" % m.get("localhost"))
        check("oldlocal" not in m,
              "~/.ECCE registrations are not read once the server copy exists")

        q = parse(dump(remote, "-queues"))
        check(q.get("shared") == "Slurm",
              "the server's queue manager for a shared machine, not the install's: %r" % q)
        check(q.get("localhost") == "Shell",
              "localhost's queue manager is the install's, not the server's")
        check(q.get("srvonly") == "Shell" and q.get("minemachine") == "Shell",
              "machines of the server and of the user have queue managers")
        qf = dump(remote, "-qfile", "shared.Q", "shared").strip()
        check(qf == os.path.join(mine, "shared.Q"), "the user's queue file wins: %s" % qf)
        os.unlink(os.path.join(mine, "shared.Q"))
        qf = dump(remote, "-qfile", "shared.Q", "shared").strip()
        check(qf == os.path.join(srv, "shared.Q"),
              "then the server's, ahead of the install's: %s" % qf)
        qf = dump(remote, "-qfile", "localhost.Q", "localhost").strip()
        check(qf == os.path.join(inst, "localhost.Q"),
              "localhost.Q is never taken from the server: %s" % qf)

        # no cache: nothing changes from local mode
        shutil.rmtree(cache)
        m = parse(dump(remote, "-machines"))
        check(m.get("shared") == "shared.inst" and "srvonly" not in m
              and m.get("oldlocal") == "old.local",
              "with no cache the lookup is the local one: %r" % m)
        m2 = parse(dump(base, "-machines"))
        check(m2 == m, "and the same as without -remote")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("")
    print("FAILED: %d" % len(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
