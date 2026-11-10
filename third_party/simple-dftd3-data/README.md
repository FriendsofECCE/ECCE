# simple-dftd3-data

Reference data of Grimme's DFT-D3 dispersion correction, used by ECCE-QM's
D3(BJ) (`src/qm/d3.cpp`, issue #453). The files are byte-identical copies of
upstream; nothing in them was edited. ECCE does not use any of the Fortran
code: at build time `src/qm/tools/d3_data_gen.cpp` reads the array literals
out of these files and writes them as a C++ table (`d3_data.inc` in the build
tree). That converter and the D3 code are ECCE's own, under ECL-2.0
(see the top-level `LICENSE`).

## From simple-dftd3 (LGPL-3.0-or-later; `COPYING`, `COPYING.LESSER`)

https://github.com/dftd3/simple-dftd3, tag v1.7.0,
commit `84cb7cddd8d6669c4ecd59b06d53e0d373656239`.

* `reference.f90` (`src/dftd3/reference.f90`): the number of reference
  systems per element, their coordination numbers and the reference C6
  coefficients for every element pair (elements 1 to 103).
* `r4r2.f90` (`src/dftd3/data/r4r2.f90`): the atomic <r4>/<r2> expectation
  values that give C8 from C6.

## From mctc-lib (Apache-2.0; `mctc-lib/LICENSE`)

https://github.com/grimme-lab/mctc-lib,
commit `25a345f5cf44e39965eac783ae3290c295171dd4`. simple-dftd3 takes its
covalent radii from this library.

* `mctc-lib/covrad.f90` (`src/mctc/data/covrad.f90`): the Pyykkö–Atsumi
  covalent radii; D3 uses 4/3 of them for the coordination number.

The damping parameters (s6, s8, a1, a2) are not a data file: the few ECCE-QM
needs are written in `src/qm/d3.cpp` with their source cited there.

To update: copy the three files again from a newer release, record its tag
and commit here, rebuild (the converter checks every array length) and run
`ctest -R qm_d3`.
