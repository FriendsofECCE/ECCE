---
type: pitfall
title: "GROMACS progress plots need the job's own output (ecce.submit.log) read live: the declared output file has it only after the job ends (#513)"
area: codereg
paths: ["scripts/parsers/gromacs.desc", "scripts/eccejobmonitor", "scripts/gensub", "tests/progress/gromacs_test.py"]
issues: [513, 477]
---
For GROMACS the parse file and the declared output are the same file (the
symlink to mdrun's log), so `jobOutputFile2` is no second file and the
monitor's progress tail read nothing else. mdrun's `-v` step lines, the
`ECCE-EMTOL` and `ECCE-DENSITY` echoes go to `ecce.submit.log`, which gensub
appends to the declared output after `-----ECCE Log Information-----` when the
job ends; so those plots appeared only then. `Stream=ecce.submit.log` in a
`[PROGRESS]` entry of the `.desc` makes the monitor tail that file (next to
the parse file's directory) when `jobOutputFile2` is not another file, and
skip the declared file's lines after the marker so nothing is counted twice.
Not checked on a real Slurm run: the monitor runs beside the files there as it
does for NWChem's second file.
