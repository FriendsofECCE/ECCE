---
type: pitfall
title: "ClayFF for GROMACS (third_party/clayff): unit conversions, the cao/cah R0 misprint, and where atomipy's clayff() disagrees with Table 1"
area: codereg
paths: [third_party/clayff/clayff.ff, scripts/minff_topology.py, tests/clayff/run_tests.py, third_party/python/atomipy/forcefield.py, third_party/python/atomipy/charge.py]
issues: [930]
---
- **Source.** `clayff.ff` is transcribed from Cygan, Liang and Kalinichev, J.
  Phys. Chem. B 2004, 108, 1255 (Table 1 nonbonded, Table 2 bonded) and, for the
  optional `-DCLAYFF_MOH`, Pouvreau et al., J. Phys. Chem. C 2017, 121, 14757.
  Numbers and citations only; the PDFs are licensed and never go into the repo.
  `tests/clayff/run_tests.py` re-derives every sigma/epsilon/charge from the
  printed table and checks sigma of mgh, ao, oh, o* against Pouvreau's Table 2.
- **Conversions.** Paper LJ is `D0[(R0/r)^12 - 2(R0/r)^6]`: sigma = R0/2^(1/6)
  (Å to nm /10), epsilon = D0 x 4.184. R0 arithmetic, D0 geometric: comb-rule 2.
  The paper's harmonic terms are `k(x-x0)^2` without the 1/2, so GROMACS
  `kb = ktheta = 2k` (O-H 463700 kJ/mol/nm2, H-O-H 383.0, generic M-O-H 251.04,
  Pouvreau Al-O-H 125.52, Mg-O-H 50.21 kJ/mol/rad2). Morse O-H of Pouvreau:
  GROMACS function 3, D 553.33 kJ/mol, beta 21.35 /nm, b0 0.09572 nm.
- **M-O-H is off by default.** Cygan 2004 calls the bend optional and does not
  use it for the bulk phases; Pouvreau's ClayFF-orig is the same without it.
  `min.itp` lists the angles only inside `#ifdef CLAYFF_MOH` (Al, Mg only,
  Morse O-H) and `#ifdef CLAYFF_MOH_2004` (all metals, 30 kcal/mol/rad2,
  109.47 deg); `minff_topology.py --clayff-bend` writes the `#define`. atomipy's
  own writer emits O-M-O angles (MINFF's) and M-O-H at 110 deg / 125.52 for
  every metal: `clayff_itp` drops them, ClayFF has no mineral angle terms.
- **cao / cah.** Table 1 prints R0 6.2484 for cao and 6.2428 for cah, but
  every other octahedral/hydroxide pair shares one value (mgo/mgh, st/at).
  Both are kept as printed; min.ff's own CLAYFF_2004 block uses 6.2484 for
  both (sigma 0.5567), which fits a misprint but proves nothing. Do not
  "correct" it without the paper's source data.
- **atomipy clayff() versus Table 1** (checked on the unit cells of the
  minff repository):
  - oxygen charges are computed from the valence sum, not taken from Table 1.
    For the clay phases the result equals Table 1 to rounding; oxides and
    oxyhydroxides (periclase, lime, forsterite, goethite, diaspore, akdalaite,
    maghemite, hectorite-F) get other values, and Table 1 charges would leave
    them charged (`clayff_charges` still writes Table 1 and warns about a net
    charge). Montmorillonite ends at +0.0096 e because Table 1's rounded
    oxygen charges are not exactly neutral.
  - Lio: atomipy 0.86, Table 1 0.525.
  - water: atomipy 0.4238/-0.8476 (SPC/E), Table 1 0.41/-0.82 (SPC).
  - Ca of Ca(OH)2 is typed Cao; Table 1 has cah for hydroxide calcium
    (`clayff_retype` fixes it).
  - types without a Table 1 entry: Ale, Oalh, Oalhh, Oalsi, Ome, Omhh, Oli,
    Osih, Osi, Oal, Ohli, Obt, Fe3t, Ti, F. `clayff.ff` has no parameters for
    them, so the build lists them under `types_missing_from_ff` and grompp
    would stop on them.
  - min.ff's CLAYFF_2004 block has ohs -1.0809 (paper -1.0808).
- **Sigma of Alo.** Pouvreau Table 2 prints 4.2718 Å for ao, Table 1 of Cygan
  gives R0 4.7943 = sigma 4.2712; the tests accept the 6e-4 Å difference.
- **NPT check (`tests/clayff/run_tests.py --long`).** Use a damped anisotropic
  barostat (Berendsen, tau-p 5 ps). The strained experimental cell starts at
  ~1e4 bar and the pressure fluctuates by 1e4 bar in a cell of 2000 atoms; with
  Parrinello-Rahman the soft c axis of brucite, kaolinite and pyrophyllite
  flew apart within ~10 ps (50-400 nm after 250 ps) and, with the layers beyond
  the 1 nm cut-off, never came back. Berendsen reproduces the cells of Cygan
  2004 and Pouvreau 2017 (ClayFF-orig) to 1 % in a, b, c; the triclinic angles
  of soft shear modes (pyrophyllite alpha/beta, gibbsite beta) and the
  portlandite angles of the paper (82/98 deg, a distortion of that run) are
  not reproduced and are reported, not tuned away.
