---
type: map
title: "The project table's Progress column: made by the Organizer from the live or stored snapshot, one cell per message (#284)"
area: services
section: "Pitfalls"
paths: ["data/client/config/ResourceDescriptor.xml", "data/client/config/ResourceDescriptorRxn.xml", "src/dsm/xml/GridProperty.C", "src/wxgui/wxtools/GridPropElement.C", "src/apps/organizer/ProjectContextPanel.C", "src/apps/organizer/CalcMgr.C", "src/util/genutil/JobProgress.C", "tests/apps/progress_table_test.py"]
issues: [284, 254]
---
- **A `<Property>` of type `progress`** (`GridProperty::PT_PROGRESS`) after
  State in the `basic`, `launch` and `detail` views, in *both*
  `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` (only one is read).
  Later columns' `<Order>` moved up by one. Saved sort columns are by
  position, so a user's saved sort may land on another column once.
- **Not a stored value.** `GridPropElement` leaves the cell empty for this
  type; `ProjectContextPanel` sets it from `CalcMgr::progressCellText()`:
  `JobProgress::shortLine()` of the live snapshot, else the stored
  `ecce:progress`, and only while the state is Running.
- **One cell per message.** `CalcMgr::progressUpdate` also calls
  `ProjectContextPanel::updateProgress(url)`, which finds the row and sets and
  repaints that cell (`RefreshBlock`); the table is not rebuilt (#254). A
  state change takes the ordinary refresh, which builds the cells again.
- **Text.** "Opt step 4 · SCF 10", "Freq 14/54", "SCF 7 · ΔE 4e−5",
  "MD 100/500"; the last of opt/freq in the snapshot is the current one. No
  bar in the cell (a text cell only).
- **Test.** `tests/apps/progress_table_test.py` (ctest `apps_progress_table`),
  hook command `table [project-url]`.
