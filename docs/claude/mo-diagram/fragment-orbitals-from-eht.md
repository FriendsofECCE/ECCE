---
type: map
title: "Fragment orbitals from extended Hueckel, and the MO composition in them (#301, route B stage 1)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/FragmentOrbitals.H", "src/tdat/chemistry/FragmentOrbitals.C", "data/client/config/EHTParameters", "tools/modiagram/fragorb.C", "tests/fragorb"]
issues: [301, 252, 162]
---
`FragmentOrbitals` (tdat, no wx) splits a complex into the metal, each ligand
and the ligand shell, gives each fragment extended Hueckel orbitals (FOs) at
the parent geometry and projects every parent MO on them. The MO energies a
diagram draws stay the parent's; FO energies are qualitative. No GUI yet
(stage 2); `build-cmake/fragorb CALC` prints the table.

- **Projection**: the FO set is symmetrically orthogonalised (S_FF^-1/2) and
  w_k = |<FO~_k|psi>|^2 with psi normalised in the parent basis, so
  capture = sum w <= 1. A valence EHT set cannot describe 1s/2s/2p/3s/3p
  cores: capture there is ~0 and the class is "core" (below 0.5 capture).
  metal = >= 0.8 of the captured part on the metal FOs, metal-centred = 0.6
  to 0.8, ligand = >= 0.8 on the ligands, else mixed (Cr(CO)6 t2g: 72%
  metal, metal-centred).
- **Mixed overlaps** <FO|parent AO> use `EspField::overlapOf`, the same
  routine as the parent S (one integral code). Checked against Multiwfn's
  CDA matrices to 1e-5 (`tests/fragorb`).
- **FO set choice changes the numbers**: Cr(CO)6 t2g is 80% Cr with occupied
  fragment orbitals only, 68-73% with all of them (the CO pi* virtuals can
  take weight). EHT-derived and ORCA-derived FOs agree to a few percent.
  Degenerate FOs (and the same-formula ligands of a per-ligand set) are
  reported as one level because their split is arbitrary.
- **Parent must be non-orthogonal**: MOPAC-style coefficients are in an
  orthonormalised basis; `fragorb` declines them (not implemented).
- **Ligand charges** are the ionic convention (odd electron count -> -1);
  the metal takes the rest. No metal: `autoFragments` returns false
  ("not applicable") and the existing constructions apply unchanged.
- `Huckel` (H-Ne, the diagram's EHT path) is now a thin wrapper over this
  engine and reads `EHTParameters`; `HuckelParameters` and its STO-3G
  code are gone. Against the old path (10 molecules H2O to B2H6): orbital
  order, degeneracy and symmetry unchanged, energies move by 0-2 eV
  (virtuals 4-8 eV), and CO's 3sigma/2sigma polarisation changes (the
  Alvarez C/O parameters put 3sigma 54% on O, the old ones 67% on C):
  the Hoffmann set, not a bug. tests/modiagram's fifteen dumps are
  byte-identical (none reaches Huckel).
- ORCA Cr atom for Gaussian FOs: ROHF sextet/septet converges; a fragment
  calc must be in the parent's basis and atom order, and Multiwfn needs the
  fragments to be consecutive blocks of the parent's atoms
  (`tests/fragorb/multiwfn_ref.py`).
