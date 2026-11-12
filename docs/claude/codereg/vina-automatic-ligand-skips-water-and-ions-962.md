---
type: pitfall
title: "Vina's automatic ligand was a lone crystal water; the choice now skips water and ions and is reported (#962)"
area: codereg
paths:
  - scripts/parsers/docking.pl
  - scripts/parsers/ai.vina
  - tests/dock/vina_autoligand_test.py
issues: [962]
---

`dockSplit` took the smallest connected component as the ligand. In a crystal
structure waters are lone O atoms (no hydrogens), so the "ligand" was one
oxygen (`LIGAND_ATOMS=4459`).

- Water (residue HOH/WAT/..., or a component of O plus H only) and any
  monatomic ion are skipped, but only when at least two other molecules
  remain, so a host-guest pair with a bare-ion guest still works as before.
- If a polymer is present (more than half the atoms in standard amino-acid or
  nucleotide residues) the ligand is the largest molecule that is not the
  polymer; otherwise the smallest remaining one, as before.
- Residue names come from the `.frag`'s `pdb_list:` block (fixed columns:
  residue name 7-10, number 11-16, chain 25), read in `ai.vina`'s
  `readChemSys`. `ai.xtb` does not pass it, so it gets only the size/element
  rules.
- The choice and the warnings (waters or identical chains left in the
  receptor) go to STDERR as `ai.vina warning [LIGAND_ATOMS]: ...`, the
  channel the Calculation Editor shows under Verify, anchored on the
  `#| LIGAND_ATOMS=` line. They only warn; nothing removes the waters.
