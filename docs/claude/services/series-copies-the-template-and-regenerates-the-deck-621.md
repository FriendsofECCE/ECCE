---
type: map
title: Series (#621) copies the template calculation and regenerates the deck through DeckWriter
area: services
paths: [src/dsm/edsiimpl/Series.C, src/dsm/edsiimpl/DeckWriter.C, src/apps/launch/ecce-series.C, src/apps/calced/ESInputController.C, tests/series/series_test.py, tests/launch/launchjob.C, tests/apps/deck_regen_test.py, tests/apps/another_code_test.py, src/apps/organizer/AnotherCodeDialog.C]
issues: [621, 728, 1037]
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
- New from This > In Another Code (#1037) is `Series::first` (the first calculation,
  the user's setup step in the Calculation Editor) then `Series::make` with the
  first as template; both load a source through one `loadStructure`. The editor
  is a separate process, so the Organizer cannot see it close: a small window
  with "Make the Others" ends the setup step. `Series::makeReady` generates the
  first one's deck when the editor left it unsaved.
