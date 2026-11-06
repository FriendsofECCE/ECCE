---
type: pitfall
title: "Session::createChild re-reads the linkbase: members added but not saved are dropped"
area: codereg
paths: [src/dsm/edsiimpl/Session.C, src/apps/organizer/WorkflowSteps.C]
issues: [710]
---
`Session::createChild` calls `getChildren(true)` and `loadLinkbase()` before
creating the child, so any `addMember` not yet written by `saveLinkbase()`
is lost, and the session's member objects are replaced. Making several steps
in one call then fails in `addLink` with "Source resource does not exist"
(links compare pointers). Create all children first, take the session's own
objects by URL from `getChildren(false)`, then `addMember` each and add the
links (`WorkflowSteps::applyRecipe`).
