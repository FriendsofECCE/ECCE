---
type: pitfall
title: "ECCE's tree-list control must not reuse wx 3.x class names (exit crash in libwx_gtk3u_core)"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/TreeListCtrl.C", "include/wxgui/TreeListCtrl.H", "src/apps/builder/MullikenPanel.C"]
issues: [983]
---
`TreeListCtrl.C` is the old contrib tree-list control. Its classes were
called `wxTreeListCtrl`, `wxTreeListItem`, ... and `wxTreeListCtrlNameStr`,
names wx 3.x core now exports itself (`wx/treelist.h`, `treelist.cpp`). The
dynamic linker binds wx core's references to the program's symbols of the
same name, so wx core's static event table and ECCE's are one object, and
both destructors run at `exit()`: SIGSEGV in `__cxa_finalize` of
`libwx_gtk3u_core` (`wxEventTableEntryBase::~wxEventTableEntryBase`,
`treelist.cpp`). Only a program that links the control crashes: the Builder
always did, which is why `Builder::quit` uses `_exit(0)`; the Organizer did
once it linked the Builder's objects (#983 one process).

The ECCE classes are now `ewxTreeList*`. Keep that prefix for anything
added there; `ewxStyledWindow.C` still means wx's own `wxTreeListCtrl`.
A symbolised backtrace for such crashes: run gdb with
`-iex "set debuginfod enabled on"` (`one_process_test.py --gdb` does).
