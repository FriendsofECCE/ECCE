---
type: map
title: PDBQT preparation (receptor and ligand) for docking
area: codereg
paths:
  - include/dock/DockMol.H
  - include/dock/DockPrep.H
  - src/dock/DockMol.C
  - src/dock/DockPrep.C
  - src/dock/DockIO.C
  - src/apps/pdbqt/ecce-pdbqt.C
  - tests/dock/meeko_compare.py
  - tests/dock/redock_1iep.py
issues: [455]
---

ECCE writes AutoDock PDBQT files itself; there is no Open Babel and no Meeko
in the product. The library (`eccedock`) is wx-free and the command
`ecce-pdbqt` is the only caller the job script uses, so the editor, the deck
and the command cannot disagree about a structure.

**Atom types.** The rules are AutoDock 4's, applied in order with later rules
overriding earlier ones: H; HD for a hydrogen on N/O/F/P/S; C for aliphatic
and A for aromatic carbon; NA for nitrogen and OA for oxygen; SA for
two-connection sulfur and S otherwise; then N (not an acceptor) for a neutral
three-connection, valence-three nitrogen next to an aromatic atom, next to an
sp2 carbon, in a triazene, or for any positively charged nitrogen.

**Traps found while matching the reference preparation:**

- *Aromaticity has to be tried ring by ring before the fused system.* Taking
  the union first makes diazepam's benzo ring aliphatic, because it is fused
  to a saturated seven-ring. Taking only single rings makes indole and purine
  aliphatic, because neither five-ring reaches 4n+2 on its own. Both passes
  are needed.
- *A ring atom with a double bond to an exocyclic O, N or S kills its ring.*
  Without that, caffeine's pyrimidinedione, warfarin's lactone and
  sildenafil's pyrimidinone all come out aromatic and their carbonyl carbons
  are typed A instead of C.
- *The amide test must skip aromatic atoms.* An anilino-pyrimidine (imatinib)
  and an N-acyl indole (indomethacin) both match "N-C=N/O" on Kekule bond
  orders, and the bond would wrongly be frozen. The reference matches on
  aromatic bonds, which never equal a double bond, so the aromatic case has
  to be excluded explicitly.
- *A PDB ligand has no bond orders.* Bonds perceived from distance are all
  single, so every amide would rotate; short C-O, C-N, C-C and C-S distances
  are promoted to double bonds, and ring aromaticity falls back to the
  planarity test AutoDock has always used on receptors.
- *Do not complete the valence of an aromatic nitrogen.* A pyridine-type N
  read from a crystal structure looks one bond short and picks up a spurious
  H, which types it HD and turns an acceptor into a donor. Only a five-ring
  two-connection N gets a hydrogen.
- Meeko may open a macrocycle with G0/CG0 pseudo-atoms so the ring can flex.
  That is a Meeko feature, not AutoDock typing; ECCE keeps rings rigid and
  the comparison test normalises those types away.

**Checks.** `tests/dock/meeko_compare.py` compares atom types and the set of
torsions against Meeko for eleven drug-like ligands (all eleven agree
exactly); it skips when Meeko is absent and Meeko is never shipped.
`tests/dock/redock_1iep.py` redocks imatinib into 1IEP from an entirely
ECCE-prepared receptor and ligand and requires the top pose within 2 A of the
crystal ligand; it skips without the PDB entry or without Vina.
