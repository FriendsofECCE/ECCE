import os

from templates import *

# ECCE Quantum ESPRESSO "Theory Details" dialog (phase 1 first cut).
#
# Deliberately minimal, same principle as orcatheory.py / mopactheory.py:
# every field here corresponds to an AbiDict key that
# scripts/parsers/ai.qe actually reads and turns into a real pw.x namelist
# variable.  Widgets whose value the input generator would silently drop
# are not added.  Every variable reachable from this dialog was accepted
# by the real installed Quantum ESPRESSO 6.7 at /usr/bin/pw.x -- which
# matters more than usual here, because pw.x treats an UNRECOGNIZED
# namelist variable as a fatal error rather than ignoring it, so a typo
# anywhere in ai.qe's output stops the job instead of being silently
# dropped.
#
# NOTE ON THE METHOD: there is no method/theory chooser here.  For a
# plane-wave pseudopotential code the "theory" is the plane-wave method
# itself, chosen in CalcEd's own Theory list from
# QuantumESPRESSO.edml's single <Theory category="PW" name="PW">, and the
# exchange-correlation functional is a property of the PSEUDOPOTENTIALS
# (Debian's packaged SSSP library is entirely PBE).  The "Functional
# override" combo below writes input_dft, which overrides the functional
# the pseudopotentials were built for -- physically inconsistent unless
# you know exactly what you are doing, which is why its default is "From
# pseudopotentials" and ai.qe then emits no input_dft at all.
#
# NOTE ON THE UNIT CELL: there is no cell entry on this dialog, on
# purpose.  QE takes its cell from the fragment's own lattice (the
# .frag's lattice_vectors: section, written by Fragment::dumpMVM whenever
# the fragment has a LatticeDef) and from nowhere else.  nwpw's theory
# dialog has nine lattice-vector fields of its own that are
# unsynchronized with the Builder's Periodic Builder panel and override
# it; repeating that would repeat a known wart (docs/QE_ROADMAP.md
# section 3).  Use the Builder's "Periodic Builder" tool panel.
#
# NOTE ON MEMORY: CLAUDE.md's standing preference is that memory settings
# are entered and labeled in GB for every code.  pw.x HAS NO MEMORY
# KEYWORD -- /opt/QE/qe-7.5/Doc/INPUT_PW.txt has no memory-allocation
# input variable at all, only max_seconds (a wall-clock limit) and
# disk_io (an I/O strategy).  So there is deliberately no Memory field
# here, exactly as for MOPAC (issue #86): the preference is about how a
# memory setting is *labeled* where one exists, not a reason to invent
# one the code would reject.
#
# NOTE ON BASIS SETS: there are none.  QuantumESPRESSO.edml marks the
# theory needsBasis="false", which greys out the Basis Set Tool and stops
# ESInputController from writing a .basis file at all.  The plane-wave
# "basis" is the Wavefunction Cutoff field below.


# ---------------------------------------------------------------------
# Pseudopotential library scanning.
#
# WHY THE SELECTION IS NOT A PER-ELEMENT COMBO, which is what the plan
# asked for: this dialog is a standalone Python process that CalcEd
# launches with a fixed argument list (see scripts/codereg/globals.py's
# Globals.__init__) carrying the calculation name, theory category,
# theory name, runtype, electron/orbital/normal-mode counts and the
# symmetry group -- and NOTHING about which chemical elements the
# structure contains.  There is no way for this dialog to know that it
# should offer a row for Si and O and not for the other 101 elements, and
# offering all 103 is not a usable dialog.
#
# So the per-element resolution lives in ai.qe instead, which does have
# the element list (it reads the .frag) and does the same directory scan
# this dialog does.  The user never types a UPF filename either way --
# which was the actual requirement.  What this dialog contributes is the
# DIRECTORY and an optional TYPE PREFERENCE, and it shows what the
# directory actually contains so a wrong path is obvious immediately
# rather than at launch time.
#
# The type choices are built from what is really present, not from a
# hardcoded list: the packaged SSSP set mixes ultrasoft, PAW and
# norm-conserving pseudopotentials with exactly one file per element, so
# a type is applied by ai.qe as a PREFERENCE with fallback, never as a
# filter that could leave an element uncovered.
# ---------------------------------------------------------------------

DEFAULT_PSEUDO_DIR = "/usr/share/espresso/pseudo"

PSEUDO_AUTO = "Automatic (whatever pseudo_dir provides)"

# Keep these label strings in sync with ai.qe's pseudoTypePreference(),
# which matches on the words "ultrasoft", "paw" and "norm".
PSEUDO_TYPE_LABELS = [("US", "Prefer ultrasoft"),
                      ("PAW", "Prefer PAW"),
                      ("NC", "Prefer norm-conserving")]


def ClassifyPseudo(filename):
    """Same classification as ai.qe's pseudoType(); "" means unknown."""
    lower = filename.lower()
    if "oncv" in lower:
        return "NC"
    if "paw" in lower:
        return "PAW"
    if ("rrkjus" in lower or "uspp" in lower or ".us." in lower
            or "_us." in lower or "_us-" in lower or "_us_" in lower):
        return "US"
    return ""


def ScanPseudoDir(directory):
    """Return (element count, file count, set of types present).

    Mirrors ai.qe's resolvePseudos(): the element token is the leading
    run of letters before the first '.', '_' or '-', which covers every
    naming shape in the packaged library (Si.pbe-..., be_pbe_...,
    Ag_ONCV_..., Hf-sp....).  Never raises: an unreadable or missing
    directory is simply reported as empty, because the user is allowed
    to type a path that only exists on the compute host.
    """
    elements = set()
    files = 0
    types = set()
    try:
        names = os.listdir(directory)
    except (OSError, IOError):
        return (0, 0, types)
    for name in names:
        if not name.lower().endswith(".upf"):
            continue
        token = ""
        for ch in name:
            if ch.isalpha():
                token += ch
            else:
                break
        if not token or name[len(token):len(token) + 1] not in (".", "_", "-"):
            continue
        elements.add(token.capitalize())
        files += 1
        kind = ClassifyPseudo(name)
        if kind:
            types.add(kind)
    return (len(elements), files, types)


class QeTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = QeTheoryPanel(self, helpURL)
        self.Finalize()


class QeTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # -------------------------------------------------------------
        # PLANE-WAVE BASIS
        #
        # ecutwfc is the plane-wave kinetic-energy cutoff and is the one
        # setting that plays the role a basis set plays for every other
        # code -- it is REQUIRED by pw.x and has no default.  Rydberg,
        # which is pw.x's own unit for it; not converted anywhere,
        # because converting would mean the number in this dialog and
        # the number in the deck disagree, and every published
        # pseudopotential's recommended cutoff is quoted in Ry.
        #
        # ecutrho (the charge-density cutoff) defaults inside pw.x to
        # 4*ecutwfc, which is right for norm-conserving pseudopotentials
        # and too small for ultrasoft/PAW ones, where 8-12x is the usual
        # recommendation.  It is gated behind its own checkbox so the
        # common case emits nothing and pw.x uses its own rule.
        # -------------------------------------------------------------
        basisSizer = EcceBoxSizer(self, label="Plane-Wave Basis", cols=2)
        self.ecutwfc = EcceFloatInput(self,
                                      unit="Ry",
                                      name="ES.Theory.PW.EcutWfc",
                                      default=30.0,
                                      hardRange="(0..)",
                                      softRange="[10..200]",
                                      label="Wavefunction Cutoff:",
                                      export=1)
        basisSizer.AddWidget(self.ecutwfc)
        basisSizer.AddSpace()

        self.useEcutRho = EcceCheckBox(self,
                                       label=" Charge Density Cutoff:",
                                       name="ES.Theory.PW.UseEcutRho",
                                       default=0,
                                       export=1)
        basisSizer.AddWidget(self.useEcutRho)

        self.ecutrho = EcceFloatInput(self,
                                      unit="Ry",
                                      name="ES.Theory.PW.EcutRho",
                                      default=240.0,
                                      hardRange="(0..)",
                                      softRange="[40..1600]",
                                      export=1)
        basisSizer.AddWidget(self.ecutrho)
        self.panelSizer.Add(basisSizer)

        # -------------------------------------------------------------
        # BRILLOUIN ZONE
        #
        # ECCE has no k-point machinery anywhere else -- NWChem's BAND
        # theory has been gamma-only forever because nedtheory.py has no
        # k-point widget at all (docs/QE_ROADMAP.md section 2.8.1).  This
        # is deliberately the minimum viable set (roadmap section 8 Q4):
        # an automatic Monkhorst-Pack grid with half-grid offsets, or
        # gamma.  Explicit k-point lists and band paths are phase 5.
        #
        # "Gamma point only" is not merely nk = 1 1 1: pw.x's
        # "K_POINTS gamma" switches on a real-wavefunction optimization
        # that halves memory and time, which is the right thing for a
        # molecule in a box.
        # -------------------------------------------------------------
        kSizer = EcceBoxSizer(self, label="Brillouin Zone Sampling", cols=2)
        kSchemeChoice = ["Monkhorst-Pack grid",
                         "Gamma point only"]
        self.kScheme = EcceComboBox(self,
                                    choices=kSchemeChoice,
                                    name="ES.Theory.PW.KPointScheme",
                                    default=0,
                                    label="k-points:",
                                    export=1)
        kSizer.AddWidget(self.kScheme)
        kSizer.AddSpace()

        self.nk1 = EcceSpinCtrl(self, hardRange="[1..)", softRange="[1..32]",
                                name="ES.Theory.PW.KPointsNk1",
                                default=4, label="Grid n1:", export=1)
        kSizer.AddWidget(self.nk1)
        self.sk1 = EcceSpinCtrl(self, hardRange="[0..1]",
                                name="ES.Theory.PW.KPointsSk1",
                                default=0, label="Offset s1:", export=1)
        kSizer.AddWidget(self.sk1)

        self.nk2 = EcceSpinCtrl(self, hardRange="[1..)", softRange="[1..32]",
                                name="ES.Theory.PW.KPointsNk2",
                                default=4, label="Grid n2:", export=1)
        kSizer.AddWidget(self.nk2)
        self.sk2 = EcceSpinCtrl(self, hardRange="[0..1]",
                                name="ES.Theory.PW.KPointsSk2",
                                default=0, label="Offset s2:", export=1)
        kSizer.AddWidget(self.sk2)

        self.nk3 = EcceSpinCtrl(self, hardRange="[1..)", softRange="[1..32]",
                                name="ES.Theory.PW.KPointsNk3",
                                default=4, label="Grid n3:", export=1)
        kSizer.AddWidget(self.nk3)
        self.sk3 = EcceSpinCtrl(self, hardRange="[0..1]",
                                name="ES.Theory.PW.KPointsSk3",
                                default=0, label="Offset s3:", export=1)
        kSizer.AddWidget(self.sk3)
        self.panelSizer.Add(kSizer)

        # -------------------------------------------------------------
        # OCCUPATIONS
        #
        # This is the one setting whose wrong value makes pw.x stop
        # rather than give a wrong answer, so it gets its own box.  With
        # occupations='fixed' (pw.x's own default) a METAL will not
        # converge, and an odd number of electrons is refused outright:
        #   "Error in routine electrons (1): charge is wrong: smearing
        #    is needed"
        # (reproduced with a tot_charge = -1 silicon cell).  ai.qe prints
        # a warning on stderr for the odd-electron case at generation
        # time, but the fix is here.
        #
        # All four smearing functions are real pw.x values; the names
        # ai.qe writes ('gaussian', 'marzari-vanderbilt',
        # 'methfessel-paxton', 'fermi-dirac') were each accepted and
        # echoed by the real 6.7 binary.  Marzari-Vanderbilt ("cold") is
        # the usual recommendation for metals.
        # -------------------------------------------------------------
        occSizer = EcceBoxSizer(self, label="Occupations", cols=2)
        occChoice = ["Fixed (insulator)",
                     "Smearing (metal)",
                     "Tetrahedra"]
        self.occupations = EcceComboBox(self,
                                        choices=occChoice,
                                        name="ES.Theory.PW.Occupations",
                                        default=0,
                                        label="Occupations:",
                                        export=1)
        occSizer.AddWidget(self.occupations)
        occSizer.AddSpace()

        smearChoice = ["Gaussian",
                       "Marzari-Vanderbilt (cold)",
                       "Methfessel-Paxton",
                       "Fermi-Dirac"]
        self.smearing = EcceComboBox(self,
                                     choices=smearChoice,
                                     name="ES.Theory.PW.Smearing",
                                     default=0,
                                     label="Smearing:",
                                     export=1)
        occSizer.AddWidget(self.smearing)

        self.degauss = EcceFloatInput(self,
                                      unit="Ry",
                                      name="ES.Theory.PW.Degauss",
                                      default=0.02,
                                      hardRange="(0..)",
                                      softRange="[0.001..0.5]",
                                      label="Width:",
                                      export=1)
        occSizer.AddWidget(self.degauss)
        self.panelSizer.Add(occSizer)

        # -------------------------------------------------------------
        # SPIN
        #
        # nspin = 2 is forced by ai.qe whenever the structure's spin
        # multiplicity is not a singlet, regardless of this setting -- a
        # triplet computed with nspin = 1 is silently just a closed-shell
        # calculation, which is the kind of quiet wrong answer this
        # project keeps finding.  This combo is for the other case: an
        # open-shell singlet, or an antiferromagnet, where the
        # multiplicity does not reveal it.
        #
        # starting_magnetization is a per-species starting guess in units
        # of the valence charge (0 to 1).  ai.qe writes the same value for
        # every species; a per-species guess would need the element list
        # this dialog does not get (see the header note).
        # -------------------------------------------------------------
        spinSizer = EcceBoxSizer(self, label="Spin Polarization", cols=2)
        spinChoice = ["Restricted",
                      "Unrestricted"]
        self.spinType = EcceComboBox(self,
                                     choices=spinChoice,
                                     name="ES.Theory.PW.SpinType",
                                     default=0,
                                     label="Spin:",
                                     export=1)
        spinSizer.AddWidget(self.spinType)

        self.startMag = EcceFloatInput(self,
                                       name="ES.Theory.PW.StartingMagnetization",
                                       default=0.1,
                                       hardRange="[-1..1]",
                                       label="Starting Magnetization:",
                                       export=1)
        spinSizer.AddWidget(self.startMag)
        self.panelSizer.Add(spinSizer)

        # -------------------------------------------------------------
        # SCF CONVERGENCE
        #
        # conv_thr is in Rydberg and is an absolute energy threshold, not
        # a per-atom one, so the default is deliberately tight-ish:
        # pw.x's own default of 1e-6 Ry is fine for an energy but too
        # loose for forces.  EcceExpInput enforces an exact power of ten,
        # which is how this number is always quoted.
        #
        # mixing_beta is the density-mixing factor; lowering it is the
        # standard first response to a non-converging metal.
        # -------------------------------------------------------------
        scfSizer = EcceBoxSizer(self, label="SCF Convergence", cols=2)
        self.convThr = EcceExpInput(self,
                                    unit="Ry",
                                    name="ES.Theory.PW.ConvergenceEnergy",
                                    default=1e-6,
                                    hardRange="(0..)",
                                    label="Energy Convergence:",
                                    export=1)
        scfSizer.AddWidget(self.convThr)

        self.maxIterationsSpin = EcceSpinCtrl(self,
                                              hardRange="[0..)",
                                              softRange="[1..500]",
                                              name="ES.Theory.PW.ConvergenceIterations",
                                              default=100,
                                              label="Max. Iterations (0 = pw.x default):",
                                              export=1)
        scfSizer.AddWidget(self.maxIterationsSpin)

        self.mixingBeta = EcceFloatInput(self,
                                         name="ES.Theory.PW.MixingBeta",
                                         default=0.7,
                                         hardRange="(0..1]",
                                         label="Mixing Factor:",
                                         export=1)
        scfSizer.AddWidget(self.mixingBeta)
        self.panelSizer.Add(scfSizer)

        # -------------------------------------------------------------
        # PSEUDOPOTENTIALS  (see the long header note on why this is a
        # directory plus a preference rather than a per-element grid)
        # -------------------------------------------------------------
        ppSizer = EcceBoxSizer(self, label="Pseudopotentials", cols=1)
        self.pseudoDir = EcceTextInput(self,
                                       size=(320, -1),
                                       name="ES.Theory.PW.PseudoDir",
                                       default=DEFAULT_PSEUDO_DIR,
                                       label="Directory:",
                                       export=1)
        ppSizer.AddWidget(self.pseudoDir)

        elementCount, fileCount, types = ScanPseudoDir(DEFAULT_PSEUDO_DIR)

        # Only offer type preferences the directory can actually satisfy.
        # Automatic is always first and always available; it is also the
        # default, because the packaged library has exactly one file per
        # element and a preference can only ever be a tie-break.
        self.pseudoTypeChoices = [PSEUDO_AUTO]
        for kind, label in PSEUDO_TYPE_LABELS:
            if kind in types:
                self.pseudoTypeChoices.append(label)
        self.pseudoType = EcceComboBox(self,
                                       choices=self.pseudoTypeChoices,
                                       name="ES.Theory.PW.PseudoType",
                                       default=0,
                                       label="Selection:",
                                       export=1)
        ppSizer.AddWidget(self.pseudoType)

        # Live feedback on the directory: a wrong path is otherwise only
        # discovered when ai.qe refuses to generate the deck.
        self.pseudoInfo = wx.StaticText(self, -1,
                                        self.PseudoInfoText(elementCount,
                                                            fileCount))
        self.pseudoInfo.SetFont(EcceGlobals.ScriptFontDefault)
        ppSizer.AddWidget(self.pseudoInfo)
        self.panelSizer.Add(ppSizer)

        # -------------------------------------------------------------
        # FUNCTIONAL OVERRIDE
        #
        # input_dft overrides the exchange-correlation functional the
        # pseudopotentials were generated with.  Default is to emit
        # nothing: overriding it is physically inconsistent (the packaged
        # library is all PBE) and pw.x only WARNS about the mismatch, it
        # does not refuse -- so this is exactly the kind of setting that
        # gives a plausible-looking wrong answer.  Every name below is a
        # real pw.x shortname.
        # -------------------------------------------------------------
        dftSizer = EcceBoxSizer(self, label="Functional Override", cols=1)
        dftChoice = ["From pseudopotentials",
                     "PBE",
                     "PBESOL",
                     "REVPBE",
                     "BLYP",
                     "PW91",
                     "PZ",
                     "PBE0",
                     "B3LYP",
                     "HSE"]
        self.xcFunc = EcceComboBox(self,
                                   choices=dftChoice,
                                   name="ES.Theory.PW.XCFunctionals",
                                   default=0,
                                   label="input_dft:",
                                   export=1)
        dftSizer.AddWidget(self.xcFunc)
        self.panelSizer.Add(dftSizer)

        self.AddButtons()

    def PseudoInfoText(self, elementCount, fileCount):
        if fileCount == 0:
            return ("   No .UPF files found in this directory (it may only "
                    "exist on the\n   compute host).  ai.qe will refuse to "
                    "generate an input file unless\n   every element in the "
                    "structure has one.")
        return ("   %d pseudopotential files covering %d elements found.  "
                "ai.qe picks the\n   file for each element in the structure; "
                "no file name is typed by hand."
                % (fileCount, elementCount))

    def CheckDependency(self):
        self.ecutrho.Enable(self.useEcutRho.GetValue())

        grid = (self.kScheme.GetValue() != "Gamma point only")
        for widget in (self.nk1, self.nk2, self.nk3,
                       self.sk1, self.sk2, self.sk3):
            widget.Enable(grid)

        smear = self.occupations.GetValue().startswith("Smearing")
        self.smearing.Enable(smear)
        self.degauss.Enable(smear)

        self.startMag.Enable(
            self.spinType.GetValue() == "Unrestricted")

        # Re-scan whenever the directory text changes, so a typo shows up
        # here instead of at input-generation time.
        elementCount, fileCount, types = ScanPseudoDir(
            self.pseudoDir.GetValue().strip())
        self.pseudoInfo.SetLabel(self.PseudoInfoText(elementCount, fileCount))


frame = QeTheoryFrame(None,
                      title="ECCE Quantum ESPRESSO Editor: Theory Details",
                      app=app,
                      helpURL="")
