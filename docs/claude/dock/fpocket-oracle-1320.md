---
type: map
title: "ecce-pockets against fpocket on the #1320 pocket benchmark"
area: dock
paths: [tests/dock/fpocket_oracle.py, tests/dock/fpocket_oracle.csv, tests/dock/pocket_rank_bench.py]
issues: [1320]
---
Measured 2026-11-28 on nerdegg. ecce-pockets built from 1fcac4c54 (`wip/1320-pocket-rank`), run with
`ECCE_HOME` set to that tree; fpocket 4.2.3 (commit 4bb0d84, `Discngine/fpocket`).
73 complexes, cleaned once by `pocket_rank_bench.py --keep` and fed identically to both tools.
Hit = pocket centre within 8 A of any ligand atom (as in the bench). ecce-pockets: its own reported
centre, pockets in score order. fpocket: mean of the alpha-sphere centres in `pocketN_vert.pqr`,
pockets in fpocket's own order (`pocketN` = rank N).

## fpocket modes
fpocket 4.2.3 has no command-line switch for the expensive calculations: `fparams.c:82` sets
`flag_do_asa_and_volume_calculations = 1` and only mdpocket sets it to 0 (`mdpocket.c:89`).
`descriptors.c:243` computes `set_ASA`, `volume` and `convex_hull_volume` only under that flag.
The final, active assignment in `score_pocket` (`pscoring.c:241-247`) is
`-0.0378 + 0.4846*nas_norm + 0.0909*as_density + 0.000416*convex_hull_volume - 0.003995*surf_pol_vdw14 - 0.004072*surf_apol_vdw14`
(it overrides the earlier assignment at `pscoring.c:94`); pockets are sorted by it (`fpocket.c:206`).
- **default**: `fpocket -f p.pdb`, flag = 1.
- **expensive off**: a second build of the same tag with line `fparams.c:82` changed to `= 0`
  (build with `make CWARN=-w QCFLAGS="-O -g -ansi -Isrc/qhull/src" CFLAGS="-w -Wno-incompatible-pointer-types -DM_OS_LINUX -DMNO_MEM_DEBUG -O2 -g -std=gnu99 -Iplugins/include -Iplugins/LINUXAMD64/molfile" bin/fpocket`;
  stock flags fail on GCC 14 in `fparams.c:296`). With it off the three terms are 0 and the
  score scale changes (about 1.5 against 0.08 for the top pocket of 1r9o).

## Results (all 73 / test half, 36)
| tool | top-1 | top-5 | detection |
|---|---|---|---|
| ecce-pockets | 84.9% / 80.6% | 97.3% / 97.2% | 97.3% (71/73) / 97.2% (35/36) |
| fpocket default | 56.2% / 58.3% | 82.2% / 77.8% | 97.3% / 97.2% |
| fpocket expensive off | 74.0% / 75.0% | 94.5% / 94.4% | 97.3% / 97.2% |

Detection (any pocket hits, any rank) is the same for all three, and the same two complexes
are missed by all three: **1apt, 1hwi**. "fpocket detects, ecce-pockets does not": none.
"ecce-pockets detects, fpocket does not": none (either mode).

Both detect, fpocket ranks higher (all 73): default 1bma(2<4) 1c5x(1<2) 1com(1<3) 2yhx(1<3);
expensive off 1a4g(1<2) 1bma(1<4) 1c5x(1<2) 1com(2<3) 2yhx(1<3).
Both detect, ecce-pockets ranks higher: 26 complexes against the default, 13 against expensive off.
The nine sites ecce-pockets leaves below rank 1 and still detects (1a4g 1ai5 1b9v 1bma 1c5x 1com
1cx2 1hvr 2yhx, ranks 2-4) are found by fpocket too, at ranks 1-11 (default/off: 1b9v 8/11, 1cx2 8/2).

Reading: ecce-pockets' misrankings are ranking, not detection; fpocket finds a pocket at the
ligand in the same complexes, and ranks it first in 3 (default) or 4 (off) of those nine. The two real detection
misses are shared. The per-complex table (code, ligand, pocket counts, first-hit rank per tool,
`none` = no hit) is `tests/dock/fpocket_oracle.csv`.

## Limits
One chain per complex, apo-style protein with the ligand removed, 73 complexes, one run each
(fpocket and ecce-pockets are deterministic here; fpocket's volume term uses Monte Carlo
sampling but is not in the sort for the 'off' mode). The 8 A centre test is lenient for large
pockets. The expensive-off numbers come from a one-line patch, not a supported option.
