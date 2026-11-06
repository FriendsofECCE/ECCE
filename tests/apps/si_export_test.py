#!/usr/bin/env python3
"""
File > Export > Supporting Information in the Organizer (#625), headless.

Three stored calculations (water: NWChem-type optimisation, a G16 frequency
run, an ECCE-QM geometry+frequency run) are installed as local data.  The
ECCE_TEST_ORGANIZER hook "siexport <out.xyz> <csv|latex|none> <urls>" runs
what the menu runs; its xyz and tables must equal ecce-si-export's on the
original folders: coordinates to 1e-6 Angstrom, E and G to 1e-8 Eh, the other
fields exactly.  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
CLI = os.path.join(ROOT, "scripts", "ecce_si_export.py")
SOURCES = [os.path.join(fixture.FIXTURES, "calc-water-opt"),
           os.path.join(fixture.FIXTURES, "calc-water-freq-g16"),
           os.path.join(ROOT, "tests", "ecceqm", "fixtures", "h2o-geovib")]
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + str(detail)) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def parse(path):
    lines = open(path).read().splitlines()
    out, i = [], 0
    while i < len(lines):
        n = int(lines[i])
        head = lines[i + 1].split()
        fields = dict(f.split("=", 1) for f in head[1:] if "=" in f)
        atoms = [(l.split()[0], [float(v) for v in l.split()[1:4]])
                 for l in lines[i + 2:i + 2 + n]]
        out.append((head[0], fields, atoms))
        i += 2 + n
    return out


def same(got, want, label):
    ok = len(got) == len(want)
    for g, w in zip(got, want):
        ok = ok and g[0] == w[0] and [a[0] for a in g[2]] == [a[0] for a in w[2]]
        ok = ok and all(abs(a[1][k] - b[1][k]) < 1e-6
                        for a, b in zip(g[2], w[2]) for k in range(3))
        for key in ("charge", "mult", "code", "method"):
            ok = ok and g[1].get(key) == w[1].get(key)
        for key in ("E", "G"):
            a, b = g[1].get(key), w[1].get(key)
            ok = ok and ((a == b == "-") or (a != "-" and b != "-" and
                                              abs(float(a) - float(b)) < 1e-8))
    return check(ok, label, (got[:1], want[:1]))


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-si-data-")
    work = tempfile.mkdtemp(prefix="ecce-si-")
    try:
        project = os.path.join(data, "users", "local", "si-test")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        urls = []
        for src in SOURCES:
            dst = os.path.join(project, os.path.basename(src))
            shutil.copytree(src, dst, symlinks=True)
            urls.append("file://" + dst)
        outs = {k: os.path.join(work, "gui-" + k + ".xyz") for k in ("none", "csv", "latex")}
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as h:
            for k, o in outs.items():
                h.write("siexport %s %s %s\n" % (o, k, ",".join(urls)))
            h.write("siexport %s none file://%s\n" % (os.path.join(work, "bad.xyz"),
                                                      os.path.join(project, "nonexistent")))
        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": urls[0],
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        ans = re.findall(r"ECCE_TEST_ORGANIZER: siexport .*?: (ok|failed.*)", log)
        if not check(len(ans) == 4 and ans[:3] == ["ok"] * 3, "the menu path ran", ans or log[-1500:]):
            return
        check(ans[3].startswith("failed"), "a missing calculation is reported, not hidden", ans[3])

        ref = os.path.join(work, "cli.xyz")
        subprocess.run([sys.executable, CLI] + SOURCES + ["-o", ref, "--table", "csv"], check=True)
        subprocess.run([sys.executable, CLI] + SOURCES + ["-o", ref, "--table", "latex"], check=True)
        want = parse(ref)
        got = parse(outs["none"])
        same(got, want, "xyz equals the CLI's on the stored folders")
        check(os.path.splitext(outs["csv"])[0] + ".csv" and
              os.path.exists(os.path.splitext(outs["csv"])[0] + ".csv"), "csv table written")
        rows = lambda p: [l.split(",") for l in open(p).read().splitlines()]
        g, w = rows(os.path.join(work, "gui-csv.csv")), rows(os.path.join(work, "cli.csv"))
        check(len(g) == len(w) == 4 and g[0] == w[0] and all(
              a[:5] == b[:5] and a[7] == b[7] and
              all((x == y == "-") or abs(float(x) - float(y)) < 1e-8
                  for x, y in zip(a[5:7], b[5:7])) for a, b in zip(g[1:], w[1:])),
              "csv equals the CLI's", (g, w))
        tex = open(os.path.join(work, "gui-latex.tex")).read()
        check(tex.count(r"\begin{tabular}") == tex.count(r"\end{tabular}") == 1 and
              not re.search(r"(?<!\\)_", re.sub(r"\$[^$]*\$", "", tex)),
              "LaTeX table is balanced with escaped underscores")
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("si-export", "SI export wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
