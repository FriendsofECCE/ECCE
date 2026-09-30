#!/usr/bin/env python3
"""Compare the scheduler directive lines of two sets of golden scripts.

    directive_diff.py [REV]     golden/ in the working tree against REV (default HEAD)

A change to the shell syntax must leave every directive alone; the only
permitted difference is the line that names the job script's shell.
"""
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DIRECTIVE = re.compile(r"^#(?:SBATCH|PBS|BSUB|MSUB|CONDOR)\b|^#\$ ")
SHELL_LINE = re.compile(r"^#(?:PBS -S|BSUB -L|\$ -S) /bin/(?:c?sh)\s*$")


def directives(text):
    return [l for l in text.splitlines() if DIRECTIVE.match(l)]


def main():
    rev = sys.argv[1] if len(sys.argv) > 1 else "HEAD"
    repo = subprocess.check_output(["git", "-C", HERE, "rev-parse", "--show-toplevel"]).decode().strip()
    bad = 0
    shell_changes = 0
    for path in sorted(glob.glob(os.path.join(HERE, "golden", "*.sh"))):
        rel = os.path.relpath(path, repo)
        old = subprocess.check_output(["git", "-C", repo, "show", "%s:%s" % (rev, rel)]).decode()
        new = open(path).read()
        a, b = directives(old), directives(new)
        a_rest = [l for l in a if not SHELL_LINE.match(l)]
        b_rest = [l for l in b if not SHELL_LINE.match(l)]
        shell = [(x, y) for x, y in zip([l for l in a if SHELL_LINE.match(l)],
                                        [l for l in b if SHELL_LINE.match(l)]) if x != y]
        shell_changes += len(shell)
        ok = a_rest == b_rest and len(a) == len(b)
        print("%-28s %3d directives  %s%s" % (os.path.basename(path), len(a),
              "identical" if ok else "DIFFERENT",
              ("  (shell line: %s -> %s)" % shell[0]) if shell else ""))
        if not ok:
            bad += 1
            for l in sorted(set(a) ^ set(b)):
                print("    %s %s" % ("-" if l in a else "+", l))
    print("%d files differ in directives, %d shell-selection lines changed" % (bad, shell_changes))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
