---
type: map
title: "RESOLVED (#81, fixed `9a3004e`, confirmed live 2026-09-21): the Vibrational Frequencies panel's Animation/Vector radio b"
area: wx-viewer
section: ""
issues: [78, 81, 109]
---
**RESOLVED (#81, fixed `9a3004e`, confirmed live 2026-09-21): the
Vibrational Frequencies panel's Animation/Vector radio box did not
deliver its click event under wx3.2/GTK3.** Kept because the *cause*
generalises to any `ewxRadioBox` (and any other ewx control with a
pushed handler chain): `ewxRadioBox::Create()` does
`PushEventHandler(new ewxHelpHandler(this))`, so a command event's
route from the control to its panel is not the plain propagation wx
documents, and a **static `EVT_RADIOBOX` table entry never arrives**.
Symptom was total: no row swap, no vector/animate switch, and the Play
button never appeared, since the whole display-mode switch is gated on
that event.
Fix: bind dynamically on the widget itself
(`radbox->Bind(wxEVT_RADIOBOX, ...)`). **Use that pattern for any new
ewx radio box rather than the static table** — the Graph/Table box
added later for #109 is wired the same way and works.
Two dead ends worth not repeating: an `EVT_UPDATE_UI` idle-poll
fallback was added first and reported as making "no visible
difference" — it was inert because wxGTK does not send
`wxUpdateUIEvent` to ordinary child controls during idle unless they
carry `wxWS_EX_PROCESS_UI_UPDATES`, which nothing set. Setting that
style turns it into a real fallback, and it is kept alongside the Bind
as layered defence (same approach as #78). And a reported *flicker*
during the newly-working animation was **not reproducible on retest**;
the `touchChemDisplay()` blank-then-rebuild mechanism proposed for it
is an untested hypothesis for a symptom that may not exist — do not
change that shared function on its strength without reproducing
flicker first.
