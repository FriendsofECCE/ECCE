# Input-deck checker suite

Runs `scripts/parsers/verifyinput` over real decks and checks what it
says about each, then builds the report dialog on a headless X server to
make sure it does not crash on construction.

    ./run_tests.py            everything
    ./run_tests.py -v         every finding, not just the failures

Needs python3 and perl.  The dialog check additionally needs a
`build-cmake` tree, g++, wx-config and Xvfb, and SKIPs cleanly without
them.

## What this is for

GitHub #148.  A generated deck can be broken in ways nothing in the
calculation editor would notice, and the failure then happens in a
queue, an hour later, to a student.  Two happened on 2026-09-25 alone:
a Gen basis block that emerged as six bytes of binary (#146, Gaussian
read them as an atomic symbol and took a segmentation violation), and a
route card promising `/GEN` with nothing following it.  Both looked
entirely normal on screen.

## Why the suite is weighted the way it is

The two failure modes are not symmetric, and the tests are built around
that.

A **missed fault** leaves the user where they already were: a job that
fails later.  Bad, but no worse than having no checker.

A **false alarm** is worse than having no checker.  A user told their
working deck is broken either loses an afternoon to a non-problem or,
having been wrong about them once, stops reading the light — including
the time it is right.  So most of the fixtures are WORKING decks, and
the suite fails on any unexpected `BAD`, not merely on a missing one.

This is also why the checker judges no keyword.  This tree has been
wrong about keyword validity from reading documentation repeatedly:
NWChem 7.2.3 rejects its own documented dispersion spellings, ORCA
accepts `6-31++G**` and refuses `6-31++G`, Gaussian wants `GD3BJ` where
ECCE wrote `GD3-BJ`.  Anything that cannot be decided from the file's
structure alone is reported `UNSURE`, which is a real answer and not a
weak `BAD`.

## Adding a check

Add the rule to `scripts/parsers/verifyinput`, and add BOTH kinds of
fixture: a deck that the new rule should flag, and — more importantly —
a real working deck that it must leave alone.  A rule with only the
first kind of test is how a false alarm ships.

Before adding one, ask whether it can be decided from the file.  If it
needs to know what a particular version of a particular code accepts,
it belongs in `UNSURE` or nowhere.
