# Excluded calculations

A calculation can be marked as excluded, with a reason, for example "SCF
converged to neutral Na, not the intended state". The calculation and its
results stay as they are; the mark says it should not be used.

In the Organizer, right-click a calculation and choose **Exclude...**, then
type the reason. The calculation is drawn grey and struck through in the
tree, and the reason is shown as a tooltip when the pointer rests on it.
Choose **Include again** on the same menu to remove the mark.

An excluded calculation is

- left out of **File > Export > Supporting Information...** unless you
  choose to include it (the table then has an `excluded` column with the
  reason), and
- not launched by **Launch Ready Steps** in a workflow.

From a terminal, the same mark is set with

    ecce-annotate --exclude <calculation>... "reason"
    ecce-annotate --include <calculation>...

`ecce-si-export` has the matching option `--include-excluded`. The mark is
stored in the calculation's data as the property `excluded`; an empty or
missing value means the calculation is not excluded. It works with a local
data folder and with a data server.
