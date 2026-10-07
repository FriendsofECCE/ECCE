---
type: pitfall
title: "macOS: Terminal.app takes no command, and open(1) returns at once (#133)"
area: services
paths: [scripts/ecce-macos-terminal, src/util/genutil/UserEditor.C, src/wxgui/wxtools/WxEditSessionMgr.C, src/comm/commtools/EcceShell.C, tests/transport/macos_terminal_test.sh, tests/transport/testEditorCommand.C]
issues: [133]
---
ECCE starts every terminal as `TERM [opts] -e command args` (EcceShell,
UserEditor). macOS has no xterm and Terminal.app has no `-e`; the default
terminal there is `$ECCE_HOME/scripts/ecce-macos-terminal`, which writes the
command to a `.command` file, opens it with `open -a Terminal` and waits for
a marker the script touches on exit, so an editor session lasts as long as
the editor. `ECCE_TERMINAL_DRYRUN=1` prints the file instead.

An edit session ends when the editor process exits (WxEditProcess), and
`finishSession` then deletes the file. `open -e file` exits at once, so the
file was gone before TextEdit read it. UserEditor adds `-W -n` to `open`
(wait for a copy of its own); the session then ends when that TextEdit is
quit, not when its window is closed. Default editor on macOS: `open -t`.
