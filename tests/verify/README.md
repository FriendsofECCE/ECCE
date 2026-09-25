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

## The codes themselves

Where NWChem, MOPAC, ORCA or Gaussian are installed, every fixture is
also run through the real program and its verdict compared with the
checker's. That is the only authority on the question the checker is
trying to answer, and it is what stops the rules drifting from what
the codes actually do. It skips where a code is absent, so CI is
unaffected; locally it adds about ninety seconds.

Only ONE DIRECTION is a failure: if the checker calls a deck wrong,
the code must refuse it. The reverse is not — the checker is
deliberately narrow and lets through everything it cannot judge from
the file's structure, and a miss is the cheap failure.

It found three disagreements on its first run, and none was a false
alarm. Each deck RUNS and produces something useless: Gaussian warns
that two hydrogens have no basis functions and exits 0; NWChem reads
a deck with no task directive and does nothing; ORCA with no `!` line
runs on its own defaults. A code's parser is the authority on whether
a deck is legal, not on whether the calculation means anything — and
that gap is exactly what the checker is for. Those three are listed
in `ACCEPTS_ANYWAY` with what the code does, so the distinction is
recorded and a NEW disagreement still fails the suite.

(Exit status is not the signal, and assuming it would have made this
test lie: MOPAC exits 0 on a deck it has refused, writing the
complaint into its output file instead.)

## Adding a check

Add the rule to `scripts/parsers/verifyinput`, and add BOTH kinds of
fixture: a deck that the new rule should flag, and — more importantly —
a real working deck that it must leave alone.  A rule with only the
first kind of test is how a false alarm ships.

Before adding one, ask whether it can be decided from the file.  If it
needs to know what a particular version of a particular code accepts,
it belongs in `UNSURE` or nowhere.
