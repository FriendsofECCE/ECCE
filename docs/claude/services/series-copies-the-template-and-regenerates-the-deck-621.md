---
type: map
title: Series (#621) copies the template calculation and regenerates the deck through DeckWriter
area: services
paths: [src/dsm/edsiimpl/Series.C, src/dsm/edsiimpl/DeckWriter.C, src/apps/launch/ecce-series.C, src/apps/calced/ESInputController.C, tests/series/series_test.py, tests/launch/launchjob.C, tests/apps/deck_regen_test.py]
issues: [621, 728]
---
`Series::make` copies the template (`Resource::copy` to a new URL in the folder,
so the stored code, theory, GUI values, basis and `launch_*` properties come
along), `resetForRerun`, stores the new structure, charge and multiplicity, calls
`DeckWriter::rebuildBasis` only when the unique tag string differs from the
template's, and `DeckWriter::generate`. The Calculation Editor's Save uses the
same `DeckWriter::generate` (no second copy); `apps_deck_regen` keeps the decks
of eight codes byte for byte (stored under `tests/apps/fixtures/decks/`).
- `generate` prepends `Ecce::ecceDataControllersPath()` to PATH: the basis
  translators `std2<code>` are run by name and a terminal has no wrapper.
- A calculation's `Inputs/` folder must exist for `putInputFile`; git drops
  the empty folder from a fixture, so tests create it.
- The template's theory is kept: a triplet under a restricted theory gets a
  warning note, not a changed theory.
- `rebuildBasis` does not call the editor's `makeRuntypeNoSphericalConsistent`
  (GUI state).
- `make` clears the copy's job data (`jobdata(Jobdata())`): the copied
  `job_path` is the template's run directory. The Organizer's Launch and the
  Launcher recompute it from the copy's URL, but anything that reads it
  before a launch used the template's: Clean Run Directory (RunMgmt::
  cleanServerDirs, `rm -f dir/*`) on an unlaunched copy would empty the
  template's directory, and tests/launch/launchjob.C ran every copy there
  (#728).
