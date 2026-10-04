# The MO correlation diagram (#132)

Split out of CLAUDE.md 2026-10-04.

Runs `SymmetryOps::find()` (autosym — **reorients and symmetrises**) →
`MoFragments` (shells out to `symops`) → `CharacterTable::reduce()` →
`MoDiagram::{classify, connect, placeFragments}` → `MoDiagramCanvas`.
Two callers build the model and must stay in step:
`MoDiagramPanel::build()` (from a calculation) and
`include/tdat/MoSpec.H` (from a spec file, used by the offline tools).

- **Start from the AO composition, not symmetry machinery** (Andy,
  2026-09-27). Every MO's share per atom and per shell is known
  (`MoComposition`). Non-bonding = on one atom; ligand orbital = no
  share on the metal; metal–ligand = mixed. Reach for irreps and
  overlap-population statistics only for what composition cannot say.

A **standalone diagram program is planned as a separate Python
project, not part of ECCE** (Andy's call, 2026-09-24). A C++ one was
built and removed; `tools/modiagram/fromxyz.py` (XYZ → autosym →
MOPAC → spec) is the bridge it would reuse.

- **A fragment must be a union of orbits** — otherwise the group maps
  it outside itself and it has no symmetry orbitals at all. This is
  why the panel's chooser groups *orbits* rather than atoms, and why
  ethene-as-two-CH2 needed a different mechanism entirely: a CH2
  crosses D2h's orbits ("both carbons", "all four hydrogens"). The way
  through is to analyse each half in the **subgroup** that preserves
  it, then combine in and out of phase — `buildHalves()` /
  `inducedIrreps()` in `MoFragments.C`. Which irreps a combination
  spans is the induced representation (computed, and it declines
  rather than rounding); which combination is in phase comes from the
  sign of the character on a class that swaps the halves.
- **A complex is classified by its coordination skeleton, not the
  molecule.** Six ammonia rotors cannot all be octahedral, so a
  whole-molecule search returns Th at best and C1 in practice. The
  panel runs its symmetry search on `MoFragments::coordinationSkeleton()`.
  Polyatomic ligands contribute **one sigma donor each**, not their
  whole basis; the pi set comes from subtracting sigma from the p-shell
  reduction.
- **connect() runs BEFORE placeFragments()**, so fragment levels still
  carry tabulated energies in eV while the molecular column is in
  Hartree. Anything comparing the two magnitudes directly is wrong —
  normalise each column to its own range first.

- **Look at `tools/modiagram/render`'s output before believing any
  layout claim.** It runs the real engine and paints the real
  `MoDiagramCanvas` to a PNG in seconds. `draw.py` is a *second*
  implementation of the picture and drifted far enough that the
  diagram looked right in the tool and wrong in builder for hours.
- It needs `$ECCE_HOME/bin/symops`; without it every fragment column
  comes back **empty and no line is drawn**, which looks exactly like
  a logic bug and is not one.
- A fragment column is **not sorted by energy** — `placeFragments()`
  moves each level to the mean of the orbitals it became. Anything
  that walks it in array order and assumes monotonic energy is wrong.
- Fragment columns may be opened out for legibility; the molecular
  column never is, because its energies are the measurement. Whatever
  computes a correlation line's endpoint must use the same `columnGeometry()`
  answer the levels are drawn with, or the lines point at nothing.

