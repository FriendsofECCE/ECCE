---
type: map
title: "Thermochemistry and solvation keys: what each code prints, and where NWChem's come from"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["scripts/parsers/gaussian-16.thermo", "scripts/parsers/gaussian-09.thermo", "scripts/parsers/nwchem.thermo", "scripts/parsers/orca.solv", "scripts/parsers/gaussian-16.solv", "scripts/parsers/nwchem.desc", "data/client/config/properties"]
issues: [176]
---
EZEROPT/ETHERM/ENTHALPY/ENTROPY/EGIBBS hold Hartree for every code;
ENTROPY is T*S (not S), the others are absolute (ZPE is the correction).
THERMOTEMP carries T. Gaussian prints the sums directly. NWChem prints
only corrections, so `nwchem.thermo` adds them to the first total energy
after the Hessian banner. MOPAC prints no Gibbs energy; EGIBBS is left
unset on purpose (see `mopac.thermo`).

NWChem's ecce_print trace contains no thermochemistry and no COSMO data,
so `nwchem.desc` reads the stdout file (`nwch.nwout`) through a `File=`
entry; the old `begin%zero point` EZEROPT entry can never fire. That
`File=` path is replayed in tests/parsers but not yet seen in a live
ECCE job.

ESOLV is E(solution) - E(gas) and is set only where both totals are
printed (NWChem COSMO). Gaussian SMD and ORCA SMD print only the
cavity-dispersion term, stored as ESOLVCDS; a full solvation energy from
them needs a separate gas-phase job. ORCA's last `SMD CDS (Gcds)` comes
from the last SCF of the numerical frequency step, a displaced geometry
(about 1e-5 Eh from the final geometry's value).
