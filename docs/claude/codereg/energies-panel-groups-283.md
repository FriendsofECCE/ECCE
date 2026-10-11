---
type: map
title: "The Energies panel groups its rows by a column of data/client/config/properties"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["data/client/config/properties", "src/dsm/edsiimpl/PropFactory.C", "include/dsm/PropFactory.H", "src/apps/builder/KeyValuePropertyPanel.C", "tests/apps/energies_groups_test.py"]
issues: [283]
---
The optional sixth tab-separated column of `data/client/config/properties`
is the heading a scalar Energy property is listed under in the Energies
panel: Total, Components, Thermochemistry, Solvation, Derived. A new energy
property needs its line there with a group, or it lands under "Other" at
the end. `PropFactory::initialize()` is the only reader of the file; rows
keep the file's order inside a group (`Property_Ref::order`), so Total
Energy stays first.

`KeyValuePropertyPanel` is shared: a panel none of whose properties has a
group (S**2) keeps the old alphabetical list. THERMOTEMP is not a row; the
panel puts it in the Thermochemistry heading. The pressure and the
solvation model/solvent are not stored by any parser, so the headings do
not carry them. Fold state is `Energies/Fold/<group>` in `wxbuilder.ini`
(Components defaults to folded). `tests/apps/energies_groups_test.py` reads
the rows back through the Builder script commands `rows` and `fold`.
