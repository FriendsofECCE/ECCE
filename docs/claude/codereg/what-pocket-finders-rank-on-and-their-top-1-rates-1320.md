---
type: map
title: What published pocket finders rank on, and the top-1 rates they report
area: codereg
paths: [src/dock/PocketFinder.C, include/dock/PocketFinder.H, tests/dock/pocket_rank_bench.py]
issues: [1320]
---
Read from the primary papers (full text via Europe PMC). A number the paper does
not state is marked "not stated". Success criteria differ between papers, so the
rates are not interchangeable.

**LIGSITE / LIGSITEcsc** (Huang & Schroeder, BMC Struct Biol 2006, 6:19). Grid
protein-solvent-protein scans (our family). Clusters are ranked by number of grid
points only; the top three are kept. LIGSITEcsc re-ranks those three by mean
conservation of residues within 8 A of the cluster centre (needs ConSurf-HSSP, so
not usable here). Benchmark: 48 bound/unbound pairs (19 from PASS, 29 from Chen
et al.) plus 210 bound structures from the PLD. Top-1: LIGSITEcs 60 % unbound and
69 % bound, LIGSITEcsc 71 % and 79 %. Conservation re-ranking is what moved
top-1; the geometric ranking by size alone did not.

**PocketPicker** (Weisel, Proschak, Schneider, Chem Cent J 2007, 1:7). Grid
buriedness: scans along 30 directions per grid probe, buriedness index; pockets
ordered by the number of grid points ("largest concavity"). Same 48-pair set.
Top-1 69 % unbound, 72 % bound; top-3 85 % both (these figures as tabulated in
the fpocket paper).

**fpocket** (Le Guilloux, Schmidtke, Tuffery, BMC Bioinformatics 2009, 10:168).
Voronoi alpha spheres. Score is a 3-component PLS model fitted to ligand coverage
from five descriptors: normalised number of alpha spheres, normalised mean local
hydrophobic density, normalised proportion of apolar alpha spheres (an alpha
sphere is apolar when it touches at least 3 low-electronegativity atoms, mostly
C and S), polarity score of the lining amino acids, and alpha-sphere density. The
fitted weights are not stated in the paper. Top-1 / top-3 (PocketPicker
criterion), same 48-pair set: 83 / 92 % bound, 69 / 94 % unbound. Cheng et al.
set (20 files): 75 / 95 %. Astex Diverse set (85): 67 / 82 %.

**P2Rank** (Krivak & Hoksza, J Cheminform 2018, 10:39). Random forest over
local surface features, trained on CHEN11 (251 proteins, 476 ligands). DCC
criterion, 4 A, "Top-n" (n = number of ligands). COACH420 (420 chains): fpocket
56.4 %, P2Rank 72.0 %, P2Rank using protrusion alone 64.2 %. HOLO4K: fpocket
52.4 %, P2Rank 68.6 %. HOLO4K's size is not stated in the sections read. The
paper says fpocket "finds most of the known binding sites, but they are not
always ranked at the top", and that re-scoring them (PRANK) raised COACH420
Top-n from 56.4 to 63.6 %.

**Reading for ECCE.** The published geometric-only finders reach top-1 of about
60-70 % on the 48-pair set and 52-56 % on COACH420; ours measured about 40 % on
a harder own set, so the gap is mostly the benchmark. Buriedness, apolar
(hydrophobic) fraction and size all appear among the published ranking
descriptors; pocket-mouth size does not appear in any of the four. Benchmark sets
to use if a larger one is wanted: the 48 pairs (Chen/PASS), Astex Diverse (85),
Cheng (20), COACH420 (420), HOLO4K, CHEN11 (251).
