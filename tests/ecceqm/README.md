# tests/ecceqm

`fixtures/o2-roks`: a calculation folder (local data mode) holding triplet O2,
B3LYP/6-31G*, restricted open-shell, run with the bundled engine through Launch
and the job monitor. Made with `tests/launch/ecceqm_test.py --export DIR`; used
by `tests/apps/ecceqm_builder_test.py` (ctest `apps_ecceqm_builder`) and
`tools/screenshots/qm_shots.py`.

`fixtures/h2o-geometry`, `h2o-vibration`, `h2o-geovib`: water, B3LYP/6-31G*, Geometry,
Vibration and GeoVib runs, made the same way (`--export`, 2026-10-11); used by
`tests/apps/ecceqm_vib_builder_test.py` (ctest `apps_ecceqm_vib_builder`).

`fixtures/o2-roks-loc`: the same O2 triplet ROKS run with the engine that writes
a localised set per spin (`LOCMO*` and `LOCMO*BETA` properties, 2026-10-25); used by
`tests/apps/ecceqm_motable_test.py` for the open-shell Localised MO table.
