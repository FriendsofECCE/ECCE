---
type: checklist
title: "`<DataFiles>` filenames need a distinctive extension"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["$STATEDIR/httpd.conf", ".edml", ".in", "httpd.conf.ecce", "packaging/dataserver/httpd.conf.ecce", "data/client/config/mimetypes", "src/dsm/edsiimpl/FileEDSI.C", "tests/filedsi"]
issues: [38]
---
**`<DataFiles>` filenames need a distinctive extension**, not
something generic like `.in`/`.out` — Apache has no built-in MIME
mapping for those, so the uploaded file's `Content-Type` silently
becomes `DefaultType text/plain`, and ECCE's mimetype-filtered
"find the primary input file" lookup then can't match it against
the `.edml`'s declared mimetype *even though the file exists on
disk* — looks exactly like a save failure, isn't one. Add a matching
`AddType` line to `packaging/dataserver/httpd.conf.ecce` (see the
other codes' `.g16in`/`.nw`/`.gki`-style entries there). This is a
per-user template resolved into `$STATEDIR/httpd.conf` fresh on
every `ecce-dataserver-start` — but only when the dataserver isn't
already running (early exit if the port's listening), so testing a
`httpd.conf.ecce` change needs `ecce-dataserver-stop` first, not
just an app relaunch.

Local data mode types files without Apache, from
`data/client/config/mimetypes` (the same table; `tests/filedsi` checks the
two agree): add the extension there too. An extension missing from it
gets the bare extension as its type, so the lookup fails there as well.
