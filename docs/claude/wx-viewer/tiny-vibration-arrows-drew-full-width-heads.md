---
type: pitfall
title: "A near-zero normal-mode arrow drew a full-width arrowhead: yellow discs in Wireframe (#312)"
area: wx-viewer
paths: [src/viz/nodes/VRVector.C, src/viz/propsgcommands/NModeVectCmd.C, tests/apps/vib_frame_builder_test.py]
issues: [312]
---
`fixedThickness` gives every arrow a head of radius 0.09 A whatever its
length, and atoms that barely move in a mode have arrows of 0.001 A. That
is a flat disc of 0.09 A at the atom centre: hidden inside the sphere in
Ball And Stick, plainly visible in Wireframe, where it looks like a yellow
disc floating at the line end. Start positions were correct all along
(`vibstarts` in the Builder script hooks reports start, sphere and head
radius per arrow). Below two head lengths the radii now shrink with the
arrow. The canvas `snap` did not show the arrows in this test; `offscreen`
did.
