"""
Cases for the post-hoc importers (``<Code>.expt``).  See expt.py.

Reuses the same fixture output files as the live-monitor cases, so this
costs no new checked-in data.

`expect` holds a few shape assertions -- the things that are unambiguously
wrong if absent (no atoms, no charge, a runtype the job plainly is not).
Everything else is left to the golden file, on purpose: these scripts have
behaviours that are arguable rather than wrong, and a golden file records
what happens without pretending to judge it.  The arguable ones are written
down under NOTES below so the next person does not have to rediscover them.
"""

CASES = [
    dict(name="expt-g16-h2o-optfreq", script="Gaussian-16.expt",
         output="gaussian-16/h2o_optfreq.log",
         expect=dict(natoms=3, symbols=["O", "H", "H"], charge="0",
                     Theory="RHF", RunType="GeoVib"),
         # The OPTIMISED geometry, i.e. the last "Standard orientation" in
         # the file -- not the input structure that the first one holds.
         # Opening a finished job should show what it converged to.
         geometry=[("O", 0.0, 0.127142, 0.0),
                   ("H", 0.757982, -0.508615, 0.0),
                   ("H", -0.757982, -0.508525, 0.0)]),
    dict(name="expt-g16-co-freq", script="Gaussian-16.expt",
         output="gaussian-16/co_freq.log",
         expect=dict(natoms=2, symbols=["C", "O"], charge="0")),
    dict(name="expt-g16-oh-uhf", script="Gaussian-16.expt",
         output="gaussian-16/oh_uhf.log",
         expect=dict(natoms=2, charge="0")),
    dict(name="expt-g16-h2o-nmr", script="Gaussian-16.expt",
         output="gaussian-16/h2o_nmr.log",
         expect=dict(natoms=3, charge="0")),
    dict(name="expt-g16-h2o-ccsdt", script="Gaussian-16.expt",
         output="gaussian-16/h2o_ccsdt.log",
         expect=dict(natoms=3, charge="0")),

    dict(name="expt-orca-h2o-opt", script="ORCA.expt",
         output="orca/h2o_opt.out",
         expect=dict(natoms=3, symbols=["O", "H", "H"], Charge="0",
                     RunType="Geometry"),
         # Likewise the optimised geometry: ORCA.expt already took the last
         # "CARTESIAN COORDINATES (ANGSTROEM)" block.
         geometry=[("O", 0.0, 0.0, 0.123868),
                   ("H", 0.0, 0.75808, -0.511934),
                   ("H", 0.0, -0.75808, -0.511934)]),
    dict(name="expt-orca-h2o-optfreq", script="ORCA.expt",
         output="orca/h2o_optfreq.out",
         expect=dict(natoms=3, Charge="0", RunType="GeoVib",
                     Theory="RHF", Category="SCF")),
    dict(name="expt-orca-oh-uhf", script="ORCA.expt",
         output="orca/oh_uhf.out",
         expect=dict(natoms=2, Theory="UHF", RunType="Energy",
                     **{"ChemSys.Multiplicity": "2"})),
    dict(name="expt-nwchem-h2o-opt", script="NWChem.expt",
         output="nwchem/h2o_opt_stdout.out",
         expect=dict(natoms=3, symbols=["O", "H", "H"], charge="0",
                     Category="SCF", Theory="RHF", RunType="Geometry"),
         # The CONVERGED geometry: the last "Output coordinates in
         # angstroms" table, not the echoed input deck's starting guess.
         geometry=[("O", -0.15019108, -0.00009839, 0.0),
                   ("H", 0.48199253, -0.76124639, 0.0),
                   ("H", 0.48925752, 0.75490937, 0.0)]),

    dict(name="expt-orca-h2o-nmr", script="ORCA.expt",
         output="orca/h2o_nmr_chelpg.out",
         expect=dict(natoms=3, RunType="Magnetic")),
]

# ---------------------------------------------------------------------------
# NOTES -- observed behaviours that are arguable, not asserted either way.
# Recorded here so a golden-file diff can be read with context.
# ---------------------------------------------------------------------------
NOTES = """
* FIXED: the Gaussian importers used to take the FIRST "Standard
  orientation" block in the file, so importing a completed geometry
  optimisation reconstructed the STARTING structure.  A user opening a
  finished job expects the geometry it converged to, so getMolecule() now
  keeps scanning and the last block wins.  It restores the file position to
  just after the first block before returning, because getGeneralBasis() and
  friends read on from wherever it left the handle and would otherwise be
  silently starved.  Same change in all five Gaussian importers, and in all
  three orientation loops (Standard, Input, Z-Matrix) since a NoSymm job
  prints only "Input orientation".
  The `geometry` assertions on the optimisation cases pin this down.

  (An earlier note here claimed ORCA.expt had the same problem.  It does
  not -- it already used the LAST "CARTESIAN COORDINATES (ANGSTROEM)"
  block, and its case now asserts that too.)

* NWChem.expt could not read an externally run job AT ALL until now, which
  is worth recording because the importer exists precisely for externally
  run jobs (#44, #94). Its parse loop skipped every line until it saw
  "%begin%input file" -- ECCE's OWN marker, written by the ecce_print
  directive that ai.nwchem puts in the deck. Given a plain NWChem output
  file it read to EOF and wrote an empty .frag: no title, no charge, no
  atoms, exit status 0. It now also accepts NWChem's own "echo of input
  deck" banner, and defaults a missing charge to neutral.

  The geometry gap is now closed too: NWChem's native "Output coordinates
  in angstroms" tables are read in their own pass over the file, and the
  last one wins, so a finished optimisation imports the structure it
  converged to. That pass is deliberately separate from parseInputFile's
  loop, which stops as soon as it has what it came for -- on a real
  optimisation that was block 2 of 6.

  One gap remains, tracked rather than fixed: no .gbs at all for a library
  basis ("* library STO-3G"). The basis branch handles only an explicit
  primitive specification -- its own comment says so -- so a named library
  reference produces nothing.

* GAMESS-UK.expt, Gaussian-03/09/94/98.expt have no cases either, purely for
  want of fixture output.  Gaussian-03/09/98 share Gaussian-16's lineage
  closely enough that a fixture for any of them is likely to be cheap.
"""
