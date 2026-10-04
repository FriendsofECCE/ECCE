---
type: pitfall
title: "\"The code accepted the keyword\" is not \"the keyword works.\""
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["<basis>/C"]
---
**"The code accepted the keyword" is not "the keyword works."** All
eleven ORCA correlated/double-hybrid keywords passed ORCA's input
check; two of them then died at runtime — double hybrids route their
correlation through RI-MP2 and need a `<basis>/C` auxiliary basis
(`ERROR: RI-MP2 needs an AuxC basis but none was defined!`, exit 55).
Offering them without it would have shipped decks that always fail,
*after* reaching a queue. Run a real job, not a syntax check. The same
discipline found that GROMACS's double-row energy blocks and QE's
header-carried units both silently produce wrong values rather than
none.
