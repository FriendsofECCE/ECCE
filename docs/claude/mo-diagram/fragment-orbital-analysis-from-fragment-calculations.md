---
type: map
title: "Fragment-orbital analysis from real fragment calculations: ecce-fragorb (#201 step 1)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/FragmentAnalysis.H", "src/tdat/resultsh5/FragmentAnalysis.C", "src/tdat/resultsh5/ecce_fragorb.C", "tests/fragorb_h5"]
issues: [201, 171]
---
Library + CLI on results.h5 (#690): the complex and one results.h5 per
fragment, same geometry and basis. Not the extended-Hueckel `FragmentOrbitals`
(#301, `tests/fragorb`); ctest is `fragorb_analysis` (`fragorb` is #301's).
No GUI yet; the diagram and recipe come next.

- **Expansion, not projection.** b = C_FO^T S c is only the coefficient vector
  when all FOs are orthonormal. FOs of different fragments overlap, so
  a = S_FO^-1 b (= Multiwfn's coFO), Mulliken share a_i b_i, SCPA a_i^2/sum a^2.
  remainder = 1 - a.b (about 0 when the fragment bases cover the complex's).
  A Mulliken share can exceed 100 % (Cr 4s, 118 %): negative shares elsewhere
  compensate, as in Multiwfn.
- **Matching:** atoms by element and position (1e-3 Angstrom), fragment basis
  per atom must equal the complex's (shells, exponents, contraction ratios),
  else refused; shared atoms, uncovered atoms (`--allow-uncovered`) refused.
- **Spin:** a restricted fragment (including ROHF) serves both spins of an
  unrestricted complex; an unrestricted fragment of a restricted complex is
  refused. Occupations are printed as stored (restricted 0..2, UHF 0..1).
- **Degenerate FOs** (`--levels`): shares of one fragment's FOs within 1e-4 Eh
  are summed; individual shares inside a degenerate set are arbitrary
  (an RHF singlet Cr atom has 3 of 5 d orbitals doubly occupied at an
  arbitrary orientation: use the ROHF septet, `tests/fragorb_h5/fixtures/cr7`,
  for a meaningful d set).
- **OPDOS:** OP_i(A,B) = 2 sum c c S (complex S, A/B atom sets), PDOS from the
  Mulliken gross population, closed-shell MOs weighted 2; sum n_i OP_i equals
  2 sum P S (tested).
- **Oracle:** Multiwfn 2026.9.20 CDA (main function 16, option 2) from the
  same orca_2mkl molden files; `tests/fragorb_h5/multiwfn_reference.py`
  regenerates `reference/*.tsv` (needs `chmod +x` on the noGUI binary and a
  settings.ini with compthresCDA=0.01). Agreement 0.006 pp (SCPA), 0.05 pp
  (Mulliken, a 0.05 % share of a high Cr virtual).
- **Field check** (`fragorb::fieldCheck`, JSON `field_check`, box warning
  `FragOrbBoxResult::warning`): for a one-atom fragment with a d set (>= 5 FOs
  of >= 50 % d population) every OCCUPIED complex MO with a Mulliken d share
  >= 0.30 must have >= 0.80 of that share on the atom's occupied d FOs
  (occupation > 0.5). Else "field-split guess failed" (the recipe's field step,
  recipes-are-text-files-applied-by-workflowsteps-710, picks the occupied d set).
  Thresholds are arguments (dMin, occMin), warn only; not applicable and no
  warning when no occupied MO has 30 % d.
