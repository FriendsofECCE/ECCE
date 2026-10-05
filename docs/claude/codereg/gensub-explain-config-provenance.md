---
type: map
title: "`GENSUB_EXPLAIN=1 gensub` prints each CONFIG key's effective value and where it came from"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["scripts/gensub", "tests/queues/config_test.py"]
issues: []
---
**For the Register Machines tabs.** Call gensub as for a job
(`GENSUB_EXPLAIN=1 perl $ECCE_HOME/scripts/gensub -p <params>`; only `-H host`
and the usual required options matter, the code and queue manager do not change
the merge). It exits before the submit file is opened, so nothing is generated
or submitted; no network or broker is touched. `readConfig` is the same
function a real run uses and records the layers as it merges, so the output
cannot drift from the job script.

One JSON object per line, keys sorted, ASCII-escaped:

```
{"file":"/home/u/.ECCE/CONFIG.m","key":"shell","overridden":[{"file":"/ecce/siteconfig/submit.site","source":"submit.site","value":"sh"},{"file":"/ecce/siteconfig/CONFIG.m","source":"site","value":"tcsh"}],"source":"user","value":"bash"}
```

- `key`: lower-case. `value`: the effective text (blocks are trimmed, newlines
  are real newlines inside the JSON string), or `null` when the last layer was
  `key: -`, in which case `source` is `"cleared"` and `file` is the clearing file.
- `source`: `submit.site`, `vendor` (CONFIG.VENDOR[.MODEL[.PROC]]), `site`
  (siteconfig/CONFIG.<m>; the server's copy in `-remote`) or `user`
  (~/.ECCE/CONFIG.<m>).
- `overridden`: earlier layers in merge order, each `{source,file,value}`;
  `value` null is a layer that cleared. For a block a user replaced
  (`setup`, `wrapup`, the queue-manager header, `<code>environment`,
  `<code>command`) this holds the site text: show it read-only. Blocks are
  replaced whole, never merged.
- A key in no layer is not printed: the built-in default applies, and the GUI
  must supply that default itself.

`tests/queues/config_test.py` checks the effective values against
`GENSUB_DUMP_CONFIG` and `configdump`, and the tags against files built in the
test.
