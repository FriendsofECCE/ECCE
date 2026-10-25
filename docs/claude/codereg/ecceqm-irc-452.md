---
type: map
title: "ECCE-QM IRC (#452 stage 2): runtype irc, Gonzalez-Schlegel, output keys the IRC panel reads"
area: codereg
paths: ["src/qm/irc.cpp", "src/qm/main.cpp", "src/qm/output.cpp", "scripts/parsers/ecceqm.desc", "scripts/parsers/ecceqm.parse", "scripts/parsers/ai.ecceqm", "scripts/parsers/verifyinput", "scripts/codereg/ecceqmruntype.py", "tests/qm/irc_test.py"]
issues: [452]
---
`runtype irc` starts from the editor's structure as the transition state:
Hessian there (the one frequency runs use), then each direction leaves along
the imaginary mode, Gonzalez-Schlegel steps in mass-weighted Cartesians
(constrained minimum on a sphere of radius step/2 about a pivot half a step
downhill, Bofill-updated Hessian, translation and rotation projected out).
Keywords: `irc_step` (0.1 bohr*sqrt(amu)), `irc_max_points` (20 per
direction), `irc_direction both|forward|backward`, `irc_hessian_every`.
A direction ends at rms/max gradient 5e-4/2e-3 (ORCA's), when the energy
rises, or at the point limit. Forward is the way the largest component of
the mode is positive; the sign of an eigenvector means nothing.

* Output is the keys the IRC panel already reads for ORCA: `IRCCOORD`
  (unit `sqrt(amu)*bohr`, the panel treats anything but `Point` as a
  mass-weighted coordinate), `IRCENERGY`, `IRCGEOM` (one table per point, TS
  in the middle). `begin irc_table` is rewritten after every point
  (`Frequency=last`), the `irc_geometry` blocks come once at the end, in path
  order; an aborted run has the table but no structures.
* Progress box: `progress irc point=k/N` (N = directions * max points; an
  early end fills the bar). The stage is named `irc`.
* A Run type named `IRC`, like ORCA's; not offered for a single atom. No
  constraints or dummy atoms. Step, max points and direction are in the
  runtype panel (`ES.Runtype.IRC.*`).
* Trap: a lambda returning an Eigen product (`return P * g;`) dangles
  (zero gradient, singular overlap one point later); return a `Vec`.
* Oracle: ORCA 6.1.1 IRC from its own OptTS geometry (`gen_irc_oracle.py`,
  `irc_oracle.json`, HF/STO-3G; 6-31G* not generated). Both ends agree to
  1e-5 Eh and 0.0003 A; ORCA's frame spacing varies, so the profile is
  compared at its path lengths (mass-weighted chord sums) to 3e-3 Eh.

* IRC panel (#452): plot click and table row both go through
  `IRCPanel::selectPoint` (as `ScanPanel`); a point without a structure puts
  that in the status line. Graph/Table is a "Show" radio box; script hooks
  `ircclick <i>` (logs `ircstatus`) and `ircview graph|table`. The panel
  showed the right structures on the real Props of a finished ECCE-QM IRC and
  on `tests/parsers/fixtures/ecceqm/hcn_irc.qmout` (`ecceqm-hcn-irc` case,
  `irc_panel_test.py`): the parser delivers one IRCGEOM table per point.
