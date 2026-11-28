# tests/dock

`pocket_rank_bench.py` and `fpocket_oracle.py` need `ECCE_HOME=<source tree>`; see
`docs/claude/dock/fpocket-oracle-1320.md`. To build fpocket for the oracle (MIT, run only, never
installed): `git clone --branch 4.2.3 https://github.com/Discngine/fpocket.git && cd fpocket &&
make -j2 CWARN=-w QCFLAGS="-O -g -ansi -Isrc/qhull/src" CFLAGS="-w -Wno-incompatible-pointer-types
-DM_OS_LINUX -DMNO_MEM_DEBUG -O2 -g -std=gnu99 -Iplugins/include -Iplugins/LINUXAMD64/molfile" bin/fpocket`
(needs no netcdf). The "expensive calculations off" variant sets `fparams.c:82` to 0 first.
