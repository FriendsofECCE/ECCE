---
type: pitfall
title: "A copy-back from a host that cannot be asked used to 'succeed' with nothing copied, and the job store stored an empty main output (#732)"
area: services
paths: ["src/comm/rcommand/RCommand.C", "src/comm/rcommand/OpenSshTransport.C", "src/comm/rcommand/SshTransport.C", "src/comm/commxt/JobStore.C", "include/comm/OutputCopyCheck.H", "scripts/eccejobmonitor"]
issues: [732]
---
`RCommand::transportCopy` keeps scp's habit of skipping a missing source.
It used to call any source missing when `remoteKind()` returned < 0, and
`remoteKind()` returned -1 for a failed `run()` too (dead login, a 30 s
timeout). Every source then counted as missing, the copy returned true
("Successfully copied files from server"), and `calcPutOutFileName` stored
the main output it could not open as a 0-byte file. A retained `.gbw` was
missing from the store at the same time, because that one is checked for
existence first. Those two together, plus a "copy" that took about 2 min,
are how this shows in a run log.

Now `remoteKind()` returns -2 when the host could not be asked, and the
copy then fails. The monitor sends each file's size on the host as
"1:<bytes>" in the count field of `jmFILE`; an older store reads it with
atoi. The store copies a file again when its copy is missing or shorter
than that size, never stores a file that did not arrive (or arrived
empty while the host's copy has data), and turns Complete into
Unsuccessful with "The output copied back is incomplete: ..." when the
main output is short or lacks the code's normal-end line
(`OutputCopyCheck::normalTerminationMarker`, the same lines as gensub's
table plus CP2K's `PROGRAM ENDED AT`). A runStatusReason left by a
failed reconnect login ("Could not reconnect to the job ...") is about
contact with the job, not about the job, so the job's own reason always
replaces it, even a generic "(exit code N)" one.
