---
type: pitfall
title: "Unlit butane thumbnails have exactly three colours on white (#1103)"
area: wx-viewer
paths: [src/apps/builder/EnsemblePropertyPanel.C, tests/apps/ensemble_patterns_test.py]
issues: [708, 1103]
---
Conformer thumbnails use `SoLightModel::BASE_COLOR`, with no shading or
antialiasing. For default Jmol butane the carbon atoms are RGB (144,144,144),
the bonds (110,110,110), and the hydrogen atoms white. On white there are
exactly three colours, even with atoms and bonds correctly rendered.

The switch to a white viewer default in `c63336602` made the previous
`apps_ensemble_patterns` assertion (more than three colours) obsolete:
black had supplied a fourth colour. Inspect the exported table before
diagnosing a blank render. The test now requires the carbon and bond
colours explicitly, and sets `background 1 1 1` before exporting.
