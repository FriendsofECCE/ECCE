---
type: pitfall
title: "Windows: a text-mode stream's tellg() is not the length of what it reads (DAV PUT 400)"
area: services
paths: ["src/dsm/dav/EcceDAVClient.C", "src/apps/calced/ESInputController.C", "src/comm/commxt/JobStore.C", "src/comm/commxt/Launch.C"]
issues: [247]
---
**A Windows client saving to a data server got "input file copy to DAV
failed" for every input deck** (Apache: 400, "An error occurred while
reading the request body"). The deck is written by Strawberry Perl with
CRLF line ends and handed to `putInputFile()` as a text-mode `ifstream`.
`EcceDAVClient::getStreamSize()` took the Content-Length from
`seekg(end)/tellg()`, which counts bytes on disk, while reading the
stream drops each CR: the body came up short and the server gave up.
Local mode never noticed, because it copies the file.

On Windows the size is now counted by reading the stream, so what is sent
is LF-only text and the length matches. Binary files must still be opened
with `ios::binary` by the caller: a text-mode read also stops at a
Ctrl-Z byte.

Test: `tests/windows/central_win.py` (a Windows client against a central
server: first-start answer, login through `-pipe`, a calculation made and
its input saved and read back over WebDAV).
