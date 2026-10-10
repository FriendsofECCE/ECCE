---
type: map
title: "The release check: where it runs and what switches it off"
area: services
section: "The Gateway window and session end"
paths: ["include/util/ReleaseCheck.H", "src/util/genutil/ReleaseCheck.C", "src/apps/organizer/ReleaseNotice.C", "src/wxgui/wxdialogs/GlobalPrefs.C", "tests/releasecheck/releasecheck_test.C"]
issues: [273]
---
The Organizer asks the GitHub releases API for a newer release at most once
a day (wxWebRequest, asynchronous, User-Agent `ECCE/<version>` only) and
shows a wxInfoBar. Which release counts is decided in the wx-free
`ReleaseCheck::newerRelease`: same major version; a preview install sees
newer previews and finals, anything else only finals; drafts are skipped.

- Off: preference `UpdateCheck` (Edit > Preferences > General) or
  `ECCE_NO_UPDATE_CHECK=1`. Off means no request and no preference writes.
- Test hook: `ECCE_UPDATE_CHECK_URL` is an http(s) URL or `file://` path to
  a JSON file; a file path is read directly, so a headless Organizer needs
  no network.
- State lives in the global preferences: `UpdateCheck.Last` (a day gate that
  is stamped before the request, so a dead network is retried tomorrow),
  `.Found`/`.FoundUrl` (the cached answer shown on the days in between),
  `.Dismissed` (the version the user said not to hear about again).
- Failures are one line on stderr, never a dialog.
