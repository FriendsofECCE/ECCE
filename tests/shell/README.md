# tests/shell — the local shell connection, for real

Everything ECCE does on a compute machine — submit a job, ask whether a
file exists, start job monitoring — goes through one class,
`RCommand` (`src/comm/rcommand/RCommand.C`), talking to a shell over a
pty and matching its output with `expect` patterns. The machine's shell
is a per-machine registration setting: `csh`/`tcsh` or `bash`.

That makes the shell *dialect* a correctness surface, and it has been a
buggy one. This suite opens a real connection to the local machine, once
per dialect, and checks the handshakes the rest of the system depends
on.

```
tests/shell/run_tests.py            both shells
tests/shell/run_tests.py --shell bash -v
tests/shell/run_tests.py --keep     keep the work directories
cd build-cmake && ctest -R shell --output-on-failure
```

It needs the build tree only — no install, no ECCE services, no X. When
`mopac` is on `PATH` it also runs a real calculation through the real
submission path.

## The probe

`shellprobe.C` is linked against the compiled `RCommand`, not a
reimplementation of it. That is the point: the bugs below were all in
the real class's interaction with a real shell on a real pty, and a
simulation of the connection reproduced none of them — the #69
investigation says so explicitly, twice.

## What each check is about

| check | the bug it keeps fixed |
| --- | --- |
| `exec.output`, `exec.failure-detected` | `execout()`'s exit status: csh's `$status`, bash's `$?` |
| `glob.expanded` | #59 — the local connection hardcoded csh's `-f` ("skip .cshrc"), which in bash means **disable globbing**. Every wildcard in every command silently stopped expanding. |
| `echo.exact-match` | #69 bug 2 — `JobStore::initMon()` waits for a byte-for-byte echo of the command it sent. With readline enabled, a command wider than the terminal comes back with readline's horizontal-scroll `<` marker in place of the real text, so the match never succeeds and job monitoring hangs at "submitted" forever. Hence the deliberately over-long command here. |
| `bg.jobid-is-a-pid` | `execbg()` reports `$!` as the job id; Launch parses it. |
| `bg.survived-teardown` | #69 bug 1 — the connection that submits a job is destroyed as soon as the submit returns, which hangs up the pty. The job must not die with it. |
| `mopac.completed-after-teardown` | the same thing with a real code actually computing. |

The backgrounded job installs its own `SIGHUP` handler on purpose.
`nohup` protects a job by setting `SIGHUP` to `SIG_IGN`, which children
inherit — but a code that installs a handler of its own throws that
protection away, and Gaussian does exactly that, which is how
"Error: hangup" reached it in #69. A job that only inherits `SIG_IGN`
would pass this check without proving anything.

## Adding a shell

`run_tests.py --shell zsh` already works if the dialect is handled in
`RCommand::exec()` and `shellCommand()`. A shell that is not installed
is skipped, not failed.
