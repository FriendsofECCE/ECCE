---
type: pitfall
title: "A plain GROMACS run is a gromacs_md_dynamics task whose parent is a project; a second resource type with the same content type cannot be told apart on reload"
area: codereg
paths: [data/client/config/ResourceDescriptorRxn.xml, data/client/config/ResourceDescriptor.xml, src/dsm/xml/ResourceDescriptor.C, src/apps/organizer/GromacsImport.C, src/comm/commxt/Launch.C, scripts/gensub, scripts/parsers/gromacs.launchpp, data/client/cap/GROMACS.edml]
issues: [1038]
---
A stored resource is mapped back to its type by (resource type, content type,
application type), first match in the descriptor file
(`ResourceDescriptor::getResourceType(TNode*)`). A `gromacs_md_run` type
sharing `ecceMdDynamics`/`GROMACS` with `gromacs_md_dynamics` would resolve to
whichever comes first, so the run is made as `gromacs_md_dynamics` itself
(ContainedBy `project` added; not in the project's ContainsResource, so no
"New" menu entry). `createGromacsRun` builds it.

- Launch: the restart/topology provider search is inside `if (session != 0)`;
  a task with no session takes its own attached start file and topology.
- A ready `.tpr` is stored as `run.gmxtpr` (an auxiliary input of its own
  type, `chemical/x-gromacs-run-tpr`; `.tpr` is the task's output type and
  Launch ships the output list, not inputs). gensub copies it to `topol.tpr`
  and skips grompp; gromacs.launchpp skips its grompp check.
- `MdTask::getSystemName/getCalcName` are constants, so `mdSystemName` and
  `mdCalcName` are non-empty for a task outside a session.
- The MDDynamics editor stays enabled for such a task and would regenerate
  the `.mdp`; a run from files is changed by making a new one.
