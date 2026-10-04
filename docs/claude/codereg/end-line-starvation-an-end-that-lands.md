---
type: checklist
title: "`End`-line starvation: an `End` that lands on another entry's `Begin` line makes that entry unable to EVER fire"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: ["PropertyPanelDescriptor.xml", "mopac.desc", "orca.desc"]
---
**`End`-line starvation: an `End` that lands on another entry's
`Begin` line makes that entry unable to EVER fire.** `eccejobmonitor`
consumes the `End`-matching line as part of the block it closes, so
that line is gone before the other entry's `Begin` is ever tested
against it. No error, job completes normally, the property is simply
always absent. Found twice in one file on 2026-09-21, in `orca.desc`:
`[VIBFREQ]`'s `End=NORMAL MODES` ate `[VIB]`'s `Begin=NORMAL MODES`,
and `[SHIELDTENSOR]`'s `End=CHEMICAL SHIELDING SUMMARY` ate
`[ISOSHIELD][ANISOSHIELD]`'s `Begin`. The `[VIB]` one was
particularly costly because `PropertyPanelDescriptor.xml` gates the
"Vibrational Frequencies" panel on `VIB`, not `VIBFREQ` — so every
ORCA frequency job extracted correct frequencies and then showed no
vibration panel at all. Note it also hides itself in chains: `[VIB]`
had the identical bug against `[VIBIR]` one link further down, latent
only because `[VIB]` never fired, so fixing one entry can expose the
next. Fix by ending on a *separator* line instead of the next
section's header — an anchored unindented dashes run
(`End=^-{4,}\s*$`) works and can't match the indented separators
inside other blocks — or by re-anchoring the starved `Begin` onto
something past the other entry's `End` (e.g. the table's own column
header). `mopac.desc`'s header warns about this hazard; whenever two
entries read adjacent sections of one output, check the boundary.
