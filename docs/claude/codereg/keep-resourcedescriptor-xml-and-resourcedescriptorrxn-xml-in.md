---
type: checklist
title: "Keep `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` in step"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: ["ResourceDescriptor.C", "ResourceDescriptor.xml", "ResourceDescriptorRxn.xml", "bin/dirdyed"]
issues: [86]
---
**Keep `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` in
step.** `ResourceDescriptor.C` uses the `Rxn` variant whenever
`bin/dirdyed` exists, so a code missing from the plain file's
`project` `<Contains>` list is invisible on this build and vanishes
from the New-Calculation menu only where `dirdyed` is absent.
