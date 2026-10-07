---
type: pitfall
title: "A failed live property update must not free the cached property; a panel's timer must die with the panel (#217)"
area: wx-viewer
paths: ["src/dsm/edsiimpl/PropertyTask.C", "src/wxgui/wxtools/PlaybackControl.C", "src/apps/builder/NModePanel.C", "src/apps/builder/GeomTracePropertyPanel.C", "tests/apps/geomtrace_stress.py"]
issues: [217]
---
**`PropertyTask::updateProperty()` works on the cached object.**
`getProperty()` puts everything it returns into `p_properties`, so the
object a running job's message is applied to is the one every panel and
`loadStep()` will read next. When an update failed (a step whose size
disagrees with its header, a step the data server could not deliver, an
empty value), it used to `delete` that object and leave the cache entry
in place. The next `getProperty("GEOMTRACE")` then returned freed memory:
"Bad Geomtrace" (garbage step size) followed by SIGSEGV, or a crash in
`dynamic_cast`. A failed update now returns 0 and leaves the cached
property as it was.

**Timers owned by a panel are not stopped by wx when the panel goes.**
`PlaybackControl` re-arms a one-shot `wxTimer` for every playback step and
never deleted it; `removePropertyPanels()` (a calculation reset while its
trace or vibration is animating) destroyed the panel and the pending
timer then delivered its tick to freed memory. Delete the timer in the
destructor (`~wxTimer` stops it).

`tests/apps/geomtrace_stress.py` reproduces both (with `--gdb` for a
backtrace); the Builder's `gt...` scene commands are its driver.
