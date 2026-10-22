---
type: pitfall
title: "An X-H stretch's arrows ended inside the heavy atom's ball: the head was hidden (#430)"
area: wx-viewer
paths: [src/viz/propsgcommands/NModeVectCmd.C, src/viz/nodes/VRVector.C, tests/apps/vib_arrows_builder_test.py]
issues: [430]
---
Arrows start on their own atom's sphere (#228), but the longest arrow is 1 A
and an X-H bond is about 1 A: an arrow on H that points along its bond
reaches past the heavy atom's centre, so its head lies inside that ball and
only the shaft in the bond shows. Starting the arrow elsewhere cannot fix
this. Each vibration arrow now also gets `VRVector::showThroughMolecule()`,
the blended depth-test-off pass the dipole arrow uses, so the hidden part
shows at 40 %. `tests/apps/vib_arrows_builder_test.py` counts arrow-tinted
pixels inside the nitrogen ball of an ammonia N-H stretch (Ball And Stick and
CPK); the viewer draws the negative of the mode by default.
