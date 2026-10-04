---
type: map
title: "labels, not the stored key"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: [".desc", "nwchem.desc"]
---
A `.desc` bracket group's key names are **labels, not the stored
key** — what actually gets stored is whatever the parser script
`print`s as `key: NAME`. The two can disagree (see `[TGRADCPVEC]` in
`nwchem.desc`, whose script emits `EGRADVEC`), so audit the scripts'
emissions, not the `.desc` brackets.
