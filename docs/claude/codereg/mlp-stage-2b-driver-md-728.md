---
type: map
title: "MLP (MACE) stage 2B: ecce-mlp-driver (C++) runs the optimisers, cell filter, MD and vibrations; Python only evaluates the network; ASE is the test reference (#728)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["src/mlp", "scripts/ecce-mlp", "scripts/ecce-mlp-setup", "scripts/parsers/mlp.desc", "scripts/parsers/mlp.geomtrace", "scripts/parsers/ai.mlp", "scripts/codereg/mlpruntype.py", "data/client/cap/MLP.edml", "scripts/gensub", "src/apps/machregister/WxMachineRegisterTools.C", "tests/mlp/driver_test.py"]
issues: [728]
---
- **Entry point unchanged.** gensub still inlines `scripts/ecce-mlp` and runs
  it with the registered Python. The script reads line 1 of the deck and
  `execv`s `ecce-mlp-driver <deck> <python> <script>` when it finds one
  (`$ECCE_MLP_DRIVER` — gensub sets it to `$ECCE_HOME/bin/ecce-mlp-driver`,
  harmless where that path does not exist — then `sys.prefix/bin`, then
  PATH). No driver, or `engine=ase` / `ECCE_MLP_ENGINE=ase`: the former ASE
  path, which prints `Engine: ASE` and refuses MD.
- **The driver must reach clusters with no ECCE**, so it is
  dependency-free C++17 (own expm, Frechet derivative, Householder/QL
  eigensolver). `src/mlp/CMakeLists.txt` concatenates hpp+cpp+main into one
  `ecce-mlp-driver.cpp`, installed in `scripts/`; Register Machines' Set up
  sends it inline next to `ecce-mlp-setup`, which compiles it into
  `venv/bin` in step 5 (no compiler: a log line, ASE fallback). Keep
  `#include "mlpdriver.hpp"` spelled exactly so; the concatenation removes
  that string. The inline command is one `sh -c` argument: the 128 KiB
  per-argument limit (MAX_ARG_STRLEN) bounds setup script + driver source.
- **Worker protocol** (`ecce-mlp --worker`): requests on stdin, replies on
  fd 3, because mace/torch print warnings to stdout. `init n pbc model
  dtype disp` + symbols → `ok\tname\tversion\tlicence\tweights`; `eval`,
  cell (9 or `none`), 3n positions → `ok E`, 3n forces, 6 Voigt stress (ASE
  sign) or `none`. `%r`/`%.17g` text: the evaluation, not the pipe, is the
  cost (64-atom NaCl, float32: see the ticket).
- **ASE equivalence is exact, not approximate.** FIRE and BFGS reproduce
  ASE's update rules and defaults (FIRE dt 0.1, maxstep 0.2 on the whole
  vector; BFGS H0 = 70, per-row 0.2 cap, |eigenvalue| step), and the cell
  DOF are ASE FrechetCellFilter's (N·log F, forces = −dH/dQ through the
  Frechet derivative of exp, H = E + pV). With the built-in LJ, ASE and the
  driver take the same number of steps to the same energy (tests/mlp/
  driver_test.py). A change that breaks step-count equality is a change of
  algorithm, not a refactor.
- **`ECCE_MLP_MOCK=lj` is built into the driver** (ASE LennardJones σ 2.5,
  ε 0.1, rc 8, shifted energy, all periodic images within rc), so the mock
  path needs no Python packages; `emt` still goes through the worker (ASE).
- **MD frames carry `time:` (ps)**; `mlp.geomtrace` turns it into TIMEVEC,
  same length as GEOMTRACE/TEVEC per frame, which is what the MD step plots
  use for their time axis. Run type name in the EDML is "Molecular
  Dynamics" (as xTB); `ai.mlp` writes `runtype=MD`. Every saved frame is a
  stored property, so `every` must keep frames to thousands.
- **NVT is Bussi velocity rescaling** applied after each full Verlet step;
  Ndf = 3N − 3 (COM momentum removed at the start and conserved).
