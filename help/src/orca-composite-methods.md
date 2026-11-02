# ORCA composite methods (-3c)

The ORCA Theory Details dialog offers the composite methods of ORCA 6 in the
**Functional** list for DFT calculations: **r2SCAN-3c**, **B97-3c**,
**PBEh-3c**, **wB97X-3c** and **HF-3c**.

Each composite is defined together with its own basis set, a geometric
counterpoise correction and a dispersion correction. When one is chosen:

- the dialog shows "the composite carries its own basis" and the
  **Dispersion** choice is greyed out;
- the input file has only the composite keyword on the `!` line, for example
  `! RKS r2SCAN-3c Opt`. No basis-set keyword, no `%basis` block and no D3 or
  D4 keyword is written, so the basis set picked in the Basis Set tool is not
  used.

Choose an ordinary functional if you want to pick the basis set or the
dispersion correction yourself. HF-3c is a Hartree-Fock method and is written
as `HF-3c` without the `RKS` keyword.
