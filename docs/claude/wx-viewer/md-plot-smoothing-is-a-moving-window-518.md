---
type: pitfall
title: "The MD plot's smoothing curve is a moving window, not a cumulative mean (#518)"
area: wx-viewer
paths: ["src/apps/builder/MovingAverage.H", "src/apps/builder/StepPlotAxes.H", "src/apps/builder/GeomTracePropertyPanel.C", "tests/plots/moving_average_test.C"]
issues: [518]
---
A cumulative mean from t = 0 keeps the start-up transient and lies below a
series that rises and levels off. `MovingAverage::centred` (wx-free, unit
test `moving-average`) averages 10 % of the points (at least 2); the legend
names the window ("moving average (5 ps window)"). The Geometry Trace has a
"Plot:" choice above the canvas for every logged series (the tear-off menu
held them too, unseen); it shares `GeomTrace/Prop` with the menu. The x axis
title is anchored to the canvas bottom edge in `XYPlotCanvas`.
