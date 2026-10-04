---
type: map
title: "NWChem's `.desc` `Begin` patterns (`%begin%`/`%end%`/`task_*`) don't match raw NWChem stdout at all, and look bizarre if"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: [".desc", "nwch.tpl", "nwchem.desc"]
---
**NWChem's `.desc` `Begin` patterns (`%begin%`/`%end%`/`task_*`) don't
match raw NWChem stdout at all, and look bizarre if you try** — they
match a *separate*, machine-tagged trace file that NWChem writes via
its own built-in `ecce_print <file>` directive (still supported as of
NWChem 7.2.3, confirmed live), wired up by `nwch.tpl`'s `ecce_print
##parseFile##` line and `nwchem.launchpp` (rewrites that line to an
absolute path at launch time). This *is* the file `eccejobmonitor`
actually reads for NWChem — auditing `nwchem.desc` against plain
stdout instead will wrongly conclude the whole file is dead.
