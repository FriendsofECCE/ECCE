---
type: pitfall
title: "Automatic atom-count quality and explicit slider values are separate states (#880)"
area: wx-viewer
paths: [src/tdat/chemistry/DisplayDescriptor.C, include/tdat/DisplayDescriptor.H, src/wxviz/viztools/VizStyleChooser.C, src/viz/sgcommands/SGContainer.C, src/inv/moiv/ChemDisplaySpheres.C, src/inv/moiv/ChemDisplayCylinders.C, tests/apps/sphere_quality_test.py]
issues: [880]
---
`DisplayDescriptor` keeps the nominal quality default 50 but tracks whether
each quality is automatic. Automatic sphere and cylinder complexity is 0.9
through 2,000 drawn atoms, 0.7 through 4,000, then 0.5. Screen-size adaptation
and its existing vertex budget can raise automatic detail further; explicit
quality bypasses that adaptation. This supersedes the slider-as-floor policy
in [the tessellation map](sphere-and-bond-tessellation-follows-screen-size.md).
`SGContainer::touchChemDisplay` publishes the total displayed atom count
(including visible bond endpoints) to every style through `qualityAtomCount`;
counting each style separately would make split-style MD boxes slower.

Calling `setSphereRQ` or `setCylinderRQ` marks the value explicit unless the
second argument preserves automatic mode. Copying and unrelated UI changes
must preserve that state. Preferences and style strings omit automatic
quality and include every explicit value, even 50. Older versions discarded
explicit 50, so an omitted old value cannot be distinguished from a default.

Cell-boundary images bypass ChemDisplay and use plain `SoSphere`. They need
their own scoped `SoComplexity`; otherwise the copies remain faceted even
when the real atoms are smooth. The same scene graph and quality nodes are
used by offscreen PNG/PostScript output.
