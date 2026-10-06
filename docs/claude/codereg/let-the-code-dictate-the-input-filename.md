---
type: checklist
title: "Let the *code* dictate the input filename, not the checklist"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: [".in", "packaging/dataserver/httpd.conf.ecce", "data/client/config/mimetypes", "scripts/gensub"]
issues: [86]
---
**Let the *code* dictate the input filename, not the checklist.** The
["use a distinctive extension, not `.in`/`.out`"](datafiles-filenames-need-a-distinctive-extension.md) rule is about
MIME mapping, and following it naively broke MOPAC: MOPAC
derives its output name by stripping a known extension **by
substring**, so `mopac.mopin` makes it write a file literally named
`mopac    in.out`. Pick whatever extension the code demands, add the
`AddType` and `mimetypes` line for *that* (both the data server and
local data mode type files by extension); and if the code forces a generic output name,
symlink the declared name onto it in `gensub` **before** the run —
renaming afterwards is too late, live monitoring needs to tail the
file during the job.
