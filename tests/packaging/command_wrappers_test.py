#!/usr/bin/env python3
"""Check installed bin programs against PATH wrappers using the install manifest.

Run after installation; no GUI or services are started. Unknown bin entries
must have a wrapper or be classified explicitly as internal here.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


# Startup, service and dialog plumbing is called by ECCE, never by users.
INTERNAL = {
    "ecce-dialog", "ecce-find-mosquitto", "ecce-first-start",
    "ecce-broker-win", "ecce-prewarm", "ecce-splash",
    "ecce-session-lib.sh", "ecce-apparmor-mosquitto", "ecce-broker-run",
    "ecce-flock", "ecce-nwchem-datadir",
}
# These public job drivers live in scripts/ because gensub also stages them.
SCRIPT_COMMANDS = {"ecce-mlp", "ecce-confsearch", "ecce-csh2sh"}


def staged(path):
    """The file of an installed path, under $DESTDIR for a staged install."""
    root = os.environ.get("DESTDIR")
    return Path(root) / path.relative_to("/") if root else path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--home", required=True, type=Path)
    parser.add_argument("--wrappers", required=True, type=Path)
    args = parser.parse_args()
    manifest = args.build / "install_manifest.txt"
    if not manifest.is_file():
        parser.error("install first: install_manifest.txt is missing")
    installed = {Path(line) for line in manifest.read_text().splitlines()}
    programs = sorted(p for p in installed if p.parent == args.home / "bin")
    if not programs:
        parser.error("install manifest contains no ECCE bin programs")
    programs += sorted(args.home / "scripts" / name for name in SCRIPT_COMMANDS)
    launch = staged(args.wrappers / "ecce-launch").read_text()
    failures = []
    checked = 0
    for program in programs:
        name = program.name.removesuffix(".exe")
        if name in INTERNAL:
            print("INTERNAL", name)
            continue
        command = name if name.startswith("ecce-") else "ecce-" + name
        wrapper = args.wrappers / command
        if program not in installed or not staged(program).is_file():
            failures.append("installed program missing: " + str(program))
            continue
        if wrapper not in installed or not staged(wrapper).is_file():
            failures.append("no installed wrapper for %s: %s" % (name, command))
            continue
        checked += 1
        print("WRAPPED %s -> %s" % (name, command))
        text = staged(wrapper).read_text()
        target = program.relative_to(args.home).as_posix()
        # The plain CLI wrappers must forward to the installed name, including
        # scripts renamed from Python sources. Legacy GUI wrappers do more work.
        if name.startswith("ecce-") and name != "ecce-csh2sh":
            if 'exec "$ECCE_HOME/%s" "$@"' % target not in text:
                failures.append("wrong exec target or argument forwarding: " + command)
        if text != launch.replace("bin/ecce-launch", target):
            continue
        # Replace the target with a probe, preserving spaces and empty args.
        # ECCE_LOCAL_DATA prevents the wrapper from consulting user settings.
        with tempfile.TemporaryDirectory(prefix="wrapper-probe-", dir=args.build) as tmp:
            root = Path(tmp)
            probe = root / target
            probe.parent.mkdir(parents=True, exist_ok=True)
            probe.write_text('#!/bin/bash\n'
                             'test "$#" = 2 && test "$1" = "two words" && '
                             'test -z "$2" || exit 91\n'
                             'test -n "$ECCE_REALUSER" && '
                             'test "$ECCE_REALUSERHOME" = "$HOME" && '
                             'test -n "$HOST" || exit 92\n'
                             'exit 37\n')
            probe.chmod(0o755)
            env = dict(os.environ, ECCE_HOME=str(root), HOME=str(root),
                       ECCE_LOCAL_DATA=str(root / "data"))
            for key in ("ECCE_REALUSER", "ECCE_REALUSERHOME", "HOST"):
                env.pop(key, None)
            result = subprocess.run(["bash", str(staged(wrapper)), "two words", ""],
                                    env=env, capture_output=True, text=True, timeout=15)
            if result.returncode != 37:
                failures.append("environment/arguments/exit status: %s: %s %s" %
                                (command, result.returncode, result.stderr))
    for failure in failures:
        print("FAIL", failure)
    print("%s: %d installed commands checked" % ("FAIL" if failures else "PASS", checked))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
