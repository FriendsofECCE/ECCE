# Diagnosing a job that stages but never runs

For the case where ECCE copies the files, reports that the calculation
has started, and the submit script is never executed (issue #141).

This is designed to be done **once**. Everything below is collected in
one pass; there should be no need for a second round of questions.

## The cause, confirmed

**This was found on 2026-09-25 and is fixed in 8.13.2.** If you are
reading this because your jobs stage and never run, check this first;
you probably do not need the rest of the document.

ECCE backgrounds the submit script with this line:

    nohup ./submit__<name> > /dev/null 2>&1 & echo RC_EXECBG_PID=$!

`> /dev/null 2>&1` is Bourne-shell syntax. csh reads it as **two**
output redirections -- `2` is an ordinary word and `>&1` is csh's own
redirect-both operator -- and rejects the command with:

    Ambiguous output redirect.

Nothing is started and no process id comes back. ECCE reports the
calculation as started anyway, because the code that backgrounds a job
treats "command sent" as success. Hence: files staged, job reported
running, no output, no error.

**Why it took so long to see.** The two csh implementations disagree.
Debian's default `/usr/bin/csh` is `bsd-csh`, which accepts the Bourne
form; Ubuntu and RHEL ship `tcsh` as `/usr/bin/csh`, which rejects it.
The same ECCE build therefore launches jobs on one machine and silently
fails on another, with a working `csh` installed on both. An early test
of "can csh parse this?" was run against bsd-csh and passed, and that
wrong answer was recorded here as fact -- which is why this section now
leads with evidence rather than with a conclusion.

**The lesson for future testing:** test against `tcsh` by name, never
against whatever `csh` happens to resolve to locally.

The fix sends the redirection in the dialect of the shell that will
parse it -- `>& /dev/null` for csh and tcsh, the Bourne form only where
the machine is registered as using bash. That is the same csh syntax
ECCE's own generated submit scripts have always used.

**Workaround without upgrading:** in Machine Registration, set the
machine's shell to `bash`. Only ECCE's connection to the machine
changes; the submit script is still generated as csh and still runs
under csh.

## If it still happens on 8.13.2

Then it is something else, and the rest of this document applies. The
collector now tests both redirect forms, so its output tells you
immediately whether you are looking at this bug again or a new one.

## Step 1 — reproduce it once, normally

Build and submit the job the way you normally do, and let it fail.

Note the **run directory** — the path ECCE reports when it says it has
copied the files. Something like
`~/ecce-jobs/<calculation-name>` or wherever your machine registration
points.

Do not delete it. Steps 2 and 3 read it.

## Step 2 — run the collector

```
ecce-diagnose /path/to/that/run/directory
```

It writes `ecce-diagnostics-<host>-<date>.txt` in the current
directory and prints the name.

It reads everything relevant — OS and versions, which shells exist and
what `csh` actually resolves to, whether the shells behave the way
ECCE's launch code assumes, whether the computational codes are on the
PATH a *non-interactive* shell sees (a code present in your login
shell but absent there is a job that dies the instant it starts),
SELinux state, the machine and queue configuration, which services are
listening, and the full contents of the run directory including every
log.

It changes nothing, and **it does not collect any password or
credential file**. It does one thing that runs rather than reads: it
copies the run directory aside and launches the submit script there
exactly as ECCE does, so we learn whether the script works when
started the same way. Your real run directory is not touched.

## Step 3 — one trace we cannot get any other way

The collector cannot see the conversation between ECCE and the shell,
and that is where the answer probably is. Start ECCE with logging on
and repeat the submission:

```
ECCE_RCOM_LOGMODE=1 ecce > ecce-trace.txt 2>&1
```

Then build and submit the same calculation again and let it fail.
Quit ECCE.

`ecce-trace.txt` will contain the command ECCE sent to the shell and
what the shell replied — including whether the `cd` into the run
directory succeeded, whether the script was found executable, and what
came back from the submit itself.

## Step 4 — send both files

`ecce-diagnostics-*.txt` and `ecce-trace.txt`.

Both are plain text and both contain host names, user names and full
paths, so read them first if that matters to you.

## What each outcome will mean

So you can see this is a real decision tree and not a fishing trip:

| In the diagnostics | What it means |
|---|---|
| `ecce.submit.log` absent in the run directory, present in the reproduction copy | ECCE never started the script; the fault is in the launch path |
| `ecce.submit.log` absent in both | the script itself fails immediately — most likely the code is not on the non-interactive PATH |
| `ecce.submit.log` present in the run directory | the script did run; the fault is later, in monitoring |
| "the backgrounded job DID NOT RUN" under your `csh` | the shell does not behave as the launch code assumes on your system |
| no `submit__*` script at all | generation failed upstream, before any of this |
