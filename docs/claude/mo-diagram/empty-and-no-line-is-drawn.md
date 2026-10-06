---
type: map
title: "Without `symops` every fragment column is empty and no line is drawn"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["$ECCE_HOME/bin/symops"]
issues: [132]
---
It needs `$ECCE_HOME/bin/symops`; without it every fragment column
comes back **empty and no line is drawn**, which looks exactly like
a logic bug and is not one.
