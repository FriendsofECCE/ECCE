---
type: pitfall
title: "ORCA RI summaries need legacy keys and effective output settings"
area: codereg
paths: ["include/dsm/OrcaRIFitting.H", "scripts/parsers/ai.orca", "scripts/parsers/orca-ri.defaults", "scripts/parsers/ORCA.expt", "src/dsm/edsiimpl/DavCalculation.C", "src/apps/calced/CalcEd.C", "src/apps/organizer/CalculationContextPanel.C", "tests/apps/calced_basis_box_test.py", "tests/apps/summary_test.py"]
issues: [894]
---
`Parameters/SetupParams` can contain the older `ES.Theory.SCF.UseRIJCOSX`
checkbox instead of `ES.Theory.SCF.RIMode`. The generator accepts both;
an explicit mode wins. An absent auxiliary basis uses the shared
`orca-ri.defaults`, also read by the summaries. RI-J auxiliary fitting
belongs in CD Fitting; COSX does not supply an XC fitting basis.

No RI keyword in the generated input does not prove the run used no RI.
The real ORCA 6.1.1 B3LYP water output `h2o_smd.out` echoes no RI keyword,
but reports def2/J and `RIJ-COSX ... on`. Read the effective SCF flags and
reported auxiliary basis, not the author banner's mention of RIJCOSX.
The importer preserves those settings; older calculations without RI keys
fall back to their stored output/input. A calculation with neither RI keys
nor an output cannot reveal an engine-selected default. Do not invent a
functional table for that case.

Theory Details sends changes through `CalcEd::processTheoryInput`.
Refreshing its theory rows alone leaves CD Fitting stale: refresh the Basis
Set fields on live updates and after the initial held values arrive.
The combined theory line belongs to the Organizer; the editor uses a theory
choice and separate EDML summary rows.
