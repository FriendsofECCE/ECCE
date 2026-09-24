# tools/mopac — verifying the MOPAC Slater basis before rendering it

`verify_slater_basis.py <job>.mgf` reads a MOPAC GRAPHF file, rebuilds
the Slater basis from it, and numerically integrates each molecular
orbital.

```
$ ./verify_slater_basis.py water.mgf
atoms=3  basis functions=6  MOs=6  coeffs/MO=6
applied INVERSE_MATRIX (Lowdin S^-1/2) to the coefficients

  MO   <psi|psi>
    1     1.0000
    ...
6 of 6 MOs normalised
```

## Why this exists

3-D orbital rendering for MOPAC (#128) depends on getting four things
right at once, and **every one of them produces a picture that looks
entirely plausible when wrong**:

- the order of the basis functions within each atom,
- the `(a,b,c)` angular exponents, especially for the d set,
- the `r^d` principal quantum numbers, which carry MOPAC-specific
  adjustments per element,
- the Löwdin back-transformation of the coefficients.

`<psi|psi>` tests all four simultaneously. MOPAC's printed coefficients
are in an **orthonormalised** basis — `sum(c²)` is exactly 1 for every
orbital — so combining them directly with real, non-orthogonal Slater
functions gives a value that is *not* 1. Only when the `INVERSE_MATRIX`
block (the symmetric inverse square root of the overlap) is applied, and
the basis is built in the right order with the right exponents, does
every orbital integrate to 1.

That makes this a complete check of the specification with no 3-D
viewer, no ECCE, and nothing to eyeball.

## Results so far

| system | basis | result |
|---|---|---|
| water, PM7 | 6 functions, s/p only | 6 of 6 normalised |
| SF6, PM7 | 33 functions, **d on sulphur** | 16 of 16 normalised |

SF6 is the one that matters: it exercises the d ordering
(dx2-y2, dxz, dz2, dyz, dxy — not the conventional order) and `NPQd`.

## Where the specification came from

Jmol's `MopacGraphfReader` / `MopacSlaterReader` (**LGPL 2.1**). The
algorithm and physical constants are recorded in #128 and reimplemented
here; no Jmol code is copied into this tree.

Needs `numpy`, and takes a few seconds — it is a verification tool, not
a regression test.
