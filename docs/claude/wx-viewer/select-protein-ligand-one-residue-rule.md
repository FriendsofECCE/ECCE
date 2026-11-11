---
type: rule
title: "Select Protein / Select Ligand: one name-based residue rule, in SGFragment"
area: wx-viewer
paths: [src/viz/sgcommands/SGFragment.C, src/viz/sgcommands/SelectCmd.C, src/apps/builder/BuilderScript.C, src/apps/builder/BuilderGUI.C, tests/apps/builder_select_protein_ligand_test.py]
issues: [967, 962]
---
`SGFragment::residueClass(name)` is the only place that decides whether a
residue is polymer (amino acid or nucleic acid from `data/client/config/ResidueList`,
plus AMBER NALA/CALA terminals, ACE/NME caps and CHARMM/protonation variants),
water, ion, crystallisation additive (GOL, EDO, SO4, PO4, ACT, DMS, PEG ...)
or other. Select Protein is every polymer residue; Select Ligand is the largest
"other" residue of more than one atom (a lone atom is an ion). Reuse the same
function for the docking ligand choice rather than keeping a second list.

- The message comes back through `SelectCmd::message()` and is shown with
  `Builder::showMessage`; a structure without residues gets a warning, never a
  silent empty selection.
- The two menu items are greyed through `EVT_UPDATE_UI` (`OnUpdateSelectResidueItem`),
  not `Builder::updateMenus`, with the reason put in the label.
- Both add to the current selection, like Select Solvent. Script hooks:
  `select_protein`, `select_ligand` (`ECCE_BUILDER_SCRIPT`).
- A residue missing from `ResidueList` and from the extra sets (modified
  residues such as phosphoserine) counts as "other" and can be chosen as the ligand.
