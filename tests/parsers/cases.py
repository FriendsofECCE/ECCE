"""Declarative test cases for the parser/property-pipeline regression suite.

Each CASE replays one real, checked-in job output file through one real
``.desc`` file and the real ``scripts/parsers/*`` scripts.  See README.md.

``expect`` is the hand-verified part: parse types that MUST fire on this
fixture, and property values that MUST come out.  Everything else in the
run is compared against the golden file in ``expected/``, so silent drift
anywhere in the pipeline shows up as a diff even where no explicit
assertion was written.

Value syntax in ``keys``:
    'KEY': {'values': '<exact whitespace-normalised values string>'}
    'KEY': {'values_contain': '<substring>'}
    'KEY': {'units': 'e'}
    'KEY': {}                      -- must merely be emitted, non-empty
``instance`` selects which delivered block's output to check (default -1,
the last one delivered -- which is what the ECCE property store ends up
holding for an overwriting scalar property).

``parse_args`` is what ``DavCalculation::getParseScriptArgs()`` would hand
the scripts for this calculation: ``('.', <runtype>, <theory category>,
<theory name>, <open shells>)``.  The runtype names are ECCE's own, from
``data/client/cap/<Code>.edml``: Energy, Gradient, Geometry, Vibration,
GeoVib, IRC, Magnetic, Polarizability, Car-Parrinello, Property.  Several
parser scripts gate their entire body on the runtype, so getting this
wrong makes a property look broken when it is only inapplicable.

``silent_ok`` lists parse types whose script deliberately emits nothing for
*this* calculation's runtype, with the reason.  Without an entry here, a
fired-but-silent parse type fails the suite (that is #85's shape).
"""

CASES = [
    # -----------------------------------------------------------------
    # Gaussian 16
    # -----------------------------------------------------------------
    dict(
        name='g16-h2o-optfreq',
        desc='gaussian-16.desc',
        fixture='gaussian-16/h2o_optfreq.log',
        # HF/STO-3G Opt Freq, closed shell
        parse_args=('.', 'GeoVib', 'SCF', 'RHF', '0'),
        expect={
            'MULLIKEN': dict(keys={
                'MULLIKEN': {'values': '-0.330647 0.165312 0.165335',
                             'units': 'e'},
            }),
            # The vibrational block: one [VIB][VIBFREQ][VIBIR][VIBRAM][VIBSYM]
            # parse type, five properties out of one script invocation.
            'VIB][VIBFREQ][VIBIR][VIBRAM][VIBSYM': dict(keys={
                'VIBFREQ': {'values': '2169.8577 4141.5426 4392.6459'},
                'VIBSYM': {'values': "A' A' A'"},
                'VIB': {},
                'VIBIR': {},
                'VIBRAM': {},
            }),
            # Skip=5 must land exactly on the first atom row, so the very
            # first value the geometry parser sees is atom 1.  (This is the
            # off-by-one that silently produced zero output for ORCA.)
            'GEOMTRACE1': dict(min_blocks=2, keys={'GEOMTRACE': {}}),
            # gaussian-16.energy emits ESCF/ESCFVEC and, when the theory
            # matches, TE/TEVEC -- all from the one 'SCF Done:' line.
            'ESCF1][ESCFVEC': dict(keys={'ESCF': {}, 'TE': {}}),
        },
    ),
    dict(
        name='g16-co-freq',
        desc='gaussian-16.desc',
        fixture='gaussian-16/co_freq.log',
        parse_args=('.', 'Vibration', 'SCF', 'RHF', '0'),
        silent_ok={
            'DELTAE': 'gaussian-16.deltae runs only for runtype =~ /Geo/i; a '
                      'Vibration job has no SCF-step energy trace to record.',
            'RMSDP': 'gaussian-16.rmsdp runs only for runtype =~ /Geo/i.',
            'ESCF1][ESCFVEC': 'gaussian-16.energy gates its whole body on '
                              'runtype =~ /Geo/i. For a non-optimisation job '
                              'the total energy is picked up from the archive '
                              'block instead, by gaussian-16.db.',
        },
        # Regression guard for issue #85: a diatomic frequency job used to
        # produce NO vibrational data at all.
        expect={
            'VIB][VIBFREQ][VIBIR][VIBRAM][VIBSYM': dict(keys={
                'VIBFREQ': {'values': '2600.5673'},
                'VIBIR': {'values': '38.4596'},
                'VIBRAM': {'values': '5.7433'},
                'VIBSYM': {'values': 'SG'},
                'VIB': {'rowlabels': '1-C 2-O'},
            }),
        },
    ),
    dict(
        name='g16-oh-uhf',
        desc='gaussian-16.desc',
        fixture='gaussian-16/oh_uhf.log',
        # UHF doublet with Pop=Full, so the alpha/beta orbital path runs.
        parse_args=('.', 'Energy', 'SCF', 'UHF', '1'),
        silent_ok={
            'DELTAE': 'runtype-gated to /Geo/i (see g16-co-freq).',
            'RMSDP': 'runtype-gated to /Geo/i (see g16-co-freq).',
            'ESCF1][ESCFVEC': 'runtype-gated to /Geo/i (see g16-co-freq).',
        },
        expect={
            # ---------------------------------------------------------
            # REGRESSION GUARD, found by this suite on 2026-09-21 and
            # fixed the same day (same family as #80).  Gaussian 16
            # prints "Mulliken charges and spin densities:" for ANY
            # open-shell (UHF/UDFT/ROHF) job, but the Begin demanded a
            # colon straight after "charges", so Mulliken charges were
            # silently never extracted for any open-shell Gaussian
            # calculation.  Closed-shell jobs print "Mulliken charges:"
            # and worked, which is exactly why it hid.
            # The Begin must keep NOT matching the "... with hydrogens
            # summed into heavy atoms:" line that follows a few lines
            # later -- that is different data.
            # Pin the values too: the charge column is field 4, and the
            # spin-density 5th column must be ignored, not mistaken for
            # a charge.  Full precision is asserted deliberately -- the
            # parser used to chop() the last character, which removed
            # the trailing newline in closed-shell output (harmless) but
            # ate a real digit here, where the 5th column carries the
            # newline instead.
            # ---------------------------------------------------------
            'MULLIKEN': dict(
                keys={'MULLIKEN': {'values': '-0.164453 0.164453',
                                   'units': 'e'}}),
            'ORBOCC1][ORBOCCBETA': dict(keys={'ORBOCCBETA': {}}),
        },
    ),

    # -----------------------------------------------------------------
    # NWChem (ecce_print traces)
    # -----------------------------------------------------------------
    dict(
        name='nwchem-h2o-opt',
        desc='nwchem.desc',
        fixture='nwchem/h2o_opt.eprint',
        parse_args=('.', 'Geometry', 'SCF', 'RHF', '0'),
        expect={
            'GEOMTRACE': dict(min_blocks=2, keys={'GEOMTRACE': {}}),
        },
    ),
    dict(
        name='nwchem-oh-uhf',
        desc='nwchem.desc',
        fixture='nwchem/oh_uhf.eprint',
        parse_args=('.', 'Energy', 'SCF', 'UHF', '1'),
        expect={
            # S^2 for a doublet.  The old regex here matched any line
            # containing the digit 2; this pins the real value.
            # nwchem.scalar reprints the value in %e form; the raw number in
            # the ecce_print trace is 0.753263362179062.
            'S2': dict(keys={'S2': {'values': '7.53263362179062e-01'}}),
        },
    ),
    dict(
        name='nwchem-oh-udft',
        desc='nwchem.desc',
        fixture='nwchem/oh_udft.eprint',
        parse_args=('.', 'Energy', 'DFT', 'UDFT', '1'),
        expect={},
    ),
    dict(
        name='nwchem-cp',
        desc='nwchem.desc',
        fixture='nwchem/cp_h2o.eprint',
        parse_args=('.', 'Car-Parrinello', 'Plane Wave', 'PSPW', '0'),
        silent_ok={
            'GEOMPROP': 'nwchem.geomtrace deliberately discards a coordinate '
                        'block that is not from the task matching the runtype '
                        '(for Car-Parrinello it keeps only task_car-parrinello '
                        'blocks), so the task_energy block emits no key.',
        },
        # Regression guard for issue #84: this block used to emit the key
        # EGRADVEC instead of the declared TGRADCPVEC.
        expect={
            'TGRADCPVEC': dict(min_blocks=5,
                               keys={'TGRADCPVEC': {}}),
        },
    ),

    # -----------------------------------------------------------------
    # ORCA
    # -----------------------------------------------------------------
    dict(
        name='orca-h2o-opt',
        desc='orca.desc',
        fixture='orca/h2o_opt.out',
        parse_args=('.', 'Geometry', 'SCF', 'RHF', '0'),
        expect={
            'GEOMTRACE': dict(min_blocks=2, keys={'GEOMTRACE': {}}),
        },
    ),
]


# ---------------------------------------------------------------------------
# Documented, accepted deviations.  Anything NOT listed here fails the suite.
# Each entry must carry a reason; a stale entry (one that no longer applies)
# is itself reported as a failure, so this list cannot rot silently.
# ---------------------------------------------------------------------------

# Begin-string collisions.  PDFileRead keys %gPdTable by the literal Begin
# string and "silently ignore[s] duplicate parse handles", so the second and
# later entries are never stored and can never match.
KNOWN_COLLISIONS = {
    ('gaussian-16.desc', 'EUMP2'):
        "[EMP22][EMP2VEC] duplicates [EMP21][EMP2VEC]'s Begin. Harmless: both "
        "name the same Script (gaussian-16.energy), which picks its output "
        "key from the matched line, so the surviving entry does the work of "
        "both.",
    ('gaussian-09.desc', 'EUMP2'):
        "Same duplicated-MP2-entry shape as gaussian-16.desc; harmless for "
        "the same reason.",
    ('gaussian-03.desc', 'EUMP2'):
        "Same duplicated-MP2-entry shape as gaussian-16.desc; harmless for "
        "the same reason.",
    ('gamess-uk.desc', 'total energy'):
        "[TENERGY] (Frequency=first) duplicates [TEVEC]'s Begin (Frequency="
        "all), so only [TEVEC] survives. Harmless, and in fact the pattern "
        "CLAUDE.md recommends: the one script gamess-uk.te emits both TE and "
        "TEVEC on every invocation, so the scalar TE ends up holding the last "
        "value while TEVEC accumulates the whole trace. Nothing is lost by "
        "[TENERGY] being dropped -- but note the dropped entry is the one "
        "that asked for Frequency=first, so the surviving behaviour is "
        "Frequency=all. (No fixture yet; found by the .desc lint.)",
    ('gaussian-98.desc', 'EUMP2'):
        "Same duplicated-MP2-entry shape as gaussian-16.desc; harmless for "
        "the same reason.",
}

# Parse types that fire but whose script legitimately emits no property.
KNOWN_SILENT_SCRIPTS = {
    'gaussian-16.null': 'A deliberate no-op; the entry exists only to consume '
                        'a block of output so other Begins do not match '
                        'inside it.',
    'gaussian-09.null': 'Deliberate no-op (see gaussian-16.null).',
    'gaussian-03.null': 'Deliberate no-op (see gaussian-16.null).',
    'nwchem.null': 'Deliberate no-op (see gaussian-16.null).',
    'orca.null': 'Deliberate no-op (see gaussian-16.null).',
    'nwchem.symlab': 'Emits no property by design: it writes the symmetry '
                     'labels to a side file ("$key/parseSym", $key being '
                     'argv[1] = the job directory) for other scripts to read.',
}

# Parse types whose script intentionally emits property keys that share
# nothing with the type's own bracket names.  A .desc type name is only a
# section label for JCode::parseScript (which looks up Script= by the text
# before the first ']'), so it is allowed to differ from the property key --
# but when it does, nothing but a human reading the script says which key
# actually lands in the property store.  That is how #84 hid, so every case
# has to be written down here with its reason.  Keyed by (desc, parse type).
KNOWN_KEY_ALIASES = {
    ('gaussian-16.desc', 'POLARIZ1][HYPERPOL1'):
        "Script=gaussian-16.db parses Gaussian's archive block and emits "
        "whatever it finds there (TE, ESCF, CPUSEC, S2, S2A, POLARIZ, ...); "
        "the bracket names are just this entry's section label.",
    ('gaussian-16.desc', 'FERMICON'):
        "Script=gaussian-16.fermicon emits the property key FERMI; FERMICON "
        "is the section label only.",
    ('nwchem.desc', 'MOLAB1'):
        "Script=nwchem.molab emits ORBSYM / ORBSYMBETA; the MOLAB<n> names "
        "are section labels distinguishing several .desc entries that share "
        "the one script.",
    ('nwchem.desc', 'MOLAB2'): "See MOLAB1.",
    ('nwchem.desc', 'MOLAB3'): "See MOLAB1.",
    ('nwchem.desc', 'MOLAB4'): "See MOLAB1.",
    ('nwchem.desc', 'MOLAB5'): "See MOLAB1.",
    ('nwchem.desc', 'GEOMPROP'):
        "Script=nwchem.geomtrace emits GEOMTRACE; GEOMPROP is the section "
        "label for the coordinates printed by a property task.",
    ('nwchem.desc', 'GEOMCAR'):
        "Script=nwchem.geomtrace emits GEOMTRACE; GEOMCAR is the section "
        "label for the plane-wave (nwpw) cartesian coordinate block.",
    ('nwchem.desc', 'TEPROP'):
        "Script=nwchem.te emits TE; TEPROP is the section label for the "
        "energy printed by a property task.",
}

# Parse types known never to fire on any fixture we have, with the reason.
# Listed here they are reported but do not fail; if one starts firing, the
# suite fails so the note can be removed.
KNOWN_DEAD = {
    ('gaussian-16.desc', 'CPUSEC'):
        "Standalone [CPUSEC] entry never fires: its Begin 'Job cpu time\\:' "
        "sits inside the region consumed by another parse type on every real "
        "job we have. The same data is captured by gaussian-16.db, so this is "
        "harmless -- but the entry is dead code.",
}
