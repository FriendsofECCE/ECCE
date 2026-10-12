---
type: map
title: "ECCE-QM extra charge sets: input keyword, output blocks, three ECCE keys"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/qm/charges.cpp", "scripts/parsers/ecceqm.desc", "scripts/parsers/ecceqm.parse", "scripts/parsers/ai.ecceqm", "scripts/codereg/ecceqmtheory.py", "tests/qm/charges_test.py"]
issues: [272]
---
The Theory Details tick boxes `ES.Theory.ECCEQM.{Loewdin,CHELPG,MK}` become
one `charges loewdin chelpg mk` line in `ai.ecceqm` (none ticked: no line).
The engine writes `begin charges <scheme>` blocks; `ecceqm.desc` has one entry
each, `ecceqm.parse` maps them to `LOEWDIN`, `ESPFIT` (CHELPG, the key ORCA's
CHELPG uses, so the ESP surface finds it) and `MKFIT` (own panel, "Merz-Kollman
Charges"). MK is not in the ESP surface's fallback chain (`ComputeMoCmd`,
`MoPanel`), by design for now.
Gaussian does one ESP fit per job (`Pop=(MK,CHelpG)` fits once), so its dialog
has a choice, not two tick boxes; ECCE-QM computes both from one SCF.
