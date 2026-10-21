---
type: pitfall
title: "GROMACS Prepare: MERGE=1 gives one posre.itp (no per-chain restraints); unmerged chains all restrain under the same POSRES; editconf -center needs -c"
area: codereg
paths: [scripts/gensub, scripts/parsers/md.gmxprep, tests/gromacs_md/prepare_options_test.py]
issues: [410]
---
- `pdb2gmx -merge all` (ECCE's default, `MERGE=1`) writes one molecule and
  one `posre.itp`, so a single chain cannot be restrained; untick "Merge
  chains" for a per-chain topology (`topol_Protein_chain_A.itp`, ...).
- Unmerged, pdb2gmx puts every chain's `posre_*.itp` under the same
  `#ifdef POSRES`. gensub appends a `#ifdef POSRES_<id>` include to each chain
  itp, so the run's "Restrain positions" name field (several names are
  accepted: `POSRES_A POSRES_B`) picks the chains. Chain groups for index
  files are `mol:Protein_chain_A` in the group definitions (gromacs.mkndx).
- `BoxCenter` (recipe `CENTER`, "x y z" nm) becomes `editconf -c -center x y z`;
  empty keeps plain `-c`. The box is sized round the solute first, so an
  off-centre solute can reach outside it: pick a box with room (distance to
  edge or a larger system).
