#!/usr/bin/env python3
"""The Launcher's machine list for a code, from site and user files.

    tests/launch/machines_test.py --build <build dir>

`launchjob machines <code>` prints what WxLauncher::populateMachinesList
shows (MachinePreferences::itemsForCode).  A machine is offered for a code
when its Machines line lists the code or CONFIG.<machine>, site or user,
gives the code a path.  The case that broke: a user MyMachines line for
localhost, saved when CONFIG.localhost named only some codes, shadows the
site line, so a code whose path was added afterwards was not offered.

Exit status 77 (CTest SKIP) without launchjob.
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

OPTS = "MN:RD:SD:UN:PW"


def line(name, codes, opts=OPTS):
    return "\t".join([name, name, "Unspecified", "Unspecified", "Unspecified",
                      "2:1", "ssh", ":" + ":".join(codes), opts]) + "\n"


SITE_MACHINES = (line("localhost", ["NWChem", "Gaussian-16", "ORCA", "MOPAC"], "WS")
                 + line("sitebox", ["NWChem"]))
# As Register Machines writes it: the codes that had a path when it was saved.
USER_LOCALHOST = line("localhost", ["Gaussian-16", "NWChem"])
USER_MACHINES = line("mybox", ["NWChem", "Gaussian-16"])


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as h:
        h.write(text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    args = ap.parse_args()
    launchjob = os.path.join(os.path.abspath(args.build), "launchjob")
    if not os.access(launchjob, os.X_OK):
        print("SKIP: no %s" % launchjob)
        return SKIP

    failures = []
    tmp = tempfile.mkdtemp(prefix="ecce-machines-")
    try:
        home = os.path.join(tmp, "home")
        user = os.path.join(tmp, "user")
        prefs = os.path.join(user, ".ECCE")
        os.makedirs(os.path.join(home, "data"))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(home, "data", "client"))
        os.makedirs(os.path.join(home, "siteconfig"))
        shutil.copy(os.path.join(REPO, "siteconfig", "DataServers"),
                    os.path.join(home, "siteconfig", "DataServers"))
        write(os.path.join(home, "siteconfig", "Machines"), SITE_MACHINES)
        write(os.path.join(home, "siteconfig", "CONFIG.sitebox"),
              "NWChem: /site/nwchem\nORCA: /site/orca\n")
        write(os.path.join(prefs, "MyMachines"), USER_LOCALHOST + USER_MACHINES)
        write(os.path.join(prefs, "CONFIG.localhost"),
              "NWChem: /usr/bin/nwchem\nGaussian-16: g16\nORCA: orca\n"
              "perlPath: /usr/bin/perl\n")
        write(os.path.join(prefs, "CONFIG.mybox"),
              "NWChem: nwchem\nMOPAC: -\n")
        env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user,
                   ECCE_REALUSER="eccetest")

        def listed(code):
            r = subprocess.run([launchjob, "machines", code], env=env,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True)
            if r.returncode != 0:
                failures.append("launchjob machines %s: exit %d %s"
                                % (code, r.returncode, r.stderr.strip()))
            return sorted(r.stdout.split())

        def expect(code, names, why):
            got = listed(code)
            ok = got == sorted(names)
            print("%s  %-12s %-34s %s" % ("ok  " if ok else "FAIL", code,
                                          " ".join(got) or "(none)", why))
            if not ok:
                failures.append("%s: got %s, want %s" % (code, got, sorted(names)))

        expect("NWChem", ["localhost", "mybox", "sitebox"],
               "listed on every Machines line")
        expect("ORCA", ["localhost", "sitebox"],
               "user CONFIG.localhost / site CONFIG.sitebox path")
        expect("MOPAC", [],
               "shadowed site line; 'MOPAC: -' is no path")
        expect("", ["localhost", "mybox", "sitebox"], "no code: all")

        # Without the user's own localhost line the site line applies.
        write(os.path.join(prefs, "MyMachines"), USER_MACHINES)
        expect("MOPAC", ["localhost"], "site Machines line lists MOPAC")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    for f in failures:
        print("  " + f)
    print("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
