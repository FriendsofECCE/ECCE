---
type: pitfall
title: "A code's theory NAME must not equal its CATEGORY"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: [".edml"]
---
**A code's theory NAME must not equal its CATEGORY.** `CalcEd::
getTheoryName()` reverses `populateTheories()`'s display convention —
the combo shows `name()` for every theory except one literally named
`"None"`, where it shows `category()` — and it used to detect that
case by comparing the label against the category. For Quantum
ESPRESSO, whose theory is `category="PW" name="PW"`, that returned
`("PW","None")`: a theory in no `.edml`. **Two unrelated-looking
faults followed from it**, which is why it took a while to see:
`JCode::theoryNeedsBasis()` returns `true` for a theory it cannot
find, so the Basis Set Tool stayed enabled for a plane-wave code
*despite* `needsBasis="false"` being correct; and `populateRuntypes()`
found nothing, giving "No runtypes are supported for the given
code/theory combination" with nothing to edit or launch. Fixed by
asking the code whether a `"None"`-named theory exists rather than
inferring it from the label. Audited: QE was the only code where name
equals category, which is why it survived every other integration —
but check it when adding one.
