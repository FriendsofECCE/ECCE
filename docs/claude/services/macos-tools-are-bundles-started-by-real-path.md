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

wxSplashScreen is a floating tool window on macOS and the system hides it
whenever its app is not the active one (CGWindowList: layer 3, no
`kCGWindowIsOnscreen`), so the splash never showed; `ewxShowSplash` uses a
plain stay-on-top frame there (layer 8, on screen). It is the Organizer's:
the gateway is a UIElement and only sets `ECCE_SPLASH` for it.

What the Dock starts is `CFBundleExecutable` = `ecce-tool`, a script: without
`ECCE_HOME` (no session) it runs `open -a ECCE.app`, else it execs the
program. A check in C++ cannot do this: the ECCE libraries' static
initialisers assert on a missing `ECCE_HOME` before any early hook of ours
runs on Mach-O. A bundle name differing from the process's executable does
not matter to LaunchServices.

TCC attributes a folder-access request to the *responsible process*, which
follows the bundle's `CFBundleExecutable`. While `ECCE.app/Contents/MacOS/ecce`
was a bash script the TCC log (`log show --predicate 'subsystem ==
"com.apple.TCC"'`) said `responsible={identifier=com.apple.bash}`, so the
`NS*FolderUsageDescription` keys never applied and ~/Downloads silently
listed empty with no prompt (#344). `ECCE.app`'s executable is now a small
Mach-O (`packaging/macos/launcher.c`) that `posix_spawn`s `Resources/ecce.sh` (not in MacOS: codesign would want it signed) as a
child; never `exec` there (that would make bash the responsible process
again). A tool started by the gateway is a background UIElement app and is
not activated by macOS: `ewxApp` activates it and raises its first window.
