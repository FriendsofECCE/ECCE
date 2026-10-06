---
type: map
title: "Start from the AO composition, not symmetry machinery"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/MoComposition.H"]
issues: [132]
---
**Start from the AO composition, not symmetry machinery** (Andy,
2026-09-27). Every MO's share per atom and per shell is known
(`MoComposition`). Non-bonding = on one atom; ligand orbital = no
share on the metal; metal–ligand = mixed. Reach for irreps and
overlap-population statistics only for what composition cannot say.
