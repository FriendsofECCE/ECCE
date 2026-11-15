---
type: map
title: "Preferences > Codes: one filter, CodeChoice, hides codes from NEW calculations only"
area: services
paths: ["include/dsm/CodeChoice.H", "src/dsm/xml/CodeChoice.C", "src/dsm/xml/ResourceType.C", "src/wxgui/wxdialogs/GlobalPrefs.C", "src/apps/organizer/CalcMgr.C", "src/apps/organizer/WorkflowPanel.C", "src/apps/calced/CalcEd.C", "src/apps/machregister/WxMachineRegister.C", "tests/apps/code_choice_test.py"]
issues: [1049]
---
`ResourceType::getOfferedContains()` filters through `CodeChoice::typeShown()`,
so File > New, the right-click New menu and the workflow panel's offers share
one filter; `getContains()` stays whole, so existing calculations open.
Codes outside `CodeChoice::known()` are never hidden. The stored list starts
with a `chosen` marker because an empty list reads back as "not set". Every
list that hides codes ends with `moreLabel(N)`; the CalcEd bar rebuilds on
each context change and the workflow bar in `updateTexts`, so a tick shows
without restart. Register Machines saves tick a code on when it gets a path.
