"""
The post-hoc importers: ``scripts/parsers/<Code>.expt``.

A different pipeline from the rest of this suite.  ``eccejobmonitor`` tails a
*running* job and feeds matched blocks to the per-property parser scripts;
an ``.expt`` script is handed a *finished* output file and reconstructs the
calculation from it:

    <Code>.expt <outfile>   ->   <out>.frag    the structure, in mvm format
                                 <out>.param   title, theory, runtype, charge
                                 <out>.gbs     the basis set

That is the path used when a job run outside ECCE is imported, and it is the
path issue #94 (dummy submission, for 2FA-blocked HPC) would lean on almost
entirely.  Nothing tested it before: the main suite excludes it by design.

These checks are deliberately light on assertions and heavy on golden files.
The `.expt` scripts are the oldest, least-touched code in the tree, and the
useful question right now is "did this change" rather than "is this right" --
several of their behaviours are debatable and are written up in
``expt_cases.py`` rather than encoded as pass/fail.
"""

import os
import re
import shutil
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
FIXTURES = os.path.join(HERE, "fixtures")

# What an .expt run produces, by extension.
PRODUCTS = (".frag", ".param", ".gbs")


class ExptError(Exception):
    pass


def run(script, outputPath):
    """Run one .expt against one output file in a scratch directory.

    Copies the output file first: these scripts write their products next to
    their input, and a suite that scattered .frag files through a checked-in
    fixture tree would be a nuisance at best.

    Returns {extension: text} plus 'stdout'/'stderr'/'returncode'.
    """
    path = os.path.join(SCRIPTS, script)
    if not os.path.exists(path):
        raise ExptError("no such importer: %s" % path)
    if not os.path.exists(outputPath):
        raise ExptError("no such fixture output: %s" % outputPath)

    workdir = tempfile.mkdtemp(prefix="ecce-expt-")
    try:
        local = os.path.join(workdir, os.path.basename(outputPath))
        shutil.copyfile(outputPath, local)
        env = dict(os.environ)
        env.setdefault("ECCE_HOME", REPO)
        result = subprocess.run(["perl", path, os.path.basename(local)],
                                cwd=workdir, env=env,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=120)
        produced = {
            "stdout": result.stdout.decode("utf-8", "replace"),
            "stderr": result.stderr.decode("utf-8", "replace"),
            "returncode": result.returncode,
        }
        stem = os.path.splitext(local)[0]
        for extension in PRODUCTS:
            candidate = stem + extension
            if os.path.exists(candidate):
                with open(candidate, errors="replace") as handle:
                    produced[extension] = handle.read()
        return produced
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


def atoms(frag):
    """[(symbol, x, y, z), ...] from a .frag, for shape assertions."""
    found = []
    inList = False
    for line in (frag or "").splitlines():
        if line.startswith("atom_list:"):
            inList = True
            continue
        if not inList:
            continue
        parts = line.split()
        if len(parts) == 4:
            try:
                found.append((parts[0], float(parts[1]), float(parts[2]),
                              float(parts[3])))
            except ValueError:
                break
    return found


def params(text):
    """key -> value from a .param file."""
    found = {}
    for line in (text or "").splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        # Several values are written with a leading \\| marker meaning
        # "free text"; strip it so cases can assert on the value itself.
        found[key.strip()] = value.strip().lstrip("\\|")
    return found


def normalise(text):
    """Blank out values that legitimately differ between runs.

    The .param carries the job's own start/end date and time, straight out of
    the output file.  Those are stable for a checked-in fixture, but the
    scripts also stamp a few fields from the CURRENT clock, which would make
    a golden file fail tomorrow for no reason.
    """
    out = []
    for line in (text or "").splitlines():
        if re.match(r'^\s*(date|Date|CreationDate|ModifiedDate)\s*:', line):
            line = re.sub(r':.*', ': <normalised>', line)
        out.append(line)
    return "\n".join(out) + ("\n" if text and text.endswith("\n") else "")
