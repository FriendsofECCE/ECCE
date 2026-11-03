---
type: pitfall
title: "\"No longer on disk\" in the Organizer means the node's path is gone; check the folder's ctime"
area: services
section: "Pitfalls"
paths: ["src/apps/organizer/NoAccessContextPanel.C", "src/apps/organizer/ContextPanelFactory.C", "src/dsm/edsiimpl/FileEDSI.C", "src/dsm/edsiimpl/Resource.C", "tests/apps/organizer_missing_node_test.py"]
issues: [683]
---
**"No longer on disk" in the Organizer means the node's path is gone.**
With a local data folder each selection re-reads the metadata
(`ContextPanelFactory` -> `Resource::recheckAccess`). `FileEDSI::getMetaData`
reports `RESOURCE_NOT_FOUND` only when `stat()` says ENOENT/ENOTDIR; any
other error is `UNABLE_TO_READ` with the errno text, shown as "could not
be read". Both EDSIs clear `m_msgStack` at the start of `getMetaData`, so
an earlier failure cannot make a later one read as missing.

When a user reports the message for a calculation that "is right there
on disk", compare the path in the message with where the folder is, and
look at the folder's ctime (`stat -c %z`): a `rename()` sets it. A move
made outside ECCE (a shell, a file manager, another tool) after the
Organizer loaded the tree leaves nodes at the old path until Refresh. The
panel names the missing path and, if a folder of the same name now sits
in a sibling folder, where it is.
