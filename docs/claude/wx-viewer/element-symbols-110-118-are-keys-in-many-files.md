---
type: pitfall
title: "Elements 110-118 (Ds ... Og) are keys in data files, cursor file names and basis tables; old Uun... symbols are still read"
area: wx-viewer
paths: [data/client/config/AtomicElements, data/client/config/AtomicOxidationStates, data/client/config/Isotopes.txt, data/client/cursors, data/admin/basissets, scripts/gbsDAVConverter, tools/basissets/baseline.json, src/tdat/chemistry/TPerTab.C]
issues: [930]
---
The symbol is the key of `AtomicElements`/`AtomicOxidationStates`, part of the
`Isotopes.txt` isotope id, the prefix of `data/client/cursors/<Sym>-<Order>.xpm`
(the Builder builds the cursor path from the symbol, so a missing file fails
silently), and the element list of the CRENB*/ECP/MASTER.TBL basis files. A
rename must touch all of them together.

Calculations and structures stored with `Uun`..`Uuo` still load: `TPerTab`
keeps the old names as aliases in its symbol hash, and
`TPerTab::canonicalSymbol()` is applied in `mixedCase`, `TAtm` and
`TGBSConfigTags`, so the new symbol is what gets written back. Any new reader
that compares symbol strings itself must call `canonicalSymbol()` first.
