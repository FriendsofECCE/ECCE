---
type: map
title: "MLP site screening (#728 stage 2C): ecce-sites finds symmetry-distinct voids and vacancy pairs; a series of MLP calculations per site; results as an ensemble"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["src/apps/organizer/CalcMgr.C", "src/apps/organizer/WorkflowSteps.C", "tests/launch/site_screen_test.py", "src/mlp/sites.hpp", "src/mlp/sites.cpp", "src/mlp/sites_main.cpp", "src/dsm/edsiimpl/Series.C", "src/dsm/edsiimpl/SiteScreen.C", "src/dsm/edsiimpl/PropertyTask.C", "tests/launch/launchjob.C", "tests/mlp/sites_test.py", "help/src/mlp.md"]
issues: [728]
---
- **Symmetry is ECCE's own, not autosym.** autosym/SymmetryOps work on
  molecular point groups; a periodic host needs its space group with the
  centring and supercell translations. `find_symmetry` tries every integer
  rotation part with entries -1/0/1 that keeps the metric (W^T G W = G) and
  every translation that takes one atom of the rarest species onto another,
  and keeps the (W, t) that map all atoms onto atoms of the same element
  within symprec. Each op carries its atom permutation, which is all the
  vacancy-pair classing needs. Fm-3m conventional cell: 192 ops; 2x2x2
  supercell: 1536. A strongly sheared (non-reduced) cell finds fewer ops:
  fewer reductions, never a wrong merge.
- **Voids = local maxima of the nearest-atom distance** on a grid (0.2 Å),
  climbed to the true maximum by a pattern search (the grid alone ranked
  rutile's 4d channel site, 1.82 Å, below two spurious maxima), then moved to the average of its symmetry images that
  fall within max(0.3, 2 grid) of it, three passes: this puts special
  positions exactly on (1/4,1/4,1/4) etc., which the grid alone misses by up
  to a grid diagonal. Candidates within --min-sep of an accepted orbit are
  dropped (largest clearance wins).
- **Environment uses every periodic image within 4 Å, not the minimum
  image**: in a conventional fluorite cell the six Ca at ±a/2 from 4b are
  three atoms seen twice, and a minimum-image count gave CN 3 for Ca.
  Coordination number = neighbours within 1.1 x the nearest distance (1.2
  pulled fluorite's Ca at 2.73 Å into the 2.37 Å F shell).
- **site-NN.xyz comment lines avoid `mult=`/`charge=`**: Series' xyz reader
  takes those words as multiplicity and charge. The comment uses `weight=`.
- **Series reads `Lattice="..."` from an .xyz** (readXyz) and gives the
  Fragment a LatticeDef; before #728 an xyz source was always molecular.
- References used by tests/mlp/sites_test.py are International Tables
  Vol. A: rock salt 8c and fluorite 4b (Fm-3m), rutile 4d (P4_2/mnm), and the fcc pair shells of a
  2x2x2 supercell (192/48/192/48/16 = C(32,2)).
- **One code path for GUI and CLI**: `SiteScreen` (src/dsm/edsiimpl,
  include/dsm) runs ecce-sites and Series::make; `ecce-site-screen make|results`
  and the Organizer ("New from This" > Screen Sites with MLP..., Site Screening
  Results; hooks `sitescreen`, `sitescreenresults`) both call it. Calcs are
  named host and site-NN because Series names them after the xyz files.
- **The site description is the calculation's Annotation** (`screen=...
  site=N weight=W cn=C env="..."`): arbitrary resource properties are not
  read back by getProp, the Annotation survives Resource::copy and is
  readable without wx. A user editing the Annotation loses that site's
  description.
- **Results are an ensemble calculation "<folder> results"**: a copy of the
  best finished site with its Props removed and GEOMTRACE (final structures,
  sorted), SITEDE (eV) and SITEENV written through
  `EDSIFactory::getVdoc(url)->addProperty(name, istream*)`, as JobStore does
  (`PropertyTask::putProperty` is a no-op). ScanPoint::ensembleKeys accepts
  SITEDE as a third score next to CONFREL/POSESCORE, so the Sites panel is
  EnsemblePropertyPanel: superposition, Export and #708 New calculation from
  selected unchanged. The host (different atom count) is not in the
  ensemble, only in the CLI/JSON rows; members whose atom count differs from
  the first site are dropped. SITEENV values use `_` for spaces because the
  property parser splits on whitespace.
- **MLP had never been through Series**: JCode::theoryNeedsBasis and
  DeckWriter's writeGbs crashed on an MLP calculation stored without a
  theory list/theory; both now guard the null.
- **A step-limited site is usable, flagged.** ecce-mlp ends a Geometry run
  stopped at its step limit as unsuccessful (`NOT CONVERGED`, %notConverged)
  with TE and the last structure stored. `SiteScreen::results` takes
  COMPLETED and UNSUCCESSFUL members alike when TE and a structure exist,
  sets `Row::converged = false` for the latter, prefixes its SITEENV value
  with "not converged; " (the Sites table's Environment column), and the
  report counts them (warn, never block). FAILED/system failure stay out.
- **Organizer hooks** `sitescreendialog <url> [png]` (the dialog's defaults
  and a capture), `sitescreen`, `sitescreenresults`;
  tests/launch/site_screen_test.py --organizer drives them under Xvfb, --mace
  PYTHON uses the real network, --png photographs the Sites panel (Builder
  script `background 1 1 1`, ECCE_OPEN_PANEL=Sites).
- **The Organizer's result line for the results calculation** says "N sites":
  it copies the MLP run type, which ensembleWords would call conformers.
- **A one-site screen had no Sites panel**: `PropertyTask::propertyNames`
  drops GEOMTRACE when it has one step (a single-point trace is not worth a
  panel), and every panel of the ensemble family is keyed on GEOMTRACE. With
  one member (Li in 8-atom NaCl) the key vanished and ECCE_OPEN_PANEL found
  no panel; two or more sites worked. GEOMTRACE now stays when
  `ScanPoint::ensembleSize` > 0 (also a one-conformer CREST run).
- **The results calculation drops the copied site's outputs and job_path**:
  its Outputs/results.h5 added the site's TEVEC to the property list and
  would serve the site's trace through ResultsFromCalc.
- **Series copies carried the template's job_path** (see the Series entry).
  tests/launch/launchjob.C `launch` used the stored job_path as it was, so
  host and sites ran in the template's run directory at once; with real MACE
  (seconds per job) they overlapped and overwrote each other's output (host
  and site 'unsuccessful', dE 0), with the mock they finished before the
  next launch. It was the harness launch, not the Organizer: both paths
  launch through launchjob. launchjob now stores the run directory as
  LaunchBatch does, and the test checks one run directory per calculation.
