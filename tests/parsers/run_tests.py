#!/usr/bin/env python3
"""
ECCE parser / property-pipeline regression suite.

Replays real, checked-in chemistry-code output through the real
``eccejobmonitor`` Begin/Skip/Lines/End/Frequency algorithm (re-implemented
faithfully in eccejobmonitor_sim.py) and the real ``scripts/parsers/*``
scripts, then compares what comes out against checked-in expectations.

Needs nothing but Python 3 and perl: no GUI, no ECCE services, no chemistry
codes.

    ./run_tests.py                 run everything
    ./run_tests.py -v              also list every entry that fired
    ./run_tests.py --case g16-co-freq
    ./run_tests.py --update        regenerate the golden files in expected/
    ./run_tests.py --list          list cases

Exit status is 0 only if every check passed.
"""

import argparse
import re
import difflib
import os
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, 'scripts', 'parsers')
FIXTURES = os.path.join(HERE, 'fixtures')
EXPECTED = os.path.join(HERE, 'expected')

sys.path.insert(0, HERE)

import cases as CASEDEFS                                    # noqa: E402
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output, DescError)


class Results(object):
    def __init__(self):
        self.failures = []
        self.notes = []
        self.xfails = []       # known bugs: reported loudly, do not fail CI
        self.xpasses = []      # a known bug that stopped reproducing
        self.checks = 0
        self.xfail_reason = None
        self.alias_needed = set()   # (desc, type) where an alias really applied
        self.types_fired = {}       # desc -> set of parse types that fired

    def check(self, ok, where, message):
        self.checks += 1
        if self.xfail_reason is not None:
            if ok:
                self.xpasses.append('%s: %s -- %s'
                                    % (where, self.xfail_reason,
                                       'this check now PASSES; remove the '
                                       'xfail from cases.py'))
            else:
                self.xfails.append('%s: %s\n        %s'
                                   % (where, message, self.xfail_reason))
            return ok
        if not ok:
            self.failures.append('%s: %s' % (where, message))
        return ok

    def note(self, text):
        self.notes.append(text)


def norm(text):
    return ' '.join(text.split())


# ---------------------------------------------------------------------------
# .desc structural checks -- run once per .desc file, independent of fixtures
# ---------------------------------------------------------------------------

def check_desc_structure(desc_name, desc, res):
    where = desc_name

    for entry, which, err in desc.bad_regex:
        res.check(False, where,
                  '[%s] (line %d): %s= pattern does not compile: %s'
                  % (entry.type, entry.line, which.capitalize(), err))

    seen = set()
    for dropped, kept in desc.collisions:
        key = (desc_name, dropped.begin)
        seen.add(key)
        reason = CASEDEFS.KNOWN_COLLISIONS.get(key)
        if reason is None:
            res.check(False, where,
                      'Begin COLLISION: [%s] (line %d) has the same Begin as '
                      '[%s] (line %d) -- PDFileRead silently drops it, so '
                      '[%s] can NEVER match. Begin=%r'
                      % (dropped.type, dropped.line, kept.type, kept.line,
                         dropped.type, dropped.begin))
        else:
            res.note('%s: accepted Begin collision [%s] vs [%s] (Begin=%r): %s'
                     % (desc_name, dropped.type, kept.type, dropped.begin,
                        reason))

    # stale allowlist entries
    for (dname, begin), reason in CASEDEFS.KNOWN_COLLISIONS.items():
        if dname != desc_name:
            continue
        if (dname, begin) not in seen:
            res.check(False, where,
                      'KNOWN_COLLISIONS is stale: no collision on Begin=%r '
                      'any more -- remove the entry.' % begin)

    # every Script= named must exist, or nothing can ever be parsed
    for entry in desc.entries:
        path = os.path.join(SCRIPTS, entry.script)
        res.check(os.path.exists(path), where,
                  '[%s] (line %d): Script=%s does not exist in scripts/parsers'
                  % (entry.type, entry.line, entry.script))


# ---------------------------------------------------------------------------
# one case
# ---------------------------------------------------------------------------

def run_case(case, res, verbose=False):
    desc_name = case['desc']
    desc_path = os.path.join(SCRIPTS, desc_name)
    fixture = os.path.join(FIXTURES, case['fixture'])
    where = case['name']

    desc = read_desc(desc_path)
    result = replay(desc, fixture)

    if result.runaway is not None:
        res.check(False, where,
                  'runaway parse type [%s]: EOF reached mid-block. The real '
                  'eccejobmonitor Dies here.' % result.runaway.type)

    # ambiguous Begin matches: a line matching two different Begin patterns.
    # PDMatchBegin returns the first one in @gPdMatchList, whose order comes
    # from a Perl hash and is therefore not deterministic -- so in production
    # it is luck which parse type gets the line, and the other is starved.
    ambig_pairs = set()
    for lineno, text, hits in result.ambiguous:
        ambig_pairs.add(tuple(sorted(e.type for e in hits)))
    for pair in sorted(ambig_pairs):
        res.note('%s: AMBIGUOUS Begin -- one output line matches the Begin of '
                 'both %s. PDMatchBegin takes whichever comes first in a Perl '
                 'hash-ordered list, so which one wins is nondeterministic and '
                 'the loser is starved for the whole run.'
                 % (case['name'], ' and '.join('[%s]' % p for p in pair)))

    # run the real parser scripts, in a scratch dir (some write side files)
    workdir = tempfile.mkdtemp(prefix='ecce-parsertest-%s-' % case['name'])
    report = []
    emitted = {}          # parse type -> list of (block, records)
    for entry in desc.live_entries():
        blocks = result.delivered_for(entry)
        if not blocks:
            continue
        recs_per_block = []
        for block in blocks:
            try:
                out, err, rc = run_parser(SCRIPTS, entry, block,
                                          case.get('parse_args'),
                                          workdir=workdir)
            except FileNotFoundError as exc:
                res.check(False, where, 'missing parser script %s' % exc)
                continue
            recs = parse_parser_output(out)
            recs_per_block.append((block, recs, rc, err))
        emitted[entry.type] = (entry, recs_per_block)
        res.types_fired.setdefault(desc_name, set()).add(entry.type)

        report.append(format_entry(entry, result, recs_per_block))

        # --- systemic checks, applied to every entry that actually fired ---
        declared = set(entry.keys)
        got_keys = set()
        for block, recs, rc, err in recs_per_block:
            for rec in recs:
                got_keys.add(rec['key'])

        if not got_keys:
            reason = (CASEDEFS.KNOWN_SILENT_SCRIPTS.get(entry.script)
                      or case.get('silent_ok', {}).get(entry.type))
            if reason is None:
                res.check(False, where,
                          '[%s] matched %d block(s) but its script %s emitted '
                          'NO property at all. (This is the shape of #85 and '
                          'of the ORCA Skip= off-by-one: job completes, no '
                          'data, no error.)'
                          % (entry.type, len(blocks), entry.script))
        else:
            if entry.script in CASEDEFS.KNOWN_SILENT_SCRIPTS:
                res.check(False, where,
                          'KNOWN_SILENT_SCRIPTS is stale: %s emitted %s'
                          % (entry.script, sorted(got_keys)))
            if entry.type in case.get('silent_ok', {}):
                res.check(False, where,
                          "silent_ok is stale for [%s]: it emitted %s"
                          % (entry.type, sorted(got_keys)))

        # Key-declaration check.  A parse type's bracket names double as
        # section names for JCode::parseScript, so duplicates get a numeric
        # suffix ([DIPOLE1]/[DIPOLE2] both emit DIPOLE) -- strip it before
        # comparing.  A few scripts are deliberate multi-property dumps
        # (gaussian-16.db), so the meaningful failure is a *disjoint* key
        # set: the script emits nothing the .desc declares.  That is exactly
        # issue #84's shape (declared TGRADCPVEC, emitted EGRADVEC).
        base = set(declared) | set(re.sub(r'\d+$', '', k) for k in declared)
        if got_keys and not (got_keys & base):
            alias = CASEDEFS.KNOWN_KEY_ALIASES.get((desc_name, entry.type))
            if alias is None:
                res.check(False, where,
                          '[%s] declares key(s) %s but its script %s emitted '
                          'only %s -- nothing the .desc declares. (Issue #84 '
                          'shape: the property lands under a key the .desc '
                          'never names.)'
                          % (entry.type, sorted(declared), entry.script,
                             sorted(got_keys)))
            else:
                res.alias_needed.add((desc_name, entry.type))
        else:
            if verbose:
                for key in sorted(got_keys - base):
                    res.note('%s: [%s] script %s also emits undeclared key %s'
                             % (case['name'], entry.type, entry.script, key))

    # --- per-case declared expectations ---
    for typ, spec in case.get('expect', {}).items():
        res.xfail_reason = spec.get('xfail')
        if typ not in emitted:
            fired = [e.type for e in desc.live_entries()
                     if result.blocks_for(e)]
            res.check(False, where,
                      'expected parse type [%s] never delivered a block on '
                      'this fixture (its Begin never matched, or its block '
                      'was buffered away). Types that did fire: %s'
                      % (typ, sorted(set(fired))))
            continue
        entry, recs_per_block = emitted[typ]
        nblocks = len(recs_per_block)
        if 'min_blocks' in spec:
            res.check(nblocks >= spec['min_blocks'], where,
                      '[%s] delivered %d block(s), expected at least %d'
                      % (typ, nblocks, spec['min_blocks']))
        if 'blocks' in spec:
            res.check(nblocks == spec['blocks'], where,
                      '[%s] delivered %d block(s), expected exactly %d'
                      % (typ, nblocks, spec['blocks']))

        inst = spec.get('instance', -1)
        try:
            block, recs, rc, err = recs_per_block[inst]
        except IndexError:
            res.check(False, where, '[%s] has no block at instance %d'
                      % (typ, inst))
            continue
        by_key = {}
        for rec in recs:
            by_key.setdefault(rec['key'], rec)

        for key, want in spec.get('keys', {}).items():
            rec = by_key.get(key)
            if not res.check(rec is not None, where,
                             '[%s] instance %d did not emit key %s (emitted: '
                             '%s)' % (typ, inst, key, sorted(by_key))):
                continue
            flat = rec['flat']
            if 'values' in want:
                res.check(flat.get('values', '') == want['values'], where,
                          '[%s] %s values = %r, expected %r'
                          % (typ, key, flat.get('values', ''), want['values']))
            if 'values_contain' in want:
                res.check(want['values_contain'] in flat.get('values', ''),
                          where, '[%s] %s values = %r, expected to contain %r'
                          % (typ, key, flat.get('values', ''),
                             want['values_contain']))
            if 'units' in want:
                res.check(flat.get('units', '') == want['units'], where,
                          '[%s] %s units = %r, expected %r'
                          % (typ, key, flat.get('units', ''), want['units']))
            for sect in ('rowlabels', 'collabels', 'columnlabels',
                         'vectorlabels'):
                if sect in want:
                    res.check(flat.get(sect, '') == want[sect], where,
                              '[%s] %s %s = %r, expected %r'
                              % (typ, key, sect, flat.get(sect, ''),
                                 want[sect]))
            if 'size' in want:
                res.check(flat.get('size', '') == want['size'], where,
                          '[%s] %s size = %r, expected %r'
                          % (typ, key, flat.get('size', ''), want['size']))
            if not want:
                res.check(bool(flat.get('values', '').strip()), where,
                          '[%s] %s emitted an empty values section'
                          % (typ, key))

    res.xfail_reason = None
    # --- cross-property invariant: len(TEVEC) <= len(GEOMTRACE) ------------
    #
    # GeomTracePropertyPanel plots any PropTSVector<Geometry Step> alongside
    # GEOMTRACE, and OnPointClick passes the curve index straight to
    # GTStepCmd. An index past the last GEOMTRACE frame trips
    # PropTSVecTable::value()'s bounds check and the atoms collapse to the
    # origin. So a code that prints a per-cycle energy but not per-cycle
    # geometries must not map that energy to TEVEC.
    #
    # Each firing of a Frequency=all entry contributes one step, so the step
    # count is the number of blocks whose script emitted that key. CLAUDE.md
    # records this rule and two cases note it in comments, but nothing
    # actually checked it until now -- and it is a crash, not a cosmetic.
    steps = {}
    for entryType, (entry, recs_per_block) in emitted.items():
        for block, recs, rc, err in recs_per_block:
            for rec in recs:
                steps[rec['key']] = steps.get(rec['key'], 0) + 1
    nTrace = steps.get('GEOMTRACE', 0)
    nEnergy = steps.get('TEVEC', 0)
    if nTrace and nEnergy and nEnergy > nTrace:
        allowed = CASEDEFS.KNOWN_LONG_TEVEC.get(case['name'])
        res.check(allowed is not None, case['name'],
                  'TEVEC has %d step(s) but GEOMTRACE only %d. '
                  'GeomTracePropertyPanel indexes the trace with the energy '
                  'curve\'s own index, so the extra point(s) run past the '
                  'last frame and the atoms collapse to the origin '
                  '(PropTSVecTable::value bounds check).'
                  % (nEnergy, nTrace))

    # --- cross-property invariant: the vibration vectors are row-aligned ---
    #
    # NModePanel's table and its spectrum plot index VIBFREQ, VIBIR, VIBRAM
    # and VIBSYM by the same row number, not by any mode-number column the
    # code printed. A vector of a different length is therefore not a
    # missing value, it is every row after the divergence showing the wrong
    # mode's number -- silently, with no error and a perfectly plausible
    # looking spectrum.
    #
    # This is the single most repeated trap in this codebase's parsers:
    # ORCA's IR SPECTRUM omits the translation/rotation modes entirely
    # (orca.vibir pads them), and MOPAC's DESCRIPTION OF VIBRATIONS
    # collapses degenerate modes, 4 stanzas for CH4's 9 (mopac.vibir
    # expands them). Both were found by hand; neither was checkable until
    # now. A C2v water fixture cannot show the MOPAC case at all.
    sizes = {}
    for entryType, (entry, recs_per_block) in emitted.items():
        for block, recs, rc, err in recs_per_block:
            for rec in recs:
                if rec['key'] in ('VIBFREQ', 'VIBIR', 'VIBRAM', 'VIBSYM'):
                    #  "size:" is the leading dimension; VIB itself is
                    #  modes x atoms x 3 and is checked by its first field.
                    dims = ' '.join(
                        rec.get('sections', {}).get('size', [])).split()
                    if dims:
                        sizes[rec['key']] = dims[0]
    reference = sizes.get('VIBFREQ')
    if reference is not None:
        for key in ('VIBIR', 'VIBRAM', 'VIBSYM'):
            if key in sizes and sizes[key] != reference:
                res.check(False, case['name'],
                          '%s has %s row(s) but VIBFREQ has %s. NModePanel '
                          'indexes them by the same row, so every row past '
                          'the divergence shows a different mode\'s value '
                          'with no error anywhere.'
                          % (key, sizes[key], reference))

    return result, ''.join(report)


def format_entry(entry, result, recs_per_block):
    """One deterministic golden-file stanza for a parse type that fired."""
    out = []
    blocks = result.blocks_for(entry)
    delivered = [b for b in blocks if b.delivered]
    out.append('[%s]  Script=%s  Freq=%s  Skip=%s  Lines=%s\n'
               % (entry.type, entry.script, entry.frequency,
                  entry.rules.get('skip', '-'), entry.rules.get('lines', '-')))
    out.append('  Begin=%s\n' % entry.begin)
    if 'end' in entry.rules:
        out.append('  End=%s\n' % entry.rules['end'])
    out.append('  begins=%d delivered=%d buffered_away=%d\n'
               % (len(blocks), len(delivered), len(blocks) - len(delivered)))
    for i, (block, recs, rc, err) in enumerate(recs_per_block):
        first = block.lines[0].rstrip() if block.lines else '(no lines fed)'
        out.append('  block %d: begin-line=%d fed=%d rc=%d first-fed=%r\n'
                   % (i + 1, block.begin_line, len(block.lines), rc, first))
        if not recs:
            out.append('    (script emitted no property)\n')
        for rec in recs:
            out.append('    key: %s\n' % rec['key'])
            for name, val in rec['flat'].items():
                if len(val) > 400:
                    val = val[:400] + ' ...[%d chars]' % len(val)
                out.append('      %s: %s\n' % (name, val))
    out.append('\n')
    return ''.join(out)


def make_golden(case, result, body):
    head = []
    head.append('# ECCE parser regression golden file -- regenerate with '
                'run_tests.py --update\n')
    head.append('case:    %s\n' % case['name'])
    head.append('desc:    %s\n' % case['desc'])
    head.append('fixture: %s (%d lines)\n' % (case['fixture'], result.nlines))
    head.append('args:    %s\n' % ' '.join(
        repr(a) for a in (case.get('parse_args') or ())))
    head.append('\n')
    return ''.join(head) + body


# ---------------------------------------------------------------------------

def coverage_report(res, ran_cases, verbose):
    """Per-.desc: which entries never matched ANY fixture we have."""
    by_desc = {}
    for case, result in ran_cases:
        by_desc.setdefault(case['desc'], []).append(result)

    lines = []
    for desc_name, results in sorted(by_desc.items()):
        desc = results[0].desc
        live = desc.live_entries()
        fired = set()
        for result in results:
            # Each case re-reads the .desc, so entry objects differ between
            # results and blocks_for()'s identity test only ever matched the
            # first result's.  Use each result's own entries (line numbers
            # are the stable identity across re-reads of the same file).
            for entry in result.desc.live_entries():
                if result.blocks_for(entry):
                    fired.add(entry.line)
        never = [e for e in live if e.line not in fired]
        lines.append('%s: %d parse types stored, %d fired on our fixtures, '
                     '%d never fired' % (desc_name, len(live),
                                         len(live) - len(never), len(never)))
        for entry in never:
            reason = CASEDEFS.KNOWN_DEAD.get((desc_name, entry.type))
            uncovered = getattr(CASEDEFS, 'UNCOVERED', {}).get(
                (desc_name, entry.type))
            if reason:
                lines.append('    [%s] line %d -- documented dead: %s'
                             % (entry.type, entry.line, reason))
            elif uncovered:
                lines.append('    [%s] line %d -- no fixture (should work): '
                             '%s' % (entry.type, entry.line, uncovered))
            else:
                #  Every non-firing entry must be classified, as dead or as
                #  merely uncovered. An unclassified one is how [DEWVEC]
                #  hid: its Begin matched nothing NWChem emits, so the
                #  property was never extracted for any job ever, and the
                #  suite reported it in the same breath as two dozen
                #  entries that are simply waiting for a fixture. Forcing
                #  the distinction to be written down is what separates
                #  "nobody has tested this" from "this cannot work".
                res.check(False, desc_name,
                          '[%s] line %d never fires and is not classified.\n'
                          '      Begin=%r\n'
                          '      Check whether that text appears in any '
                          'fixture: if it does, the entry is BROKEN and the '
                          'property is silently never extracted. If it does '
                          'not, add it to cases.UNCOVERED with what job '
                          'would exercise it, or to cases.KNOWN_DEAD with '
                          'why it can never match.'
                          % (entry.type, entry.line, entry.begin))
                if verbose:
                    lines.append('    [%s] line %d Begin=%r (UNCLASSIFIED)'
                                 % (entry.type, entry.line, entry.begin))
        # stale KNOWN_KEY_ALIASES: listed for a type that fired somewhere but
        # never actually needed the exemption
        for (dname, typ), reason in CASEDEFS.KNOWN_KEY_ALIASES.items():
            if dname != desc_name:
                continue
            if typ in res.types_fired.get(dname, ()) and \
                    (dname, typ) not in res.alias_needed:
                res.check(False, desc_name,
                          'KNOWN_KEY_ALIASES is stale: [%s] now emits a key '
                          'the .desc declares -- remove the entry.' % typ)

        # stale KNOWN_DEAD: an entry listed as dead that actually fired
        for (dname, typ), reason in CASEDEFS.KNOWN_DEAD.items():
            if dname != desc_name:
                continue
            if typ not in [e.type for e in never]:
                if typ in [e.type for e in live]:
                    res.check(False, desc_name,
                              'KNOWN_DEAD is stale: [%s] did fire on a '
                              'fixture -- remove the entry.' % typ)
    return lines


def run_expt_cases(res, args):
    """The post-hoc importers: <Code>.expt on a finished output file.

    A different pipeline from everything above -- eccejobmonitor tails a
    running job, these reconstruct a calculation from a completed file --
    and the one issue #94 (dummy submission) would rely on almost entirely.
    It had no coverage at all before.
    """
    import expt
    import expt_cases

    if not expt_cases.CASES:
        return
    print()
    for case in expt_cases.CASES:
        t0 = time.time()
        name = case['name']
        try:
            produced = expt.run(case['script'],
                                os.path.join(FIXTURES, case['output']))
        except expt.ExptError as exc:
            res.check(False, name, str(exc))
            continue

        res.check(produced['returncode'] == 0, name,
                  '%s exited %d\n%s' % (case['script'], produced['returncode'],
                                        produced['stderr'].strip()))
        for extension in expt.PRODUCTS:
            res.check(extension in produced, name,
                      '%s produced no %s file' % (case['script'], extension))

        atoms = expt.atoms(produced.get('.frag'))
        params = expt.params(produced.get('.param'))
        expect = dict(case.get('expect') or {})

        if 'natoms' in expect:
            want = expect.pop('natoms')
            res.check(len(atoms) == want, name,
                      'expected %d atoms in the .frag, got %d'
                      % (want, len(atoms)))
        if 'geometry' in case:
            want = case['geometry']
            ok = (len(atoms) == len(want) and
                  all(a[0] == w[0] and
                      all(abs(a[i] - w[i]) < 1e-5 for i in (1, 2, 3))
                      for a, w in zip(atoms, want)))
            res.check(ok, name,
                      'geometry is not the expected one -- for an optimisation '
                      'this usually means the importer took the first block in '
                      'the file rather than the last.\n      expected %s\n'
                      '      got      %s' % (want, atoms))

        if 'symbols' in expect:
            want = expect.pop('symbols')
            got = [a[0] for a in atoms]
            res.check(got == want, name,
                      'expected atoms %s, got %s' % (want, got))
        for key, want in expect.items():
            res.check(params.get(key) == want, name,
                      '.param %s is %r, expected %r'
                      % (key, params.get(key), want))

        golden_path = os.path.join(EXPECTED, name + '.txt')
        golden = make_expt_golden(case, produced)
        if args.update:
            with open(golden_path, 'w') as fh:
                fh.write(golden)
            print('  updated %s' % os.path.relpath(golden_path, HERE))
        elif not os.path.exists(golden_path):
            res.check(False, name,
                      'no golden file %s -- run with --update and review the '
                      'result before committing it'
                      % os.path.relpath(golden_path, HERE))
        else:
            with open(golden_path) as fh:
                want = fh.read()
            if want != golden:
                diff = list(difflib.unified_diff(
                    want.splitlines(True), golden.splitlines(True),
                    fromfile='expected', tofile='actual', n=2))
                res.check(False, name, 'output drifted from %s:\n%s'
                          % (os.path.relpath(golden_path, HERE),
                             ''.join(diff[:120])))
        print('  %-24s %5.1fs  %d atoms, %d .param keys'
              % (name, time.time() - t0, len(atoms), len(params)))


def make_expt_golden(case, produced):
    import expt
    lines = ['# %s' % case['name'],
             '# %s %s' % (case['script'], case['output']),
             '# Golden record of what the importer DOES, not of what it',
             '# should do -- see expt_cases.NOTES before changing one.',
             '']
    for extension in expt.PRODUCTS:
        lines.append('=== %s' % extension)
        lines.append(expt.normalise(produced.get(extension, '(not produced)')))
    return '\n'.join(lines).rstrip('\n') + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--case', action='append', help='run only these cases')
    ap.add_argument('--update', action='store_true',
                    help='rewrite the golden files instead of comparing')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('-v', '--verbose', action='store_true')
    args = ap.parse_args()

    if args.list:
        for case in CASEDEFS.CASES:
            print('%-20s %-20s %s' % (case['name'], case['desc'],
                                      case['fixture']))
        return 0

    selected = [c for c in CASEDEFS.CASES
                if not args.case or c['name'] in args.case]
    if not selected:
        print('no such case', file=sys.stderr)
        return 2

    os.makedirs(EXPECTED, exist_ok=True)
    res = Results()
    ran = []
    checked_descs = set()

    # Structural lint of EVERY .desc in scripts/parsers, not only the ones a
    # fixture exercises: Begin collisions, uncompilable patterns and missing
    # Script= files are findable without any job output at all.
    if not args.case:
        for name in sorted(os.listdir(SCRIPTS)):
            if not name.endswith('.desc'):
                continue
            checked_descs.add(name)
            try:
                check_desc_structure(name, read_desc(os.path.join(SCRIPTS,
                                                                  name)), res)
            except DescError as exc:
                res.check(False, name, 'PDFileRead would reject this file: %s'
                          % exc)
        print('  linted %d .desc files' % len(checked_descs))

    for case in selected:
        t0 = time.time()
        try:
            desc_path = os.path.join(SCRIPTS, case['desc'])
            if case['desc'] not in checked_descs:
                checked_descs.add(case['desc'])
                check_desc_structure(case['desc'], read_desc(desc_path), res)
            result, body = run_case(case, res, args.verbose)
        except DescError as exc:
            res.check(False, case['name'], 'cannot read %s: %s'
                      % (case['desc'], exc))
            continue
        ran.append((case, result))

        golden_path = os.path.join(EXPECTED, case['name'] + '.txt')
        golden = make_golden(case, result, body)
        if args.update:
            with open(golden_path, 'w') as fh:
                fh.write(golden)
            print('  updated %s' % os.path.relpath(golden_path, HERE))
        elif not os.path.exists(golden_path):
            res.check(False, case['name'],
                      'no golden file %s -- run with --update and review the '
                      'result before committing it'
                      % os.path.relpath(golden_path, HERE))
        else:
            with open(golden_path) as fh:
                want = fh.read()
            if want != golden:
                diff = list(difflib.unified_diff(
                    want.splitlines(True), golden.splitlines(True),
                    fromfile='expected', tofile='actual', n=2))
                res.check(False, case['name'],
                          'output drifted from %s:\n%s'
                          % (os.path.relpath(golden_path, HERE),
                             ''.join(diff[:120])))
        print('  %-20s %5.1fs  %d parse types fired'
              % (case['name'], time.time() - t0,
                 len(set(b.entry.type for b in result.blocks))))

    run_expt_cases(res, args)

    print()
    for line in coverage_report(res, ran, args.verbose):
        print(line)

    if res.xfails:
        print('\nKNOWN BUGS reproduced by this suite (xfail -- see cases.py '
              'for the write-up; these do NOT fail the run):')
        for xf in res.xfails:
            print('  ! %s' % xf)
    if res.xpasses:
        print('\nUNEXPECTED PASSES (%d):' % len(res.xpasses))
        for xp in res.xpasses:
            print('  ? %s' % xp)

    if res.notes:
        print('\nNotes (%d):' % len(res.notes))
        for note in res.notes:
            print('  - %s' % note)

    print('\n%d checks run' % res.checks)
    if res.failures:
        print('\nFAILURES (%d):' % len(res.failures))
        for fail in res.failures:
            print('  * %s' % fail)
        print('\nFAILED')
        return 1
    print('PASSED')
    return 0


if __name__ == '__main__':
    sys.exit(main())
