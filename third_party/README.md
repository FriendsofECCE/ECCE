# third_party

Copies of two projects that ECCE's GROMACS Prepare (MINFF source) runs. Update
both before every release; the rule and the checks are in
`docs/claude/codereg/every-release-updates-bundled-atomipy-and-minff.md`.

## python/atomipy

atomipy 1.1 (MIT, `python/atomipy/LICENSE`), from
https://pypi.org/project/atomipy/1.1/ (sdist `atomipy-1.1.tar.gz`).
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

`minff/unit_cells/` holds the 34 mineral unit-cell `.pdb` files of the same
commit's `Systems/UC_conf` (the `.log` files are left out). They are what the
Prepare editor's unit-cell Browse... opens; installed to `data/minff/unit_cells`.
They come from the same repository and commit as min.ff and go out under the
same permission, which covers everything in the repository
(`minff/README-ECCE.md`).

## clayff/clayff.ff

ClayFF for GROMACS, written for ECCE by transcribing Tables 1 and 2 of Cygan,
Liang and Kalinichev, J. Phys. Chem. B 2004, 108, 1255, and the M-O-H bend and
Morse O-H of Pouvreau et al., J. Phys. Chem. C 2017, 121, 14757 (full citations
and the unit conversions in `clayff.ff/forcefield.doc`). Only numbers from the
papers are used. Installed to `data/clayff/clayff.ff`; selected with
`minff_topology.py --forcefield clayff`; checked by `tests/clayff`.
