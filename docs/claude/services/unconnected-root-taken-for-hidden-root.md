---
type: pitfall
title: "A tree root without a resource (a server not connected) was taken for the hidden root: findNode recursed until the stack overflowed"
area: services
paths: ["src/wxgui/wxtools/WxResourceTreeCtrl.C", "src/wxgui/wxtools/WxResourceTreeItemData.C", "tests/launch/ssh_folder_launch_test.py"]
issues: [767]
---
- `WxResourceTreeItemData::getUrl()` is `EcceURL()` whenever the node has no
  resource. That used to mean only the hidden root, and
  `WxResourceTreeCtrl::findNode` recognised the hidden root by
  `currentUrl == EcceURL()`. The "(not connected)" roots of added ECCE servers
  (`addServerRoot`) and SSH folders (`addSshRoot`) also have no resource, so a
  search that reached one walked all roots again, itself included, without end.
- The Organizer died with SIGSEGV (wrapper exit 139) on a stack overflow: gdb
  showed ~22 700 frames of `findNode` (WxResourceTreeCtrl.C:666, the
  hidden-root loop) under `wxTimerImpl::SendEvent`. Any `findNode` for a URL
  not in an earlier root triggers it, e.g. the message about a calculation
  another process created, or the `panelmessage`/`toolsmenu` hooks. It looked
  like an SSH-folder or launch problem because that was the first test with an
  unconnected root after Local data and a URL outside Local data.
- The Organizer's own SIGSEGV handler cannot run on an overflowed stack, and an
  LD_PRELOAD handler is replaced by it: attach gdb instead. The test runs inside
  `unshare`, so give gdb the executable and `set sysroot /` (it cannot open
  `target:` paths across the mount namespace).
- Fix: the hidden root is the item data of `GetRootItem()`; any other node
  without a resource holds nothing and returns 0. Code that tells the hidden
  root by an empty URL or a null resource has the same trap.
- Regression check: `ssh_folder_launch_test.py` sends `panelmessage` for a URL
  in no tree while both SSH roots are unconnected; it must answer
  "not in the tree".
