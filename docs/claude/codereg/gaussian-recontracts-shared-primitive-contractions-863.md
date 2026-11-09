---
type: pitfall
title: "Gaussian re-contracts general contractions that share primitives: fort.7 is in its basis, not the one ECCE wrote"
area: codereg
paths: ["scripts/parsers/gaussian-16.mo", "scripts/parsers/gaussian-16.codebasis", "scripts/parsers/ai.gauss16", "src/tdat/chemistry/MoAoOrder.C", "include/tdat/MoBasisChange.H", "data/client/cap/Gaussian-16.edml", "tests/basisflatten"]
issues: [863, 921]
---
Given a Dunning set (cc-pVXZ: the 1s and 2s contractions of F share ten
primitives), G16 transforms the contractions so that fewer primitives are
shared (with GFInput it echoes 9 and 7 primitives instead of 10 and 10).
The span and the energy are unchanged, but the individual functions are
not the ones in the input, and fort.7 coefficients refer to the
transformed set. Evaluated on ECCE's own copy of the basis, the HF/cc-pV5Z
occupied MOs give trace(PS) = 10.186 instead of 10, with the error in the
core s functions; it is not an MOOrdering fault, and no sign flip fixes it.

Consequences:
- To test an MOOrdering table against G16, build the basis from G16's
  GFInput echo (`tests/basisflatten/fixtures/hf_5z_g16.nb`).
- Resolved in #921: `ai.gauss16` puts GFInput in the route, the
  `[CODEBASIS]` entry of `gaussian-16.desc` stores the echo
  (`gaussian-16.codebasis`; a PropString, so one line: `<centre> <L> exp
  coef ...` records joined by `|`, centres are atom numbers), and
  `MoAoOrder::reorderToNative()` (which every MO consumer calls) then
  calls `projectFromCodeBasis()`, which maps `MoBasisChange::project`
  over the echoed and the ECCE basis. It returns the input table
  unchanged when there is no CODEBASIS (older calcs, other codes) or the
  two bases differ in length or span. Tests: `tests/basisflatten` rc6/rc7
  (trace(PS) = N), `g16-hcl-gfinput-codebasis` in `tests/parsers`.
- A PropString is stored with its line breaks dropped
  (`PropertyInterpreter::processValues`), so a multi-line text cannot be
  stored in one.

G16's spherical order is m = 0, +1, -1, ..., +l, -l, standard real solid
harmonics without Condon-Shortley phase, through h: with the echoed basis
the overlap built from the Gaussian-16.edml rows matches G16's own
(IOp(3/33=1)) to 1e-6, and negating or swapping any g or h row breaks it
by more than 0.1.
