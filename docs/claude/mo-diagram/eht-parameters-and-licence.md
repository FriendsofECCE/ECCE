---
type: rule
title: "EHTParameters: Alvarez 1993 via YAeHMOP (BSD), what was checked and what disagrees"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["data/client/config/EHTParameters", "tests/fragorb/make_yaehmop_ref.py"]
issues: [301]
---
`data/client/config/EHTParameters` is S. Alvarez, "Tables of parameters for
extended Hueckel calculations", Universitat de Barcelona (1993), copied
unchanged from `tightbind/eht_parms.dat` of YAeHMOP
(github.com/greglandrum/yaehmop), which is under this licence:

    Copyright (C) 1995 Greg Landrum. All rights reserved.
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that (1) source redistributions
    retain the copyright notice, these conditions and the disclaimer, and
    (2) binary redistributions reproduce them in the documentation and/or
    other materials. THE SOFTWARE IS PROVIDED "AS IS" ... (BSD 2-clause;
    full text in the YAeHMOP repository's license.txt). The file's header
    carries the notice; keep it if the file is redistributed.

Rows with exponent 0 or no row at all in the source (Ar, Kr, Xe, Y, Ag,
Zn 3d, Cd 4d, and the Hf 6p and 5d exponents, of which only H_ii is given)
are left out: those elements are declined, Kr included. (Hg 5d is in the
file; the earlier note that Hg has no d parameters was wrong.)

Gap hunt (2026-10-17, read-only web lookup): the Hoffmann-group tables that
could be read (Inorg. Chem. 24, 4108 (1985) Table II: Co, Fe, Pt, S;
J. Am. Chem. Soc. 109, 124 (1987) Table I: Nb, P, S) are per-paper and hold
none of the missing entries; the ACS supplements, ResearchGate and the
Titech EHTB page were not reachable (403 / host not found); CACAO's
parameters were not found online. Nothing was filled in. The "derive from
the valence-state ionisation energy" recipe was not applied: against the
neighbours that have Alvarez values it misses by far more than 1 eV (the
neutral-atom first ionisation energy of the s electron, NIST, against
Alvarez H_ii: Cu 7.73 vs 11.40, Ni 7.64 vs 10.95, Pd 8.34 vs 7.32,
Au 9.23 vs 10.92, Zr 6.63 vs 9.87, Ta 7.55 vs 10.10 eV; differences
3.7, 3.3, -1.0, 1.7, 3.2, 2.5 eV, no constant offset), because EHT
H_ii are charge-iterated valence-state values, not ground-state IEs. A
proper VSIE needs the configuration-averaged multiplet data (Moore) that
were not at hand.

Checks made (2026-10-17): the file equals the fetched one byte for byte;
`weighted_Hij = 1` is YAeHMOP's default (driver.c) with K = 1.75 and the
Ammeter 1978 weighting, which `FragmentOrbitals` uses; H -13.6 and
Au 6s/6p -10.9/-5.6 eV agree with the Albright-Burdett-Whangbo (2013)
text; the double-zeta d functions of 22 of the 30 elements are normalised
to 1 +- 0.001 (an identity that does not depend on the source), eight are
not: Ni 1.07, Mo 1.07, Pd 1.11, Ta 1.25, W 1.08, Re 1.09, Ir 1.11, Au 1.13
(the engine renormalises the whole function and YAeHMOP agrees with it:
Mo/W/Ni carbonyls match `bind` to 0.03 eV). ABW prints no parameter table,
so Hii/zeta of C, N, O, Cl, Ti, Cr, Fe, Co, Ni, Mo have no second printed
source in ~/tmp/books; they were checked by running YAeHMOP itself instead
(tests/fragorb: CO, M(CO)6, TiF6, Co(NH3)6 energies agree to 0.004-0.04 eV
for bound orbitals). Cl 3s/3p zeta differed from the older
`HuckelParameters` (2.033: Slater's rules there), which is gone: `Huckel`
(H-Ne) now runs on this file (stage 2, #301).
