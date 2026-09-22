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
                     Theory="RHF", RunType="GeoVib")),
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
                     RunType="Geometry")),
    dict(name="expt-orca-h2o-optfreq", script="ORCA.expt",
         output="orca/h2o_optfreq.out",
         expect=dict(natoms=3, Charge="0", RunType="GeoVib",
                     Theory="RHF", Category="SCF")),
    dict(name="expt-orca-oh-uhf", script="ORCA.expt",
         output="orca/oh_uhf.out",
         expect=dict(natoms=2, Theory="UHF", RunType="Energy",
                     **{"ChemSys.Multiplicity": "2"})),
    dict(name="expt-orca-h2o-nmr", script="ORCA.expt",
         output="orca/h2o_nmr_chelpg.out",
         expect=dict(natoms=3, RunType="Magnetic")),
]

# ---------------------------------------------------------------------------
# NOTES -- observed behaviours that are arguable, not asserted either way.
# Recorded here so a golden-file diff can be read with context.
# ---------------------------------------------------------------------------
NOTES = """
* The Gaussian importers take the FIRST "Standard orientation" block in the
  file, not the last: getMolecule() does `last` on the first match and is
  called near the top of the file.  So importing a completed geometry
  optimisation reconstructs the STARTING structure, not the optimised one.
  Whether that is wrong depends on what the .frag is meant to be -- these
  scripts reconstruct the calculation's SETUP (.frag/.basis/.param), and the
  optimised geometry arguably belongs to the results side (GEOMTRACE), not
  the setup.  It matters a great deal for issue #94 (dummy submission),
  where the imported file is the only record of the job that exists, so
  settle it before building on this path.
  ORCA.expt reads its geometry from the input echo, so it has the same
  property for the same reason.

* NWChem has no cases here: NWChem.expt parses real NWChem stdout, and the
  nwchem fixtures in this tree are `.eprint` files -- the separate
  machine-tagged trace NWChem writes via its own `ecce_print` directive,
  which is what the LIVE monitor consumes.  Adding NWChem coverage needs a
  captured stdout file, not a reuse of what is already here.

* GAMESS-UK.expt, Gaussian-03/09/94/98.expt have no cases either, purely for
  want of fixture output.  Gaussian-03/09/98 share Gaussian-16's lineage
  closely enough that a fixture for any of them is likely to be cheap.
"""
