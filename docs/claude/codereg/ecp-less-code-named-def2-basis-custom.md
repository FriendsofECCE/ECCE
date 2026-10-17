---
type: pitfall
title: "def2-SVP on light elements showed as \"Custom\" for ECCE-QM: removing an ECP that covers nothing left its key in the aggregate"
area: codereg
paths: ["src/dsm/chemistry/TGBSGroup.C", "src/dsm/edsiimpl/DavCalculation.C", "src/dsm/edsiimpl/ICalcUtils.C", "tests/basisload/run_tests.py", "tests/basisload/loadBasis.C"]
issues: []
---
The def2 aggregates carry `Def2-ECP` (it covers Rb and heavier). A code
whose `.edml` says `<ECPS>False</ECPS>` (ECCE-QM) gets its ECP removed by
`ICalcUtils`' consistency step (`EE_GBS_ECP` -> `clearType(ecp)`), but
`TGBSGroup::removeGBS` left `Def2-ECP:ecp` in the orbital mapping, so
`TGBSGroup::name()` counted it as a removed part and said "Custom". The
Basis Set box, the stored `ecce:name` and the group's `Name` all read
"Custom", although the basis is exactly def2-SVP. Of ECCE-QM's picks only
def2-SVP has an ECP part.

Now an ECP that covers none of the group's elements is also dropped from
the mapping when it is removed (`forgetAggregatePart`); one that does cover
an element still makes the group Custom, which it is. Files saved before
(9.1/9.2) are corrected when read: `forgetUncoveredEcp()` in
`DavCalculation.C` looks the aggregate up (the ECP has no library entry of
its own, only as a part) and forgets the part if it covers none of the
tag's elements.

`tests/basisload/run_tests.py` checks every ECCE-QM pick on C, H, O: named
right on the pick, after the ECP removal (`LOADBASIS_NO_ECP`), after
reading back, and the 9.1.0-saved benzoic acid fixture (156 functions).
