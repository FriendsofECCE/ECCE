---
type: pitfall
title: "QE final-density guess: retain two .save members as a flat archive"
area: codereg
paths: ["data/client/cap/QuantumESPRESSO.edml", "scripts/parsers/ai.qe", "scripts/gensub", "src/dsm/edsiimpl/OrbitalGuess.C", "src/apps/calced/CalcEd.C", "tests/launch/qe_guess_test.py"]
issues: [407]
---
QE's restart is a directory under the launch-resolved `outdir`, with a name
that follows `prefix`. The data server stores files, so `gensub` packs only
`charge-density.dat` and `data-file-schema.xml` into `qe.qesave`; the edml's
`RetainOutputFile` sends and stores it. Wavefunctions can be much larger and
need not exist for a density guess.

The duplicate's distinct auxiliary input is `guess.qesavein` (both MIME
tables are required). The existing binary-safe Launch transfer ships it;
`gensub` reads `prefix` and `outdir` from the postprocessed deck on the run
machine and restores the two members there before pw.x. Extract each fixed
member to stdout into a fixed destination, rather than extracting archive
paths. This also handles a duplicate with a different prefix and a remote
scratch directory without installing another ECCE script on the run host.
The compute host needs `tar`.

The deck uses `startingpot='file'`, with fresh `atomic+random` wavefunctions,
not `restart_mode='restart'` (which resumes the same interrupted job).
The editor records atoms, positions, cell vectors and cutoff controls at
attachment and warns when they differ. For a relaxation, take positions
from the final `GEOMTRACE` and the cell from the final `LATTICEVEC` when
present; the stored input fragment can still contain the initial structure.
Unselected charge-density cutoff
values are ignored; switching automatic/explicit cutoff modes warns
conservatively. Keep the same pseudopotentials and spin settings.

`tests/launch/qe_guess_test.py` exercises local and central storage, archive
members and byte contents, removal of source scratch data, a changed prefix,
compatibility warnings, fewer SCF iterations and the same energy in Ry.
