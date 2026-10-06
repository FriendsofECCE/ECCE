---
type: checklist
title: "Two parallel resource-graph files, not one"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: [".edml", "ResourceDescriptorRxn.xml", "SessionContextPanel.C"]
issues: [38]
---
**Two parallel resource-graph files, not one**: `ResourceDescriptor.
xml` *and* `ResourceDescriptorRxn.xml` (used for reaction-study
projects) each need the new code's full `<ResourceType>` block *and*
a `<ContainsResource name="..._es"/>` entry on `project` — missing
either makes the "New Calculation" menu (and, in the Rxn file,
the CalcEd code-switch toolbar) silently omit the code, with no
error anywhere. `CodeFactory::getFullySupportedCodes()` (used by
CalcEd's code-switch *buttons*) auto-discovers `.edml` files by
directory scan; the "New..." *menu* (`CalcMgr::getContextMenu`/
`SessionContextPanel.C`) instead reads the clicked node's own
`ResourceType::getContains()` — two different mechanisms, only one
of which is automatic.
