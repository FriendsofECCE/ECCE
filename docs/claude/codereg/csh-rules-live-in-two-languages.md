---
type: pitfall
title: "The csh-to-sh rules exist twice: `ecce_cshtosh.py` (ecce-csh2sh) and `CshToSh.pm` (gensub)"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["scripts/ecce-csh2sh", "scripts/ecce_cshtosh.py", "scripts/CshToSh.pm", "scripts/gensub", "tests/csh2sh/run_tests.py"]
issues: [868]
---
`ecce-csh2sh` is Python so that clients need no bundled Perl. `gensub` is still
Perl (it runs where Perl is, including remote hosts without Python 3) and
`use`s `CshToSh.pm`, so that file stays. A change to a rule must be made in both;
`tests/csh2sh` ("shared rules") runs every fixture through both implementations
in basic and structured mode and fails on any difference in text, changes or
issues. Delete `CshToSh.pm` only when gensub itself is ported.
