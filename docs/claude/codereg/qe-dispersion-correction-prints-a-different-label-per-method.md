---
type: map
title: "QE dispersion (vdw_corr): one label per method in the output, all read into EDISP"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["scripts/parsers/qe.desc", "scripts/parsers/qe.disp", "scripts/parsers/ai.qe", "data/client/config/properties", "tests/qe_features/run_tests.py"]
issues: [329]
---
pw.x 6.7 accepted `vdw_corr` = `grimme-d2`, `grimme-d3` (with `dftd3_version`
3 = zero damping, 4 = Becke-Johnson), `ts` (and its long name
`tkatchenko-scheffler`) and `xdm`. After the converged `!` energy it prints
the term under a different label for each: `Dispersion Correction`
(D2), `DFT-D3 Dispersion`, `Dispersion T-S Correction`,
`Dispersion XDM Correction`. `[EDISP]` in `qe.desc` has all four in one
`Begin`; a new method with a new label needs adding there, or the energy is
silently not extracted. EDISP is listed under "Components" of the Energies
panel: it is already in the total. For D2/D3 the total differs from the
uncorrected run by exactly the printed term (the correction is added after
the SCF cycle); for Tkatchenko-Scheffler the density feeds back, so the two differ by
a few 1e-7 Ry.

XDM needs PAW pseudopotentials: with a norm-conserving file (the SSSP argon)
pw.x did not stop, and had not finished after four minutes (a PAW carbon monoxide took 25 s).
`ai.qe` does not choose the pseudopotential type for it; the user sets
"Prefer PAW" (help page for QE).
