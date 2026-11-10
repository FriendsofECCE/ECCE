---
type: pitfall
title: "Organizer folder energy cells must not load point orbitals (#928)"
area: services
paths: ["src/apps/organizer/ProjectContextPanel.C", "src/dsm/edsiimpl/PropertyTask.C", "src/dsm/xml/VDoc.C", "include/dsm/VDoc.H", "src/dsm/edsiimpl/FileEDSI.C", "src/dsm/edsiimpl/Resource.C", "src/wxgui/wxtools/WxResourceTreeCtrl.C", "tests/apps/organizer_big_folder_test.py"]
issues: [928]
---
The Basic table reads final energies through `PropertyTask::getProperty`.
Only MO, MOBETA, ORBENG, ORBOCC, their beta forms and LOCMO* need point
orbital selection. Testing `orbitalsAtSomePointsOnly()` for every request
also reads PTMO/IRCGEOM and lists the property collection for ordinary
scalar energies. Keep that test behind the orbital-name check; large
point-orbital tables otherwise load just to show a folder.

VDoc construction shares a base-metadata snapshot across its document-name
loaders and property-existence checks. Local files include empty values in
that snapshot; DAV also needs one property-name query because an empty
property can be omitted from the value response. The snapshot is discarded
after construction, so runtime reads and mutations retain their normal
freshness and optional-document fallbacks.

FileEDSI's structured `getMetaData` delegates descendant gathering to the
vector overload. Do not gather it again afterwards. Pass the already-read
MetaStore into the recursive helper, including each subdirectory's store.
The gathered formula and basis metadata must remain available to the table
and calculation summary.

`organizer_big_folder_test.py` copies the stored water calculation 200 times.
`ECCE_TEST_ORGANIZER_TIMING` prints listing/metadata, table fill/energy/grid,
and tree icon/sort times. Fill includes child loading and energy reads;
the tree can reuse children already loaded by the table. There are no
thumbnail reads in the folder table. `ECCE_TEST_ORGANIZER_TABLE_SNAPSHOT`
writes the table cells, using the drawn application caption for Type.
`ECCE_BIG_FOLDER_REPORT` preserves the log and TSV from the Python test.
