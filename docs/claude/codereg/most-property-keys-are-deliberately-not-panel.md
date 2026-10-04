---
type: map
title: "Most property keys are deliberately *not* panel triggers"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["PropertyPanelDescriptor.xml", "src/apps/builder/*.C"]
---
**Most property keys are deliberately *not* panel triggers** — a
panel is triggered by one key and reads its companions itself. `MO`
triggers `MoPanel`, which then reads `ORBENG`/`ORBENGBETA`/`ORBOCC`/
`ORBOCCBETA`/`ORBSYM`/`MOBETA` directly; `VIB` triggers `NModePanel`,
which reads `VIBFREQ`/`VIBIR`/`VIBRAM`; `MULLIKEN` triggers
`MullikenPanel`, which reads `MLKNSHELL`. Cross-referencing emitted
keys against `PropertyPanelDescriptor.xml` alone therefore reports a
pile of false "no display path" gaps — grep `src/apps/builder/*.C`
for the key before believing one.
