# Diagnosing a job that stages but never runs

For the case where ECCE copies the files, reports that the calculation
has started, and the submit script is never executed (issue #141).

This is designed to be done **once**. Everything below is collected in
one pass; there should be no need for a second round of questions.

## What we already know

Not a distribution problem — it has been seen on both Ubuntu and RHEL.
Two plausible causes have already been tested and ruled out, so please
don't spend time on them:

- **`./ecmd` being invoked by a relative path.** That is a real bug
  (#134, fixed after v8.13.0), but it is on a different code path and
  is not what runs the submit script.
- **csh being unable to parse the redirection ECCE uses.** The exact
  command line was tested against real csh and the job runs correctly.

What we do not yet know is whether the submit script is never started,
or is started and dies immediately. That is what step 2 settles.

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
