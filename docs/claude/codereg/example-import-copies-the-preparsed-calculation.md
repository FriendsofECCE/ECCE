---
type: map
title: "Example import copies a pre-parsed calculation; the parse is the fallback"
area: codereg
paths: ["data/client/examples", "src/dsm/edsiimpl/StoredCalcCopy.C", "src/apps/organizer/CalcMgr.C", "tools/examples/preparse.py", "tests/apps/examples_preparsed_test.py"]
issues: [448, 444]
---
Each file-output example holds `<example>/stored/<calc>/` (the calculation
folder as the data layer writes it: files plus `.ecce-meta` sidecars) and
`<example>/stored/stamp` (`<format> <FNV-1a-64 of the output> <size>`).
`CalcMgr::openExampleSet` copies it through the EDSI (`StoredCalcCopy::copy`:
PUT per file, PROPPATCH per resource, so data server and local mode alike)
when the stamp matches the shipped output, and calls the importer otherwise.
`ECCE_NO_PREPARSED=1` forces the parse. The GROMACS example is a directory and
always parses.

- The copy leaves out what belongs to the importing session (`kSkip`: owner,
  dates, job path, launch host); the target's own are set by `createChild`.
  Keep `kSkip` and `VOLATILE` in `examples_preparsed_test.py` equal.
- The stored files are generated: change an example or a parser, install, run
  `tools/examples/preparse.py`, commit. A stale copy is not silent:
  a changed output falls back to the parse (stamp), but a changed parser keeps
  its old stored result until `apps_examples_preparsed` (parse arm against copy
  arm in fresh states) fails.
- Bump `kFormatId` in `StoredCalcCopy.C` and `stamp()` in `preparse.py`
  together when the copy or the stored layout changes.
- Packaging needs nothing: `data/client` is installed as a whole, dot-files
  included (deb, rpm, MSI and dmg alike).
