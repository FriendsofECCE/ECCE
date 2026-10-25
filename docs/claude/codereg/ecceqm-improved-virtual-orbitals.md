---
type: map
title: "ECCE-QM improved virtual orbitals (IVO): keyword, output line, parser note"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/qm/scf.cpp", "src/qm/output.cpp", "scripts/parsers/ecceqm.parse", "scripts/parsers/ecceqm.desc", "scripts/parsers/ai.ecceqm", "scripts/codereg/ecceqmtheory.py", "scripts/codereg/ecceqmtheory.dlg"]
issues: []
---
`virtuals ivo` (default `canonical`, never written) re-solves the virtual space
of a converged closed-shell run with F' = F - J_h + 2 K_h (h = HOMO, singlet
hole coupling; Hunt and Goddard 1969), in the space orthogonal to the occupied
orbitals. Occupied orbitals and the energy are unchanged. For RKS the same
HF-type correction is applied to the Kohn-Sham matrix (J_h and K_h unscaled by
the functional's exact-exchange fraction). Open-shell and unconverged runs keep
canonical virtuals and write `virtual_orbitals_note <why>`; a made IVO set
writes `virtual_orbitals ivo`. Both are stored as `MOVIRTNOTE` (spaces as `_`).
The Theory Details combo is `ES.Theory.ECCEQM.Virtuals` ("Canonical" |
"Improved (IVO)"); the .py and .dlg must stay in step. The MOs panel does not
read `MOVIRTNOTE` yet.
