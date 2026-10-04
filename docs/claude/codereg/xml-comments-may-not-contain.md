---
type: checklist
title: "XML comments may not contain `--`"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: ["MOPAC.edml"]
---
**XML comments may not contain `--`**, which the prose style used
throughout this file uses constantly. It silently made `MOPAC.edml`
and both `ResourceDescriptor` files non-well-formed.
