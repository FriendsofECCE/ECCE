#!/usr/bin/env python3
r"""
A faithful, dependency-free re-implementation of the parts of
``scripts/eccejobmonitor`` that decide *which text from a job output file
reaches which parser script*.

This is deliberately a re-implementation rather than a wrapper: the real
``eccejobmonitor`` cannot run without a JMS broker, an ECCE server, a job
directory and a live calculation, and this test suite must run with no GUI,
no ECCE services and no chemistry codes installed.  Everything below was
written by reading ``scripts/eccejobmonitor`` directly; each routine names
the Perl subroutine and line range it mirrors so the two can be re-diffed
when the Perl changes.

Mirrored routines
-----------------
``PDFileRead``        -> :func:`read_desc`
``PDTypesInit``       -> :func:`_types_init`
``PDMatchListCreate`` -> :func:`_match_list`
``PDMatchBegin``      -> :func:`_match_begin`
``JobOutputGet``      -> :func:`replay` (the Begin/Skip/Lines/End loop)
``MsgSendBlock*``     -> :func:`replay` (the Frequency buffering rules)

Known, deliberate divergences (documented, not accidental):

* **Begin match order.**  Perl builds ``@gPdMatchList`` from
  ``keys %gPdTable`` -- a hash, so the order is unspecified and in modern
  Perl actually randomised per process.  ``PDMatchBegin`` returns the
  *first* entry whose Begin matches and never looks at the rest, so when
  two different Begin patterns can match the same line, which one wins is
  genuinely nondeterministic in production.  We use ``.desc`` definition
  order and separately *report every line that matches more than one
  Begin* as an ambiguity, because that is the real hazard.
* **Regex dialect.**  Perl and Python agree on everything the shipped
  ``.desc`` files actually use (character classes, ``\:``-style escapes,
  ``(?:...)``, alternation, anchors).  :func:`read_desc` reports any Begin
  or End that Python cannot compile rather than silently skipping it.
* ``eccejobmonitor``'s incremental/restart machinery (bookmarks, reopening
  a growing file, batch parse files) is not modelled; we replay a complete,
  finished output file in one pass.
"""

import shutil
import atexit
import os
import re
import subprocess
import tempfile
import sys
from collections import OrderedDict

# ---------------------------------------------------------------------------
# constants, mirroring eccejobmonitor lines 46-62
# ---------------------------------------------------------------------------

PD_KEYS = ('begin', 'end', 'file', 'frequency', 'lines', 'prefix', 'script',
           'skip')
FREQ_ALL = 'all'
FREQ_FIRST = 'first'
FREQ_FLAST = 'firstlast'
FREQ_LAST = 'last'
FREQ_NONE = 'none'
FREQ_VALUES = (FREQ_ALL, FREQ_FIRST, FREQ_FLAST, FREQ_LAST, FREQ_NONE)
DEF_FREQ = FREQ_ALL

# gConfig{'regExp'} defaults to 1 (eccejobmonitor line 199), i.e. Begin/End
# are treated as regular expressions and are NOT quotemeta-escaped.
REGEXP_MODE = True


class DescError(Exception):
    """A .desc file error that would make PDFileRead bail out."""


class ParseType(object):
    """One stored parse descriptor -- a row of Perl's %gPdTable."""

    __slots__ = ('type', 'rules', 'line', 'keys')

    def __init__(self, type_name, rules, line):
        self.type = type_name
        self.rules = rules
        self.line = line
        # "[VIB][VIBFREQ][VIBIR]" declares four property keys; PDFileRead's
        # greedy ^\s*\[(.+)\]\s*$ stores that whole string as the type name.
        self.keys = [k for k in type_name.split('][') if k]

    def __repr__(self):
        return '<ParseType [%s] line %d>' % (self.type, self.line)

    @property
    def begin(self):
        return self.rules['begin']

    @property
    def frequency(self):
        return self.rules.get('frequency', DEF_FREQ).lower()

    @property
    def script(self):
        return self.rules.get('script')


class Desc(object):
    """The result of reading a .desc file the way PDFileRead does."""

    def __init__(self, path):
        self.path = path
        self.entries = []          # every [TYPE]...[END] block, in file order
        self.table = OrderedDict() # begin-string -> ParseType  (%gPdTable)
        self.collisions = []       # (dropped ParseType, kept ParseType)
        self.bad_regex = []        # (ParseType, which, error-string)
        self.type_freq = {}        # %gPdTableFreq: parse type -> frequency

    def live_entries(self):
        """Entries PDMatchListCreate would put in the match list: stored in
        %gPdTable (no Begin collision) and without a File= rule."""
        return [e for e in self.table.values() if 'file' not in e.rules]


def read_desc(path):
    """Mirror ``PDFileRead`` (eccejobmonitor 3727-4010) plus ``PDTypesInit``
    (4238-4285).  Raises :class:`DescError` where the Perl sets ``$errMsg``
    and bails; records -- rather than raises -- the two failure modes the
    Perl handles *silently*: a duplicate Begin (``# silently ignore duplicate
    parse handles``) and, for us, an uncompilable pattern."""
    desc = Desc(path)
    parse_type = None
    buf = None
    start_line = 0

    with open(path, encoding='utf-8', errors='replace') as fh:
        lines = fh.readlines()

    for lineno, raw in enumerate(lines, 1):
        line = raw.rstrip('\n')
        if re.match(r'^\s*#', line):
            continue
        if re.match(r'^\s*$', line):
            continue

        if parse_type is not None:
            m = re.match(r'^\s*(\S+?)\s*=(.*)$', line)
            if m:
                key = m.group(1).lower()
                val = m.group(2)
                if key not in PD_KEYS:
                    raise DescError("line %d of %s: unrecognized rule key "
                                    "'%s'" % (lineno, path, key))
                if key in buf:
                    raise DescError("line %d of %s: rule key '%s' already "
                                    "given" % (lineno, path, key))
                if key != 'end' and len(val) == 0:
                    raise DescError("line %d of %s: no value given for rule "
                                    "key '%s'" % (lineno, path, key))
                if key == 'end' and len(val) == 0:
                    val = '^$'
                elif key == 'lines':
                    if not val.isdigit() or int(val) <= 0:
                        raise DescError("line %d of %s: value for lines must "
                                        "be > 0" % (lineno, path))
                elif key == 'skip':
                    if not val.isdigit():
                        raise DescError("line %d of %s: value for skip must "
                                        "be >= 0" % (lineno, path))
                elif key == 'frequency':
                    if val.lower() not in FREQ_VALUES and not val.isdigit():
                        raise DescError("line %d of %s: bad frequency '%s'"
                                        % (lineno, path, val))
                buf[key] = val
            elif line == '[END]':
                _finish_entry(desc, parse_type, buf, start_line, lineno, path)
                parse_type = None
                buf = None
            else:
                raise DescError("line %d of %s: unrecognized line '%s' while "
                                "parsing [%s]" % (lineno, path, line,
                                                  parse_type))
        else:
            m = re.match(r'^\s*\[(.+)\]\s*$', line)
            if not m:
                raise DescError("line %d of %s: unrecognized line '%s'"
                                % (lineno, path, line))
            parse_type = m.group(1)
            buf = {}
            start_line = lineno

    if parse_type is not None:
        raise DescError("%s: unterminated parse type [%s]" % (path, parse_type))

    _types_init(desc)
    return desc


def _finish_entry(desc, parse_type, buf, start_line, end_line, path):
    """The ``elsif ($_ eq '[END]')`` arm of PDFileRead: validate, then store
    into %gPdTable keyed by the literal Begin string, first-one-wins."""
    if 'begin' not in buf:
        raise DescError("line %d of %s: rule key 'begin' must be given in "
                        "parse type [%s]" % (end_line, path, parse_type))
    if 'file' not in buf and 'end' not in buf and 'lines' not in buf:
        raise DescError("line %d of %s: rule key 'end' or 'lines' must be "
                        "given in parse type [%s]"
                        % (end_line, path, parse_type))
    if 'script' not in buf:
        raise DescError("line %d of %s: rule key 'script' must be given in "
                        "parse type [%s]" % (end_line, path, parse_type))
    if 'end' in buf and 'lines' in buf:
        raise DescError("line %d of %s: rule keys 'end' and 'lines' cannot "
                        "both be given in parse type [%s]"
                        % (end_line, path, parse_type))

    entry = ParseType(parse_type, dict(buf), start_line)
    desc.entries.append(entry)

    # %gPdTableFreq -- keyed by parse *type*, and the value that decides the
    # MsgSendBlockBegin buffering behaviour.  Numeric 0 means firstlast, 1
    # means all (PDFileRead 3929-3947).
    if 'frequency' in buf:
        freq = buf['frequency']
        if freq.isdigit():
            if int(freq) == 0:
                freq = FREQ_FLAST
            elif int(freq) == 1:
                freq = FREQ_ALL
        if parse_type not in desc.type_freq:
            desc.type_freq[parse_type] = freq
        elif desc.type_freq[parse_type] != freq:
            raise DescError("line %d of %s: frequency '%s' in parse type [%s] "
                            "differs from earlier '%s'"
                            % (end_line, path, freq, parse_type,
                               desc.type_freq[parse_type]))

    handle = buf['begin']
    if handle in desc.table:
        # "# silently ignore duplicate parse handles" -- eccejobmonitor 3977.
        desc.collisions.append((entry, desc.table[handle]))
    else:
        desc.table[handle] = entry


def _types_init(desc):
    """Mirror PDTypesInit (4238-4285): canonicalise Frequency, drop
    Frequency=none types entirely, and check the patterns compile."""
    for entry in list(desc.table.values()):
        freq = entry.rules.get('frequency')
        if freq is not None:
            entry.rules['frequency'] = freq.lower()
        else:
            entry.rules['frequency'] = DEF_FREQ

    # Frequency=none -> PDTableDeleteType removes EVERY handle of that type.
    dead_types = set(e.type for e in desc.table.values()
                     if e.rules['frequency'] == FREQ_NONE)
    for handle in [h for h, e in desc.table.items() if e.type in dead_types]:
        del desc.table[handle]

    for entry in desc.table.values():
        for which in ('begin', 'end'):
            pat = entry.rules.get(which)
            if pat is None:
                continue
            if not REGEXP_MODE:
                pat = re.escape(pat)
            try:
                re.compile(pat)
            except re.error as exc:
                desc.bad_regex.append((entry, which, str(exc)))


# ---------------------------------------------------------------------------
# the replay loop
# ---------------------------------------------------------------------------

class Block(object):
    """One data block delivered to (or buffered away from) a parser script."""

    __slots__ = ('entry', 'count', 'begin_line', 'lines', 'end_line',
                 'buffered', 'delivered')

    def __init__(self, entry, count, begin_line):
        self.entry = entry
        self.count = count           # 1-based instance number for this type
        self.begin_line = begin_line
        self.lines = []              # the raw text MsgSendBlockLine accumulated
        self.end_line = None
        self.buffered = False        # MsgSendBlockBegin decided to buffer it
        self.delivered = False       # it actually reached a parser script

    @property
    def text(self):
        return ''.join(self.lines)


class ReplayResult(object):
    def __init__(self, desc, output_path):
        self.desc = desc
        self.output_path = output_path
        self.blocks = []             # every block begun, in file order
        self.ambiguous = []          # (lineno, text, [entries]) multi-Begin hits
        self.runaway = None          # entry left open at EOF
        self.nlines = 0

    def blocks_for(self, entry):
        return [b for b in self.blocks if b.entry is entry]

    def delivered_for(self, entry):
        return [b for b in self.blocks if b.entry is entry and b.delivered]


def replay(desc, output_path, record_ambiguity=True):
    """Mirror the ``JobOutputGet`` line loop (eccejobmonitor 2786-2905) and
    the ``MsgSendBlock*`` Frequency buffering (3428-3512).

    Returns a :class:`ReplayResult`.  The critical fiddly bits, all verified
    against the Perl:

    * ``Skip=N`` consumes N lines *counting the Begin-matching line itself*
      (``if ($lineSkip-- == 0)`` runs on the Begin line).
    * ``Lines=N`` uses post-decrement: ``if ($lineTake-- > 1) {feed} elsif
      ($lineTake == 0) {feed; end}``, which tests the *post*-decrement value
      in the elsif.
    * ``End`` is an unanchored match on the whole line; the End line itself
      IS fed to the script.
    * ``Frequency=first`` deletes every handle of that parse type once one
      block completes.
    * ``Frequency=last`` / ``firstlast`` / numeric N buffer blocks instead of
      sending them; only the last buffered block per type is flushed at EOF.
    """
    result = ReplayResult(desc, output_path)
    match_list = desc.live_entries()
    compiled = []
    for entry in match_list:
        pat = entry.begin if REGEXP_MODE else re.escape(entry.begin)
        try:
            compiled.append((entry, re.compile(pat)))
        except re.error:
            continue

    # live copy of %gPdTable, so Frequency=first deletion is modelled
    alive = {id(e): True for e, _ in compiled}
    counts = {}       # %gPdCount: parse type -> instances begun
    buffered = {}     # %gMsgBlockBuf: parse type -> the one buffered block

    with open(output_path, encoding='utf-8', errors='replace') as fh:
        lines = fh.readlines()
    result.nlines = len(lines)

    handle = None     # current ParseType, or None
    block = None
    line_skip = 0
    line_take = 0

    def finish(blk):
        blk.end_line = lineno
        if blk.buffered:
            buffered[blk.entry.type] = blk
        else:
            buffered.pop(blk.entry.type, None)
            blk.delivered = True
        if blk.entry.frequency == FREQ_FIRST:
            # PDTableDeleteType: every handle sharing this parse type dies
            for e, _ in compiled:
                if e.type == blk.entry.type:
                    alive[id(e)] = False

    for lineno, text in enumerate(lines, 1):
        if handle is not None:
            # next if ($lineSkip-- > 0);
            if line_skip > 0:
                line_skip -= 1
                continue
            line_skip -= 1
            if 'lines' in handle.rules:
                pre = line_take
                line_take -= 1
                if pre > 1:
                    block.lines.append(text)
                elif line_take == 0:
                    block.lines.append(text)
                    finish(block)
                    handle = None
                    block = None
                # else: pre == 0 -> Perl feeds nothing and never closes the
                # block; a runaway.  Reproduced by falling through.
            elif re.search(handle.rules['end'], text):
                block.lines.append(text)
                finish(block)
                handle = None
                block = None
            else:
                block.lines.append(text)
            continue

        # not in a block: PDMatchBegin
        hits = []
        for entry, rx in compiled:
            if not alive[id(entry)]:
                continue
            if rx.search(text):
                hits.append(entry)
                if not record_ambiguity:
                    break
        if not hits:
            continue
        if len(hits) > 1 and record_ambiguity:
            result.ambiguous.append((lineno, text.rstrip('\n'), list(hits)))
        entry = hits[0]

        count = counts.get(entry.type, 0) + 1
        counts[entry.type] = count
        handle = entry
        block = Block(entry, count, lineno)
        block.buffered = _should_buffer(desc, entry.type, count)
        result.blocks.append(block)

        line_skip = int(entry.rules.get('skip', 0))
        line_take = int(entry.rules['lines']) if 'lines' in entry.rules else 0

        # if ($lineSkip-- == 0) { feed; if ($lineTake-- == 1) { end } }
        if line_skip == 0:
            line_skip -= 1
            block.lines.append(text)
            pre = line_take
            line_take -= 1
            if pre == 1:
                finish(block)
                handle = None
                block = None
        else:
            line_skip -= 1

    if handle is not None:
        # Perl Dies here: "end of file encountered while reading parse type".
        result.runaway = handle

    # MsgSendBlockFlush: everything still buffered goes out at EOF.
    for blk in buffered.values():
        blk.delivered = True

    return result


def _should_buffer(desc, parse_type, count):
    """MsgSendBlockBegin (eccejobmonitor 3428-3470)."""
    freq = desc.type_freq.get(parse_type)
    if freq is None:
        return False
    if freq == FREQ_FLAST:
        return count > 1
    if freq == FREQ_LAST:
        return True
    if freq.isdigit():
        return (count % int(freq)) != 1
    return False


# ---------------------------------------------------------------------------
# running the real parser scripts
# ---------------------------------------------------------------------------

DEFAULT_PARSE_ARGS = ('.', 'Single Point Energy', 'Hartree Fock', 'RHF', '0')


def run_parser(script_dir, entry, block, parse_args=DEFAULT_PARSE_ARGS,
               workdir=None, timeout=60):
    """Invoke the real parser script exactly the way the client does.

    ``JobParser::storeProperty`` (src/comm/commxt/JobParser.C 270-300) builds

        <script> <parseArgs> >parseOut <parseIn

    where ``parseIn`` holds the raw matched block text and ``parseArgs`` comes
    from ``DavCalculation::getParseScriptArgs`` (src/dsm/edsiimpl/
    DavCalculation.C 2044): ``. "<runtype>" "<theory category>" "<theory
    name>" "<open shells>"``.  The scripts read them as
    ``($key, $runtype, $ucCategory, $theory, $openShells) = @ARGV;``.

    Returns (stdout, stderr, returncode)."""
    path = os.path.join(script_dir, entry.script)
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    argv = [path] + list(parse_args)
    if not os.access(path, os.X_OK):
        argv = ['perl'] + argv

    # Several parser scripts do
    #     push(@INC,"$ENV{ECCE_HOME}/scripts/parsers");
    #     require "pertab.pl";
    # so ECCE_HOME must point at an ECCE tree.  (Nothing in the repo sets
    # PERL5LIB, and modern Perl dropped '.' from @INC, so without ECCE_HOME
    # those scripts die with "Can't locate pertab.pl" and emit nothing.)
    env = dict(os.environ)
    ecce_home = os.path.dirname(os.path.dirname(os.path.abspath(script_dir)))
    env['ECCE_HOME'] = ecce_home
    env['PATH'] = script_dir + os.pathsep + env.get('PATH', '')
    env.pop('PERL5LIB', None)

    # Run in a scratch directory, never in scripts/parsers: the real client
    # runs these in the job's own temp dir, and some scripts write side files
    # there (nwchem.symlab writes "$key/parseSym", with $key = argv[1] = '.').
    cwd = workdir
    if not cwd:
        #  Ours to clean up; a caller's own workdir is not.
        cwd = tempfile.mkdtemp(prefix='ecce-parsertest-')
        atexit.register(shutil.rmtree, cwd, True)
    proc = subprocess.run(argv, input=block.text, capture_output=True,
                          text=True, timeout=timeout, cwd=cwd, env=env)
    return proc.stdout, proc.stderr, proc.returncode


PROP_KEY_RE = re.compile(r'^\s*key:\s*(\S+)\s*$')


def parse_parser_output(text):
    """Split a parser script's stdout into property records.

    The contract every scripts/parsers/* script follows is a sequence of

        key: NAME
        size:
        <dims>
        values:
        <values...>
        units:
        <unit>
        END

    with several optional sections (rowlabels:, collabels:, units:, ...).
    We keep it generic: a record starts at ``key:`` and ends at the next
    ``key:`` or at ``END``; sections are collected verbatim, whitespace
    normalised, so a value-formatting change is caught but trailing-space
    noise is not.
    """
    records = []
    cur = None
    section = None
    for raw in text.splitlines():
        m = PROP_KEY_RE.match(raw)
        if m:
            if cur is not None:
                records.append(cur)
            cur = {'key': m.group(1), 'sections': OrderedDict()}
            section = None
            continue
        if cur is None:
            continue
        stripped = raw.strip()
        if stripped == 'END':
            records.append(cur)
            cur = None
            section = None
            continue
        sm = re.match(r'^([a-z]+):\s*(.*)$', stripped)
        if sm and sm.group(1) in ('size', 'values', 'units', 'rowlabels',
                                  'columnlabels', 'collabels', 'vectorlabels',
                                  'labels', 'type', 'dim'):
            section = sm.group(1)
            cur['sections'][section] = []
            if sm.group(2):
                cur['sections'][section].append(sm.group(2))
            continue
        if section is not None:
            cur['sections'][section].append(stripped)
    if cur is not None:
        records.append(cur)
    for rec in records:
        rec['flat'] = OrderedDict(
            (name, ' '.join(' '.join(vals).split()))
            for name, vals in rec['sections'].items())
    return records
