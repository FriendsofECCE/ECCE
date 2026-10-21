---
type: pitfall
title: "SingleGrid data runs x fastest (a cube file is z fastest); a Gaussian 16 calculation offers Density (ESP) when it has MO and ORBOCC"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/SingleGrid.H", "src/wxviz/viewer/SceneScript.C", "src/apps/builder/MoPanel.C", "tests/apps/g16_esp_test.py"]
issues: [410]
---
- `SingleGrid::fieldData()` / `colorFieldData()` are indexed
  `ix + nx*(iy + ny*iz)`; Gaussian cube files and the synthetic `esptest`
  grid are z fastest. Reading one as the other gives plausible but wrong
  potentials (differences of whole Hartree/e only near the molecule).
- MoPanel offers "Density (ESP)" for any code when the calculation has `MO`
  and `ORBOCC` (`MOBETA` and `ORBOCCBETA` for open shells); the potential is
  integrated from the orbitals and basis, nothing is read from the output.
  Gaussian 16 jobs written by ECCE carry `Punch=(MO)`, and the occupations
  come from "Orbital symmetries" in the plain output, so no extra keyword is
  needed. The mocomp fixtures hold `MO`/`ORBENG` only, so a test adds `ORBOCC`.
- Scene commands `esp [res]` then `espdump <file>` write the grid (dimensions,
  origin, corner in Angstrom; density and potential per node, z fastest);
  `tests/apps/g16_esp_test.py` uses them. Within about 0.5 A of a C or O
  nucleus the grid's density (the 1s core) is not exact at 0.2 A spacing; the
  surfaces drawn (0.002 isovalue) are far outside that.
