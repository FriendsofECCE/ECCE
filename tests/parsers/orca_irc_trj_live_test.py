#!/usr/bin/env python3
"""ORCA IRC structures arrive while the IRC runs (#949).

    tests/parsers/orca_irc_trj_live_test.py

The output is replayed through orca.desc as the monitor feeds it, and the
_IRC_F_trj.xyz / _IRC_B_trj.xyz files are fed to orca.irctrj the way the
monitor's Follow=yes read sends them ("#ECCE-FOLLOW <file> <offset>" and the
complete lines added since): a frame sometimes before its row of the output
(ORCA's output is buffered), sometimes after, sometimes cut after its first
lines, and once the whole file again from offset 0 (a restarted monitor).
hcn_irc4 is a real ORCA 6.1.1 run with its own _F/_B files; for the other
outputs they are made from _IRC_Full_trj.xyz (backward half reversed,
forward half).  Checked:

* frames come in run order (TS, forward, backward), each once; the TS is the
  output's last structure before the path, and equals the middle frame of
  _IRC_Full_trj.xyz;
* the vectors are in the same run order, point i has frame i's energy;
* sorted by coordinate (what the readers do), frames, coordinates and
  energies are those an import stores from _IRC_Full_trj.xyz and the
  summary; at the end the _IRC_Full_trj.xyz entry adds no frame;
* cut in the forward and in the backward path: one frame per point so far;
* without followed files (an import) the stored result is unchanged:
  _IRC_Full_trj.xyz frames and the summary in path order.
"""
import atexit
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
EX = os.path.join(REPO, "data", "client", "examples", "organic", "reactions")
FIX = os.path.join(HERE, "fixtures", "orca")
sys.path.insert(0, HERE)
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output, Block)

ROW = re.compile(r"^\s+(\d+)\s+(-\d+\.\d+)\s+-?\d+\.\d+\s+\d+\.\d+\s+\d+\.\d+\s*$")
BANNER = re.compile(r"^\s*\*\s+(FORWARD|BACKWARD) IRC\s+\*")
F_NAME, B_NAME = "orca.orcain_IRC_F_trj.xyz", "orca.orcain_IRC_B_trj.xyz"
ARGS = (".", "IRC", "SCF", "DFT", "0")
DESC = read_desc(os.path.join(SCRIPTS, "orca.desc"))


def frames_of(text):
    """[(comment, [[x, y, z], ...])] of an xyz trajectory."""
    lines, out, i = text.splitlines(), [], 0
    while i < len(lines) and lines[i].strip():
        n = int(lines[i])
        out.append((lines[i + 1], [[float(v) for v in ln.split()[1:4]]
                                   for ln in lines[i + 2:i + 2 + n]]))
        i += n + 2
    return out


def xyz(frames, tag):
    s = ""
    for comment, c in frames:
        e = re.search(r"\sE\s+(\S+)", comment).group(1)
        s += "%d\nCoordinates from ORCA-job orca.orcain_IRC_%s E %s\n" % (len(c), tag, e)
        s += "".join("  X %12.6f %12.6f %12.6f\n" % tuple(a) for a in c)
    return s


class Run(object):
    def __init__(self, fw, bw):
        self.work = tempfile.mkdtemp(prefix="ecce-orcatrj-")
        atexit.register(shutil.rmtree, self.work, True)
        self.files = {F_NAME: fw, B_NAME: bw}
        self.sent = {F_NAME: 0, B_NAME: 0}
        self.recs = []

    def script(self, name, text):
        entry = next(e for e in DESC.entries if e.script == name)
        blk = Block(entry, 1, 0)
        blk.lines = [text]
        out, err, rc = run_parser(SCRIPTS, entry, blk, ARGS, workdir=self.work)
        if rc != 0:
            sys.exit("%s failed: %s" % (name, err))
        self.recs.extend((name, r) for r in parse_parser_output(out))

    def feed(self, name, upto, start=None):
        """What a Follow read sends: complete lines from the last read to upto."""
        text = self.files[name]
        upto = text.rfind("\n", 0, min(upto, len(text))) + 1
        start = self.sent[name] if start is None else start
        if upto <= start:
            return
        self.script("orca.irctrj", "#ECCE-FOLLOW %s %d\n%s" % (name, start, text[start:upto]))
        self.sent[name] = max(self.sent[name], upto)

    def frame_end(self, name, k, extra_lines=0):
        """Byte offset just after frame k (0-based) plus extra_lines lines."""
        text, pos = self.files[name], 0
        for _ in range(k + 1):
            if pos >= len(text):
                return len(text)
            n = int(text[pos:text.index("\n", pos)])
            for _ in range(n + 2):
                pos = text.index("\n", pos) + 1
        for _ in range(extra_lines):
            nl = text.find("\n", pos)
            pos = nl + 1 if nl >= 0 else pos
        return pos

    def last(self, key):
        got = [r for _n, r in self.recs if r["key"] == key]
        return got[-1] if got else None

    def frames(self):
        out = []
        for _n, r in self.recs:
            if r["key"] == "IRCGEOM":
                v = [float(x) for x in r["flat"]["values"].split()]
                out.append([v[i:i + 3] for i in range(0, len(v), 3)])
        return out


def vec(r):
    return [float(x) for x in r["flat"]["values"].split()] if r else []


def live(path, fw, bw, cut=None):
    """Replay the output (to line cut) with the trajectories fed alongside."""
    run = Run(fw, bw)
    lines = open(path, errors="replace").read().splitlines(True)
    if cut is not None:
        lines = lines[:cut]
    src = os.path.join(run.work, "out.txt")
    open(src, "w").write("".join(lines))
    result = replay(DESC, src)
    blocks = sorted(((b.begin_line, e, b) for e in DESC.live_entries()
                     for b in result.delivered_for(e)), key=lambda t: t[0])
    direction, j, restarted = None, 0, False
    summary = []
    for line, entry, block in blocks:
        if entry.script == "orca.irc":
            summary.append(block)
            continue
        text = block.text
        m = BANNER.match(text)
        if m:
            direction = F_NAME if m.group(1) == "FORWARD" else B_NAME
        m = ROW.match(text)
        if not (m and direction):
            run.script(entry.script, text)
            continue
        k = int(m.group(1))
        if direction == B_NAME and k == 1 and not restarted:
            run.feed(F_NAME, run.sent[F_NAME], start=0)   # a restarted monitor
            restarted = True
        upto = run.frame_end(direction, k, extra_lines=2 if j % 3 == 0 else 0)
        if j % 2 == 0:
            run.feed(direction, upto)
            run.script(entry.script, text)
        else:
            run.script(entry.script, text)
            run.feed(direction, upto)
        j += 1
    if cut is None:
        for name in (F_NAME, B_NAME):          # the drain when the job ends
            run.feed(name, len(run.files[name]))
        for block in summary:
            run.script("orca.irc", block.text)
    return run


def post(path, full):
    """An import: the output, then the _IRC_Full_trj.xyz entry."""
    run = Run("", "")
    result = replay(DESC, path)
    for _l, entry, block in sorted(((b.begin_line, e, b) for e in DESC.live_entries()
                                    for b in result.delivered_for(e)), key=lambda t: t[0]):
        run.script(entry.script, block.text)
    run.script("orca.ircgeom", full)
    return run


def close(a, b, tol):
    return len(a) == len(b) and all(abs(x - y) <= tol for x, y in zip(a, b))


def same_geom(f, g):
    return len(f) == len(g) and all(close(a, b, 2e-6) for a, b in zip(f, g))


def check_case(label, path, full, fw, bw, bad):
    full_frames = frames_of(full)
    fwf, bwf = frames_of(fw), frames_of(bw)
    ts = len(bwf)
    if len(full_frames) != len(fwf) + len(bwf) + 1:
        bad.append("%s: fixture: %d full frames for %d+%d" % (label, len(full_frames),
                                                            len(fwf), len(bwf)))
        return
    ref = post(path, full)
    ref_c, ref_e, ref_g = vec(ref.last("IRCCOORD")), vec(ref.last("IRCENERGY")), ref.frames()

    run = live(path, fw, bw)
    g, c, e = run.frames(), vec(run.last("IRCCOORD")), vec(run.last("IRCENERGY"))
    want_g = [full_frames[ts][1]] + [f[1] for f in fwf] + [f[1] for f in bwf]
    ok = True
    if not (len(g) == len(want_g) and all(same_geom(a, b) for a, b in zip(g, want_g))):
        bad.append("%s: %d live frames, not TS + %d forward + %d backward in run order"
                   % (label, len(g), len(fwf), len(bwf)))
        for i, (a, b) in enumerate(zip(g, want_g)):
            if not same_geom(a, b):
                bad.append("%s: frame %d is %s, want %s" % (label, i, a[:1], b[:1]))
                break
        ok = False
    want_c = [0.0] + [k + 1.0 for k in range(len(fwf))] + [-(k + 1.0) for k in range(len(bwf))]
    if c != want_c or len(e) != len(c):
        bad.append("%s: IRCCOORD %s, not run order %s" % (label, c, want_c))
        ok = False
    fe = [float(re.search(r"\sE\s+(\S+)", f[0]).group(1)) for f in fwf + bwf]
    if not close(e[1:], fe, 6e-7):
        bad.append("%s: point i does not have frame i's energy" % label)
        ok = False
    order = sorted(range(len(c)), key=lambda i: c[i])
    if not ([c[i] for i in order] == ref_c and close([e[i] for i in order], ref_e, 1e-9)
            and all(same_geom(g[i], r) for i, r in zip(order, ref_g)) and len(ref_g) == len(g)):
        bad.append("%s: sorted by coordinate, not what an import stores" % label)
        ok = False
    n_before = len(run.frames())
    run.script("orca.ircgeom", full)
    if len(run.frames()) != n_before:
        bad.append("%s: the _IRC_Full_trj.xyz entry added frames after live ones" % label)
        ok = False
    print("%s: %d frames live %s" % (label, len(g), "checked" if ok else "FAILED"))

    # Cut in the forward and the backward path.
    lines = open(path, errors="replace").read().splitlines()
    banners = [i for i, ln in enumerate(lines) if BANNER.match(ln)]
    for where, start in zip(("forward", "backward"), banners):
        rows = [i for i in range(start, len(lines)) if ROW.match(lines[i])]
        rows = rows[:next((k for k in range(1, len(rows)) if rows[k] != rows[k - 1] + 1),
                          len(rows))]
        run = live(path, fw, bw, cut=rows[len(rows) // 2] + 1)
        g, c, e = run.frames(), vec(run.last("IRCCOORD")), vec(run.last("IRCENERGY"))
        nf = len([x for x in c if x > 0])
        nb = len([x for x in c if x < 0])
        good = (len(g) == len(c) == len(e) and c == want_c[:len(c)] and
                all(same_geom(a, b) for a, b in zip(g, want_g[:1 + nf] if nb == 0
                                                     else want_g[:1 + len(fwf) + nb])))
        if where == "backward" and nb == 0:
            good = False
        if not good:
            bad.append("%s cut in the %s path: %d frames, %d points %s"
                       % (label, where, len(g), len(c), c))
        print("%s cut in the %s path: %d frames, %d points %s"
              % (label, where, len(g), len(c), "checked" if good else "FAILED"))


def main():
    bad = []
    cases = [("hcn_irc4 (ORCA's own _F/_B)", os.path.join(FIX, "hcn_irc4.out"),
              open(os.path.join(FIX, "hcn_irc4_IRC_Full_trj.xyz")).read(),
              open(os.path.join(FIX, "hcn_irc4_IRC_F_trj.xyz")).read(),
              open(os.path.join(FIX, "hcn_irc4_IRC_B_trj.xyz")).read())]
    for name, out, full in (
            ("hcn_irc", os.path.join(FIX, "hcn_irc.out"),
             os.path.join(FIX, "hcn_irc_IRC_Full_trj.xyz")),
            ("sn2_irc", os.path.join(EX, "orca-sn2_irc", "sn2_irc.out"),
             os.path.join(EX, "orca-sn2_irc", "sn2_irc_IRC_Full_trj.xyz")),
            ("diels_alder_irc", os.path.join(EX, "orca-diels_alder_irc", "diels_alder_irc.out"),
             os.path.join(EX, "orca-diels_alder_irc", "diels_alder_irc_IRC_Full_trj.xyz"))):
        text = open(full).read()
        fr = frames_of(text)
        summ = open(out, errors="replace").read().split("IRC PATH SUMMARY")[-1]
        ts = next(i for i, ln in enumerate(re.findall(r"^\s*\d+\s+-\d+\.\d+.*$", summ, re.M))
                  if "<= TS" in ln)
        cases.append((name, out, text, xyz(fr[ts + 1:], "F"), xyz(fr[:ts][::-1], "B")))
    for case in cases:
        check_case(*case, bad=bad)
    for b in bad:
        print("FAIL " + b)
    print("ok" if not bad else "%d failures" % len(bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
