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

    dict(
        name='g16-h2o-nmr',
        desc='gaussian-16.desc',
        fixture='gaussian-16/h2o_nmr.log',
        # RHF/STO-3G NMR on water. Added to cover the magnetic-shielding
        # entry, which no fixture exercised before -- gaussian-16.desc had
        # 30 of 50 parse types never firing, and the ORCA equivalent of
        # this one turned out to be silently dead (see orca.desc's
        # [ISOSHIELD] End-starvation fix), so it was worth checking that
        # Gaussian's is not.
        parse_args=('.', 'Magnetic', 'SCF', 'RHF', '0'),
        silent_ok={
            'DELTAE': 'runtype-gated to /Geo/i (see g16-co-freq).',
            'RMSDP': 'runtype-gated to /Geo/i (see g16-co-freq).',
            'ESCF1][ESCFVEC': 'runtype-gated to /Geo/i (see g16-co-freq).',
        },
        expect={
            # One script emits all four keys from the one shielding block.
            # Values checked by hand against the log: O is isotropic
            # 365.6728 / anisotropy 4.1792 with eigenvalues 363.1113
            # 365.4481 368.4589; H is isotropic 33.6821.
            'SHIELDTENSOR][ISOSHIELD][ANISOSHIELD][SHIELDEIGVAL': dict(keys={
                'ISOSHIELD': {'values_contain': '365.6728'},
                'ANISOSHIELD': {'values_contain': '4.1792'},
                'SHIELDTENSOR': {},
                'SHIELDEIGVAL': {'values_contain': '363.1113'},
            }),
        },
    ),

    dict(
        name='g16-h2o-ccsdt',
        desc='gaussian-16.desc',
        fixture='gaussian-16/h2o_ccsdt.log',
        # CCSD(T)/6-31G on water. Covers the correlated-method energy
        # branches, which no fixture exercised: [EMP21]/[EMP22] (the
        # documented EUMP2 Begin collision) and [ECCSDTPERT].
        parse_args=('.', 'Energy', 'CC', 'CCSD(T)', '0'),
        silent_ok={
            'DELTAE': 'runtype-gated to /Geo/i (see g16-co-freq).',
            'RMSDP': 'runtype-gated to /Geo/i (see g16-co-freq).',
            # gaussian-16.energy gates its whole body on runtype =~ /Geo/i
            # (line 114), by design and documented in its own header: for a
            # non-optimisation job the energies come from the archive block
            # via gaussian-16.db instead, which the POLARIZ1 assertions
            # below verify actually happens. So every one of these matching
            # a block and emitting nothing is correct, not #85's shape --
            # worth spelling out, because six entries firing silently is
            # exactly what that bug looks like from the outside.
            'ESCF1][ESCFVEC': 'runtype-gated to /Geo/i; archive supplies it.',
            'EMP21][EMP2VEC': 'runtype-gated to /Geo/i; archive supplies it.',
            'EMP3][EMP3VEC': 'runtype-gated to /Geo/i; archive supplies it.',
            'EMP4DQ][EMP4DQVEC': 'runtype-gated to /Geo/i; archive supplies it.',
            'EMP4SDQ][EMP4SDQVEC': 'runtype-gated to /Geo/i; archive supplies it.',
            'ECCSDTPERT': 'runtype-gated to /Geo/i; archive supplies it.',
            'ECCD][ECCDVEC][ECCSD][ECCSDVEC':
                'runtype-gated to /Geo/i; archive supplies it.',
        },
        expect={
            # The archive block is where a single-point job's correlated
            # energies actually come from. Cross-checked against the
            # per-line values in the log: EUMP2 = -0.76111680142148D+02
            # and CCSD(T) = -0.76119325411D+02 agree with EMP2 and
            # ECCSDTPERT here.
            'POLARIZ1][HYPERPOL1': dict(keys={
                'ESCF': {'values': '-75.9838311'},
                'EMP2': {'values': '-76.1116801'},
                'EMP3': {'values': '-76.1133812'},
                'EMP4DQ': {'values': '-76.1169769'},
                'EMP4SDQ': {'values': '-76.117589'},
                'ECCSD': {'values': '-76.1183396'},
                'ECCSDTPERT': {'values': '-76.1193254'},
            }),
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
            # Regression guard: Car-Parrinello's per-step lattice vectors
            # were matched by no .desc entry at all, so LATTICEVEC never
            # appeared for a CP run and nwchem.latvec's CP branch was dead
            # code. One block per CP step; the fixture's cell is the 8 Bohr
            # cube from its simulation_cell, unchanged across steps here
            # (CP at fixed cell), so pin the values as well as the count.
            'LATVECCPTRACE': dict(
                min_blocks=5,
                keys={'LATTICEVEC': {
                    'size': '1 3 3',
                    'values': '8.000000000000000e+00 0.000000000000000e+00 '
                              '0.000000000000000e+00 '
                              '0.000000000000000e+00 8.000000000000000e+00 '
                              '0.000000000000000e+00 '
                              '0.000000000000000e+00 0.000000000000000e+00 '
                              '8.000000000000000e+00'}}),
        },
    ),

    # -----------------------------------------------------------------
    # ORCA
    # -----------------------------------------------------------------
    dict(
        name='orca-h2o-opt',
        desc='orca.desc',
        fixture='orca/h2o_opt.out',
        #  Captured before ai.orca could emit "! UseSym", so ORCA's
        #  ORBITAL ENERGIES block here has no Irrep column and
        #  orca.orbsym correctly emits nothing.  Emitting anything from
        #  this output would be the bug -- see orca-h2o-sym.
        silent_ok={'ORBSYM][ORBSYMBETA': 'no UseSym in this deck, so ORCA '
                                         'prints no Irrep column'},
        parse_args=('.', 'Geometry', 'SCF', 'RHF', '0'),
        expect={
            'GEOMTRACE': dict(min_blocks=2, keys={'GEOMTRACE': {}}),
        },
    ),

    # -----------------------------------------------------------------
    # NWChem, issue #45 round 2: the property/correlated-method paths the
    # first pass could not cover for lack of the right job types.
    # -----------------------------------------------------------------
    dict(
        # `task scf property` on a closed-shell molecule run UHF -- the one
        # wavefunction NWChem 7.2.3 will compute *all* of shielding,
        # spin-spin, efield, efieldgrad and electrondensity for.  (RHF
        # refuses spin-spin: "needs UHF wave function"; a genuinely
        # open-shell UHF refuses it too: "needs closed shell UHF!".)
        name='nwchem-h2o-property',
        desc='nwchem.desc',
        fixture='nwchem/h2o_prop.eprint',
        parse_args=('.', 'Property', 'SCF', 'UHF', '0'),
        silent_ok={
            'MOLAB2': 'This job is C1, so NWChem emits no "group irrep '
                      'names" block; [SYMLAB] never fires, the parseSym '
                      'side file nwchem.molab reads is never written, and '
                      'the numeric MO symmetry indices have nothing to map '
                      'to. NWChem itself prints no labels for C1 either.',
            'MOLAB3': 'See MOLAB2 -- same C1 case, beta spin.',
        },
        expect={
            # NMR shielding: one nwchem.nmr invocation, four properties.
            # Values are the raw ecce_print numbers, unchanged.
            'SHIELDTENSOR': dict(blocks=1, keys={
                'ISOSHIELD': {
                    'values': '3.65691596574806e+02 3.36736926051824e+01 '
                              '3.36736926051824e+01',
                    'rowlabels': 'O H H',
                    'units': 'ppm'},
                'ANISOSHIELD': {
                    'values': '4.02300283803942e+00 1.51661427193216e+01 '
                              '1.51661427193216e+01',
                    'units': 'ppm'},
                # 3 atoms x 9 tensor components, in the atom order of the
                # geometry -- pinned via the label vector, whose generation
                # is the fiddly part of nwchem.nmr.
                'SHIELDTENSOR': {
                    'size': '27',
                    'rowlabels': 'O-1-XX O-1-YX O-1-ZX O-1-XY O-1-YY O-1-ZY '
                                 'O-1-XZ O-1-YZ O-1-ZZ H-2-XX H-2-YX H-2-ZX '
                                 'H-2-XY H-2-YY H-2-ZY H-2-XZ H-2-YZ H-2-ZZ '
                                 'H-3-XX H-3-YX H-3-ZX H-3-XY H-3-YY H-3-ZY '
                                 'H-3-XZ H-3-YZ H-3-ZZ',
                    'values_contain': '3.68373598466833e+02'},
                'SHIELDEIGVAL': {
                    'size': '9',
                    'values_contain': '3.63677154189583e+02',
                    'units': 'ppm'},
            }),
            # Property module: [EFIELD]'s Begin/End are both '^EField', and
            # the block really does run from the module's entry line to its
            # exit line -- pin the whole table so an off-by-one block would
            # show up as a short/shifted value list.
            'EFIELD': dict(blocks=1, keys={'EFIELD': {
                'size': '3 3',
                'columnlabels': 'X Y Z',
                'values': '8.326672684688670e-16 -7.706535093075340e-17 '
                          '3.910117256700740e-01 -8.553998786128360e-02 '
                          '-5.613401779950160e-17 -2.756576639075070e-02 '
                          '8.553998786128340e-02 1.041423697688570e-16 '
                          '-2.756576639075090e-02',
                'units': 'e/Bohr**2'}}),
            # End='^EFG' (upper case) really is what closes the lower-case
            # '^efg' block: NWChem prints the module name capitalised on the
            # exit line.  If that ever changes the block runs away to EOF.
            'EFIELDGRAD': dict(blocks=1, keys={
                'EFIELDGRAD': {'size': '27',
                               'values_contain': '-2.34593896764136e+00',
                               'units': 'e/Bohr**3'},
                'EFIELDASYMM': {'values': '7.62015616059500e-01 '
                                          '1.27020322453368e-01 '
                                          '1.27020322453367e-01'},
                'EFIELDEIGVAL': {'size': '9'},
            }),
            'EDENS': dict(blocks=1, keys={'EDENS': {
                'size': '3',
                'values': '1.933138809532820e+02 3.627096967971920e-01 '
                          '3.627096967971920e-01',
                'units': 'e/Bohr**3'}}),
            'SPINSPIN': dict(blocks=1, keys={
                'SPINSPIN': {'size': '1 9',
                             'rowlabels': '2-3',
                             'columnlabels': 'XX YX ZX XY YY ZY XZ YZ ZZ',
                             'units': 'Hertz'},
                'SSISOTROPY': {'values': '-2.05163418156684e+01',
                               'units': 'Hertz'},
            }),
        },
    ),
    dict(
        # `property; hyperfine` needs a genuinely open-shell wavefunction,
        # so it cannot share the fixture above.
        name='nwchem-oh-hyperfine',
        desc='nwchem.desc',
        fixture='nwchem/oh_hfine.eprint',
        parse_args=('.', 'Property', 'SCF', 'UHF', '1'),
        expect={
            # Regression guard for a typo this suite found on 2026-09-21:
            # nwchem.mlknshell did print "\nn" instead of "\n" before the
            # units: header, so whenever the shell count was not a multiple
            # of 10 (i.e. nearly always) the record read "nunits: e" and the
            # units section was silently lost.
            'MLKNSHELL': dict(keys={'MLKNSHELL': {'units': 'e'}}),
            'FERMI': dict(blocks=1, keys={'FERMI': {
                'values': '-5.098865998460240e+01 -1.135131040009480e+02',
                'units': 'MHertz'}}),
            # [SPINDIPOLE] is a section label; the real keys are
            # DIPOLETENSOR / DIPOLEEIGVAL (see KNOWN_KEY_ALIASES).
            'SPINDIPOLE': dict(blocks=1, keys={
                'DIPOLETENSOR': {'size': '18',
                                 'values_contain': '-9.06057990151224e+01',
                                 'units': 'au'},
                'DIPOLEEIGVAL': {'size': '6',
                                 'rowlabels': 'O-1-1 O-1-2 O-1-3 H-2-1 '
                                              'H-2-2 H-2-3'},
            }),
        },
    ),
    dict(
        # `task mp2` then `task ccsd(t)` in one deck: the correlated-method
        # energies, plus the two nwchem.scalar branches nothing had ever
        # triggered.
        name='nwchem-h2o-mp2-ccsd',
        desc='nwchem.desc',
        fixture='nwchem/h2o_mp2ccsd.eprint',
        parse_args=('.', 'Energy', 'MP2', 'MP2', '0'),
        expect={
            # nwchem.emp2 picks its output key off the matched Begin line,
            # so all three of these share one script and must not be
            # confused for one another.  ECCSDTPERT2 in particular exists
            # only because /ccsd\(t\)/ does NOT match "ccsd+t(ccsd)".
            'EMP2': dict(blocks=1, keys={
                'EMP2': {'values': '-7.61128253843450e+01',
                         'units': 'Hartree'}}),
            'ECCSDTPERT': dict(blocks=1, keys={
                'ECCSDTPERT': {'values': '-7.61203498558917e+01'}}),
            'ECCSDTPERT2': dict(blocks=1, keys={
                'ECCSDTPERT2': {'values': '-7.61204485951318e+01'}}),
            # -------------------------------------------------------
            # REGRESSION GUARDS for two nwchem.desc fixes made while
            # closing #45 (same family as #80: the ecce_print tag
            # wording is not what the Begin was written against).
            #
            # [RESIDNORM] Begin was 'begin%rms error'.  NWChem 7.2.3
            # emits that tag only as 'ccsd rms error' -- the short form
            # appears nowhere in the binary -- so the entry could never
            # fire and Residual Norm was silently never extracted for
            # any job, ever.  Nine CCSD iterations, so nine blocks.
            'RESIDNORM': dict(blocks=9, keys={
                'RESIDNORM': {'values': '7.81549290660813e-08'}}),
            # [CORRELTN] Begin was 'begin%correlation energy', which the
            # mp2 and dft modules do emit but ccsd does not: it tags its
            # own as 'ccsd correlation energy'.  A plain `task ccsd(t)`
            # job therefore produced no Correlation Energy at all.  With
            # both matched this deck fires twice; the scalar overwrites,
            # so the value that survives is the later task's (CCSD).
            'CORRELTN': dict(blocks=2, keys={
                'CORRELTN': {'values': '-1.35379522206962e-01',
                             'units': 'Hartree'}}),
            # nwchem.te picks up an MP2 job's total energy as TE (see the
            # header comment in nwchem.emp2); here the last task is
            # CCSD(T), so TE holds the CCSD(T) energy.
            'TEPROP': dict(keys={'TE': {
                'values': '-7.61203498558917e+01'}}),
        },
    ),
    dict(
        # `task scf freq`.  The vibrational entries were hand-checked
        # against real output while closing #45 but had no fixture; this
        # turns that into a guard.
        name='nwchem-h2o-freq',
        desc='nwchem.desc',
        fixture='nwchem/h2o_freq.eprint',
        parse_args=('.', 'Vibration', 'SCF', 'RHF', '0'),
        silent_ok={
            'MOLAB1': 'C1 geometry, so no "group irrep names" block and '
                      'nothing for nwchem.molab to map (see '
                      'nwchem-h2o-property).',
        },
        expect={
            # All 3N modes are kept, the six near-zero translational/
            # rotational ones included, exactly as the trace prints them --
            # so a parser that silently dropped or shifted a column would
            # show here.  The three real modes are the last three.
            'VIBFREQ': dict(blocks=1, keys={'VIBFREQ': {
                'size': '9',
                'values': '-1.09717334689780e-05 0.00000000000000e+00 '
                          '0.00000000000000e+00 0.00000000000000e+00 '
                          '8.33018468071379e-06 5.05042273434360e-05 '
                          '2.04328901326101e+03 4.48845003855228e+03 '
                          '4.76758517976390e+03',
                'units': 'cm-1'}}),
            # Begin must stay on the '(KM/mol)' variant: NWChem also prints
            # a bare 'projected intensities' block and a
            # 'projected intensities (debye/ang)2' one just before it.
            'VIBIR': dict(blocks=1, keys={'VIBIR': {
                'size': '9',
                'values_contain': '2.00108732044751e+01',
                'units': 'KM/Mole'}}),
            # 9 modes x 3 atoms x 3 cartesian components.
            'VIB': dict(blocks=1, keys={'VIB': {
                'size': '9 3 3',
                'columnlabels': 'X Y Z',
                'units': 'Angstrom'}}),
        },
    ),

    # -----------------------------------------------------------------
    # ORCA round 2 (the 15 parse types no fixture exercised).  Two real
    # bugs came out of writing these; both are fixed in orca.desc and
    # both are guarded below -- see orca-h2o-optfreq's [VIB] note and
    # orca-h2o-nmr's [ISOSHIELD][ANISOSHIELD] note.
    # -----------------------------------------------------------------
    dict(
        # RHF/STO-3G Opt Freq: the whole optimisation + vibration +
        # thermochemistry side of orca.desc in one job.
        name='orca-h2o-optfreq',
        desc='orca.desc',
        fixture='orca/h2o_optfreq.out',
        #  Captured before ai.orca could emit "! UseSym", so ORCA's
        #  ORBITAL ENERGIES block here has no Irrep column and
        #  orca.orbsym correctly emits nothing.  Emitting anything from
        #  this output would be the bug -- see orca-h2o-sym.
        silent_ok={'ORBSYM][ORBSYMBETA': 'no UseSym in this deck, so ORCA '
                                         'prints no Irrep column'},
        parse_args=('.', 'GeoVib', 'SCF', 'RHF', '0'),
        expect={
            'GEOMTRACE': dict(min_blocks=5, keys={'GEOMTRACE': {
                'size': '1 3 3',
                'rowlabels': '1-O 2-H 3-H',
                'values': '0.000000 0.000000 0.123868 0.000000 0.758080 '
                          '-0.511934 0.000000 -0.758080 -0.511934',
                'units': 'Angstrom'}}),
            # One entry, Frequency=all: TE keeps being overwritten and ends
            # up holding the converged energy while TEVEC accumulates the
            # trace (the combined-entry pattern orca.desc documents).
            'TE][TEVEC': dict(blocks=5, keys={
                'TE': {'values': '-74.965901192193', 'units': 'Hartree'},
                'TEVEC': {'values': '-74.965901192193'},
            }),
            'NNREPUL][ONEELEC][TWOELEC': dict(blocks=5, keys={
                'NNREPUL': {'values': '8.90649018320854', 'units': 'Hartree'},
                'ONEELEC': {'values': '-121.83412579918161'},
                'TWOELEC': {'values': '37.96173442378014'},
            }),
            # Lines=2 must take the Potential Energy line AND the Kinetic
            # Energy line that follows it; a Lines=1 would leave KE empty.
            'PE][KE': dict(blocks=5, keys={
                'PE': {'values': '-149.48450353024197', 'units': 'Hartree'},
                'KE': {'values': '74.51860233804906'},
            }),
            # Needs ai.orca's "%output Print[P_MOs] 1 end"; the fixture's
            # deck carries it.  Closed shell, so the BETA keys are
            # correctly absent -- see orca-oh-uhf for those.
            'MO][MOBETA][ORBENG][ORBENGBETA][ORBOCC][ORBOCCBETA': dict(
                keys={
                    'ORBENG': {'values': '-20.25158 -1.25755 -0.59386 '
                                         '-0.45973 -0.39262 0.58179 0.69267',
                               'units': 'Hartree'},
                    'ORBOCC': {'values': '2.00000 2.00000 2.00000 2.00000 '
                                         '2.00000 0.00000 0.00000',
                               'units': 'electrons'},
                    'MO': {'size': '7 7'},
                }),
            'EGRAD][EGRADVEC': dict(blocks=4, keys={
                'EGRAD': {'values': '0.0000258219', 'units': 'Hartree/Bohr'},
                'EGRADVEC': {'size': '3 3',
                             'values': '-0.000000000 0.000000000 0.000016520 '
                                       '0.000000000 0.000011345 -0.000008260 '
                                       '-0.000000000 -0.000011345 '
                                       '-0.000008260'},
            }),
            # ORCA prints a.u.; orca.dipole converts to Debye.
            # -0.672453762 au * 2.541746 = -1.70924... -- pinned because a
            # dropped conversion would still look like a plausible number.
            'DIPOLE': dict(keys={'DIPOLE': {
                'rowlabels': 'X Y Z',
                'values': '0 0 -1.70924162734408',
                'units': 'Debye'}}),
            # Needs ai.orca's "%elprop Quadrupole true end".
            'QUADPOLE': dict(keys={'QUADPOLE': {
                'rowlabels': 'XX XY XZ YY YZ ZZ',
                'values': '-4.554279009 -0.000000000 0.000000000 '
                          '-3.334921215 -0.000000000 -3.822777869',
                'units': 'au'}}),
            'MULLIKEN': dict(keys={'MULLIKEN': {
                'values': '-0.330530 0.165265 0.165265', 'units': 'e'}}),
            # ---------------------------------------------------------
            # REGRESSION GUARD, bug found by this suite on 2026-09-21
            # and fixed in orca.desc the same day.
            #
            # [VIBFREQ]'s End used to be `NORMAL MODES`.  eccejobmonitor
            # consumes the End-matching line as part of the block, so it
            # ate the very line [VIB]'s Begin needs -- and [VIB] could
            # therefore NEVER fire, on any ORCA job, ever.  Because
            # PropertyPanelDescriptor.xml gates the "Vibrational
            # Frequencies" panel on VIB and not on VIBFREQ, that meant
            # ORCA frequency jobs extracted correct frequencies and
            # still showed no vibration panel at all, with no error
            # anywhere.  Same End-line starvation family as the .desc
            # bugs in CLAUDE.md; mopac.desc's header warns about
            # exactly this and ORCA had it.
            #
            # The fix stops [VIBFREQ] on the dashes line that precedes
            # the header instead.  [VIB]'s own End had the identical
            # problem one step further down the chain (End=`IR
            # SPECTRUM` would have starved [VIBIR] the moment [VIB]
            # started firing -- [VIBIR] only looked healthy because
            # [VIB] never fired), so it stops on dashes too.
            #
            # All three are asserted together on purpose: they are
            # row-indexed against each other by NModePanel, so the
            # lengths must agree -- 9 = 3N for water, translations and
            # rotations included as zeros.
            # ---------------------------------------------------------
            'VIBFREQ': dict(keys={'VIBFREQ': {
                'size': '9',
                'values': '0.00 0.00 0.00 0.00 0.00 0.00 2169.82 4139.63 '
                          '4390.66',
                'units': 'cm-1'}}),
            'VIB': dict(blocks=1, keys={'VIB': {
                'size': '9 3 3',
                'rowlabels': '1 2 3',
                'columnlabels': 'X Y Z'}}),
            'VIBIR': dict(keys={'VIBIR': {
                'size': '9',
                'values': '0 0 0 0 0 0 7.24 44.27 29.93',
                'units': 'KM/Mole'}}),
            'PNTGRP': dict(keys={'PNTGRP': {'values': 'C2v'}}),
            'EZEROPT': dict(keys={'EZEROPT': {'values': '0.02437665',
                                              'units': 'Hartree'}}),
            'ETHERM': dict(keys={'ETHERM': {'values': '-74.93869172',
                                            'units': 'Hartree'}}),
            'ENTHALPY': dict(keys={'ENTHALPY': {'values': '-74.93774751',
                                                'units': 'Hartree'}}),
            'ENTROPY': dict(keys={'ENTROPY': {'values': '0.02151523',
                                              'units': 'Hartree'}}),
            'EGIBBS': dict(keys={'EGIBBS': {'values': '-74.95926274',
                                            'units': 'Hartree'}}),
            # orca.deltae matches the SCF iteration line's own shape, so
            # it fires once per iteration across all five SCF runs.  The
            # last delivered block is the last iteration of the final SCF.
            'DELTAE': dict(min_blocks=20, keys={'DELTAE': {
                'units': 'Hartree'}}),
            'ETIME': dict(keys={'ETIME': {}}),
        },
    ),
    dict(
        # UHF/STO-3G doublet: S2 and the beta-spin orbital path, neither
        # of which any closed-shell fixture can reach.
        name='orca-oh-uhf',
        desc='orca.desc',
        fixture='orca/oh_uhf.out',
        #  Captured before ai.orca could emit "! UseSym", so ORCA's
        #  ORBITAL ENERGIES block here has no Irrep column and
        #  orca.orbsym correctly emits nothing.  Emitting anything from
        #  this output would be the bug -- see orca-h2o-sym.
        silent_ok={'ORBSYM][ORBSYMBETA': 'no UseSym in this deck, so ORCA '
                                         'prints no Irrep column'},
        parse_args=('.', 'Energy', 'SCF', 'UHF', '1'),
        expect={
            'S2': dict(keys={'S2': {'values': '0.753262'}}),
            'MO][MOBETA][ORBENG][ORBENGBETA][ORBOCC][ORBOCCBETA': dict(
                blocks=1, keys={
                    'ORBENG': {'values': '-20.28571 -1.29219 -0.55098 '
                                         '-0.52456 -0.42967 0.62006'},
                    'ORBENGBETA': {'values': '-20.25724 -1.12820 -0.50287 '
                                             '-0.37779 0.36003 0.65576'},
                    # A doublet: 5 alpha, 4 beta, occupations 1.0 not 2.0.
                    'ORBOCC': {'values': '1.00000 1.00000 1.00000 1.00000 '
                                         '1.00000 0.00000',
                               'units': 'electrons'},
                    'ORBOCCBETA': {'values': '1.00000 1.00000 1.00000 '
                                             '1.00000 0.00000 0.00000'},
                    'MO': {'size': '6 6'},
                    'MOBETA': {'size': '6 6'},
                }),
            # For an open-shell job ORCA's MULLIKEN ATOMIC CHARGES table
            # grows a second column (the spin population):
            #     0 O :   -0.164453    1.069760
            # The charge is column 1; the spin population must NOT be
            # mistaken for a second atom's charge or for the charge
            # itself.  That mistake, in the mirror-image direction, is
            # the open-shell Gaussian bug this suite found the same day
            # (see g16-oh-uhf), so pin it here for ORCA too.
            'MULLIKEN': dict(keys={'MULLIKEN': {
                'size': '2',
                'values': '-0.164453 0.164453',
                'units': 'e'}}),
            'DIPOLE': dict(keys={'DIPOLE': {
                'values': '0 0 1.2784905855448', 'units': 'Debye'}}),
            'QUADPOLE': dict(keys={'QUADPOLE': {
                'values': '-3.696669441 0.296906302 0.000000000 '
                          '-3.717599094 0.000000000 -3.087273634'}}),
            'TE][TEVEC': dict(blocks=1, keys={
                'TE': {'values': '-74.362669195155'}}),
        },
    ),
    dict(
        #  RHF/STO-3G geometry optimisation with CHELPG charges -- a deck
        #  ECCE can now GENERATE, unlike orca-h2o-nmr whose CHELPG was
        #  added by hand before &ChelpgToken existed (#88).
        #
        #  THE POINT OF THIS FIXTURE IS THE MULTI-BLOCK CASE. ORCA emits
        #  a CHELPG block on every optimisation cycle -- five here -- and
        #  the charges genuinely move as the geometry relaxes: O goes
        #  from -0.618127 in the first block to -0.566360 in the last.
        #  [ESPCHARGE] is Frequency=last, so what must survive is the
        #  CONVERGED geometry's charges. Pinning the last block's values
        #  is the assertion that it does: were the frequency ever changed
        #  to first, or the buffering to deliver the wrong block, the
        #  panel would show charges for an intermediate geometry -- a
        #  perfectly plausible set of numbers for the wrong structure,
        #  which is the failure mode this suite exists to catch.
        #
        #  Verified against ORCA 6.1.1, which also accepts CHELPG
        #  alongside Opt, Freq, NMR and DFT -- checked because the
        #  checkbox is offered for every runtype.
        name='orca-h2o-opt-chelpg',
        desc='orca.desc',
        fixture='orca/h2o_opt_chelpg.out',
        #  Captured before ai.orca could emit "! UseSym", so ORCA's
        #  ORBITAL ENERGIES block here has no Irrep column and
        #  orca.orbsym correctly emits nothing.  Emitting anything from
        #  this output would be the bug -- see orca-h2o-sym.
        silent_ok={'ORBSYM][ORBSYMBETA': 'no UseSym in this deck, so ORCA '
                                         'prints no Irrep column'},
        parse_args=('.', 'Geometry', 'SCF', 'RHF', '0'),
        expect={
            #  blocks= counts DELIVERED blocks, and Frequency=last
            #  buffers the other four away -- the golden file records
            #  begins=5 delivered=1 buffered_away=4, which is the real
            #  assertion that all five were seen and the last one won.
            'ESPCHARGE': dict(blocks=1, keys={
                'ESPCHARGE': {'size': '3 1',
                              'rowlabels': '0-O 1-H 2-H',
                              'values': '-0.566360 0.283186 0.283175',
                              'units': 'e'}}),
        },
    ),
    dict(
        #  RHF/def2-SVP water with "! UseSym" -- the ONLY way ORCA emits
        #  orbital symmetry labels at all, and therefore the only way
        #  [ORBSYM][ORBSYMBETA] can ever fire.  Without the keyword the
        #  "ORBITAL ENERGIES" block has no Irrep column and the entry
        #  correctly produces nothing, which is why every other ORCA
        #  fixture here leaves it silent.
        #
        #  THE POINT OF THIS FIXTURE IS THE LENGTH.  ORCA truncates that
        #  block to the first ten virtuals unless asked for more, so this
        #  deck carries "Print[P_OrbEn] 2" exactly as ai.orca now emits
        #  it.  def2-SVP water has 24 basis functions; without the flag
        #  ORCA lists 16 of them, verified.  A 16-long ORBSYM against a
        #  24-long ORBENG is worse than no symmetry column, because
        #  MoPanel::fillTable() reverses the two vectors independently
        #  before indexing them by the same row -- it does not run out at
        #  the end, it shifts, and mislabels every row.  Pinning size=24
        #  here is the assertion that the print flag is still in the
        #  generated deck.
        name='orca-h2o-sym',
        desc='orca.desc',
        fixture='orca/h2o_sym.out',
        parse_args=('.', 'Energy', 'SCF', 'RHF', '0'),
        expect={
            'ORBSYM][ORBSYMBETA': dict(blocks=1, keys={
                'ORBSYM': {'size': '24',
                           'values': 'A1 A1 B2 A1 B1 A1 B2 B2 A1 A1 B1 '
                                     'B2 A1 A2 A1 B1 B2 B2 A1 B1 A2 A1 '
                                     'A1 B2'}}),
        },
    ),
    dict(
        #  REGRESSION GUARD, found live on a real job after the symmetry
        #  work shipped.  Cs, so the irreps are A' and A" -- and the
        #  double-prime character is the whole point.
        #
        #  orca.orbsym's label pattern allowed the apostrophe but not the
        #  double quote, so every A" row failed to match, left a hole in
        #  the vector, and the script's own completeness check then
        #  dropped the WHOLE property rather than emit a gappy one.  The
        #  result was correct symmetries for C2v water and nothing
        #  whatsoever for any Cs molecule, with no error anywhere.
        #
        #  orca-h2o-sym could never have caught it: C2v's labels are
        #  A1/A2/B1/B2, none of which carry a prime.  Any future symmetry
        #  work needs a fixture whose point group has primed labels.
        name='orca-hof-sym',
        desc='orca.desc',
        fixture='orca/hof_sym.out',
        parse_args=('.', 'Energy', 'SCF', 'RHF', '0'),
        expect={
            'ORBSYM][ORBSYMBETA': dict(blocks=1, keys={
                'ORBSYM': {'size': '33'}}),
        },
    ),
    dict(
        # RHF/STO-3G NMR CHELPG: the NMR shielding and ESP-charge entries.
        # This deck's CHELPG was added BY HAND, because when it was
        # captured ai.orca had no CHELPG path and ESPCHARGE could not be
        # reached from the GUI at all (#88). It can now: the Runtype
        # Details "CHELPG charges" checkbox drives &ChelpgToken, and
        # orca-h2o-opt-chelpg below is a fixture from a deck ECCE can
        # actually generate. Kept as-is -- it covers CHELPG alongside NMR,
        # which that one does not.
        name='orca-h2o-nmr',
        desc='orca.desc',
        fixture='orca/h2o_nmr_chelpg.out',
        #  Captured before ai.orca could emit "! UseSym", so ORCA's
        #  ORBITAL ENERGIES block here has no Irrep column and
        #  orca.orbsym correctly emits nothing.  Emitting anything from
        #  this output would be the bug -- see orca-h2o-sym.
        silent_ok={'ORBSYM][ORBSYMBETA': 'no UseSym in this deck, so ORCA '
                                         'prints no Irrep column'},
        parse_args=('.', 'Magnetic', 'SCF', 'RHF', '0'),
        expect={
            # ---------------------------------------------------------
            # REGRESSION GUARD, second bug found by this suite on
            # 2026-09-21 and fixed in orca.desc the same day, same
            # End-line starvation family as [VIB] above.
            #
            # [ISOSHIELD][ANISOSHIELD]'s Begin used to be `CHEMICAL
            # SHIELDING SUMMARY \(ppm\)`.  [SHIELDTENSOR]'s End is
            # `CHEMICAL SHIELDING SUMMARY`, and the End-matching line is
            # consumed by that block -- so this Begin could never match
            # and ORCA NMR jobs silently never produced ISOSHIELD or
            # ANISOSHIELD, while SHIELDTENSOR/SHIELDEIGVAL from the very
            # same section worked fine.
            #
            # Fixed by anchoring the Begin on the summary table's own
            # column header, which lies past [SHIELDTENSOR]'s End line.
            # Cross-check that the two entries agree: SHIELDEIGVAL's
            # three H eigenvalues average to ISOSHIELD's H value
            # ((27.026+30.232+43.780)/3 = 33.679).
            # ---------------------------------------------------------
            'ISOSHIELD][ANISOSHIELD': dict(blocks=1, keys={
                'ISOSHIELD': {'rowlabels': '0-O 1-H 2-H',
                              'values': '365.694 33.679 33.679',
                              'units': 'ppm'},
                'ANISOSHIELD': {'values': '4.029 15.151 15.151',
                                'units': 'ppm'},
            }),
            'SHIELDTENSOR][SHIELDEIGVAL': dict(blocks=1, keys={
                'SHIELDTENSOR': {
                    'size': '27',
                    'rowlabels': '0-O-XX 0-O-XY 0-O-XZ 0-O-YX 0-O-YY 0-O-YZ '
                                 '0-O-ZX 0-O-ZY 0-O-ZZ 1-H-XX 1-H-XY 1-H-XZ '
                                 '1-H-YX 1-H-YY 1-H-YZ 1-H-ZX 1-H-ZY 1-H-ZZ '
                                 '2-H-XX 2-H-XY 2-H-XZ 2-H-YX 2-H-YY 2-H-YZ '
                                 '2-H-ZX 2-H-ZY 2-H-ZZ',
                    'units': 'ppm'},
                'SHIELDEIGVAL': {
                    'size': '9',
                    'values': '363.670 365.032 368.379 27.026 30.232 43.780 '
                              '27.026 30.232 43.780',
                    'units': 'ppm'},
            }),
            'ESPCHARGE': dict(blocks=1, keys={'ESPCHARGE': {
                'size': '3 1',
                'rowlabels': '0-O 1-H 2-H',
                'values': '-0.618788 0.309403 0.309386',
                'units': 'e'}}),
        },
    ),

    # -----------------------------------------------------------------
    # MOPAC (issue #86).  Integrated 2026-09-21 with no regression
    # coverage at all; these are the first cases for it.  MOPAC is
    # semiempirical, so the energies are heats of formation in kcal/mol
    # and there is no basis set anywhere in the pipeline.
    # -----------------------------------------------------------------
    dict(
        # PM7 EF PRECISE geometry optimisation of methane.
        name='mopac-ch4-opt',
        desc='mopac.desc',
        fixture='mopac/ch4_opt.out',
        parse_args=('.', 'Geometry', 'SE', 'RPM7', '0'),
        expect={
            # A geometry job prints the "FINAL HEAT OF FORMATION" wording,
            # which is the only one mopac.energy emits TEVEC for (a FORCE
            # job's repeated non-final wording would otherwise make TEVEC
            # longer than GEOMTRACE -- see mopac-ch4-force below).
            # kcal/mol is the Energy converter's own base unit, so the
            # number must pass through unconverted.
            'TE][TEVEC][HFENERGY': dict(blocks=1, keys={
                'TE': {'values': '-14.40441', 'units': 'kcal/mol'},
                'TEVEC': {'values': '-14.40441', 'units': 'kcal/mol'},
                'HFENERGY': {'values': '-14.40441', 'units': 'kcal/mol'},
            }),
            # MOPAC prints no per-cycle geometry at ordinary verbosity, so
            # this trace is the input echo (4 decimals) plus the final
            # geometry (9 decimals) -- two frames, deliberately, and the
            # last one is what CalcMgr's "reuse the optimized geometry"
            # path reads.  Both table layouts must parse despite only one
            # Skip value being available for both (mopac.desc's note).
            'GEOMTRACE': dict(blocks=2, keys={'GEOMTRACE': {
                'size': '1 5 3',
                'rowlabels': '1-C 2-H 3-H 4-H 5-H',
                'values_contain': '0.626527549',
                'units': 'Angstrom'}}),
            # One block, two properties: MOPAC prints the dipole table
            # immediately after the charge table with no separator, which
            # is why they are a single combined entry.
            'MULLIKEN][DIPOLE': dict(blocks=1, keys={
                'MULLIKEN': {'values': '-0.603058 0.150764 0.150764 '
                                       '0.150764 0.150764',
                             'units': 'e'},
                'DIPOLE': {'rowlabels': 'X Y Z', 'units': 'Debye'},
            }),
            # Frequency=last: MOPAC prints the point group twice (before
            # and after optimisation) and the post-optimisation one wins.
            # PRECISE is what gets methane to real Td here; a loosely
            # converged run reports D2d, so this also guards the fixture.
            'PNTGRP': dict(keys={'PNTGRP': {'values': 'Td'}}),
            'IP': dict(keys={'IP': {'values': '13.727659', 'units': 'eV'}}),
            'ETIME': dict(keys={'ETIME': {}}),
        },
    ),
    dict(
        # ---------------------------------------------------------------
        # PM7 FORCE THERMO on METHANE, not water, on purpose.
        #
        # MOPAC's friendly "DESCRIPTION OF VIBRATIONS" section COLLAPSES
        # symmetry-degenerate modes: CH4 has 9 modes but only 4 distinct
        # frequencies, so sourcing VIBFREQ from there against a 9-mode
        # VIB silently misaligns the two, and NModePanel indexes VIBFREQ,
        # VIBSYM and VIB against each other row by row.  mopac.desc
        # therefore takes all three from the single NORMAL COORDINATE
        # ANALYSIS block, which lists every mode.
        #
        # This case pins the mode COUNT (9) and the degeneracy pattern
        # (2 x E, 3 x T2, 3 x T2, 1 x A1) so that cannot be reintroduced.
        # A C2v molecule such as water has no degeneracies and cannot
        # expose it at all -- which is exactly how it hid the first time.
        # ---------------------------------------------------------------
        name='mopac-ch4-force',
        desc='mopac.desc',
        fixture='mopac/ch4_force.out',
        parse_args=('.', 'Vibration', 'SE', 'RPM7', '0'),
        expect={
            'VIBFREQ][VIBSYM][VIB': dict(blocks=1, keys={
                'VIBFREQ': {
                    'size': '9',
                    'values': '1291.2 1291.2 1306.5 1306.5 1306.5 2689.9 '
                              '2689.9 2689.9 2813.7',
                    'units': 'cm-1'},
                # Four distinct frequencies, nine modes: the degeneracy
                # pattern itself.  If VIBFREQ ever came from DESCRIPTION
                # OF VIBRATIONS again it would have four rows here.
                'VIBSYM': {'size': '9',
                           'values': '1E 1E 1T2 1T2 1T2 2T2 2T2 2T2 1A1'},
                # 9 modes x 5 atoms x 3 Cartesian components: same mode
                # count as VIBFREQ/VIBSYM, structurally.
                'VIB': {'size': '9 5 3',
                        'rowlabels': '1 2 3 4 5',
                        'columnlabels': 'X Y Z',
                        'units': 'Angstrom'},
            }),
            # A FORCE-only run prints the SHORTER "HEAT OF FORMATION ="
            # wording (no "FINAL", KCALS/MOLE not KCAL/MOL), twice.  The
            # one entry's optional "FINAL " covers both wordings -- a
            # Vibration runtype would otherwise extract no energy at all.
            # TEVEC is deliberately NOT emitted for this wording, so that
            # len(TEVEC) can never exceed len(GEOMTRACE).
            'TE][TEVEC][HFENERGY': dict(blocks=2, keys={
                'TE': {'values': '-14.404414', 'units': 'kcal/mol'},
                'HFENERGY': {'values': '-14.404414'},
            }),
            'EZEROPT': dict(keys={'EZEROPT': {'values': '24.854',
                                              'units': 'kcal/mol'}}),
            'GEOMTRACE': dict(blocks=1, keys={'GEOMTRACE': {}}),
            'PNTGRP': dict(keys={'PNTGRP': {'values': 'Td'}}),
        },
    ),
    dict(
        # PM7 EF + a second OLDGEO FORCE data set in one file: what
        # ai.mopac writes for the GeoVib runtype, and the only shape in
        # which MOPAC can express "optimise then vibrate".
        name='mopac-ch4-geovib',
        desc='mopac.desc',
        fixture='mopac/ch4_geovib.out',
        parse_args=('.', 'GeoVib', 'SE', 'RPM7', '0'),
        expect={
            #  Thermochemistry.  THERMOTEMP is the load-bearing one:
            #  MOPAC prints 298 K first and then scans upward from
            #  200 K, so any "take the last stanza" reading would report
            #  the top of the scan as room temperature -- a set of
            #  perfectly plausible numbers for the wrong temperature,
            #  which is the failure this suite exists to catch.  Pinning
            #  298.00 is the assertion that the right stanza was chosen.
            #
            #  The values are hand-checked against this fixture's own
            #  TOT. line, "-14.404  2403.7949  8.6898  44.4894":
            #    ENTHALPY = -14.404 kcal/mol / 627.5094740631
            #    ENTROPY  = 44.4894 * 298.00 / 1000 / 627.5094740631
            #  ENTROPY is T*S, not S, because that is what ECCE's key
            #  holds -- its label is literally "Entropy Term (T*S)".
            #
            #  EGIBBS is absent by design, not by omission: MOPAC has no
            #  absolute energy, so H - T*S here would not mean what that
            #  key means for ORCA and Gaussian.
            #
            #  ETHERM IS NOT THE TABLE'S "ENTHALPY" COLUMN, and pinning
            #  it here is what stops it being "corrected" back to one.
            #  That column is the H(T)-H(0) correction, and its own
            #  footnote says "Hvib: Zero-point energy is not included" --
            #  using it would discard the ZPE and put a ~2 kcal/mol
            #  correction in the row where ORCA shows a total.
            #
            #  MOPAC's heat of formation already contains the zero-point
            #  and thermal terms (its manual: E_SCF = E_eq + E_zpe +
            #  E_vib + E_rot + E_tra + PV), so the internal energy is
            #  H.O.F. - RT.  The two values below differ by exactly
            #  RT = 1.987204259 * 298.00 / 1000 kcal/mol, which is ORCA's
            #  own "H = U + kB*T" seen from the other side.
            'ENTHALPY][ETHERM][ENTROPY][HEATCAP][THERMOTEMP': dict(
                blocks=1, keys={
                    'ENTHALPY': {'values': '-0.022954235107774',
                                 'units': 'Hartree'},
                    'ETHERM': {'values': '-0.0238979449538543',
                               'units': 'Hartree'},
                    'THERMOTEMP': {'values': '298.00', 'units': 'Kelvin'},
                    'HEATCAP': {'values': '8.6898',
                                'units': 'cal/(K*mol)'},
                }),
            # Three energy matches (one FINAL from the optimisation, two
            # non-FINAL from the FORCE data set) but only ONE TEVEC
            # frame, against three GEOMTRACE frames: the invariant
            # mopac.desc relies on to keep the Geometry Trace plot from
            # having more points than the trace has frames (which would
            # drive GTStepCmd into PropTSVecTable::value()'s
            # out-of-bounds path).
            'TE][TEVEC][HFENERGY': dict(blocks=3, keys={
                'TE': {'values': '-14.404414'},
                'HFENERGY': {'values': '-14.404414'},
            }),
            'GEOMTRACE': dict(blocks=3, keys={'GEOMTRACE': {
                'size': '1 5 3',
                'units': 'Angstrom'}}),
            # Both data sets' vibrational output is in one file; the
            # second data set is the one with the NORMAL COORDINATE
            # ANALYSIS block, and Frequency=last picks it.
            'VIBFREQ][VIBSYM][VIB': dict(blocks=1, keys={
                'VIBFREQ': {'size': '9'},
                'VIBSYM': {'size': '9'},
                'VIB': {'size': '9 5 3'},
            }),
            'EZEROPT': dict(keys={'EZEROPT': {'units': 'kcal/mol'}}),
            'MULLIKEN][DIPOLE': dict(blocks=1, keys={'MULLIKEN': {}}),
            'PNTGRP': dict(keys={'PNTGRP': {'values': 'Td'}}),
            'IP': dict(keys={'IP': {'values': '13.727659'}}),
        },
    ),
    dict(
        # PM7 1SCF GRADIENTS -- the only runtype that produces MOPAC's
        # PARAMETER/ATOM/TYPE gradient table, and so the only one that
        # can exercise [EGRADVEC].
        name='mopac-h2o-grad',
        desc='mopac.desc',
        fixture='mopac/h2o_grad.out',
        parse_args=('.', 'Gradient', 'SE', 'RPM7', '0'),
        expect={
            # MOPAC prints kcal/mol per Angstrom; ECCE wants Hartree/Bohr.
            # Pinned at full precision because a dropped or doubled
            # conversion still yields a plausible-looking gradient:
            # -1.252503 kcal/(mol A) * 0.001593601451 * 0.52917725 =
            # -0.0010562327 Hartree/Bohr.
            'EGRADVEC': dict(blocks=1, keys={'EGRADVEC': {
                'size': '3 3',
                'columnlabels': 'X Y Z',
                'values': '-0.0010562327 0.0000000000 -0.0026106831 '
                          '0.0007823575 0.0000000000 0.0018591557 '
                          '0.0002738752 0.0000000000 0.0007515265',
                'units': 'Hartree/Bohr'}}),
            'TE][TEVEC][HFENERGY': dict(keys={
                'TE': {'values': '-57.78812', 'units': 'kcal/mol'},
                'TEVEC': {'values': '-57.78812'},
            }),
            'MULLIKEN][DIPOLE': dict(blocks=1, keys={
                'MULLIKEN': {'values': '-0.645579 0.323084 0.322495',
                             'units': 'e'},
                # A real, non-zero dipole (water, unlike methane), so the
                # X/Y/Z row split is actually exercised.
                'DIPOLE': {'values': '1.690 -0.000 1.315',
                           'rowlabels': 'X Y Z',
                           'units': 'Debye'},
            }),
            'PNTGRP': dict(keys={'PNTGRP': {'values': 'C2v'}}),
            'IP': dict(keys={'IP': {'values': '12.092724', 'units': 'eV'}}),
        },
    ),
    dict(
        # PM7 UHF 1SCF DOUBLET methyl radical: the open-shell path.
        # MOPAC's charge table has the same shape open- or closed-shell
        # (no spin-density column, unlike Gaussian's and ORCA's), so the
        # value list must be one entry per atom and nothing else -- the
        # guard for the open-shell-charge-table bug family (#80,
        # g16-oh-uhf, orca-oh-uhf).
        name='mopac-ch3-uhf',
        desc='mopac.desc',
        fixture='mopac/ch3_uhf.out',
        parse_args=('.', 'Energy', 'SE', 'UPM7', '1'),
        expect={
            'MULLIKEN][DIPOLE': dict(blocks=1, keys={'MULLIKEN': {
                'size': '4',
                'values': '-0.411272 0.137160 0.137056 0.137056',
                'units': 'e'}}),
            'TE][TEVEC][HFENERGY': dict(keys={
                'TE': {'values': '28.40785', 'units': 'kcal/mol'},
                'HFENERGY': {'values': '28.40785'},
            }),
            'PNTGRP': dict(keys={'PNTGRP': {'values': 'D3h'}}),
            'IP': dict(keys={'IP': {'values': '9.910583', 'units': 'eV'}}),
            'GEOMTRACE': dict(blocks=2, keys={'GEOMTRACE': {
                'size': '1 4 3'}}),
            'ETIME': dict(keys={'ETIME': {}}),
        },
    ),
    # -----------------------------------------------------------------
    # Quantum ESPRESSO (pw.x), phase 1.  Integrated 2026-09-21 against
    # the real Debian quantum-espresso 6.7-3+b1 binary; these are the
    # first cases for it.  QE is a PLANE-WAVE code, so there is no basis
    # set anywhere in the pipeline (needsBasis="false"), periodicity is
    # mandatory, and every fixture deck below was GENERATED BY ai.qe
    # itself from a real .param/.frag pair and then run to completion
    # ("JOB DONE.") -- so these cases exercise the input generator and
    # the parsers together, not just the parsers.
    #
    # pw.x prints its energies in RYDBERG, and EnergyConverter
    # (src/util/units/EnergyConverter.C) has no Rydberg entry at all, so
    # every energy below is converted to Hartree (x 0.5) by the parser
    # scripts.  The values are pinned at full precision because a
    # dropped or doubled conversion still yields a plausible-looking
    # energy -- exactly the shape of the MOPAC gradient-unit assertion.
    # -----------------------------------------------------------------
    dict(
        # Bulk silicon, primitive fcc cell, 4x4x4 Monkhorst-Pack,
        # occupations='fixed', tprnfor and tstress both on.  This is the
        # reference periodic case: a real crystal with a real k-point
        # grid.
        name='qe-si-scf',
        desc='qe.desc',
        fixture='qe/si_scf.pwout',
        parse_args=('.', 'Energy', 'PW', 'PW', '0'),
        expect={
            'VERSION': dict(keys={'VERSION': {'values': 'PWSCF 6.7MaX'}}),
            # -22.82339475 Ry / 2 = -11.411697375 Hartree.  The '!' prefix
            # is the only thing distinguishing this line from the
            # per-iteration ones, which is why [TE] is anchored ^! and
            # [EWVEC] is anchored ^\s+ -- see qe.desc.
            'TE': dict(blocks=1, keys={
                'TE': {'values': '-11.411697375', 'units': 'Hartree'}}),
            #  tstress was on, so pw.x printed the tensor -- and until
            #  qe.stress existed nothing read it back, making the Runtype
            #  Details "Stress" checkbox a dead control (ORCA CHELPG, #88).
            'STRESS][PRESSURE': dict(blocks=1, keys={
                'STRESS': {'size': '9', 'units': 'kbar'},
                'PRESSURE': {'size': '1', 'units': 'kbar'}}),
            # Four SCF iterations printed an energy before convergence
            # (the fifth, converged one carries the '!' and belongs to
            # [TE]).  EWVEC, not TEVEC: TEVEC is indexed by Geometry Step
            # and an scf run has no GEOMTRACE for it to be indexed
            # against (CLAUDE.md's len(TEVEC) <= len(GEOMTRACE) rule).
            'EWVEC': dict(blocks=4, keys={
                'EWVEC': {'values': '-11.41169429', 'units': 'Hartree'}}),
            # One per iteration plus one more printed immediately after
            # the converged energy -- five in total.  That last one is
            # only reachable because [TE] uses Lines=1 and so releases
            # the block instead of swallowing it; an End= there would
            # starve this entry permanently (the ORCA VIBFREQ/VIB shape).
            'DELTAE': dict(blocks=5, keys={
                'DELTAE': {'values': '4e-07', 'units': 'Hartree'}}),
            # Silicon's forces are zero by symmetry, which is the point
            # of also having the water case below: this pins the SHAPE
            # (2 atoms x 3), the water case pins the conversion.
            'EGRADVEC': dict(blocks=1, keys={'EGRADVEC': {
                'size': '2 3',
                'rowlabels': '1 2',
                'columnlabels': 'X Y Z',
                'units': 'Hartree/Bohr'}}),
            # One line, two properties: pw.x puts the CPU and the wall
            # time on the same "PWSCF : ...s CPU ...s WALL" row.
            'CPUSEC][ETIME': dict(blocks=1, keys={
                'CPUSEC': {}, 'ETIME': {}}),
        },
    ),
    dict(
        # Water in a 10 Angstrom cubic box at the gamma point.  Two
        # species (so ATOMIC_SPECIES has two rows and two different
        # pseudopotential files were resolved from pseudo_dir), and
        # NON-ZERO forces, which is what actually pins qe.egradvec's
        # sign flip and Ry/bohr -> Hartree/bohr conversion.
        name='qe-h2o-gamma',
        desc='qe.desc',
        fixture='qe/h2o_gamma.pwout',
        parse_args=('.', 'Energy', 'PW', 'PW', '0'),
        expect={
            'VERSION': dict(keys={'VERSION': {'values': 'PWSCF 6.7MaX'}}),
            'TE': dict(blocks=1, keys={
                'TE': {'values': '-21.985236255', 'units': 'Hartree'}}),
            # pw.x prints FORCES; EGRADVEC is a GRADIENT, so the whole
            # conversion is a multiply by -0.5 (sign flip for
            # force -> gradient, times 0.5 for Ry -> Hartree).  Pinned at
            # full precision: with the sign flip dropped, or the factor
            # left at 1.0, or applied twice, the numbers still look like
            # a perfectly ordinary water gradient.
            #   O z: -0.04591947 Ry/bohr force -> +0.0229597350 Ha/bohr
            #   H x: +0.02957214 Ry/bohr force -> -0.0147860700 Ha/bohr
            'EGRADVEC': dict(blocks=1, keys={'EGRADVEC': {
                'size': '3 3',
                'rowlabels': '1 2 3',
                'columnlabels': 'X Y Z',
                'values': '0.0000000000 0.0000000000 0.0229597350 '
                          '-0.0147860700 0.0000000000 -0.0114798700 '
                          '0.0147860700 0.0000000000 -0.0114798700',
                'units': 'Hartree/Bohr'}}),
            'EWVEC': dict(blocks=9, keys={'EWVEC': {'units': 'Hartree'}}),
            'DELTAE': dict(blocks=10, keys={'DELTAE': {'units': 'Hartree'}}),
            'CPUSEC][ETIME': dict(blocks=1, keys={
                'CPUSEC': {}, 'ETIME': {}}),
        },
    ),
    dict(
        # fcc ALUMINIUM -- a METAL, with occupations='smearing' and
        # Marzari-Vanderbilt smearing.  docs/QE_ROADMAP.md section 7
        # calls for this explicitly: QE's output differs between a metal
        # and an insulator (a metal prints "the Fermi energy is ...", a
        # "smearing contrib. (-TS)" term and no "highest occupied
        # level"), and testing only insulators is the same mistake as
        # testing MOPAC frequencies only on water.  It confirms none of
        # the extra metal-only lines collide with any Begin here.
        #
        # Also the only fixture with tstress on AND a non-zero pressure,
        # so it is the one that would notice if a stress entry were ever
        # added and started starving a neighbour.
        name='qe-al-smear',
        desc='qe.desc',
        fixture='qe/al_smear.pwout',
        parse_args=('.', 'Energy', 'PW', 'PW', '0'),
        expect={
            'VERSION': dict(keys={'VERSION': {'values': 'PWSCF 6.7MaX'}}),
            'TE': dict(blocks=1, keys={
                'TE': {'values': '-19.740479745', 'units': 'Hartree'}}),
            'EWVEC': dict(blocks=2, keys={'EWVEC': {'units': 'Hartree'}}),
            'DELTAE': dict(blocks=3, keys={'DELTAE': {'units': 'Hartree'}}),
            'EGRADVEC': dict(blocks=1, keys={'EGRADVEC': {
                'size': '1 3',
                'units': 'Hartree/Bohr'}}),
            'CPUSEC][ETIME': dict(blocks=1, keys={
                'CPUSEC': {}, 'ETIME': {}}),
        },
    ),
    dict(
        #  The only QE fixture that is an OPTIMISATION rather than a single
        #  point, and therefore the only one that exercises [GEOMTRACE].
        #
        #  Like its siblings, the deck was GENERATED BY ai.qe (with
        #  RunType: Geometry) and then run to completion by the real
        #  pw.x 6.7 on this machine -- so this case exercises the whole
        #  chain: the Geometry runtype reaching calculation='relax', the
        #  &IONS namelist an scf run must not have, forc_conv_thr and
        #  nstep from the runtype dialog's keys, pw.x accepting all of it,
        #  and qe.desc reading the trajectory back.
        #
        #  4 ATOMIC_POSITIONS blocks: 3 BFGS steps plus the duplicate
        #  printed inside "Begin final coordinates". The duplicate is
        #  harmless for animation (the last frame simply repeats) and is
        #  asserted so that a future QE version dropping it is noticed
        #  rather than silently absorbed.
        name='qe-h2o-relax',
        desc='qe.desc',
        fixture='qe/h2o_relax.pwout',
        parse_args=('.', 'GeomOpt', 'PW', 'PW', '0'),
        expect={
            'VERSION': dict(keys={'VERSION': {'values': 'PWSCF 6.7MaX'}}),
            'GEOMTRACE': dict(blocks=4, keys={'GEOMTRACE': {
                'size': '1 3 3',
                'units': 'Angstrom'}}),
        },
    ),
    # -----------------------------------------------------------------
    # GROMACS (mdrun .log).  GROUNDWORK ONLY -- there is no GROMACS.edml,
    # so no calculation can be created for this code and none of this is
    # reachable from the GUI.  It is here because the monitoring half of
    # a GROMACS integration can be built and verified independently of
    # the topology/force-field question that dominates the rest of it;
    # see docs/GROMACS_ROADMAP.md.
    #
    # Both fixtures are real GROMACS 2025.2 runs performed on this
    # machine (221 SPC waters, OPLS-AA, PME), with their .mdp and .top
    # checked in beside them.
    # -----------------------------------------------------------------
    dict(
        #  Steepest-descents minimisation, converged in 5 steps.
        #  5 step headers, 5 energy blocks, so all 5 are real steps --
        #  a minimisation prints no AVERAGES summary.
        #
        #  One header/value pair per block, so "Total Energy" does not
        #  exist and gromacs.energy falls back to "Potential". Pinning
        #  the value pins that fallback: an earlier version of the parser
        #  returned Potential for DYNAMICS runs too, where Total Energy
        #  does exist and is what was wanted.
        name='gromacs-water-em',
        desc='gromacs.desc',
        fixture='gromacs/water_em.log',
        parse_args=('.', 'Energy', 'MD', 'OPLS', '0'),
        expect={
            #  A minimisation prints no pressure column at all, so
            #  PRESSURE is absent here by design -- the parser guards on
            #  the term being present rather than emitting an empty one.
            'TE][TEVEC][PRESSURE': dict(blocks=5, keys={
                'TE': {'values': '-8.19803e+03', 'units': 'kJoule/Mole'},
                'TEVEC': {'units': 'kJoule/Mole'}}),
        },
    ),
    dict(
        #  500-step NVT run, v-rescale thermostat.
        #
        #  6 step headers but SEVEN energy blocks: the extra one is the
        #  end-of-run AVERAGES summary, which carries the identical
        #  "Energies (kJ/mol)" marker and would be parsed as one more
        #  step if the .desc anchored there. blocks=6 is the assertion
        #  that it does not -- an average over the whole run appearing as
        #  a final point on an energy-vs-step plot is exactly the kind of
        #  plausible-looking wrong answer this suite exists to catch.
        #
        #  Two header/value pairs per block here, so "Total Energy" is
        #  present and must win over "Potential".
        name='gromacs-water-md',
        desc='gromacs.desc',
        fixture='gromacs/water_md.log',
        parse_args=('.', 'Dynamics', 'MD', 'OPLS', '0'),
        expect={
            #  TE is overwriting, so after the last block it holds the
            #  LAST STEP's Total Energy, -7.11568e+03. The AVERAGES
            #  summary's Total Energy is -7.36020e+03. Pinning this value
            #  is the assertion that the summary was excluded -- if the
            #  [NULL] suppressor ever stops swallowing it, TE lands on
            #  the run average and this fails loudly instead of quietly
            #  reporting a plausible wrong number.
            'TE][TEVEC][PRESSURE': dict(blocks=6, keys={
                'TE': {'values': '-7.11568e+03', 'units': 'kJoule/Mole'},
                'TEVEC': {'units': 'kJoule/Mole'},
                #  Same argument as TE: the LAST STEP's pressure, not the
                #  averages block's. Units must be one PressureConverter
                #  knows -- it throws on anything else.
                'PRESSURE': {'values': '-2.13868e+03', 'units': 'Bar'}}),
        },
    ),
    dict(
        #  Variable-cell relaxation of silicon, started from a cell
        #  compressed to 2.60 A so the optimiser has work to do. The only
        #  fixture exercising [LATTICEVEC], and the only one where the
        #  BOX CHANGES between steps.
        #
        #  GEOMTRACE and LATTICEVEC must have the SAME number of blocks:
        #  GTStepCmd assigns both by the same step index, so a cell
        #  vector shorter than the geometry trace means later frames draw
        #  the wrong box, and a longer one runs off the end. In vc-relax
        #  output the two blocks are adjacent, which also makes this the
        #  fixture that would catch [LATTICEVEC]'s End swallowing
        #  [GEOMTRACE]'s Begin.
        name='qe-si-vcrelax',
        desc='qe.desc',
        fixture='qe/si_vcrelax.pwout',
        parse_args=('.', 'GeomOpt', 'PW', 'PW', '0'),
        expect={
            'GEOMTRACE': dict(blocks=5, keys={'GEOMTRACE': {
                'size': '1 2 3', 'units': 'Angstrom'}}),
            'LATTICEVEC': dict(blocks=5, keys={'LATTICEVEC': {
                'size': '1 3 3', 'units': 'Angstrom'}}),
            #  Six stress blocks, one per vc-relax step. Both keys come
            #  from every one of them; STRESS and PRESSURE overwrite, so
            #  what survives is the converged cell's.
            'STRESS][PRESSURE': dict(blocks=6, keys={
                'STRESS': {'size': '9', 'units': 'kbar'},
                'PRESSURE': {'size': '1', 'units': 'kbar'}}),
        },
    ),
    dict(
        #  MOPAC with "VECTORS ALLVEC", the deck ai.mopac now generates
        #  when the theory dialog's "Print molecular orbitals" box is
        #  ticked.  MOPAC is the Molecular Orbital PACkage and had no MO
        #  extraction of any kind before this.
        #
        #  The Begin is ANCHORED to a line containing nothing but the
        #  word.  MOPAC echoes its keywords at the top of every output as
        #  "*  VECTORS    - FINAL EIGENVECTORS TO BE PRINTED"; an
        #  unanchored Begin matches THAT line instead, opening the block
        #  a hundred lines early and swallowing [GEOMTRACE], [PNTGRP] and
        #  [IP] whole.  Observed, not theorised.
        #
        #  ORBENG is pinned in Hartree: MOPAC prints eV, and MoPanel
        #  hard-codes its column label to "Energy Hartree", so a missing
        #  conversion would display numbers 27x too large with no error.
        name='mopac-ch4-mos',
        desc='mopac.desc',
        fixture='mopac/ch4_mos.out',
        parse_args=('.', 'Energy', 'SE', 'PM7', '0'),
        expect={
            'MO][MOBETA][ORBENG][ORBENGBETA][ORBSYM][ORBSYMBETA': dict(
                blocks=1, keys={
                    'ORBENG': {'size': '8', 'units': 'Hartree'},
                    'ORBSYM': {'size': '8'},
                    'MO': {'size': '8 8'}}),
            #  Sized over ALL orbitals, not just the filled ones --
            #  MoPanel indexes the occupancy vector over the energy
            #  vector's range, so a shorter one reads past its end.
            'ORBOCC][ORBOCCBETA': dict(blocks=1, keys={
                'ORBOCC': {'size': '8',
                           'values': '2.0000 2.0000 2.0000 2.0000 0.0000 '
                                     '0.0000 0.0000 0.0000',
                           'units': 'electrons'}}),
        },
    ),
    dict(
        #  The UHF counterpart, and the one that matters most here.
        #
        #  MOPAC prints "ALPHA EIGENVECTORS" and "BETA EIGENVECTORS" as
        #  two headings, but they arrive as ONE block: the entry's End is
        #  "NET ATOMIC CHARGES", which lies past both.  An earlier version
        #  of mopac.mo switched its output keys on seeing the BETA
        #  heading and silently dropped the entire alpha set -- pinning
        #  both ORBENG and ORBENGBETA here is what catches that.
        #
        #  The occupancies are the real assertion.  MOPAC states them
        #  ("NO. OF ALPHA ELECTRONS = 4", "BETA = 3") in a summary block
        #  that cannot be reached without starving [PNTGRP] and [IP], so
        #  mopac.orbocc reconstructs them from the electron populations
        #  plus the open-shell count ECCE passes as argv[4].  These values
        #  are the check that the reconstruction agrees with what MOPAC
        #  itself said.
        name='mopac-ch3-uhf-mos',
        desc='mopac.desc',
        fixture='mopac/ch3_uhf_mos.out',
        parse_args=('.', 'Energy', 'SE', 'UPM7', '1'),
        expect={
            'MO][MOBETA][ORBENG][ORBENGBETA][ORBSYM][ORBSYMBETA': dict(
                blocks=1, keys={
                    'ORBENG': {'size': '7', 'units': 'Hartree'},
                    'ORBENGBETA': {'size': '7', 'units': 'Hartree'},
                    'MO': {'size': '7 7'},
                    'MOBETA': {'size': '7 7'}}),
            'ORBOCC][ORBOCCBETA': dict(blocks=1, keys={
                'ORBOCC': {'size': '7',
                           'values': '1.0000 1.0000 1.0000 1.0000 0.0000 '
                                     '0.0000 0.0000'},
                'ORBOCCBETA': {'size': '7',
                               'values': '1.0000 1.0000 1.0000 0.0000 '
                                         '0.0000 0.0000 0.0000'}}),
        },
    ),
    dict(
        #  THE REGRESSION FIXTURE FOR THE VIBRATION END-ANCHOR BUG.
        #
        #  A real CH4 PM7 FORCE job from 2026-09-22 that killed
        #  eccejobmonitor on every poll:
        #
        #    FATAL ERROR: JobOutputGet: end of file encountered while
        #    reading parse type VIBIR
        #
        #  Same molecule and symmetry as ch4_force.out, and that is the
        #  point -- what differs is that this run produced no
        #  thermochemistry section, so "SYMMETRY NUMBER FOR POINT-GROUP"
        #  never appears. The withdrawn [VIBIR] entry used that string as
        #  its End, so its Begin matched, the End never came, and the
        #  monitor Died at EOF. See #103.
        #
        #  Kept as a case even though [VIBIR] is currently commented out,
        #  because the suite ALREADY fails a "runaway parse type" -- the
        #  machinery was there and only the fixture was missing. Anyone
        #  re-enabling that entry with a similarly optional End will be
        #  told immediately rather than after shipping it.
        name='mopac-ch4-force-nothermo',
        desc='mopac.desc',
        fixture='mopac/ch4_force_nothermo.out',
        parse_args=('.', 'Vibration', 'SE', 'PM7', '0'),
        expect={
            'TE][TEVEC][HFENERGY': dict(keys={'TE': {}}),
            'VIBFREQ][VIBSYM][VIB': dict(keys={
                'VIBFREQ': {'units': 'cm-1'}}),
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
    'gromacs.null': 'Deliberate no-op, and the ONLY way to exclude the '
                    'end-of-run AVERAGES summary: its "Energies (kJ/mol)" '
                    'marker is byte-identical to a real step\'s, so no '
                    'Begin pattern can tell them apart. This entry '
                    'consumes that section first. Parsed as a step it '
                    'would put a run-average on every energy-vs-step plot '
                    'and leave the scalar TE holding it. See gromacs.desc.',
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
    ('nwchem.desc', 'LATVECCPTRACE'):
        "Script=nwchem.latvec emits the property key LATTICEVEC; "
        "LATVECCPTRACE is this entry's section label, distinguishing the "
        "Car-Parrinello per-step cell from LATVECPROP/LATVECTRACE, which "
        "use the same script and the same emitted key for the "
        "task_energy and task_gradient tags.",
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
    ('nwchem.desc', 'SPINDIPOLE'):
        "Script=nwchem.spindipole emits DIPOLETENSOR and DIPOLEEIGVAL, "
        "which are the property names data/client/config/properties "
        "actually declares (SPINDIPOLE is not a property name at all); "
        "SPINDIPOLE is this entry's section label only.",
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

    #  The four below are NWChem MO-symmetry entries that cannot match any
    #  output NWChem produces, established by listing every variant the
    #  fixtures actually contain:
    #
    #    %begin%molecular orbital symmetries RHF
    #    %begin%molecular orbital symmetries UHF alpha
    #    %begin%molecular orbital symmetries UHF beta
    #    %begin%molecular orbital symmetries dft alpha
    #    %begin%molecular orbital symmetries dft beta
    #
    #  All five are covered by sibling entries that do fire. NWChem always
    #  qualifies the tag with the method, so the unqualified fallbacks can
    #  never match -- they are harmless legacy, not a gap.
    ('nwchem.desc', 'MOLAB6'):
        "Begin 'begin%molecular orbital symmetries alpha%' has no method "
        "qualifier. NWChem always writes one (UHF alpha / dft alpha), and "
        "both of those have their own entries, which fire.",
    ('nwchem.desc', 'MOLAB7'):
        "Begin 'begin%molecular orbital symmetries beta%' has no method "
        "qualifier. Same as MOLAB6: UHF beta and dft beta both have entries "
        "that fire.",
    ('nwchem.desc', 'MOLAB8'):
        "Begin 'begin%molecular orbital symmetries%' requires a literal '%' "
        "straight after 'symmetries'. Every real tag has a method qualifier "
        "there instead (' RHF%', ' UHF alpha%', ...), so this catch-all "
        "cannot match anything.",
    ('nwchem.desc', 'EZEROPT'):
        "Begin 'begin%zero point' is an ecce_print tag that NWChem 7.2.3 "
        "never writes. Checked by running a real SCF frequency job with "
        "ecce_print: the trace carries frequencies, normal modes and "
        "intensities but NO thermochemistry keys at all, and neither the "
        "trace nor the stdout contains the words 'zero point'. Was "
        "classified as merely uncovered until that was actually run. "
        "It may have worked against a PNNL-era NWChem; it cannot now.",
    ('nwchem.desc', 'ORBSYM'):
        "Begin contains the literal text 'NOT SUPPORTED', which no output "
        "will ever contain. Deliberately disabled in place rather than "
        "deleted; leave it that way unless orbital symmetries are wired up.",

    #  gaussian-16: an alternate SPELLING whose sibling already fires.
    ('gaussian-16.desc', 'ESCF2][ESCFVEC'):
        "Begin 'SCF DONE\\:' is an all-caps variant; Gaussian 16 writes "
        "'SCF Done:', which [ESCF1][ESCFVEC] above matches with the same "
        "script and the same keys. Harmless legacy alternate spelling.",
}


# ---------------------------------------------------------------------------
# Entries that are fine but have no fixture exercising them. Unlike
# KNOWN_DEAD these SHOULD fire once someone contributes the right output --
# recorded so the coverage report distinguishes "we never tested this" from
# "this can never work", which is the distinction that matters when a
# property turns out to be missing in the field.
UNCOVERED = {
    #  Still UNCOVERED rather than dead, but with a caveat worth having.
    #  An attempt to produce a fixture on 2026-09-22 -- a Gaussian-basis
    #  periodic SCF ("system crystal", LiH, STO-3G) against NWChem 7.2.3
    #  -- printed lattice parameters to stdout and emitted NO lattice
    #  tags into the ecce_print trace at all. That run also failed (exit
    #  3) before finishing, so it does not prove these are dead the way
    #  the EZEROPT run did; it does mean a fixture is harder to come by
    #  than "run something periodic", and that the only path OBSERVED to
    #  emit lattice vectors into the trace is Car-Parrinello, which
    #  LATVECCPTRACE already covers. Anyone picking this up should start
    #  by establishing whether the task_energy/task_gradient/task_hessian
    #  variants are reachable at all before building a fixture for them.
    ('nwchem.desc', 'LATVECNULL'):
        "Lattice vectors from a hessian/energy task. Needs a PERIODIC "
        "hessian or single-point fixture; the only periodic fixture we have "
        "is Car-Parrinello, which LATVECCPTRACE handles. See the note "
        "above: a periodic Gaussian-basis run emitted no lattice tags.",
    ('nwchem.desc', 'LATVECPROP'):
        "Lattice vectors from a plain energy task. Same missing fixture as "
        "LATVECNULL.",
    ('nwchem.desc', 'LATVECTRACE'):
        "Per-step lattice vectors from a gradient task, i.e. a periodic "
        "geometry optimisation. Same missing fixture.",


    #  gaussian-16: alternate output FORMS. Each has a sibling entry that
    #  fires on the form our fixtures actually contain, so these are only
    #  reachable with different Gaussian input options.
    ('gaussian-16.desc', 'ORBOCC2][ORBOCCBETA'):
        "Reads 'Molecular Orbital Coefficients', which Gaussian prints only "
        "under pop=full. [ORBOCC1] covers the default output.",
    ('gaussian-16.desc', 'NULL3'):
        "Guard entry for 'Making orbital integer symmetry', which our "
        "fixtures do not trigger.",
    ('gaussian-16.desc', 'GEOMTRACE'):
        "The 'Z-Matrix orientation:' variant, printed only for Z-matrix "
        "input. The 'Input/Standard orientation:' siblings fire.",
    ('gaussian-16.desc', 'DIPOLE1'):
        "The 'Dipole moment=' archive-section form; [DIPOLE2] covers the "
        "form our fixtures print.",
    ('gaussian-16.desc', 'POLARIZ2][HYPERPOL1'):
        "Polarizability and hyperpolarizability from the archive block; "
        "needs a polar= job.",

    #  gaussian-16: theories no fixture uses. These are not suspicious --
    #  each needs a job run at that level of theory.
    ('gaussian-16.desc', 'EMP4SDTQ][EMP4SDTQVEC'): "Needs an MP4(SDTQ) job.",
    ('gaussian-16.desc', 'EMP5][EMP5VEC'): "Needs an MP5 job.",
    ('gaussian-16.desc', 'ECISD][ECISDVEC'): "Needs a CISD job.",
    ('gaussian-16.desc', 'ECIS][ECISVEC'): "Needs a CIS job.",
    ('gaussian-16.desc', 'ECNDO1][ECNDOVEC'): "Needs an RHF CNDO job.",
    ('gaussian-16.desc', 'ECNDO2][ECNDOVEC'): "Needs a UHF CNDO job.",
    ('gaussian-16.desc', 'EINDO1][EINDOVEC'): "Needs an RHF INDO job.",
    ('gaussian-16.desc', 'EINDO2][EINDOVEC'): "Needs a UHF INDO job.",
    ('gaussian-16.desc', 'EMINDO31][EMINDO3VEC'): "Needs an RHF MINDO/3 job.",
    ('gaussian-16.desc', 'EMINDO32][EMINDO3VEC'): "Needs a UHF MINDO/3 job.",
    ('gaussian-16.desc', 'EMNDO1][EMNDOVEC'): "Needs an RHF MNDO job.",
    ('gaussian-16.desc', 'EMNDO2][EMNDOVEC'): "Needs a UHF MNDO job.",
    ('gaussian-16.desc', 'EAM11][EAM1VEC'): "Needs an RHF AM1 job.",
    ('gaussian-16.desc', 'EAM12][EAM1VEC'): "Needs a UHF AM1 job.",
    ('gaussian-16.desc', 'EPM31][EPM3VEC'): "Needs an RHF PM3 job.",
    ('gaussian-16.desc', 'EPM32][EPM3VEC'): "Needs a UHF PM3 job.",
}


# ---------------------------------------------------------------------------
# Cases where TEVEC legitimately has more steps than GEOMTRACE.
# Should stay empty: see the invariant check in run_tests.py. An entry here
# means a real code prints more per-cycle energies than geometries, and the
# GUI consequence (atoms collapsing to the origin) has to be handled some
# other way.
# ---------------------------------------------------------------------------
KNOWN_LONG_TEVEC = {}
