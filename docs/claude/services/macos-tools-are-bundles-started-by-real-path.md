---
type: pitfall
title: "macOS: Dock icon, menu name and LSUIElement need a bundle started by its real path, not through a link (#344, #354)"
area: services
paths: [packaging/macos/make-helpers.sh, packaging/macos/make-app.sh, src/wxgui/ewxClasses/ewxApp.C, src/wxgui/jms/WxJMSMessageDispatch.C, include/util/Ecce.H, src/apps/gateway/GatewayApp.C]
issues: [344, 354]
---
Each wx program lives in `ECCE.app/Contents/Helpers/<Title>.app` (made by
`make-helpers.sh`); `ecce/bin/<name>` is only a link to it. Measured on
macOS 12 with `lsappinfo`: a process started through the link is placed in
the bundle (icon) but has no bundle id, is named after the file ("organizer")
and ignores `LSUIElement` (type stays Foreground); the same binary started by
its real path gets the bundle name and `type="UIElement"`. So the gateway
(`WxJMSMessageDispatch`), `Ecce::ecceBinCommand` and the wrapper scripts
(rewritten by `make-helpers.sh`) all use the real path, and bundle names
contain spaces, so they quote it. `otool`/`osascript -e 'tell application
"System Events"...'` on a Mac without developer tools or Automation consent
open a dialog or hang: use `lsappinfo`.

The splash is the Organizer's on macOS (the gateway is a UIElement, whose
windows do not stay in front); the gateway only sets `ECCE_SPLASH` for it.
A tool started from the Dock has no `ECCE_HOME` and runs `open -a ECCE.app`
instead (`ewxApp::OnInit`).
