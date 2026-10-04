---
type: map
title: "`packaging/dataserver/`"
area: services
section: "The two background services (\"the server\")"
paths: ["httpd.conf.ecce", "packaging/dataserver/"]
---
**`packaging/dataserver/`** — per-user Apache 2.4 + `mod_dav` (config:
`httpd.conf.ecce`), the WebDAV "ECCE Server" — structure library,
basis-set library, saved calculation data, and (as of this fork) help
content and the help CGI backend all served from here, port 8096.
