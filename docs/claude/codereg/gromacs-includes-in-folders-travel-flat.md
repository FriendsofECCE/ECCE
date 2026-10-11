---
type: map
title: "A GROMACS topology's local includes (min.ff/forcefield.itp) are collected on attach and staged flat as <folder>__SLASH__<file>"
area: codereg
paths: [include/dsm/GromacsIncludes.H, src/dsm/chemistry/GromacsIncludes.C, src/wxgui/mdtools/GromacsInputsPanel.C, scripts/gensub, scripts/parsers/gromacs.launchpp, tests/gromacs_md/run_tests.py]
issues: [260]
---
Attaching a topology on the GROMACS Inputs tab follows its `#include` lines
(`GromacsIncludes::collect`): next to the including file, then next to the
topology, both branches of any `#ifdef`, recursively. What exists beside the
topology is stored as the task's auxiliary inputs; what only exists in
GROMACS's own library (`/usr/share/gromacs/top`, `GMXLIB`, `GMXDATA`) is left
to grompp; the rest is reported as not found in the status line.

The data server and Launch's transfer keep a flat list of names, so a file in
a folder is stored as `min.ff__SLASH__forcefield.itp`.
- `gensub` (job script) moves such files back under their folders before
  grompp runs.
- `gromacs.launchpp` links them back in a scratch directory and runs grompp
  there; it must not move the staged files, which Launch ships afterwards by
  their flat names.
- The includes list in the panel shows the slash form.

Only the files found at attach time are stored; a topology edited later is
not re-scanned until it is attached again.
