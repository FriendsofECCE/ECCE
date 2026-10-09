---
type: pitfall
title: "The MD editor tools are registered once, as NWChem's; the Organizer's button must name the task's code and fit its label"
area: wx-viewer
paths: [data/client/config/ResourceDescriptor.xml, src/apps/organizer/CalculationContextPanel.C, src/wxgui/wxtools/EcceTool.C, include/wxgui/EcceTool.H, src/apps/organizer/CalcMgr.C, tests/launch/md_editors_test.py, tests/launch/launchjob.C]
issues: []
---
`<Tool name="MDOptimize">` (and MDPrepare, MDEnergy, MDDynamics) carries the
label "NWChem MD Optimize Editor", and the GROMACS task types enable the
same tools. `EcceTool` drew that label under the icon on a fixed 68 px
button, so a GROMACS Optimize showed "NWChem M..." clipped.
`CalculationContextPanel::createToolButton` now puts the task's
`ecce:application` in place of "NWChem" when it is not an NWChem one
(NWChem MD tasks are `NWChemMD`, their study `MDStudy`); `shortLabel`
drops a trailing " Editor"; the button's width is its label's and the
panel gives all its buttons the widest one, so the property columns still
line up. The menu items ("NWChem MD Optimize &Editor...") still say NWChem.

Check: the Organizer hook `summary <url>` (ECCE_TEST_ORGANIZER) lists
`tool=<label>` per button, with `:clipped` when the label is wider than
the button; `tests/launch/md_editors_test.py` uses it for a GROMACS and an
NWChem MD Optimize (made with `launchjob gromacsstudy`/`nwchemmdstudy`)
and opens both in mdoptimize.
