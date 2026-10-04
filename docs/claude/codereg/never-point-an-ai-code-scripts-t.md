---
type: pitfall
title: "Never point an `ai.<code>` script's `-t` at the repo's own template"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: [".tpl", "mopac.tpl", "scripts/parsers/<code>.tpl"]
---
**Never point an `ai.<code>` script's `-t` at the repo's own
template.** `cleanup()` ends with `mv -f tmpfile "$TplFILE"`, so the
generated deck **overwrites the template you passed in**. Harmless in
normal operation, where the `.tpl` is staged into the run directory
first, but running one by hand for testing silently destroys
`scripts/parsers/<code>.tpl`. Copy the template into a scratch
directory and point `-t` at the copy. (Learned by clobbering
`mopac.tpl` and restoring it from git.)
