# Offloading long builds and test suites

Full builds and `tests/teaching` take long enough to overload the developer
machine. `run-offload.sh` runs them on radium instead (carbon on request).

    tools/offload/run-offload.sh [--host radium|carbon|auto] <git ref> <command> [args]

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

| | carbon | radium |
| --- | --- | --- |
| role | only with `--host carbon`: it switches off under full load until its power supply is replaced | everything (`--host auto`) |
| system | Debian 13 native, i7-4930K 6c/12t, 31 GB | Debian 12, 6c/12t, 15 GB; build and tests in the `ecce-trixie-build` rootless podman image (`Containerfile`, `--memory 12g`) |
| work root | `~/ecce-offload` | `~/ecce-offload` |
| concurrency | 1 job, `ninja -j8`, `ctest -j4` (its power supply is old) | 2 jobs, `ninja -j6`; nothing started 17:00-21:00 |

Each host keeps the 8 most recent build trees (8-16 GB each) and removes older
ones before a job; a job refuses to start with less than 30 GB free. The CPU
temperatures (`sensors`, package lm-sensors) are logged at the start and end
of each job. A host that does not answer within 5 s is skipped; if none is
reachable the script exits 3 and says so; run the command locally then.
Slots are `flock` files in `<work root>/locks`, so waiting jobs queue
instead of failing. One job per ref at a time (the build dir is per ref).

Packages on a native host: the list in `Containerfile`, plus `gromacs
python3-numpy python3-tqdm python3-gemmi quantum-espresso
quantum-espresso-data-sssp libeigen3-dev libxc-dev lm-sensors smartmontools`.

## Layout on a host

`<root>/src` is a shallow clone (`--depth 50`, fetching only `main` and the
requested branch); `wt/<ref>` a detached worktree per ref; `build/<ref>` the
persistent build tree (`b` normal, `bi` + `install` for `apps`), so a second
run of a ref is incremental; `home/<ref>` is `$HOME` for the job (native
hosts also set `XDG_CACHE_HOME`, `TMPDIR` under the root); `logs/`.

## Rebuilding radium's image

Only when the CI package list changes: copy `Containerfile` to
`~/ecce-offload/image` on radium and `podman build -t ecce-trixie-build .`
there (about 10 min, ~700 MB of downloads, so not casually).
Inside the container `$USER` is set explicitly; without it the data server
creates no account and every teaching case fails with "no such parent".
