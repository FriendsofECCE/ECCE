---
type: rule
title: "Element colours: Jmol (default) or ECCE classic, chosen in Preferences > Colours; one place applies them (`TPerTab::setColours`)"
area: wx-viewer
paths: [src/tdat/chemistry/TPerTab.C, src/wxgui/wxdialogs/GlobalPrefs.C, src/apps/builder/BuilderApp.C, tests/look/elementcolours.py]
issues: []
---
Every reader of an element colour (the 3-D viewer's `SGContainerManager::loadAtomColors`,
the periodic-table and element buttons, `StructLib`, thumbnails) goes through
`TPerTab::color()`/`colorName()`. `TPerTab::loadNeutralData()` keeps the
`AtomicElements` colours (the "ecce" scheme) and `setColours()` overwrites both
tables (`p_refel`, `p_userel`) with the Jmol table, by symbol, when the saved
`ElementColours` preference (EcceGlobal) is not "ecce"; a new user has none, so
gets Jmol. Elements Jmol has no entry for (the ghost atom X, Uun...) keep their
classic colour. The Jmol table is Jmol's default CPK table
(jmol.sourceforge.net/jscolors/), hydrogen to meitnerium.

The dialog saves, calls `TPerTab::applyColourScheme()` in its own process and
publishes `ecce_preferences_pertab`; the Builder re-applies and reloads its
scene graph's atom colours. Other running windows change when next opened. A
per-element colour a user once saved in the `PerTable` prefs still wins over
either scheme in the viewer.

`Color::xcolor()` rounds; it used to truncate, which made #90e050 print as
#8fe04f. `tests/look/elementcolours.py` checks the table, the classic file and
the saved choice (no display needed).
