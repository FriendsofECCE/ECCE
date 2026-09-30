#!/usr/bin/env python3
"""Introductory chemistry teaching calculations, end to end.

Each molecule is set up the way a student sets it up in the Calculation
Editor: the input deck comes from the real scripts/parsers/ai.nwchem (fed the
dialogs' own default settings), is launched through the real Launch class on
the local machine, monitored by eccejobmaster/eccejobstore/eccejobmonitor,
and the properties ECCE stored are checked against known chemistry: valence
MO ordering in the diatomics, bond angles across a group and a period,
hypervalent isomer energies, pi systems.

    tests/teaching/run_tests.py [--build build] [--jobs N] [--case NAME ...]
                                [--group A|B|C] [--transport unset|direct|ssh|both]
                                [--keep] [-v]

Needs NWChem and the prerequisites of tests/launch.  Exit status 77 (CTest
SKIP) when one is missing.
"""

import argparse
import concurrent.futures
import os
import re
import shutil
import sys
import time
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "launch"))
sys.path.insert(0, HERE)

import harness  # noqa: E402  (tests/launch/harness.py)
from harness import Session, say  # noqa: E402
sys.path.insert(0, HERE)

import analysis as A  # noqa: E402
import teaching_cases as C  # noqa: E402
import deckgen  # noqa: E402
import props as P  # noqa: E402


class Result(object):
    def __init__(self, case, mode):
        self.case, self.mode = case, mode
        self.state = "-"
        self.checks = []          # (ok, text)
        self.known = []           # faults found in ECCE that the suite only reports
        self.notes = []           # (key, text) for the table
        self.values = {}
        self.te = None
        self.final = None
        self.orbs = []
        self.group = None
        self.deck = ""
        self.seconds = 0.0
        self.rundir = None

    def note(self, key, text):
        self.notes = [n for n in self.notes if n[0] != key] + [(key, text)]

    def check(self, ok, text):
        self.checks.append((bool(ok), text))
        return ok

    @property
    def ok(self):
        return self.state == "completed" and all(c[0] for c in self.checks)

    @property
    def clean(self):
        return self.ok and not self.known


REQUIRED = ("TE", "GEOMTRACE", "MO", "ORBENG", "ORBOCC", "ORBSYM")


def stable_props(pdir, seconds=30):
    """Wait until nothing in Props/ changes for two seconds."""
    last, since = None, time.time()
    deadline = time.time() + seconds
    while time.time() < deadline:
        try:
            snap = sorted((n, os.path.getmtime(os.path.join(pdir, n)))
                          for n in os.listdir(pdir))
        except OSError:
            snap = None
        if snap != last:
            last, since = snap, time.time()
        elif time.time() - since >= 2.0:
            return
        time.sleep(0.5)


def propsDir(s, url):
    path = url.split("://", 1)[1].split("/", 1)[1]        # Ecce/users/...
    return os.path.join(s.fixture.stateDir(), "htdocs", path, "Props")


def last_block(blocks, prefix):
    tags = [t for t in blocks if t.startswith(prefix)]
    return tags[-1] if tags else None


def run_one(s, case, mode, deck, stamp):
    r = Result(case, mode)
    r.deck = deck
    t0 = time.time()
    try:
        name = "teach-%s%s-%s" % (case.name, "" if mode == "unset" else "-" + mode, stamp)
        rundir = os.path.join(s.state, "jobs")
        os.makedirs(rundir, exist_ok=True)
        deckfile = os.path.join(s.state, "deck-%s.nw" % name)
        with open(deckfile, "w") as h:
            h.write(deck)
        url, out = s.create(name, "nwchem_es", deckfile, "nwch.nw", rundir)
        if not r.check(url is not None, "calculation created"):
            r.state = "create failed"
            r.check(False, out.strip()[-300:])
            return r
        rc, out = s.launch(url)
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
               if l.startswith("run directory:")]
        r.rundir = ran[-1] if ran else None
        if not r.check(rc == 0, "Launch ran to the end"):
            r.check(False, out.strip()[-400:])
            r.state = "launch failed"
            return r
        r.state = s.waitState(url, case.timeout) or "no state"
        if not r.check(r.state == "completed", "state completed (last: %s)" % r.state):
            return r
        pdir = propsDir(s, url)
        stable_props(pdir)
        pr = P.Props(pdir)
        missing = [n for n in REQUIRED if not pr.has(n)]
        r.check(not missing, "properties present: %s%s" % (
            " ".join(n for n in REQUIRED if pr.has(n)),
            " (MISSING %s)" % " ".join(missing) if missing else ""))
        if missing:
            return r
        natoms = len(case.atoms)
        frames = pr.frames(natoms)
        r.final = frames[-1]
        r.te = pr.value("TE")
        r.orbs = A.orbitals(pr)
        r.group = (pr.strings("PNTGRP") or [None])[-1] if pr.has("PNTGRP") else None
        check_generic(r, pr, frames, case)
        check_deck(r)
        for fn in case.checks:
            try:
                res = fn(r)
            except Exception as exc:               # a check must not hide the others
                res = (False, "check raised %r" % exc)
            if res:
                r.check(*res)
    except Exception:
        r.state = "error"
        r.check(False, traceback.format_exc()[-600:])
    finally:
        r.seconds = time.time() - t0
    return r


def check_generic(r, pr, frames, case):
    """What every job must show, whatever the molecule."""
    r.note("E (Eh)", "%.6f" % r.te)
    r.note("point group", r.group or "?")
    r.check(len(r.orbs) > 0 and all(o.sym for o in r.orbs), "ORBENG/ORBOCC/ORBSYM have %d entries each" % len(r.orbs))
    if not r.rundir:
        return
    trace = os.path.join(r.rundir, "ecce.out")
    if not os.path.exists(trace):
        r.check(False, "raw trace %s not found" % trace)
        return
    blocks = P.trace_blocks(trace)
    #  Final geometry: GEOMTRACE's last frame is NWChem's last geometry.
    xyz = P.trace_last(blocks, "cartesian coordinates")
    if xyz:
        ref = [float(x) for x in xyz]
        got = [c for a in r.final for c in a]
        dev = max(abs(a - b) for a, b in zip(ref, got))
        r.check(dev < 1e-3, "GEOMTRACE last frame is NWChem's final geometry (max dev %.1e A)" % dev)
    te = P.trace_last(blocks, "total energy")
    if te:
        r.check(abs(float(te[0]) - r.te) < 1e-7, "TE %.8f is the last energy NWChem printed (%.8f)" % (r.te, float(te[0])))
    #  #198: the orbitals must belong to the final geometry.
    tag = last_block(blocks, "molecular orbital energies")
    if tag:
        ref = [float(x) for x in P.trace_last(blocks, tag)]
        first = [float(x) for x in blocks[tag][0] and " ".join(blocks[tag][0]).split()]
        dev = max(abs(a - b.e) for a, b in zip(ref, r.orbs)) if len(ref) == len(r.orbs) else 9.9
        r.check(dev < 1e-8, "ORBENG is the last orbital set NWChem printed, i.e. the final geometry's "
                "(max dev %.1e Eh, %d orbital sets in the trace)" % (dev, len(blocks[tag])))
        if len(blocks[tag]) > 1 and case.runtype == "Geometry":
            moved = max(abs(a - b) for a, b in zip(ref, first))
            r.note("orbitals moved by", "%.3f Eh" % moved)
    out = os.path.join(r.rundir, "nwch.nwout")
    if case.runtype == "Geometry" and os.path.exists(out):
        with open(out, errors="replace") as h:
            text = h.read()
        steps = len(re.findall(r"^\s+Step\s+\d+\s*$", text, re.M))
        r.check("Optimization converged" in text, "NWChem's optimiser converged (%d steps)" % steps)
        r.note("opt steps", str(steps))


#  Faults in ECCE seen by this suite and not yet fixed: listed in every run,
#  and failures under --strict, so they stay visible without turning the
#  whole suite red.
def check_deck(r):
    stray = sorted(set(re.findall(r"##\w+##", r.deck)))
    if stray:
        r.known.append("generated deck still contains the unresolved template "
                       "tag(s) %s (ai.nwchem's DispersionCorrection returns "
                       "empty without setting $_; NWChem reads it as a comment)"
                       % " ".join(stray))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(harness.REPO, "build"))
    ap.add_argument("--jobs", type=int, default=1, help="calculations to run at once")
    ap.add_argument("--case", action="append", help="only this case (repeatable)")
    ap.add_argument("--group", choices=("A", "B", "C"), help="only set A, B or C")
    ap.add_argument("--transport", default="unset",
                    choices=("unset", "direct", "ssh", "both"),
                    help="ECCE_TRANSPORT for the ECCE processes started; 'both' runs "
                    "everything under unset, then under direct")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--show-decks", action="store_true")
    ap.add_argument("--strict", action="store_true",
                    help="count the known ECCE faults as failures")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    chosen = [c for c in C.CASES
              if (not args.case or c.name in args.case)
              and (not args.group or c.group == args.group)]
    if args.list:
        for c in C.CASES:
            print("%-14s %s  %s" % (c.name, c.group, c.title))
        return 0
    if not chosen:
        say("no such case")
        return 2
    #  He2's reference energy needs the atom.
    if any(c.name == "he2" for c in chosen) and not any(c.name == "he" for c in chosen):
        chosen.insert(0, [c for c in C.CASES if c.name == "he"][0])

    build = os.path.abspath(args.build)
    harness.prerequisites(build, ("nwchem", "perl"))
    modes = ["unset", "direct"] if args.transport == "both" else [args.transport]

    say("generating %d input decks with ai.nwchem" % len(chosen))
    decks = {}
    try:
        with deckgen.Defaults() as defaults:
            for c in chosen:
                try:
                    decks[c.name] = deckgen.generate(c, defaults)[0]
                except RuntimeError as exc:
                    decks[c.name] = None
                    say("  %s: input generation FAILED: %s" % (c.name, exc))
    except deckgen.dialogs.HarnessUnavailable as exc:
        harness.skip(str(exc))
    if args.show_decks:
        for c in chosen:
            say("=== %s\n%s" % (c.name, decks[c.name]))

    s = Session(build, "teach", {"NWChem": shutil.which("nwchem")}, (8596, 8588),
                keep=args.keep, transport=modes[0])
    #  Earlier runs' calculations only make the server slower to start.
    users = os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", s.user())
    if os.path.isdir(users):
        for d in os.listdir(users):
            if d.startswith("teach-"):
                shutil.rmtree(os.path.join(users, d), ignore_errors=True)
    shutil.rmtree(os.path.join(s.state, "jobs"), ignore_errors=True)

    all_results = []
    t0 = time.time()
    try:
        if not s.services(True):
            say("FAILED: services did not start")
            return 1
        for mode in modes:
            s.transport = None if mode == "unset" else mode
            say("--- running %d calculations, %d at a time (ECCE_TRANSPORT=%s)"
                % (len(chosen), args.jobs, mode))
            stamp = str(int(time.time()))
            results = {}
            # longest first, so the slow ones do not start last
            order = sorted(chosen, key=lambda c: -c.timeout)
            with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
                futs = {}
                for c in order:
                    if decks[c.name] is None:
                        r = Result(c, mode)
                        r.state = "deck failed"
                        r.check(False, "ai.nwchem failed")
                        results[c.name] = r
                        continue
                    futs[pool.submit(run_one, s, c, mode, decks[c.name], stamp)] = c
                for f in concurrent.futures.as_completed(futs):
                    r = f.result()
                    results[r.case.name] = r
                    say("  %-13s %-10s %5.0fs  %s" % (r.case.name, r.state, r.seconds,
                                                     "ok" if r.ok else "FAIL"))
            all_results.append((mode, results, C.compare(results)))
        #  A process's exe is the only proof of which build was started.
        outside = [p for p in s.seen.values() if os.path.dirname(p) != s.build]
        for p in outside:
            say("FAIL ran from outside the build: %s" % p)
        all_results[-1][2].extend((False, "ran from outside the build: %s" % p)
                                  for p in outside)
        say("binaries seen running: %s" % ", ".join(sorted(s.seen.values())))
    finally:
        left = s.stop()
        if left:
            say("processes left running: %r" % left)

    return report(all_results, args, time.time() - t0)


def report(all_results, args, elapsed):
    failures = 0
    for mode, results, compares in all_results:
        order = [c.name for c in C.CASES if c.name in results]
        say("")
        say("=" * 100)
        say("RESULTS (ECCE_TRANSPORT=%s)" % mode)
        for name in order:
            r = results[name]
            say("")
            say("%s  [%s]  %s  -> %s  (%.0fs)" % (r.case.title, r.case.settings, r.state,
                                                  "PASS" if r.ok else "FAIL", r.seconds))
            for ok, text in r.checks:
                if args.verbose or not ok:
                    say("    %s %s" % ("ok  " if ok else "FAIL", text))
        say("")
        say("KNOWN ECCE FAULTS SEEN%s" % (" (failing: --strict)" if args.strict else ""))
        seen = {}
        for name in order:
            for text in results[name].known:
                seen.setdefault(text, []).append(name)
        for text, names in seen.items():
            say("    %s %s\n        in: %s" % ("FAIL" if args.strict else "note", text, " ".join(names)))
            failures += 1 if args.strict else 0
        if not seen:
            say("    none")
        say("")
        say("COMPARISONS")
        for ok, text in compares:
            say("    %s %s" % ("ok  " if ok else "FAIL", text))
            failures += 0 if ok else 1
        say("")
        say("%-14s %-30s %-10s %-14s %s" % ("molecule", "settings", "status", "E (Eh)", "key numbers"))
        say("-" * 100)
        for name in order:
            r = results[name]
            keys = "; ".join("%s %s" % kv for kv in r.notes if kv[0] not in ("E (Eh)",))
            say("%-14s %-30s %-10s %-14s %s" % (
                r.case.name, r.case.settings[:30],
                ("PASS" if r.ok else "FAIL") if r.state == "completed" else r.state[:10],
                "%.6f" % r.te if r.te is not None else "-", keys))
            failures += 0 if r.ok else 1
    say("")
    say("%s (%d failing checks/jobs, %.0f s)" % ("PASSED" if not failures else "FAILED",
                                              failures, elapsed))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
