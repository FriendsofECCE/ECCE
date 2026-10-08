---
type: pitfall
title: "A GROMACS MD study has the application type of its tasks (GROMACS); NWChem's study has its own (MDStudy)"
area: codereg
paths: [data/client/config/ResourceDescriptor.xml, src/apps/organizer/CalcMgr.C, src/dsm/edsiimpl/EDSIFactory.C, tests/apps/builder_overlay_test.py]
issues: []
---
`nwchem_md_study` is `applicationType="MDStudy"` and its tasks `NWChemMD`;
`gromacs_md_study` and its tasks are all `GROMACS`. Code that tells a task
from a study by application type alone treats the GROMACS study as a task:
`CalcMgr::createResource` linked any new `AT_GROMACS` resource into its
parent as a session member, the parent of a study is a project, and
Organizer > New > GROMACS MD Study failed with "Null Pointer Session"
(leaving the study made). Test for a `Session` (`dynamic_cast<Session*>`)
or the content type as well. The Organizer hook `new <parent> <type>`
(ECCE_TEST_ORGANIZER) runs the real `createResource`; `launchjob
gromacsstudy` does not, which is why the GROMACS tests passed.
