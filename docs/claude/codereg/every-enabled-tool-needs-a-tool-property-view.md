---
type: pitfall
title: "Every tool a resource type enables needs a ToolPropertyView for its content type, or selecting the resource crashes the Organizer"
area: codereg
paths: [data/client/config/ResourceDescriptor.xml, data/client/config/ResourceDescriptorRxn.xml, src/dsm/xml/ResourceDescriptor.C, src/apps/organizer/CalculationContextPanel.C, tests/gromacs_md/tool_views_test.py]
issues: [930]
---
The Organizer's summary panel (`CalculationContextPanel::createControls`)
asks `ResourceDescriptor::getToolPropertyViews` for one view per enabled
tool, matched on tool name **and** the resource's content type. A new
content type (here `ecceMdEquilibrate` of `gromacs_md_equilibrate`) that
enables MDDynamics, Builder, Launcher and CalculationViewer but has no
`<ToolPropertyView contentType="...">` for them returned a null view that
was dereferenced: selecting any GROMACS Equilibrate task killed the
Organizer (SIGSEGV), and with it the session. The missing views are now
there in both descriptors, a tool without a view is skipped with a message
on stderr, and `tests/gromacs_md/tool_views_test.py` (ctest
`resource_tool_views`) fails on any such gap. Add the views in both
`ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml`.
