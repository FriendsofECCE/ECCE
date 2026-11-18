---
type: pitfall
title: "Preferences Data hook: decode underscores only in display names (#1103)"
area: services
paths: [src/wxgui/wxdialogs/GlobalPrefs.C, tests/apps/prefs_data_test.py]
issues: [767, 1103]
---
`prefs data` encodes spaces as underscores in server display names only.
Screenshot filenames, URLs, SSH usernames and remote folders are literal.
Decoding every token changes an underscore-containing screenshot directory
into a nonexistent directory. The first `snap` returns `not saved`; a failed
save inside the Add Server modal dialog can leave subsequent commands with
no answer. This is a path parsing failure, not a missing Data page.

The defect was present when the hook was introduced in `a8df2a960`.
`apps_prefs_data` includes an underscore-containing screenshot filename;
setting `ECCE_SHOTS_DIR` to a directory containing underscores exercises the
same path through `snap`, `form` and `add`.
