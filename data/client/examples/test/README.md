# Additional hand checks

Import **Examples > Test**. Existing imported sets gain these missing topics;
delete a calculation's imported copy before importing again if it needs refreshing.

## Compare forces (#776)

In **Compare forces**, `water_forces_hf` and `water_forces_b3lyp` are energy
and gradient calculations at the final geometry of Getting Started's ECCE-QM
water. Their atom order is O, H, H. Both store `Props/EGRADVEC` in Hartree/Bohr.

Run `ecce-si-export --compare-forces <water_forces_hf folder> <water_forces_b3lyp folder>`
on the imported calculation folders. Check that the force differences are nonzero,
there is no geometry warning, and the energy difference is withheld because the
methods differ. The automated Compare Forces test also opens this pair in the dialog.

## CREST topology check (#831)

Open **CREST topology check > zinc_aqua_topology** in the Organizer, then open
the Builder's Conformers panel. The sole frame is the fallback input structure;
do not interpret its zero energy as a molecular energy.

The warning hand check is blocked: example imports become Loaded, whereas the
job store records the topology warning only for Completed jobs. The output
triggers the warning detector, but the imported example does not display it.
The intended check is a Complete calculation whose summary displays
**Warning: All conformers were removed by the topology check ...**.

The triggering structure is neutral [Zn(H2O)4]Cl2, with four equatorial water
oxygens and two axial chloride ions initially separated from zinc by 3 angstrom.
The shipped deck uses GFN2-xTB, `--mquick` and `--noreftopo`; it was run with two
threads. Without `--noreftopo`, CREST stops at its initial topology safety check.
With it, CREGEN reports 133 topology-based removals and returns the input frame.
The output includes the real ensemble, input and best-structure blocks in the
same format appended by ECCE's job script.

## Code icons (#717)

Open **Code icons**. Check the xTB icon for `water_xtb` and the CREST icon for
`zinc_aqua_topology` in both the tree and the project table. The xTB calculation
reuses the Getting Started water outputs; the CREST calculation reuses the
triggering topology-check run.

The MLP icon is not covered: MACE is unavailable and the example importer
requires a program output, so it cannot ship a Created-only calculation.
