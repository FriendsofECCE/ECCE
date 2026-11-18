---
type: map
title: "Counterpoise fragments live in the wx-free structure model and export ORCA ghost inputs"
area: codereg
paths: [include/tdat/Counterpoise.H, include/tdat/Fragment.H, include/tdat/TAtm.H, src/tdat/chemistry/Counterpoise.C, src/tdat/chemistry/Fragment.C, src/tdat/chemistry/TAtm.C, src/apps/builder/BuilderScript.C, tests/tdat/counterpoise_test.C, tests/apps/builder_counterpoise_test.py]
issues: [1091, 252]
---

`TAtm::fragmentId` stores the atom's fragment number (0 means unassigned).
`Fragment::fragmentStates` holds per-fragment charge and multiplicity.
These travel through MVM and structure copies; XYZ does not carry them.
Appending MVM offsets the imported fragment IDs. Merge/absorb into a
nonempty structure leaves incoming atoms unassigned, so export asks for a
fresh definition instead of silently conflating equal IDs from two structures.
The wx-free `Counterpoise` helpers own automatic connected-component
assignment, overrides, validation and ORCA export. Keep future fragment
consumers on this model rather than deriving separate GUI assignments.

Automatic assignment numbers connected molecules by first atom and starts
with neutral states and electron-parity multiplicities. It cannot infer the
physical ionic partition: on Na⁺···H₂O set total charge +1 and explicitly
set the sodium fragment to charge +1, multiplicity 1. Neutral defaults may
temporarily disagree with total charge; a state edit must produce the total
charge sum, and export rejects any pending mismatch. Multiplicities do not
add: they must be consistent with each fragment's electron count.
Atom overrides preserve existing states; after changing membership, correct
any incompatible charge/multiplicity explicitly before export. State edits
change one fragment at a time, so a neutral ionic pair cannot acquire +1
and −1 in two sequential edits under the strict charge-sum rule.

Builder hooks are `fragments [auto]`, `fragment N atoms LIST`, `fragment N
charge Q mult M` and `counterpoise orca DIR [MULT [ROUTE...]]`. Atom lists
are 1-based. Save marks the fragment data persistent with the structure.
`writeOrcaXyz` uses active-fragment 0 for the complex; otherwise every
partner element gets a colon, preserving its basis and coordinates while
removing its nucleus and electrons. A dummy X is not a ghost element.
The input set is `complex.inp`, then `fragment-N.inp`, with identical
geometry and route (default HF/def2-SVP), distinct charge/multiplicity.

This first part is export only. It does not alter `ai.orca`, submit jobs,
collect energies, add properties, provide a GUI checkbox, or change any
other engine. The model and Builder tests cover storage, assignments,
charge rejection and the exported geometry. The optional ORCA test runs
the three exported water-dimer decks and checks 20/10/10 electrons.

The Builder fixture needs `ecce:resourcetype=virtual_document` as well as
the calculation content type and application. Without it, the local-data
folder opens as an empty structure rather than a calculation. Stored
`ecce:spinmultiplicity` uses `Singlet`, not the integer `1`.
Fixtures inside `ECCE_LOCAL_DATA` must be under `users/local` to save:
FileEDSI permits reads elsewhere in that root but refuses writes there.
