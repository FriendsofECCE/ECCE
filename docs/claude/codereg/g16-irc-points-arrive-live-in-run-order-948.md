---
type: pitfall
title: "Gaussian 16 IRC points arrive live in run order; readers sort them by coordinate and fit the TS (#948)"
area: codereg
paths: [scripts/parsers/gaussian-16.ircgeom, scripts/parsers/gaussian-16.irc, scripts/parsers/gaussian-16.energy, scripts/parsers/gaussian-16.desc, src/tdat/chemistry/ResultsFromCalc.C, src/dsm/edsiimpl/DavPropCache.C, tests/parsers/g16_trace_align_test.py, tests/apps/irc_panel_test.py, tests/resultsh5/trace_vib_test.py]
issues: [948, 947, 417]
---
A parser change must not move data from "arrives live" to "arrives at the
end": ECCE monitors running jobs.

How a live property grows (`DavPropCache::storeProperty`): a time-series
class (`PropTS*`, e.g. IRCGEOM `PropTSVecTable`, TEVEC) is APPENDED one
record at a time and can never be replaced; any other class (`PropVector`:
IRCENERGY, IRCCOORD, SCANENERGY) is REPLACED by each record. So a series
that must be corrected at the end belongs in a replaced vector, and its
structures must already be in the order the vectors will keep.

Gaussian 16 IRC: `gaussian-16.ircgeom` takes three blocks: "Start new
reaction path calculation" (the TS: the held `g16IrcInput` structure and
the last total energy, written to `g16IrcE` by `gaussian-16.energy`, which
prints nothing else for an IRC), "in FORWARD|REVERSE path direction" (the
sign) and CURRENT STRUCTURE .. NET REACTION COORDINATE (a point: the last
energy before it, signed NET coordinate). Each appends one IRCGEOM frame
and re-sends IRCCOORD/IRCENERGY whole from `g16IrcPoints`, all in run
order (TS, forward, reverse). The final summary (`gaussian-16.irc`)
replaces the two vectors with its values, matched to the live points by
coordinate, in the same run order; on any mismatch it leaves the live ones.

`ResultsFromCalc::property` (every view) and the results.h5 writer sort the
three keys together by coordinate (reverse end, TS, forward end) and fit the
TS rigidly onto its two neighbours (a Link1 IRC prints it in the previous
job's orientation), whenever there is one frame and one energy per
coordinate; otherwise (pre-#947 stored calcs, 16 frames for 17 points) the
parsed properties are shown as they are. ORCA's IRC is unchanged and still
arrives whole at the end (summary Frequency=last, IRCGEOM from the
`_IRC_Full_trj.xyz` file).
