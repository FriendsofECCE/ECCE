---
type: pitfall
title: "A modal dialog shown while its parent is frozen leaves black boxes where it uncovers the parent"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["src/apps/calced/CalcEd.C", "src/wxgui/wxdialogs/ewxMessageDialog.C", "tests/apps/calced_code_change_test.py"]
issues: [308]
---
`CalcEd::setContext` and the code buttons wrap `doSetContext` in a
`wxWindowUpdateLocker`, and `doSetContext` asks the Code Change question.
A frozen window paints nothing, so while the question was open the code
icons, the status line and the message area were black. The question now
thaws the editor (every level of `IsFrozen()`), `Update()`s it, and freezes
it again after the answer. Look for the same shape before adding a
question inside any `Freeze`/`UpdateHold` scope.
`ECCE_TEST_DIALOG_CLOSE` makes `ewxMessageDialog` log
`parent frozen|not frozen`; `calced_code_change_test.py` checks that and
counts black pixels in a grab of the editor (the dialog's own rectangle
painted over first: a window grab shows another window's area as black).
It reproduced 30 000 black pixels before the fix, 8 after.
