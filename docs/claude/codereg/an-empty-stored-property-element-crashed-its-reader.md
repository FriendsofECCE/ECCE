---
type: pitfall
title: "A property stored with no values is an empty element; reading one used to end the app"
area: codereg
paths: ["src/dsm/faces/PropertyDoc.C", "src/dsm/edsiimpl/PropertyTask.C", "scripts/parsers/mopac.mgf"]
issues: [429]
---
A parse script that prints `values:` followed by a blank line (e.g. a
vecstring of empty strings) is stored as `<vecstring rows="7" ...></vecstring>`.
`PropertyDoc::values()` took the root's first child without checking it, so
`getProperty` on such a file dereferenced NULL: the MOPAC ammonia example
(MOs list) closed the Builder with SIGSEGV and no message. It now returns
NULL, and the `getProp*` readers drop the property (size 0 != rows).

The empty ORBSYM came from `mopac.mgf`: without a point group MOPAC's GRAPHF
writes `ORBITAL 2 1` (index only), and stripping the index left "". The .mgf
parse runs after the output's, so it replaced the good ORBSYM from the
EIGENVECTORS block; it now writes no ORBSYM when any label is empty. A parse
script must never emit a key whose values line is blank.
