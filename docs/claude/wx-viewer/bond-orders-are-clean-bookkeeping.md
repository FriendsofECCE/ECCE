---
type: rule
title: "Bond orders are Clean bookkeeping: SGFragment draws every covalent bond single; Clean must assign orders before it builds the ForceField"
area: wx-viewer
paths: [include/tdat/BondOrderRule.H, src/tdat/chemistry/BondOrderRule.C, src/viz/sgcommands/CleanCmd.C, src/viz/sgcommands/SGFragment.C, src/apps/builder/BondDropDown.C, tests/bondorder/bondorder_test.C, tests/apps/builder_bond_model_test.py]
issues: [439]
---
No UI sets a bond order. `BondOrderRule::perceive` (wx-free) derives orders
from element and neighbour count (a Nub counts as a neighbour) and
`CleanCmd` stores them on the `TBond`s. `SGFragment::getBondType` returns
SINGLE for every order >= 1 unless `SGFragment::showBondOrders(true)` (Options
> Advanced); order < 1 (metal-pi 0.5, hydrogen 0.1) stays dashed.

Trap: `ForceField` reads `TBond::order()` in its constructor (PBonds), so the
orders must be assigned before `ForceField forceField(...)` is built; assigned
after, Clean ran with all bonds single (ethene C=C stayed 1.46 A).

The Bond tool's drop-down has two kinds only (Covalent, Metal-pi); the
`BondChanged` event still carries "Single"/"Metal-Pi". Not done: MOL/SDF
export does not re-derive orders. Test: `bondorder_test`,
`apps_builder_bond_model`.
