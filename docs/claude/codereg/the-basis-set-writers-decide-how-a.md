---
type: pitfall
title: "The basis-set writers decide *how* a basis reaches the deck, and every bug in them is silent"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["tests/basis", "tests/basisload/nwchem_library_check.py", "wr<Code>GBS.pm", "wrORCAGBS.pm"]
---
**The basis-set writers decide *how* a basis reaches the deck, and every
bug in them is silent.** `TGBSConfig::dump()` always writes a
`NumericalBasis` section (explicit exponents and coefficients) and
*additionally* a `NameBasis` section when the basis can be named; each
code's `wr<Code>GBS.pm` then chooses. Things learned the hard way
(2026-09-23):
- **Gaussian's writer printed ECCE's own name, not the translated
  one**, so `%NameToBasis` was consulted only as a yes/no test and its
  value discarded. That works wherever the spellings coincide and
  produces a deck Gaussian *rejects* where they do not — `midi!` has
  to be `midix`, `dz (dunning)` → `d95`, `sv (dunning-hay)` → `d95v`.
  Fixed; ORCA's writer always printed the value.
- **Name tables must be verified by RUNNING the code.** ORCA accepts
  `6-31++G**` but rejects `6-31++G` and `6-31++G*`. Gaussian's
  `def2SVPP` is *not* def2-SVPP — it is def2-SV(P) (18 functions for
  water against def2SVP's 24), so a plausible-looking mapping silently
  substitutes a smaller basis. Record the basis-function count beside
  each entry so a future substitution shows up as a changed number.
- **A Gen section / `%basis` block does not need primitives** — each
  element group may name a basis the code ships. Naming is decided
  **per element**: an element carrying an ECP keeps explicit output so
  it cannot disagree with the separately written ECP, everything else
  is named. Before this, one unmappable element forced every element
  to be written out in full.
- **NWChem needs no table**: ECCE's names *are* its library names
  (both EMSL's). It has a blocklist instead, measured with
  `tests/basisload/nwchem_library_check.py` against NWChem 7.2.3: only
  `aug-cc-pwCV*` (no hydrogen in NWChem's set), `aug-pV7Z`/`aug-mcc-pV8Z`
  (unloadable), 5Z and up and `d-aug-cc-pVQZ` (unsettled) stay; every
  other `aug-`/`d-aug-`/`-pCV`/`IGLO` name spans the same space as
  ECCE's set. Beware that ECCE's explicit form repeats primitives, so
  NWChem drops near-dependent vectors and its energy can differ from the
  named form by up to 7e-4 Eh (QZ) until `lindep:tol` is tightened.
- **ORCA is spherical-only.** It has no cartesian basis keyword at
  all, so `wrORCAGBS.pm` ignoring `$coordinants` is correct, not a bug.
  Gaussian's writer passes it and `ai.gauss16` emits `5D 7F`/`6D 10F`.
- Perl randomises hash iteration per process, so these writers emitted
  **elements in a different order every run** until the keys were
  sorted — the same calculation producing a byte-different deck each
  time. `tests/basis` covers all of this now.
