#!/usr/bin/env python3
"""The user's server list (#767), through `ecce-localdata servers`.

    serverlist_test.py <ecce-localdata binary> <scratch dir>

No display, no services: add, list, duplicate and bad entries, remove, and
the plain one-line-per-server file.
"""
import os
import shutil
import subprocess
import sys

localdata, scratch = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
failures = []


def check(ok, what):
    print(("PASS  " if ok else "FAIL  ") + what, flush=True)
    if not ok:
        failures.append(what)


shutil.rmtree(scratch, ignore_errors=True)
os.makedirs(scratch + "/.ECCE")
env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
env["ECCE_REALUSERHOME"] = scratch
fileName = scratch + "/.ECCE/AddedServers"


def run(*a):
    r = subprocess.run([localdata, "servers", *a], env=env, capture_output=True, text=True)
    return r.returncode, r.stdout.strip(), r.stderr.strip()


check(run("list") == (0, "", ""), "no list yet: empty")
rc, out, err = run("add", "ecce", "Course server", "https://ecce.example.edu")
check(rc == 0, "add an ECCE server (%s)" % err)
rc, out, err = run("add", "ssh", "Cluster storage", "sftp://me@hpc.example.edu/home/me/ecce")
check(rc == 0, "add an SSH folder (%s)" % err)
check(open(fileName).read() ==
      "ecce\tCourse server\thttps://ecce.example.edu\n"
      "ssh\tCluster storage\tsftp://me@hpc.example.edu/home/me/ecce\n",
      "the file is one line per server: type, name, address")
rc, out, err = run("list")
check(rc == 0 and out.splitlines() == [
    "ecce\tCourse server\thttps://ecce.example.edu",
    "ssh\tCluster storage\tsftp://me@hpc.example.edu/home/me/ecce"], "list returns both in order")
check(run("add", "ecce", "Course server", "https://other.example.edu")[0] == 1, "a duplicate name is refused")
check(run("add", "ecce", "x", "http://plain.example.edu")[0] == 1, "an ECCE server must be https")
check(run("add", "ssh", "y", "sftp://hpc/home")[0] == 1, "an SSH folder needs a user")
check(run("add", "ftp", "z", "ftp://a")[0] == 1, "an unknown type is refused")
check(run("add", "ecce", "", "https://a.example.edu")[0] == 1, "an empty name is refused")
check(len(run("list")[1].splitlines()) == 2, "refused entries are not stored")
check(run("remove", "Course server")[0] == 0 and
      run("list")[1] == "ssh\tCluster storage\tsftp://me@hpc.example.edu/home/me/ecce",
      "remove takes one out")
check(run("remove", "Course server")[0] == 1, "removing an unknown name fails")
with open(fileName, "a") as f:
    f.write("garbage line\nssh\tbroken\tnot-an-address\n")
check(len(run("list")[1].splitlines()) == 1, "damaged lines are skipped on load")
sys.exit(1 if failures else 0)
