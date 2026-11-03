# Excluded calculations

A calculation can be marked as excluded, with a reason, for example "SCF
converged to neutral Na, not the intended state". The calculation and its
results stay as they are; the mark says it should not be used.

The Organizer does not show the mark and has no menu item for it.

An excluded calculation is

- left out of `ecce-si-export` unless `--include-excluded` is given (the table then has an `excluded` column with the
  reason), and
- not launched by **Launch Ready Steps** in a workflow.

From a terminal, the same mark is set with

    ecce-annotate --exclude <calculation>... "reason"
    ecce-annotate --include <calculation>...

`ecce-si-export` has the matching option `--include-excluded`. The mark is
stored in the calculation's data as the property `excluded`; an empty or
missing value means the calculation is not excluded. It works with a local
data folder and with a data server.
