# Offloading long builds and test suites

Full builds and `tests/teaching` take long enough to overload the developer
machine. `run-offload.sh` runs them on one of two other machines instead.

    tools/offload/run-offload.sh [--host radium|tellurium|auto] <git ref> <command> [args]

`<git ref>` is a branch, `origin/<branch>` or a commit that is **already
pushed to GitHub**: the host fetches from GitHub, not from this machine.

| command | does |
| --- | --- |
| `build` | configure (first time only) and `ninja -k 0` |
| `ctest [-R re ...]` | build, then `ctest -j --output-on-failure` |
| `teaching [args]` | build, then `tests/teaching/run_tests.py --jobs 4 [args]` |
| `apps [args]` | build and install into the build dir, then `tests/apps/run_tests.py` |
| `run <cmd...>` | build, then any command in the checkout (`$SRC`; build tree `$BLD/b`) |

The full log stays on the host (`<host>:.../logs/<time>-<ref>-<pid>.log`);
only the last 25 lines and one `PASS:`/`FAIL:`/`SKIP:` line come back. The
exit status is the command's (77 is SKIP). Examples:

    tools/offload/run-offload.sh origin/main build
    tools/offload/run-offload.sh wip/foo ctest -R 'parsers|tls_'
    tools/offload/run-offload.sh --host radium origin/main teaching --group B

`run-on-radium.sh` is `run-offload.sh --host radium`.

## Hosts

| | radium | tellurium |
| --- | --- | --- |
| role | long test suites | builds |
| system | Debian 12, 6c/12t, 15 GB; build and tests in the `ecce-trixie-build` rootless podman image (`Containerfile`, `--memory 12g`) | Debian 13 native, 4c/8t, 31 GB |
| work root | `~/ecce-offload` | `/mnt/games/ecce` only (never the root disk) |
| concurrency | 2 jobs, `ninja -j6` | 3 jobs, `ninja -j3` |

`--host auto` (the default) prefers tellurium for `build`/`ctest` and radium
for `teaching`/`apps`, takes the other if the first has no free slot, and
queues on the first reachable host if both are full. **tellurium is someone's gaming computer and is used only in its owner's hours** (until 2026-10-08 14:00, then evenings 17-21 and weekend mornings 07-12; a job starts only with an hour left, see `tellurium_free` in the script). radium starts nothing from 17:00 to 21:00
(in use 18-21). A host that does not answer within 5 s
is skipped. If
no host is reachable the script exits 3 and says so; run the command locally
then. Slots are `flock` files in `<work root>/locks`, so waiting jobs queue
instead of failing. One job per ref at a time (the build dir is per ref).

## Layout on a host

`<root>/src` is a shallow clone (`--depth 50`, fetching only `main` and the
requested branch); `wt/<ref>` a detached worktree per ref; `build/<ref>` the
persistent build tree (`b` normal, `bi` + `install` for `apps`), so a second
run of a ref is incremental; `home/<ref>` is `$HOME` for the job (tellurium
also sets `XDG_CACHE_HOME`, `TMPDIR` under the root); `logs/`.

## Rebuilding radium's image

Only when the CI package list changes: copy `Containerfile` to
`~/ecce-offload/image` on radium and `podman build -t ecce-trixie-build .`
there (about 10 min, ~700 MB of downloads, so not casually).
Inside the container `$USER` is set explicitly; without it the data server
creates no account and every teaching case fails with "no such parent".
