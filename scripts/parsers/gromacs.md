#!/usr/bin/env python3
################################################################################
# Purpose:
# The speed distributions and mean-square displacements of a GROMACS run, as
# the mdanalysis program writes them to ecce-parse-md.txt,
#     ECCE-MDSPEED <record>
#     ECCE-MDMSD <record>
# one record (no blanks) per atom mass or molecule type.  They become the string
# vectors MDSPEED and MDMSD of the Speed Distribution and Displacement panels,
# which decode them with include/util/MdAnalysis.H and src/util/mdanalysis.
################################################################################

import re
import sys
from xtb_parse import configure_streams

configure_streams()

speed, msd = [], []
for raw in sys.stdin.buffer:
    line = raw.decode("utf-8", "replace")
    for pattern, values in ((r"^ECCE-MDSPEED\s+(\S+)\s*$", speed),
                            (r"^ECCE-MDMSD\s+(\S+)\s*$", msd)):
        if re.search(pattern, line, re.ASCII):
            values.append(re.search(pattern.encode("ascii"), raw, re.ASCII).group(1))
for key, values in (("MDSPEED", speed), ("MDMSD", msd)):
    if values:
        n = len(values)
        header = "key: {}\nsize:\n{}\nrowlabels:\n{}\nvalues:\n".format(
            key, n, " ".join(str(i) for i in range(1, n + 1)))
        sys.stdout.buffer.write(header.encode("ascii") + b" ".join(values) + b"\nEND\n")
