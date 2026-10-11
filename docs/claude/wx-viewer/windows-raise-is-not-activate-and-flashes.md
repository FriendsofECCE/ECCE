---
type: pitfall
title: "wxMSW: Raise() does not activate; a tool another process starts may not take the foreground, so Windows flashes its caption instead (W9, W13)"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/ewxWindowUtils.C", "src/wxgui/jms/WxAppJMSMessageDispatch.C", "src/apps/builder/BuilderApp.C", "src/apps/launcher/WxLauncherApp.C", "src/util/jms/JMSPublisher.C"]
issues: [254, 256]
---
- `wxWindow::Raise()` on MSW is `BringWindowToTop()`: the window is on top
  but not active. `SetFocus()` then only moves focus inside a process that
  is not the foreground one.
- Windows lets only the foreground process (or one it names) call
  `SetForegroundWindow`. The Calculation Editor asks the Organizer to start
  the Builder or Launcher; the Organizer starts or reuses the tool, so the
  tool is a background process. `RequestUserAttention()` is `FlashWindowEx`:
  the caption toggles active/inactive about every half second, which is what
  the clip of the Launcher showed ("selected, unselected several times").
- Fix: `JMSPublisher::publish("ecce_get_app")` (the process the user is
  working in) calls `AllowSetForegroundWindow(ASFW_ANY)`; the invoke handlers
  (`WxAppJMSMessageDispatch`, Builder, Launcher) show the window and call
  `ewxRaiseWindow()` once, which on MSW is `SetForegroundWindow` and asks for
  attention only when that is denied. `ECCE_DEBUG_RAISE=1` logs which.
- Checked with a foreground-window log (poll `GetForegroundWindow`, no input):
  Builder and Launcher each become the foreground window once. A harness that
  starts the tools from a scheduled task cannot recreate "the user clicked in
  the editor", so the denied path is checked by hand.
