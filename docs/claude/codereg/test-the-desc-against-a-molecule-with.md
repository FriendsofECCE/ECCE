---
type: checklist
title: "Test the `.desc` against a molecule with degenerate vibrations"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: [".desc"]
---
**Test the `.desc` against a molecule with degenerate vibrations**
(CH₄, benzene), never just water. MOPAC's `DESCRIPTION OF VIBRATIONS`
collapses degenerate modes (7 stanzas for CH₄) while `NORMAL
COORDINATE ANALYSIS` lists all 9 — sourcing VIBFREQ and VIB from the
two different blocks silently misaligns them, the same trap as ORCA's
VIBFREQ/VIBIR. C₂ᵥ water cannot show this.
