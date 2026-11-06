---
type: map
title: "Organizer right-click menus and project table columns (#805): what is GUI and what is command line only"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "src/apps/organizer/ChecksDialog.C", "src/apps/organizer/SiExport.C", "data/client/config/ResourceDescriptor.xml", "data/client/config/ResourceDescriptorRxn.xml", "tests/apps/organizer_menus_test.py"]
issues: [805, 703, 573]
---
- **Calculation menu**: Cut/Copy/Paste/Rename/Delete, New from This (the
  three `wxID_DUPLICATE*` items), Run (Launch, Reconnect via `reconnectProblem`,
  Terminate and Shell in Run Directory = the state-based Run Mgmt tasks, listed
  always and grey when the state does not offer them). The tool list is gone.
  Hook `menulabels URL` prints the labels.
- **Command line only** (no GUI entry): Checks table, SI export, core-hours,
  New Series, Compare Forces, contacts, the excluded mark (neither set nor
  drawn; `ecce-annotate --exclude` still stores it, Launch Ready Steps still
  skips an excluded step). Their Organizer test hooks remain.
- **Compare Energies** = the old Rank (`ChecksDialog(..., rank=true)`) on the
  selected table rows (two or more) or a folder; the gather runs on the GUI
  thread (EDSI is not thread-safe) with a wxProgressDialog above three
  calculations. **Energy Calculator** = the old Tools > Compare Energies.
- **Project table**: the `totalenergy` column type (`PT_TOTAL_ENERGY`) is made by
  `CalcMgr::totalEnergyCellText` from `finalEnergyOf`; heat of formation is
  left empty. Type and application share one column: the resource-icon cell
  draws the icon and then the `ecce:application` text. Saved column sorts are
  by position, so a user's saved sort may land on another column once.
