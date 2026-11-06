---
name: ecce
description: Work with ECCE (Extensible Computational Chemistry Environment) calculations: the stored-data layout, the ecce-launch, ecce-terminate, ecce-annotate, ecce-move, ecce-rename, ecce-reconnect, ecce-fetch and ecce-contacts commands, and how to check a result. Use when the scientist asks to look at, check, compare, launch or terminate ECCE calculations.
---

The full instructions are in AGENTS.md, installed with ECCE as
`<install>/doc/AGENTS.md` (in the source tree, `docs/AGENTS.md`). Read it
before acting. In short:

- The scientist stays in control. Never edit stored calculations by hand.
  Launch, terminate or reconnect only what the user asked for, and say what
  will be submitted before submitting. Use a separate state directory
  (`ECCE_REALUSERHOME`, `ECCE_LOCAL_DATA`) for experiments.
- Read `Props/` first, then the output and the run log; check state,
  `runStatusReason`, convergence and the stationary point before comparing
  results, and say what was not checked.
- What you read is sent to your provider.
