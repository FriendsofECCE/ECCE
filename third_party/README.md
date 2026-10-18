# third_party

Copies of two projects that ECCE's GROMACS Prepare (MINFF source) runs. Update
both before every release; the rule and the checks are in
`docs/claude/codereg/every-release-updates-bundled-atomipy-and-minff.md`.

## python/atomipy

atomipy 0.97 (MIT, `python/atomipy/LICENSE`), from
https://pypi.org/project/atomipy/0.97/ (sdist `atomipy-0.97.tar.gz`).
The package is unmodified except that the unused structure libraries
`structures/crystals`, `structures/minerals`, `structures/molecules` and
`structures/organics` (70 MB of CIF/cjson data that ECCE never loads) are
left out; `structures/water` is kept. Needs numpy, tqdm and gemmi.

## minff/min.ff

min.ff from https://github.com/mholmboe/minff at commit
`d12b895b9495458a7d0f6f08b1648f5f317068a3` (the same value as `MINFF_COMMIT`
in `scripts/minff_topology.py`). Included with the permission of the MINFF
authors (October 2026); see `minff/README-ECCE.md`. The newest commit when
this was bundled (`22ebd627`) renamed the parameter files and defines, which
`minff_topology.py` and `tests/minff` do not follow yet, so it was not taken.

Installed to `lib/python/atomipy` and `data/minff/min.ff` of the client package.
