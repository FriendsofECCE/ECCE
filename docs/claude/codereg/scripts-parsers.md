---
type: map
title: "`scripts/parsers`"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["*.desc", "*.expt", "data/client/config/properties", "scripts/parsers"]
issues: [6, 7]
---
**`scripts/parsers`** — Perl. `ai.<code>`/`std2<Code>` generates the
input file before submission; `*.expt` parses full job output into the
`.frag`/`.basis`/`.param` files ECCE reads back. The **`*.desc`** file
tells ECCE when a property appears in output, which script extracts
it, and `Frequency=first|last|all` (get this wrong → silently wrong
geometry step, see #6/#7). `data/client/config/properties` maps
property keys to their GUI representation (~250 entries; find one
close to what you need rather than inventing a new shape).
