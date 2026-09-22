# Parser / property-pipeline regression suite

This suite exists because of one recurring, expensive bug shape in ECCE:

> a `.desc` entry whose `Begin` wording has drifted, whose `Skip=N` is off
> by one, whose `Begin` collides with another entry's, or whose parser
> script emits a different key than the `.desc` declares — **the job
> completes normally, nothing errors, and the property is silently missing
> or wrong.**

Recent instances: #80 (Gaussian 16 says "Mulliken charges:", not "Mulliken
atomic charges:"), #84 (VIBSYM parsed then dropped; a Car-Parrinello
gradient emitted under `EGRADVEC` instead of `TGRADCPVEC`), #85 (Gaussian
diatomic frequency jobs produced no vibrational data at all), an ORCA
`Skip=` off-by-one that fed the parser a separator line instead of the
first atom row, and an NWChem S² regex that matched any line containing
the digit 2.

Two more, both found while writing the ORCA cases on 2026-09-21 and both
of the *End-line starvation* kind — `eccejobmonitor` consumes the
`End`-matching line as part of the block, so an `End` that lands on the
next entry's `Begin` line makes that entry unable to ever fire:

* `[VIBFREQ]`'s `End=NORMAL MODES` ate `[VIB]`'s `Begin`, so **no ORCA
  job ever extracted `VIB`** — and because
  `PropertyPanelDescriptor.xml` gates the Vibrational Frequencies panel
  on `VIB` rather than `VIBFREQ`, ORCA frequency jobs produced correct
  frequencies and no vibration panel at all.  `[VIB]`'s own
  `End=IR SPECTRUM` had the identical problem one link further down the
  chain, latent only because `[VIB]` never fired.
* `[SHIELDTENSOR]`'s `End=CHEMICAL SHIELDING SUMMARY` ate
  `[ISOSHIELD][ANISOSHIELD]`'s `Begin`, so isotropic shielding and
  anisotropy were silently never extracted from any ORCA NMR job, while
  the tensor from the very same output section parsed fine.

`mopac.desc`'s header warns about precisely this hazard; `orca.desc` had
it twice.  Both are fixed and guarded by `orca-h2o-optfreq` /
`orca-h2o-nmr`.

None of those are visible by reading the `.desc` or the script alone.  All
of them are obvious the moment you replay real output through the real
pipeline and look at what comes out.  That is all this suite does.

## What it actually runs

    fixtures/<code>/<job>.log            real output from a real binary
       |
       v
    eccejobmonitor_sim.py  ............  a faithful re-implementation of
       |                                 scripts/eccejobmonitor's PDFileRead
       |                                 + the JobOutputGet Begin/Skip/Lines/
       |                                 End/Frequency line-feeding loop
       v
    scripts/parsers/<code>.<prop>  .....  the REAL parser scripts, invoked
       |                                 exactly as JobParser::storeProperty
       |                                 does: block text on stdin, argv =
       |                                 . "<runtype>" "<category>" "<theory>"
       |                                 "<open shells>"
       v
    key:/size:/values:/units: records  .  compared against cases.py
                                         assertions and expected/*.txt

Nothing is stubbed.  The `.desc` files, the parser scripts and the fixture
output are all the real thing.  It needs **python3 and perl only** — no
build, no GUI, no ECCE server, no gateway, no chemistry codes installed —
so it runs in CI and on any dev box.

### Why a re-implementation of eccejobmonitor rather than running it

`eccejobmonitor` cannot start without a JMS broker, an ECCE server, a job
directory and a live calculation; it delivers its blocks over JMS to a C++
client, which is what invokes the parser scripts.  Replaying a finished
file through it is not possible.  `eccejobmonitor_sim.py` therefore mirrors
the ~120 lines that matter, and every routine in it names the Perl
subroutine and line range it came from, so the two can be re-diffed when
the Perl changes.  Python was chosen over Perl because the fiddly parts —
`Skip=N` counting the `Begin` line itself, Perl's post-decrement semantics
in `if ($lineTake-- > 1) ... elsif ($lineTake == 0)`, the `%gPdTable`
keyed-by-`Begin` collision rule, the `Frequency=last` buffering — are
easier to write down explicitly and comment than to re-derive in Perl, and
because the runner needed no dependencies either way.

The deliberate divergences from the Perl are listed at the top of
`eccejobmonitor_sim.py`.  The important one: Perl builds its Begin-match
list from `keys %gPdTable`, so when two `Begin` patterns can match the same
line, **which one wins is genuinely nondeterministic in production**.  The
simulator uses `.desc` definition order and reports every such line as an
ambiguity, because the ambiguity is the real hazard.

## Running it

    tests/parsers/run_tests.py                     # everything
    tests/parsers/run_tests.py -v                  # + per-entry detail
    tests/parsers/run_tests.py --case g16-co-freq
    tests/parsers/run_tests.py --list
    tests/parsers/run_tests.py --update            # regenerate expected/

Exit status is 0 only when every check passed.  Also wired into CTest:

    cd build-cmake && ctest -R parsers --output-on-failure

## The `.desc` lint pass

Before any fixture runs, a full run lints **every** `.desc` file in
`scripts/parsers` (15 of them) for the failures that need no job output at
all: `Begin` collisions, `Begin`/`End` patterns that do not compile, rules
`PDFileRead` would reject, and `Script=` naming a file that does not exist.
That is why a new code's `.desc` gets checked the moment it lands, before
anyone writes a fixture for it.  `--case` skips the lint so a single case
runs fast.

## What makes it fail

* a `Begin` collision not listed in `cases.py`'s `KNOWN_COLLISIONS`
  (`PDFileRead` keys its table by the literal `Begin` string and *silently*
  drops later duplicates — the entry can then never match, ever);
* a `Begin`/`End` pattern that does not compile;
* a `Script=` naming a file that does not exist;
* a parse type that fires but whose script emits **no property at all**,
  unless listed in `KNOWN_SILENT_SCRIPTS` or the case's `silent_ok`;
* a script whose emitted keys share nothing with the `.desc`'s bracket
  declaration, unless listed in `KNOWN_KEY_ALIASES` (this is #84's shape);
* a parse type a case says must fire but which never does;
* any drift in an asserted value, or in the golden file;
* a runaway parse type (EOF mid-block — the real monitor `Die`s here);
* a **stale** allowlist entry: every allowlist above is checked in both
  directions, so a documented exception that no longer applies is itself a
  failure.  The allowlists cannot rot silently.

Things it *reports* but does not fail on: parse types that never fire on
any fixture (that is usually just missing fixture coverage, not a bug —
`-v` lists them), ambiguous `Begin` matches, and `xfail` entries, which are
open bugs written up in `cases.py` and printed under "KNOWN BUGS".

### Open bug currently reproduced (xfail)

`g16-oh-uhf` reproduces a bug this suite found: `gaussian-16.desc`'s
`MULLIKEN` `Begin` is `` Mulliken (atomic )?charges\:``, which demands a
colon straight after "charges".  Gaussian 16 prints, for **any** open-shell
(UHF / UDFT / ROHF) job, `Mulliken charges and spin densities:` — so the
`Begin` never matches and Mulliken charges are silently never extracted for
any open-shell Gaussian calculation.  Closed-shell jobs print `Mulliken
charges:` and work fine, which is exactly why this hid.  `gaussian-09.desc`
and `gaussian-03.desc` have the same shape.  Any fix must keep *not*
matching the `Mulliken charges ... with hydrogens summed into heavy atoms:`
line that follows a few lines later.

## Layout

    run_tests.py              entry point: runs cases, diffs goldens
    cases.py                  the declarative case table + allowlists
    eccejobmonitor_sim.py     the pipeline re-implementation
    fixtures/<code>/          real job output + the exact input deck
    fixtures/README.md        provenance: how to regenerate every fixture
    expected/<case>.txt       golden output, one file per case

## Adding a property assertion to an existing fixture

Add it to that case's `expect` in `cases.py`.  The parse type is the `.desc`
bracket header verbatim, minus the outermost brackets — so
`[VIB][VIBFREQ][VIBIR][VIBRAM][VIBSYM]` is written
`'VIB][VIBFREQ][VIBIR][VIBRAM][VIBSYM'` (yes, really: `PDFileRead`'s greedy
`^\s*\[(.+)\]\s*$` stores the whole string as one type name, and the suite
uses the same string so a mismatch is impossible to hide).  Then:

    ./run_tests.py --case <name>          # check it passes
    ./run_tests.py --case <name> --update # refresh the golden

Look at the golden diff before keeping it.  A golden file is a record of
what the pipeline *does*, not of what it *should* do.

## Adding a new fixture / a new code

1. Run the real code on a small, fast input (see `fixtures/README.md` for
   the decks already used — keep new ones in the same spirit: STO-3G, a few
   atoms, seconds not minutes).
2. Drop the output and the input deck in `fixtures/<code>/`, and add a
   provenance block to `fixtures/README.md`: code, version, exact
   keywords/input, date, and which properties it is there to exercise.
3. Add a `dict(...)` to `CASES` in `cases.py`.  `parse_args` matters: it is
   `('.', <runtype>, <theory category>, <theory name>, <open shells>)`, and
   several parser scripts gate their entire body on the runtype, so a wrong
   runtype makes a property look broken when it is merely inapplicable.
   The runtype names are ECCE's own, from `data/client/cap/<Code>.edml`.
4. `./run_tests.py --case <name>` and read the failures.  Each one is either
   a real finding or something to write down in an allowlist *with a
   reason* — never remove a check to make it pass.
5. `--update` to write the golden, review the diff, commit.

Keep fixtures small.  The whole `fixtures/` tree is currently ~460 KB and
should stay in that order of magnitude; a 1 MB output file for one assertion
is not worth it.

## The post-hoc importers (`*.expt`)

A second, separate pipeline, covered since the live-monitor cases above.
`eccejobmonitor` tails a *running* job; an `.expt` script is handed a
*finished* output file and reconstructs the calculation from it:

    <Code>.expt <outfile>   ->   <out>.frag    structure, mvm format
                                 <out>.param   title, theory, runtype, charge
                                 <out>.gbs     basis set

That is the path taken when a job run outside ECCE is imported, and the one
issue #94 (dummy submission, for 2FA-blocked HPC) would rely on almost
entirely.  It had no coverage at all.

Nine cases reuse the same fixture output files as the live cases, so this
cost no new checked-in data.  They are light on assertions and heavy on
golden files on purpose: these are the oldest, least-touched scripts here,
and several of their behaviours are arguable rather than wrong.  The
arguable ones are written down in `expt_cases.NOTES` so a golden diff can be
read with context — most importantly that the Gaussian importers take the
**first** "Standard orientation" in the file, so importing a completed
optimisation reconstructs the *starting* structure.  Whether that is wrong
depends on whether `.frag` is meant to describe the calculation's setup or
its result; it matters for #94 and should be settled before building on it.

Found while writing these: `ORCA.expt` recognised `HF`/`UHF`/`RKS`/`UKS` but
not `RHF`, because it was written against the vocabulary `ai.orca` itself
emits — so an ordinary externally-run `! RHF STO-3G Opt Freq` job imported
with no Category and no Theory at all.  Fixed.

Still uncovered: `NWChem.expt` (the nwchem fixtures here are `.eprint` trace
files, which is what the *live* monitor consumes — this needs a captured
stdout), `GAMESS-UK.expt` and `Gaussian-03/09/94/98.expt` (no fixture output
yet), and the basis-set import path (`rdStandardGBS.pm` and friends; issue
\#29 is live work there).

## What is deliberately NOT covered
* The `File=`-rule parse types (`fort.7`, `grid.dat`): `PDMatchListCreate`
  excludes them from line matching, and they are fed whole auxiliary files
  rather than matched blocks.
* `eccejobmonitor`'s incremental/restart machinery (bookmarks, reopening a
  growing file, multi-file batch parsing).  We replay one finished file.
* The C++ side of the pipeline: `DavPropCache::put` / `PropertyInterpreter`
  reading the scripts' stdout back, `PropFactory`'s property-name
  validation, and the DAV store.  This suite stops at the script's stdout.
* Gaussian 03/09/98, GAMESS-UK, Amica, Polyrate, the MD and reaction
  descriptors: no fixtures yet.  Adding one is step 1-5 above.
  (`nwchem.desc`, `gaussian-16.desc`, `orca.desc` and `mopac.desc` all
  have fixtures; every parse type in `orca.desc` and `mopac.desc` fires
  on at least one of them.)
