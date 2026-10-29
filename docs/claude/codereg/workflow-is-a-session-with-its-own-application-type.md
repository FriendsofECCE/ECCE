---
type: map
title: "A Workflow is a Session of application type Workflow; its steps are ordinary *_es calculations"
area: codereg
paths: [data/client/config/ResourceDescriptor.xml, data/client/config/ResourceDescriptorRxn.xml, src/apps/organizer/WorkflowPanel.C, src/apps/organizer/WorkflowSteps.C, src/dsm/edsiimpl/EDSIFactory.C, tests/apps/workflow_test.py]
issues: [341]
---
`workflow` (contentType ecceSession, applicationType Workflow, factory
category WorkflowSession) is stored like an MD study: a collection with a
`linkbase.xml`. Steps are the existing `*_es` types, made inside it and
linked with INPUT_PROVIDER links named `geometry`; `Session::addMemberAsTarget`
is not used (it needs a RootTask type), `WorkflowSteps::create` adds the
member and the link itself. Both ResourceDescriptor files need the type and
the `ContainsResource name="workflow"` on `project`. A new code is offered in
a workflow by adding it to `WorkflowSteps::codes()` and to the workflow's
`Contains`. The hooks `workflowstep`, `workflowinfo`, `workflowready`,
`workflowshot` run through ECCE_TEST_ORGANIZER.
