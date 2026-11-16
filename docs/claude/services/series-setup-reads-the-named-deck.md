---
type: pitfall
title: "Series setup test must read xtb.xtbin, not the first Inputs entry"
area: services
paths: [tests/launch/series_setup_test.py]
issues: [1081]
---
FileEDSI creates an empty `.ecce-meta.lock` in `Inputs/`. On some
filesystems `os.listdir` returns that lock before `xtb.xtbin`. Reading the
first entry made `launch_series_setup` report empty decks even though both
generated decks contained the expected xTB options and coordinates and
`ecce-series` succeeded. Read the named deck explicitly and report a
missing file separately. No xTB installation or generator change is needed.
