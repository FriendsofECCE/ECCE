---
type: pitfall
title: "Windows stream pipes are always blocking: fcntl(O_NONBLOCK) is a stub, so peek before read on the GUI thread"
area: services
paths: ["include/comm/StreamRead.H", "src/comm/commtools/TailSource.C", "src/comm/rcommand/DirectTransportWin.C", "include/util/PosixCompat.H", "src/wxgui/comm/WxTailWindow.C", "tests/transport/streamRead.C"]
issues: [1006, 204]
---
`DirectTransport::openStream` on Windows hands back CRT descriptors on
anonymous pipes (`CreatePipe` + `_open_osfhandle`). They cannot be made
non-blocking: `fcntl` in `PosixCompat.H` returns -1/ENOSYS and changes
nothing. Code written for POSIX ("set O_NONBLOCK, read until EAGAIN")
therefore blocks in `read()` as soon as the pipe is empty.

Seen with the Tail window (Run Mgmt > Tail, #1006): its timer calls
`TailSource::read` on the GUI thread; once `tail -F` had printed the file
and the file stopped growing, the Organizer stopped responding with no
CPU use, its main thread in a synchronous ReadFile (Windows wait reason
"Executive"). The cause was not the broker.

Read such descriptors with `readAvailable()` (`comm/StreamRead.H`): it
peeks the pipe (`PeekNamedPipe`, failure = writer gone) on Windows and
polls first on POSIX. JobStore's `winPoll` uses the same peek.
`ctest -R transport_stream_read` checks it on a blocking pipe, and the
Windows CI runs it.
