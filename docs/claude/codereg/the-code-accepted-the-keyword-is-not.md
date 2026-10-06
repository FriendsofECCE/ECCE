---
type: pitfall
title: "ORCA double hybrids pass the input check and need a `<basis>/C` auxiliary basis to run"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/codereg/orcatheory.py", "scripts/parsers/ai.orca"]
issues: []
---
"The code accepted the keyword" is not "the keyword works"
([verify by running](verify-by-running-the-code.md)). All eleven ORCA
correlated/double-hybrid keywords pass ORCA's input check, but double
hybrids route their correlation through RI-MP2 and die at runtime without
a `<basis>/C` auxiliary basis (`ERROR: RI-MP2 needs an AuxC basis but none
was defined!`, exit 55). Offered without it, they would ship decks that
always fail after reaching a queue. Running real jobs likewise showed that
GROMACS's double-row energy blocks and QE's header-carried units silently
produce wrong values rather than none.
