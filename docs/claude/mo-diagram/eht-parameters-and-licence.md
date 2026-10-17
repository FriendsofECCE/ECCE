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

Rows with exponent 0 in the source (Ar, Kr, Xe, Y, Ag, Hf p and d, and
every Zn/Cd/Hg d) are left out: those elements are declined, Kr included.

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
for bound orbitals). Cl 3s/3p zeta differs from the older
`HuckelParameters` (2.183/1.733 against 2.033): Slater's rules there.
