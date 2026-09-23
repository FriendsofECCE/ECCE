"""Drive ECCE's real property pipeline end to end.

Every other suite here models one layer.  This one runs the shipped
artefacts and nothing else:

    1. the real computational code, on a real input deck
    2. the real scripts/eccejobmonitor, doing the real Begin/Skip/End
       matching and the real message framing
    3. the real scripts/parsers/* scripts, on the blocks the monitor
       actually emitted

The point is the two seams.  ``tests/parsers`` drives a reimplementation
of the monitor (``eccejobmonitor_sim.py``), which by construction cannot
catch a bug *in the monitor* -- and there has been one: ``parseTypes
ALL`` was compared case-sensitively against the uppercase string
``Launch.C`` passes, silently deleting every not-yet-enabled descriptor
so that trace properties were never extracted.  And it replays captured
output, so it cannot notice a code changing its wording in a new release
(Gaussian 16 dropping "atomic" from "Mulliken atomic charges" made that
property silently unextractable for every G16 job ever run, #80).

WHY THE MONITOR CAN BE RUN AT ALL.  It looks like it needs the whole
ECCE stack, and does not: ``-commType stdio`` drops the socket and the
JMS dispatcher, ``-monitoringMode post`` runs it over a finished file,
and every other input is a command-line parameter.  No data server, no
gateway, no display.

WHAT THE MONITOR DOES AND DOES NOT DO.  It matches blocks and ships the
RAW BLOCK TEXT; it never runs a parser script.  The client does that --
``JobParser::storeProperty`` (src/comm/commxt/JobParser.C) runs
``<script> <parseArgs> >parseOut <parseIn``.  So stage 3 here is a
faithful stand-in for the C++ client, and the split has to be preserved
or the test stops resembling the product.
"""

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(ROOT, 'scripts')
PARSERS = os.path.join(SCRIPTS, 'parsers')

sys.path.append(os.path.join(ROOT, "tests", "parsers"))
from eccejobmonitor_sim import parse_parser_output   # noqa: E402

#  The wire format, from scripts/eccejobmonitor's own constants:
#    MSG_PREFIX  "\001"        first byte of every packet
#    MSG_SEQ_*   "0".."3"      1-of-1, 1-of-N, middle, last
#    MSG_FMT     '%s%s%4d%s'   prefix, sequence, 4-digit length, payload
#    MSG_SEP     "\000"        field separator within a payload
#  and newlines are mapped to \002 on the way out, "so to make processing
#  easier for the client" -- systems that expand \n to \r\n would
#  otherwise corrupt the length count.
MSG_PREFIX = '\001'
MSG_SEP = '\000'
SEQ_SINGLE, SEQ_FIRST, SEQ_MIDDLE, SEQ_LAST = '0', '1', '2', '3'


class MonitorError(Exception):
    pass


def run_monitor(job_output, desc_file, workdir, parse_types='ALL',
                calc_name='e2e', timeout=300):
    """Stage 2: the real eccejobmonitor over a finished job output file.

    Returns the path of the results file it wrote.
    """
    results = os.path.join(workdir, 'results.out')
    log = os.path.join(workdir, 'monitor.log')
    cmd = [
        'perl', os.path.join(SCRIPTS, 'eccejobmonitor'),
        '-commType', 'stdio',
        '-monitoringMode', 'post',
        '-jobOutputFile', job_output,
        '-parseDescriptorFile', desc_file,
        #  Deliberately passed exactly as Launch.C passes it (uppercase).
        #  Lower-casing it here would hide the very bug this stage exists
        #  to keep fixed.
        '-parseTypes', parse_types,
        '-outputFile', results,
        '-calcName', calc_name,
        '-logFile', log,
        #  Left empty on purpose: the default is a bare "-", which the
        #  monitor then tries to send as a file and logs as missing.
        #  Harmless, but it puts an ERROR line in every run's output.
        '-filesToTransfer', '',
    ]
    proc = subprocess.run(cmd, stdin=subprocess.DEVNULL, cwd=workdir,
                          capture_output=True, text=True, timeout=timeout)
    if proc.returncode != 0:
        raise MonitorError('eccejobmonitor exited %d\n%s'
                           % (proc.returncode, proc.stderr[-2000:]))
    if not os.path.exists(results):
        raise MonitorError('eccejobmonitor wrote no results file')
    return results


def unpack(results_file):
    """Decode the monitor's wire format into (parse_type, count, block).

    Packets are reassembled first: a property longer than msgLength is
    split across a 1-of-N / middle / last sequence, and treating each
    packet as its own message would silently truncate exactly the large
    properties (MO coefficient matrices, geometry traces) most worth
    testing.
    """
    raw = open(results_file, 'rb').read().decode('utf-8', 'replace')
    messages, buf, i = [], '', 0
    while True:
        i = raw.find(MSG_PREFIX, i)
        if i < 0:
            break
        seq = raw[i + 1]
        try:
            length = int(raw[i + 2:i + 6])
        except ValueError:
            break
        payload = raw[i + 6:i + 6 + length]
        i += 6 + length
        if seq == SEQ_SINGLE:
            messages.append(payload)
        elif seq == SEQ_FIRST:
            buf = payload
        elif seq == SEQ_MIDDLE:
            buf += payload
        elif seq == SEQ_LAST:
            messages.append(buf + payload)
            buf = ''

    blocks = []
    for msg in messages:
        fields = msg.split(MSG_SEP)
        if not fields or fields[0] != 'jmPROP':
            continue
        #  Two shapes, from MsgSendBlock and MsgSendBlockFlush:
        #    jmPROP <bookmark> <parseType> <count> <text>   (as matched)
        #    jmPROP EOF        <parseType> <count> <text>   (buffered,
        #                                                    Frequency=last)
        #  The second field distinguishes them but is not otherwise
        #  needed; both carry the same trailing four.
        if len(fields) < 5:
            continue
        parse_type, count, text = fields[2], fields[3], MSG_SEP.join(fields[4:])
        blocks.append((parse_type, count, text.replace('\002', '\n')))
    return blocks


def run_parsers(blocks, desc, parse_args):
    """Stage 3: the parser scripts, the way JobParser::storeProperty does.

    Returns {property key: [record, ...]} in the order the monitor
    emitted them, so a Frequency=last entry's surviving block is the last
    one -- the same resolution the client performs.
    """
    by_type = {e.type: e for e in desc.live_entries()}
    out = {}
    for parse_type, _count, text in blocks:
        entry = by_type.get(parse_type)
        if entry is None or not entry.script:
            continue
        script = os.path.join(PARSERS, entry.script)
        if not os.path.exists(script):
            raise MonitorError('missing parser script %s' % script)
        proc = subprocess.run([script] + list(parse_args), input=text,
                              capture_output=True, text=True, timeout=120)
        for rec in parse_parser_output(proc.stdout):
            out.setdefault(rec['key'], []).append(rec)
    return out


def run_code(argv, workdir, timeout=1800, env=None):
    """Stage 1: the real computational code.

    Returns (returncode, stdout, stderr).  Deliberately does not raise on
    a non-zero exit: a code that refuses a deck ECCE generated is a
    finding, not an infrastructure failure, and the caller reports it as
    such.
    """
    e = dict(os.environ)
    if env:
        e.update(env)
    proc = subprocess.run(argv, cwd=workdir, capture_output=True, text=True,
                          timeout=timeout, env=e)
    return proc.returncode, proc.stdout, proc.stderr


def which(name, extra_paths=()):
    """Locate a code binary, so an absent one can be skipped with a reason
    rather than failing the suite."""
    for p in extra_paths:
        if p and os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    for d in os.environ.get('PATH', '').split(os.pathsep):
        cand = os.path.join(d, name)
        if os.path.isfile(cand) and os.access(cand, os.X_OK):
            return cand
    return None


def sect(rec, name, default=None):
    """A record's section, e.g. 'values' or 'units'.

    parse_parser_output stores sections under rec['sections'] rather than
    on the record itself, so reaching for rec['values'] raises KeyError
    on every record -- including the ones that do have values.
    """
    v = rec.get('sections', {}).get(name)
    if v is None:
        return default
    #  Sections are stored as a list of lines; every caller here wants
    #  the whitespace-separated tokens, and a multi-line values block
    #  (MO coefficient rows, long vectors) must read as one sequence.
    if isinstance(v, (list, tuple)):
        return ' '.join(str(x).strip() for x in v).strip()
    return str(v).strip()


def value_of(records, key, index=-1):
    """The stored value of a property, taking the last record by default
    -- which is what a Frequency=last entry resolves to."""
    recs = records.get(key)
    if not recs:
        return None
    return sect(recs[index], 'values')


def numbers(text):
    """Whitespace-separated floats from a record's values field."""
    if text is None:
        return []
    return [float(t) for t in text.split()
            if re.match(r'^[-+]?\d*\.?\d+([eEdD][-+]?\d+)?$', t)]
