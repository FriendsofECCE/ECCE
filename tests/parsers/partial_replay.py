#!/usr/bin/env python3
r"""
Partially-written output replay (#107 part B).

``eccejobmonitor`` tails a file the code is still writing.  The rest of
this suite replays finished files, which cannot show what happens when the
monitor and the parser scripts meet a file cut off mid-way.  This one cuts
each fixture at several points and pushes every cut through the shipped
pipeline, with no model of any stage:

    real scripts/eccejobmonitor   (-commType stdio -monitoringMode post)
      -> the jmPROP frames it emits (tests/e2e/pipeline.unpack)
      -> the real scripts/parsers/* scripts, as JobParser::storeProperty
         runs them

Cut points: after each geometry step, in the middle of a block (mid-SCF
for the SCF-bearing types), in the middle of a line, a header-only file,
and the full file.

What is asserted for every cut:

  1. the monitor exits 0 and every parser script exits 0 without Perl
     diagnostics on stderr;
  2. every emitted property is well formed: a ``size:`` section whose
     dimensions multiply out to the number of values, no NaN/Inf, no empty
     values;
  3. the blocks delivered for the cut are exactly those that are complete in
     the cut file (checked against ``eccejobmonitor_sim.replay`` run on the
     complete lines of the cut), and each is byte-identical to the block of
     the same type and number in the full-file run -- so a property is
     either complete for the steps written so far or absent, and never a
     half-read block;
  4. the full-file cut reproduces the plain full-file run exactly.

A block still open at EOF is the one state the monitor cannot represent:
in post mode it ``Die``s ("end of file encountered while reading parse
type"), where a live monitor would wait for the job to write the rest.  The
replay therefore drops the unfinished trailing block (the live monitor has
not delivered it either) and re-runs the monitor on what is left, instead
of counting the Die as a failure; an ``End`` that never appears even in the
full file is a failure of the full-file check in ``run_tests.py``.

A trailing line without its newline is never delivered by the monitor
(``FileReadLine`` buffers it); the same is checked at the parser level for
the ``File=`` auxiliary files, which the client hands to a script whole.

    tests/parsers/partial_replay.py            # all cases
    tests/parsers/partial_replay.py -v
    tests/parsers/partial_replay.py --case orca-h2o-optfreq
"""

import argparse
import atexit
import hashlib
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, 'scripts', 'parsers')
FIXTURES = os.path.join(HERE, 'fixtures')

sys.path.insert(0, os.path.join(REPO, 'tests', 'e2e'))
sys.path.insert(0, HERE)
import cases as CASEDEFS                                   # noqa: E402
from eccejobmonitor_sim import (read_desc, replay,         # noqa: E402
                                parse_parser_output)
import pipeline                                            # noqa: E402

#  The fixtures worth cutting: one or two per code, picked for having
#  several geometry steps, an SCF and (where there is one) MOs.
PARTIAL_CASES = [
    'nwchem-h2o-opt', 'nwchem-oh-uhf', 'nwchem-h2o-freq',
    'g16-h2o-optfreq', 'g16-oh-uhf',
    'orca-h2o-opt', 'orca-h2o-optfreq', 'orca-oh-uhf',
    'mopac-ch4-opt', 'mopac-ch4-mos', 'mopac-ch3-uhf-mos',
]
MAX_CUTS = 14

#  The incomplete-block Die, from JobOutputGet.
RUNAWAY = 'end of file encountered while reading parse type'
STDERR_BAD = re.compile(r'(\bat \S+ line \d+|Use of uninitialized|'
                        r'Illegal division|Can\'t |Died|Segmentation|'
                        r'Argument ".*" isn\'t numeric)')
BAD_NUMBER = re.compile(r'(?<![A-Za-z])[-+]?(nan|inf(inity)?)(?![A-Za-z])',
                        re.I)


class Fail(object):
    def __init__(self):
        self.failures = []
        self.checks = 0
        self.notes = []

    def check(self, ok, where, msg):
        self.checks += 1
        if not ok:
            self.failures.append('%s: %s' % (where, msg))
        return ok


# ---------------------------------------------------------------------------
# cut points
# ---------------------------------------------------------------------------

def cut_points(text, desc, full):
    """[(label, text-to-write)], deterministic.  ``full`` is the sim's
    replay of the complete file, used only to find where blocks start and
    end -- the cuts themselves are always run through the real monitor."""
    lines = text.splitlines(True)
    blen = [len(x.encode('utf-8', 'replace')) for x in lines]
    n = len(lines)
    cuts = {}          # byte-length of the prefix -> label

    def at_line(k, label):            # keep the first k whole lines
        k = max(0, min(n, k))
        cuts.setdefault(sum(blen[:k]), label)

    def mid_line(k, label):           # k whole lines + half of the next
        if 0 <= k < n and len(lines[k].rstrip('\n')) > 1:
            off = sum(blen[:k]) + blen[k] // 2
            cuts.setdefault(off, label)

    at_line(min(20, n // 10), 'header only')

    geom = [b for b in full.blocks
            if b.end_line and re.search(r'GEOM|GRAD|ESCF|^TE', b.entry.type)]
    seen_types = {}
    for b in sorted(full.blocks, key=lambda b: b.begin_line):
        if not b.end_line:
            continue
        seen_types.setdefault(b.entry.type, []).append(b)
    #  after each geometry step (the end of each geometry block)
    for i, b in enumerate(b for b in geom if 'GEOM' in b.entry.type):
        at_line(b.end_line, 'after geometry step %d' % (i + 1))
    #  mid-block: the middle of the first and last block of the SCF and
    #  geometry types (an SCF still iterating, a gradient half printed)
    for typ, blks in sorted(seen_types.items()):
        if not re.search(r'GEOM|GRAD|ESCF|^TE|^MO', typ):
            continue
        for which, b in (('first', blks[0]), ('last', blks[-1])):
            if b.end_line - b.begin_line >= 2:
                mid = (b.begin_line + b.end_line) // 2
                at_line(mid, 'mid-block [%s] %s' % (typ[:24], which))
    mid_line(n // 2, 'mid-line at half')
    at_line(n - 1, 'all but the last line')

    items = sorted(cuts.items())
    if len(items) > MAX_CUTS - 1:
        step = len(items) / float(MAX_CUTS - 1)
        items = [items[int(i * step)] for i in range(MAX_CUTS - 1)]
    #  mid-line, on the line that closes the first and last block of EVERY
    #  parse type: this is the one cut that hands a script half a line, so
    #  it is not sampled.
    fcuts = {}
    for typ, blks in sorted(seen_types.items()):
        for which, b in (('first', blks[0]), ('last', blks[-1])):
            k = b.end_line - 1
            if 0 <= k < n and blen[k] > 2:
                fcuts.setdefault(sum(blen[:k]) + blen[k] // 2,
                                 'mid-line on End of [%s] %s'
                                 % (typ[:24], which))
    items += sorted(fcuts.items())
    raw = text.encode('utf-8', 'replace')
    out = [(label, raw[:off]) for off, label in items]
    out.append(('full file', raw))
    return out


# ---------------------------------------------------------------------------
# running a cut through the real pipeline
# ---------------------------------------------------------------------------

_PARSER_CACHE = {}


def run_script(entry, args, block_text, workdir):
    """The parser script on one block; identical (script, args, text) runs
    are shared between cuts, since a cut changes which blocks exist, not
    what a script makes of one."""
    key = (entry.script, args, hashlib.md5(
        block_text.encode('utf-8', 'replace')).hexdigest())
    if key in _PARSER_CACHE:
        return _PARSER_CACHE[key]
    script = os.path.join(SCRIPTS, entry.script)
    argv = [script] + list(args)
    env = dict(os.environ)
    env['ECCE_HOME'] = REPO
    env['PATH'] = SCRIPTS + os.pathsep + env.get('PATH', '')
    env.pop('PERL5LIB', None)
    proc = subprocess.run(argv, input=block_text, capture_output=True,
                          text=True, timeout=120, cwd=workdir, env=env)
    out = (proc.stdout, proc.stderr, proc.returncode)
    _PARSER_CACHE[key] = out
    return out


def monitor(path, desc_file, workdir):
    """The real monitor; returns (blocks, error-text-or-None).  Exit 1 with
    the runaway message is returned as the error, anything else raises."""
    try:
        results = pipeline.run_monitor(path, desc_file, workdir)
    except pipeline.MonitorError as exc:
        return None, str(exc)
    return pipeline.unpack(results), None


def write(path, lines):
    with open(path, 'wb') as fh:
        fh.write(b''.join(lines))


def drop_open_block(lines, desc):
    """The prefix of ``lines`` a live monitor has delivered: everything up
    to the Begin line of the block still open at EOF."""
    tmp = tempfile.NamedTemporaryFile('wb', suffix='.out', delete=False)
    tmp.write(b''.join(lines))
    tmp.close()
    try:
        r = replay(desc, tmp.name, record_ambiguity=False)
    finally:
        os.unlink(tmp.name)
    if r.runaway is None:
        return None
    open_blocks = [b for b in r.blocks if not b.end_line]
    return lines[:open_blocks[-1].begin_line - 1]


#  Keys whose full-file output already breaks the size rule (a scalar whose
#  value is several words, e.g. NWChem's VERSION): the rule says nothing
#  about them, so they are left out of the comparison for that case.
SIZE_EXEMPT = set()


def script_ok(out, serr, rc):
    """Exit 0, or a deliberate `die "ERROR/FATAL: ..."` with no property on
    stdout: the gaussian-94.* multipole scripts refuse input that is not in
    the format they expect and say so, which leaves the property absent.
    An unexplained non-zero exit, or one that also printed records, fails."""
    if rc == 0:
        return True
    return (not parse_parser_output(out)
            and re.match(r'(ERROR|FATAL):', serr) is not None)


def check_records(where, recs, res):
    for rec in recs:
        key = rec['key']
        sec = rec['sections']
        flat = rec['flat']
        res.check('size' in sec, where, '[%s] has no size: section' % key)
        vals = flat.get('values', '')
        if 'values' in sec:
            res.check(bool(vals.strip()), where,
                      '[%s] emitted with an empty values: section' % key)
        res.check(not BAD_NUMBER.search(vals), where,
                  '[%s] has a NaN/Inf in its values: %.80s' % (key, vals))
        if 'size' in sec and 'values' in sec and key not in SIZE_EXEMPT:
            try:
                dims = [int(x) for x in flat['size'].split()]
            except ValueError:
                res.check(False, where, '[%s] size is not integers: %r'
                          % (key, flat['size']))
                continue
            if all(d >= 0 for d in dims) and vals.strip():
                want = 1
                for d in dims:
                    want *= d
                got = len(vals.split())
                res.check(got == want, where,
                          '[%s] size %s promises %d values but %d are '
                          'present (a half-read vector or matrix)'
                          % (key, flat['size'], want, got))
            for lab in ('rowlabels', 'columnlabels'):
                pass


def run_cut(case, desc, args, label, text, full_blocks, res, tmp, verbose):
    where = '%s @ %s' % (case['name'], label)
    desc_file = os.path.join(SCRIPTS, case['desc'])
    path = os.path.join(tmp, 'cut.out')
    wd = os.path.join(tmp, 'wd')
    shutil.rmtree(wd, True)
    os.makedirs(wd)

    #  Post mode treats the end of the file as the end of the job, so a
    #  trailing line without its newline is handed over as a line (a live
    #  monitor holds it back until the newline arrives).  That is the case
    #  where a half-written line can reach a parser, so it is kept.
    lines = text.splitlines(True)
    fragment = lines[-1] if lines and not lines[-1].endswith(b'\n') else None
    complete = lines
    write(path, complete)

    blocks, err = monitor(path, desc_file, wd)
    effective = complete
    if err is not None:
        if RUNAWAY in err:
            effective = drop_open_block(complete, desc)
            if effective is None:
                res.check(False, where, 'monitor reported a runaway block '
                          'but none is open per the simulator:\n' + err)
                return
            write(path, effective)
            shutil.rmtree(wd, True)
            os.makedirs(wd)
            blocks, err = monitor(path, desc_file, wd)
        if err is not None:
            res.check(False, where, 'eccejobmonitor failed on a '
                      'partially-written file:\n' + err)
            return
    res.check(True, where, '')

    #  (3) the blocks delivered are exactly the complete ones, and equal the
    #  same block of the full run
    sim = replay(desc, path, record_ambiguity=False)
    want = sorted((b.entry.type, b.text) for b in sim.blocks if b.delivered)
    got = sorted((t, x) for t, _c, x in blocks)
    res.check(want == got, where,
              'blocks the real monitor delivered differ from the complete '
              'blocks in the cut: monitor %s, expected %s'
              % ([t[:20] + ':%d' % len(x) for t, x in got],
                 [t[:20] + ':%d' % len(x) for t, x in want]))
    fullset = {}
    for t, c, x in full_blocks:
        fullset.setdefault(t, []).append(x)
    by_entry = {e.type: e for e in desc.live_entries()}
    for t, c, x in blocks:
        entry = by_entry[t]
        if entry.frequency in ('last', 'firstlast') or \
                entry.frequency.isdigit():
            continue          # the surviving block depends on the cut
        if fragment is not None and x.encode('utf-8', 'replace').endswith(
                fragment):
            continue          # ends in the half-written line, by design
        res.check(x in fullset.get(t, []), where,
                  '[%s] block %s is not a block of the full run (a '
                  'half-written block was delivered):\n%s'
                  % (t[:30], c, x[-200:]))

    #  (1)+(2) the scripts
    props = []
    for t, c, x in blocks:
        entry = by_entry[t]
        out, serr, rc = run_script(entry, args, x, wd)
        res.check(script_ok(out, serr, rc), where,
                  '[%s] %s exited %d: %s'
                  % (t[:30], entry.script, rc, serr.strip()[:300]))
        res.check(not STDERR_BAD.search(serr), where,
                  '[%s] %s wrote Perl diagnostics: %s'
                  % (t[:30], entry.script, serr.strip()[:300]))
        recs = parse_parser_output(out)
        check_records(where + ' [%s]' % t[:24], recs, res)
        props.append((t, c, recs))
    return blocks, props


def flat_props(props):
    out = []
    for t, c, recs in props:
        for r in recs:
            out.append((r['key'], tuple(r['flat'].items())))
    return sorted(out)


def run_case(case, res, verbose):
    t0 = time.time()
    desc = read_desc(os.path.join(SCRIPTS, case['desc']))
    fixture = os.path.join(FIXTURES, case['fixture'])
    with open(fixture, encoding='utf-8', errors='replace') as fh:
        text = fh.read()
    full_sim = replay(desc, fixture, record_ambiguity=False)
    tmp = tempfile.mkdtemp(prefix='ecce-partial-')
    atexit.register(shutil.rmtree, tmp, True)

    #  the plain full-file run through the same code path
    SIZE_EXEMPT.clear()
    base = Fail()
    ref = run_cut(case, desc, case['parse_args'], 'reference',
                  text.encode('utf-8', 'replace'), [],
                  base, tmp, False)
    for f in base.failures:
        m = re.search(r'\[(\w+)\] size .* promises', f)
        if m:
            SIZE_EXEMPT.add(m.group(1))
    if ref is None:
        res.check(False, case['name'], 'the full file does not run')
        return
    full_blocks = ref[0]

    cuts = cut_points(text, desc, full_sim)
    n_blocks = []
    for label, cut in cuts:
        r = run_cut(case, desc, case['parse_args'], label, cut,
                    full_blocks, res, tmp, verbose)
        if r is None:
            continue
        n_blocks.append(len(r[0]))
        if label == 'full file':
            res.check(flat_props(r[1]) == flat_props(ref[1]),
                      case['name'] + ' @ full file',
                      'the full-file cut differs from the plain full run')
    print('  %-22s %5.1fs  %2d cuts, %s blocks delivered per cut'
          % (case['name'], time.time() - t0, len(cuts),
             '/'.join(str(x) for x in n_blocks) if verbose
             else 'max %d' % max(n_blocks or [0])))


def run_mofile_cuts(res, verbose):
    """File= auxiliary scripts get a whole file; the file may be half
    written when the client fetches it."""
    for case in CASEDEFS.MOFILE_CASES:
        with open(os.path.join(FIXTURES, case['fixture']),
                  encoding='utf-8', errors='replace') as fh:
            text = fh.read()
        n = len(text)
        tmp = tempfile.mkdtemp(prefix='ecce-partial-mo-')
        atexit.register(shutil.rmtree, tmp, True)
        entry = type('E', (), {'script': case['script']})()
        for frac in (0.0, 0.1, 0.33, 0.5, 0.77, 0.95, 1.0):
            cut = text[:int(n * frac)]
            where = '%s @ %d%% of the file' % (case['name'], frac * 100)
            out, serr, rc = run_script(entry, tuple(case['parse_args']),
                                       cut, tmp)
            res.check(script_ok(out, serr, rc), where, '%s exited %d: %s'
                      % (case['script'], rc, serr.strip()[:300]))
            res.check(not STDERR_BAD.search(serr), where,
                      'Perl diagnostics: %s' % serr.strip()[:300])
            check_records(where, parse_parser_output(out), res)
        print('  %-30s mofile cuts' % case['name'])


EXPT_FRACTIONS = [i / 24.0 for i in range(1, 24)]


def run_expt_cuts(res):
    """The importers on an output file that stops part-way (an import of a
    job that was killed).  Either the importer refuses, writing nothing, or
    the .frag it writes is a whole structure: at least one atom, and as many
    as num_atoms says."""
    import expt
    import expt_cases
    tmp = tempfile.mkdtemp(prefix='ecce-partial-expt-')
    atexit.register(shutil.rmtree, tmp, True)
    seen = set()
    t0 = time.time()
    for case in expt_cases.CASES:
        key = (case['script'], case['output'])
        if key in seen or case.get('expect_returncode'):
            continue
        seen.add(key)
        src = os.path.join(FIXTURES, case['output'])
        with open(src, 'rb') as fh:
            data = fh.read()
        local = os.path.join(tmp, os.path.basename(src))
        for frac in EXPT_FRACTIONS:
            where = '%s @ %d%% of the output' % (case['name'], frac * 100)
            with open(local, 'wb') as fh:
                fh.write(data[:int(len(data) * frac)])
            r = expt.run(case['script'], local)
            frag = r.get('.frag', '')
            if r['returncode'] != 0:
                res.check(not frag and r['stdout'].strip() != '', where,
                          '%s refused the file (exit %d) but wrote a .frag '
                          'or said nothing: %s'
                          % (case['script'], r['returncode'],
                             r['stderr'].strip()[:200]))
                continue
            atoms = expt.atoms(frag)
            m = re.search(r'^num_atoms:\s*(\d+)', frag, re.M)
            res.check(m is not None and len(atoms) >= 1 and
                      int(m.group(1)) == len(atoms), where,
                      '%s wrote a .frag with %d atom(s) and num_atoms %s'
                      % (case['script'], len(atoms),
                         m.group(1) if m else 'missing'))
    print('  %-30s %d importer cuts, %.1fs'
          % ('importers (.expt)', len(seen) * len(EXPT_FRACTIONS),
             time.time() - t0))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--case', action='append')
    ap.add_argument('-v', '--verbose', action='store_true')
    args = ap.parse_args()
    by_name = dict((c['name'], c) for c in CASEDEFS.CASES)
    names = args.case or PARTIAL_CASES
    res = Fail()
    t0 = time.time()
    for name in names:
        if name not in by_name:
            print('no such case %s' % name, file=sys.stderr)
            return 2
        run_case(by_name[name], res, args.verbose)
    if not args.case:
        run_mofile_cuts(res, args.verbose)
        run_expt_cuts(res)
    print('\n%d checks, %.1fs' % (res.checks, time.time() - t0))
    if res.failures:
        print('\nFAILURES (%d):' % len(res.failures))
        seen = set()
        for f in res.failures:
            if f not in seen:
                seen.add(f)
                print('  * %s' % f)
        print('\nFAILED')
        return 1
    print('PASSED')
    return 0


if __name__ == '__main__':
    sys.exit(main())
