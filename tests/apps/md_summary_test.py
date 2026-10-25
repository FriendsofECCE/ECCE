#!/usr/bin/env python3
"""The Builder's MD Summary panel for a GROMACS run (#334), headless.

    tests/apps/md_summary_test.py [--build build-cmake] [--png DIR]

The Trp-cage Dynamics log of data/client/examples/computational/gromacs/gromacs-trpcage goes
through the real parse scripts (the eccejobmonitor replay of tests/parsers),
the properties are stored as a calculation's Props, and the Builder (this
tree's binaries) opens "MD Summary".  The expected numbers are read from the
log text by regular expression, not from the parser.  The one-value Energies
and Pressure panels must not appear for such a run.

Exit status 77 (skip) without Xvfb, import, python3-pil/numpy or binaries.
"""
import argparse
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))

import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import eccejobmonitor_sim as sim  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
LOG = os.path.join(REPO, "data", "client", "examples", "computational", "gromacs", "gromacs-trpcage",
                   "4-dynamics", "gromacs.log")
PARSERS = os.path.join(REPO, "scripts", "parsers")
TSVEC = {"TEVEC", "TEMPVEC", "PRESSVEC", "MDPEVEC", "MDKEVEC"}


def parsedProps(log):
    """{key: [units, [values...]]} the parse scripts give for the log."""
    desc = sim.read_desc(os.path.join(PARSERS, "gromacs.desc"))
    result = sim.replay(desc, log)
    props = {}
    for block in result.blocks:
        if not block.delivered:
            continue
        out, _err, _rc = sim.run_parser(PARSERS, block.entry, block,
                                        ('.', 'Dynamics', 'MD', 'OPLS', '0'))
        for m in re.finditer(r"key: (\S+)\nsize:\n.*\nvalues:\n\s*(\S+)\nunits:\n(\S+)",
                             out):
            props.setdefault(m.group(1), [m.group(3), []])[1].append(m.group(2))
    return props


def writeProps(calc, props):
    d = os.path.join(calc, "Props")
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    for key, (units, vals) in props.items():
        if key in TSVEC:
            body = "".join('  <step number="%d">%s</step>\n' % (i + 1, v)
                           for i, v in enumerate(vals))
            doc = ('<tsvector name="%s" rowLabel="Geometry Step" rows="1" '
                   'units="%s">\n%s</tsvector>\n' % (key, units, body))
        else:
            doc = '<value name="%s" units="%s">%s</value>\n' % (key, units, vals[-1])
        with open(os.path.join(d, key), "w") as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n' + doc)


def logTerms(block):
    """name -> value of a log energies section (15-column header rows)."""
    out = {}
    lines = block.splitlines()
    for i, line in enumerate(lines[:-1]):
        vals = lines[i + 1].split()
        if re.search(r"[A-Za-z]", line) and vals and all(
                re.fullmatch(r"-?\d\.\d+e[+-]\d+", v) for v in vals):
            names = [line[j:j + 15].strip() for j in range(0, len(line), 15)]
            for n, v in zip([n for n in names if n], vals):
                out[n] = float(v)
    return out


def logValues(text):
    """Independent read of the log: the last step's terms and the averages."""
    head, _, avg = text.partition("A V E R A G E S")
    last = head[head.rindex("   Energies (kJ/mol)"):]
    return logTerms(last), logTerms(avg[:avg.index("Total Virial")])


def decimals(token):
    return len(token.split(".")[1]) if "." in token else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    try:
        import numpy  # noqa: F401
        from PIL import Image  # noqa: F401
    except ImportError:
        harness.skip("python3-pil and python3-numpy are needed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    props = parsedProps(LOG)
    check("MDTEAVG" in props and "MDPEVEC" in props,
          "the parse scripts give the summary keys")
    final, avg = logValues(open(LOG).read())
    work = tempfile.mkdtemp(prefix="mdsummary-")
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-opt"),
                    os.path.join(work, "calc-md"), symlinks=True)
    writeProps(os.path.join(work, "calc-md"), props)

    s = harness.Session(build, "mdsummary", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    say("screenshots: %s" % png)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        url = eg.install(s, "calc-md", work)
        out, shots = bo.runScript(
            s, display, env, wrappers, ["-context", url],
            ["wait 4000", 'rows MD Summary', "shot mdsummary", "quit"], png,
            "mdsummary", {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "MD Summary"})
        sets = eg.rowSets(out)
        if not check(len(sets) == 1, "one row report (got %d)" % len(sets)):
            say(out[-3000:])
            return 1
        rows = sets[0]
        say("rows:\n  " + "\n  ".join(rows))
        check("crash" not in out.lower(), "no crash in the Builder log")
        heads = eg.headings(rows)
        check(len(heads) == 2 and "Final value" in heads[0] and "Average" in heads[1],
              "Final value and Average headings: %s" % heads)

        def row(group, label):
            on = False
            for r in rows:
                if r.startswith("H "):
                    on = group in r
                elif on and r.startswith("R " + label + " |"):
                    return [x.strip() for x in r[2:].split("|")]
            return None
        for label, term in (("Potential Energy", "Potential"),
                            ("Kinetic Energy", "Kinetic En."),
                            ("Total Energy", "Total Energy"),
                            ("Temperature", "Temperature"),
                            ("Pressure", "Pressure (bar)")):
            f = row("Final value", label)
            if check(f is not None, "%s has a final value" % label):
                check(abs(float(f[1]) - final[term]) <= 0.5 * 10 ** -decimals(f[1]) + 1e-9,
                      "%s final %s is the log's %g" % (label, f[1], final[term]))
                check("e+" not in f[1] and decimals(f[1]) <= 3,
                      "%s: sensible digits" % label)
            a = row("Average", label)
            if check(a is not None and "±" in a[1], "%s has average ± s.d." % label):
                mean = a[1].split("±")[0].strip()
                check(abs(float(mean) - avg[term]) <= 0.5 * 10 ** -decimals(mean) + 1e-9,
                      "%s average %s is the log's %g" % (label, mean, avg[term]))
        check(any("kJ/mol" in r for r in rows), "units read kJ/mol")
        check(not any("kJoule" in r for r in rows), "no kJoule/Mole")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
        shutil.rmtree(work, ignore_errors=True)
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
