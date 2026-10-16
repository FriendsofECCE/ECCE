---
type: pitfall
title: "GROMACS Prepare: gmx goes by file extension (.gmxpdb, .gmxtop are not read), every GROMOS force field draws a grompp warning, and the PDB must name its histidines"
area: codereg
paths: [scripts/gensub, scripts/parsers/md.gmxprep, scripts/parsers/gromacs.launchpp, tests/gromacs_md/run_tests.py]
issues: [930]
---
Found building the Prepare job (`gromacs.gmxprep`, steps pdb2gmx, editconf,
solvate, genion).

- **Extensions.** `gmx` refuses `-f x.gmxpdb` and `-p x.gmxtop` ("does not
  have a recognizable extension"). The job script copies the staged structure
  to `prep_in.pdb`, `gromacs.launchpp` links it as `in.pdb`, and the topology
  written as `<system>.gmxtop` must be staged as `topol.top` by whatever
  uses it (Launch already does for MD tasks). Anything else that hands these
  files to gmx directly fails the same way.
- **GROMOS.** `gromos*` force fields make grompp print one WARNING (twin-range
  parametrisation), so the neutralising grompp in the Prepare job needs
  `-maxwarn 1` for them and `0` otherwise; a net charge is only a NOTE in
  2025.2. The MD tasks' grompp uses `-maxwarn 0` and will refuse a GROMOS
  topology until that is decided.
- **Histidines.** `pdb2gmx -his` is interactive and there is no
  name-based option, so Prepare leaves it to pdb2gmx; a PDB that already says
  HID/HIE/HIP (the force field's own spelling) is honoured.
- **A lone residue** (one amino acid, e.g. glycine) fails with "no residue
  type for 'GLY' as a standalone residue": Prepare is for chains, not
  single amino acids.
- **Fatal error text.** The reason for a failed gmx tool is the paragraph
  after `Fatal error:`; for grompp the cause is the `ERROR n [file ...]`
  paragraph above its own "There was 1 error" box
  (`JobFailureReason::matchGromacsError`).
