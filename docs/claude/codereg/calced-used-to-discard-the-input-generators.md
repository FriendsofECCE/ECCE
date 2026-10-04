---
type: pitfall
title: "CalcEd used to discard the input generator's error message"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["/dev/null"]
---
**CalcEd used to discard the input generator's error message.**
`execout()` captures the generator's stdout+stderr into `message`, and
the failure branch overwrote it with a generic "input parsing command
... failed". Every `ai.<code>` validates in its main flow and dies with
something specific and actionable — `ai.qe`'s periodicity check even
names the Builder panel to use — and none of it reached the screen.
Fixed (the generator's output now leads the message), but the lesson
generalises: when a script's diagnostics are the only explanation of a
failure, check that whatever shells out to it actually shows them.
Same shape as `eccejobmaster` logging to `/dev/null`.
