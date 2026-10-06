---
type: checklist
title: "Keep `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` in step"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["data/client/config/ResourceDescriptor.xml", "data/client/config/ResourceDescriptorRxn.xml", "src/dsm/xml/ResourceDescriptor.C", "src/dsm/xml/CodeFactory.C", "src/apps/organizer/SessionContextPanel.C", "src/apps/organizer/CalcMgr.C"]
issues: [38]
---
**Keep `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` in
step.** Each needs the new code's full `<ResourceType>` block *and* a
`<ContainsResource name="..._es"/>` entry on `project`; missing either
makes the "New Calculation" menu (and, in the Rxn file, the CalcEd
code-switch toolbar) silently omit the code. Only one file is read:
`ResourceDescriptor.C` uses the `Rxn` variant whenever `bin/dirdyed`
exists, so a code missing from one file vanishes only on installs of
the other kind.

Why the `.edml` alone is not enough: `CodeFactory::getFullySupportedCodes()`
(CalcEd's code-switch *buttons*) discovers `.edml` files by directory
scan, but the "New..." *menu* (`CalcMgr::getContextMenu`/
`SessionContextPanel.C`) reads the clicked node's own
`ResourceType::getContains()` from these files.
