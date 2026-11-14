---
type: pitfall
title: "AUI in a tab: floating panes must not be children of the page; wxAuiManager::Update is not virtual (#983)"
area: services
paths: ["src/apps/builder/Builder.H", "src/wxgui/jms/ToolTabs.C"]
issues: [983]
---
- `wxAuiManager::CreateFloatingFrame` gets the managed window as parent.
  In a tab that is the page, which `release` deletes on Open in Window:
  the floating frame and the panel in it died with it.
  `BuilderAuiManager::setFloatParent(this)` keeps floating frames children
  of the tool frame.
- `wxAuiManager::Update()` is non-virtual. `BuilderAuiManager::Update`
  (moves panes created later with the frame as parent into the page) only
  runs when called on a `BuilderAuiManager`; `ToolTabs::moveClient` calls
  it through `wxAuiManager*`, so there the reparenting comes from
  moveClient's own child loop, not from the override.
