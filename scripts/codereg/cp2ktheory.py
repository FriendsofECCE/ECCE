import os

from templates import *

# ECCE CP2K "Theory Details" dialog.  Every field is an AbiDict key that
# scripts/parsers/ai.cp2k reads and turns into a CP2K keyword; a widget
# whose value the generator would drop is not added.
#
# The dialog is never told which elements the structure has, so the basis
# set and potential of each element are chosen by ai.cp2k from the library
# files named here (level and functional below); the per-element rows
# override that for the few cases that need it (a smaller-core potential,
# a basis from another file).  The cell is the Builder's lattice only.
#
# CP2K has no memory keyword, so there is no memory field to label in GB.

DEFAULT_LIB_DIR = "/usr/share/cp2k"


def ScanLibraryDir(directory, basisFile, potentialFile):
    """Return (elements with a basis, elements with a potential, readable).

    A header line of a CP2K library file is an element symbol followed by
    names; the data under it starts with a number.  Never raises: the path
    may only exist on the machine that runs the job.
    """
    def elements(name):
        found = set()
        path = name if name.startswith("/") else os.path.join(directory, name)
        try:
            handle = open(path)
        except (OSError, IOError):
            return None
        with handle:
            for line in handle:
                words = line.split()
                if (len(words) >= 2 and not line.lstrip().startswith("#")
                        and words[0][:1].isupper() and len(words[0]) <= 2
                        and words[0].isalpha() and words[1][:1].isalpha()):
                    found.add(words[0])
        return found
    b = elements(basisFile)
    p = elements(potentialFile)
    return (b, p)


class Cp2kTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = Cp2kTheoryPanel(self, helpURL)
        self.Finalize()


class Cp2kTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # Functional.  The potentials are chosen from the matching GTH
        # family (GTH-PBE for PBE and revPBE, GTH-BLYP, GTH-BP, GTH-PADE),
        # which ai.cp2k reads off the label, so keep these in step with
        # its functionalInfo().
        xcSizer = EcceBoxSizer(self, label="Exchange-Correlation", cols=1)
        xcChoice = ["PBE", "revPBE", "BLYP", "BP86", "LDA (Pade)"]
        self.functional = EcceComboBox(self,
                                       choices=xcChoice,
                                       name="ES.Theory.GPW.Functional",
                                       default=0,
                                       label="Functional:",
                                       export=1)
        xcSizer.AddWidget(self.functional)
        dispChoice = ["None",
                      "DFT-D3 (zero damping)",
                      "DFT-D3 (Becke-Johnson damping)"]
        self.dispersion = EcceComboBox(self,
                                       choices=dispChoice,
                                       name="ES.Theory.GPW.Dispersion",
                                       default=0,
                                       label="Dispersion:",
                                       export=1)
        xcSizer.AddWidget(self.dispersion)
        self.panelSizer.Add(xcSizer)

        # Basis level.  The names are the MOLOPT families in BASIS_MOLOPT;
        # the short-range (SR) sets are the usual choice for condensed
        # phases and exist for fewer elements, so ai.cp2k falls back from
        # one to the other per element.
        basisSizer = EcceBoxSizer(self, label="Basis Set and Potential", cols=1)
        levelChoice = ["DZVP-MOLOPT-SR-GTH",
                       "DZVP-MOLOPT-GTH",
                       "SZV-MOLOPT-SR-GTH",
                       "SZV-MOLOPT-GTH",
                       "TZVP-MOLOPT-GTH",
                       "TZV2P-MOLOPT-GTH"]
        self.basisLevel = EcceComboBox(self,
                                       choices=levelChoice,
                                       name="ES.Theory.GPW.BasisLevel",
                                       default=0,
                                       label="Basis level:",
                                       export=1)
        basisSizer.AddWidget(self.basisLevel)
        self.panelSizer.Add(basisSizer)

        # Library files.
        libSizer = EcceBoxSizer(self, label="Library Files", cols=1)
        self.libDir = EcceTextInput(self,
                                    size=(320, -1),
                                    name="ES.Theory.GPW.LibraryDir",
                                    default=DEFAULT_LIB_DIR,
                                    label="Directory:",
                                    export=1)
        libSizer.AddWidget(self.libDir)
        self.basisFile = EcceTextInput(self,
                                       size=(200, -1),
                                       name="ES.Theory.GPW.BasisFile",
                                       default="BASIS_MOLOPT",
                                       label="Basis file:",
                                       export=1)
        libSizer.AddWidget(self.basisFile)
        self.potentialFile = EcceTextInput(self,
                                           size=(200, -1),
                                           name="ES.Theory.GPW.PotentialFile",
                                           default="GTH_POTENTIALS",
                                           label="Potential file:",
                                           export=1)
        libSizer.AddWidget(self.potentialFile)
        self.libInfo = wx.StaticText(self, -1, self.LibraryInfoText(
            DEFAULT_LIB_DIR, "BASIS_MOLOPT", "GTH_POTENTIALS"))
        self.libInfo.SetFont(EcceGlobals.ScriptFontDefault)
        libSizer.AddWidget(self.libInfo)
        self.panelSizer.Add(libSizer)

        # Per-element overrides.  Blank means ai.cp2k picks from the files.
        kindSizer = EcceBoxSizer(self, label="Per-element Basis and Potential",
                                 cols=3)
        self.kindEl = []
        self.kindBasis = []
        self.kindPot = []
        for i in (1, 2, 3):
            el = EcceTextInput(self, size=(40, -1),
                               name="ES.Theory.GPW.KindElement%d" % i,
                               default="", label="Element:", export=1)
            kindSizer.AddWidget(el)
            b = EcceTextInput(self, size=(190, -1),
                              name="ES.Theory.GPW.KindBasis%d" % i,
                              default="", label="Basis:", export=1)
            kindSizer.AddWidget(b)
            p = EcceTextInput(self, size=(130, -1),
                              name="ES.Theory.GPW.KindPotential%d" % i,
                              default="", label="Potential:", export=1)
            kindSizer.AddWidget(p)
            self.kindEl.append(el)
            self.kindBasis.append(b)
            self.kindPot.append(p)
        self.panelSizer.Add(kindSizer)

        # Plane-wave grid.  Cutoffs are in Rydberg, CP2K's own unit.
        gridSizer = EcceBoxSizer(self, label="Plane-Wave Grid", cols=2)
        self.cutoff = EcceFloatInput(self,
                                     unit="Ry",
                                     name="ES.Theory.GPW.Cutoff",
                                     default=400.0,
                                     hardRange="(0..)",
                                     softRange="[100..1200]",
                                     label="Cutoff:",
                                     export=1)
        gridSizer.AddWidget(self.cutoff)
        self.relCutoff = EcceFloatInput(self,
                                        unit="Ry",
                                        name="ES.Theory.GPW.RelCutoff",
                                        default=60.0,
                                        hardRange="(0..)",
                                        softRange="[20..200]",
                                        label="Relative Cutoff:",
                                        export=1)
        gridSizer.AddWidget(self.relCutoff)
        self.nGrids = EcceSpinCtrl(self,
                                   hardRange="[1..10]",
                                   name="ES.Theory.GPW.NGrids",
                                   default=4,
                                   label="Number of Grids:",
                                   export=1)
        gridSizer.AddWidget(self.nGrids)
        self.panelSizer.Add(gridSizer)

        # Spin.  A multiplicity above 1 forces UKS in ai.cp2k, since a
        # triplet computed restricted is a different calculation.
        spinSizer = EcceBoxSizer(self, label="Spin Polarization", cols=1)
        spinChoice = ["Restricted", "Unrestricted (UKS)"]
        self.spinType = EcceComboBox(self,
                                     choices=spinChoice,
                                     name="ES.Theory.GPW.SpinType",
                                     default=0,
                                     label="Spin:",
                                     export=1)
        spinSizer.AddWidget(self.spinType)
        self.panelSizer.Add(spinSizer)

        # DFT+U (Dudarev U-J).  The elements are typed because the dialog
        # is never told the structure's; ai.cp2k ignores a row whose
        # element is not in it.
        hubSizer = EcceBoxSizer(self, label="DFT+U (U-J)", cols=3)
        self.hubbardEl = []
        self.hubbardOrb = []
        self.hubbardU = []
        for i in (1, 2, 3):
            el = EcceTextInput(self, size=(40, -1),
                               name="ES.Theory.GPW.HubbardElement%d" % i,
                               default="", label="Element:", export=1)
            hubSizer.AddWidget(el)
            orb = EcceComboBox(self, choices=["p", "d", "f"],
                               name="ES.Theory.GPW.HubbardOrbital%d" % i,
                               default=1, label="Orbital:", export=1)
            hubSizer.AddWidget(orb)
            u = EcceFloatInput(self, unit="eV",
                               name="ES.Theory.GPW.HubbardU%d" % i,
                               default=4.0, hardRange="[0..)",
                               softRange="[0..12]", label="U-J:", export=1)
            hubSizer.AddWidget(u)
            self.hubbardEl.append(el)
            self.hubbardOrb.append(orb)
            self.hubbardU.append(u)
        self.panelSizer.Add(hubSizer)

        # SCF.  OT is robust for non-metals; without it the diagonalisation
        # with Broyden mixing runs, and smearing makes that usable for
        # metals.
        scfSizer = EcceBoxSizer(self, label="SCF", cols=2)
        self.useOT = EcceCheckBox(self,
                                  label=" Orbital transformation (OT)",
                                  name="ES.Theory.GPW.UseOT",
                                  default=True,
                                  export=1)
        scfSizer.AddWidget(self.useOT)
        scfSizer.AddSpace()
        self.otMinimizer = EcceComboBox(self,
                                        choices=["DIIS", "CG"],
                                        name="ES.Theory.GPW.OTMinimizer",
                                        default=0,
                                        label="OT minimizer:",
                                        export=1)
        scfSizer.AddWidget(self.otMinimizer)
        self.otPreconditioner = EcceComboBox(
            self, choices=["FULL_SINGLE_INVERSE", "FULL_ALL"],
            name="ES.Theory.GPW.OTPreconditioner", default=0,
            label="Preconditioner:", export=1)
        scfSizer.AddWidget(self.otPreconditioner)
        self.epsScf = EcceExpInput(self,
                                   name="ES.Theory.GPW.EpsScf",
                                   default=1e-6,
                                   hardRange="(0..)",
                                   label="SCF Convergence:",
                                   export=1)
        scfSizer.AddWidget(self.epsScf)
        self.maxScf = EcceSpinCtrl(self,
                                   hardRange="[1..)",
                                   softRange="[10..1000]",
                                   name="ES.Theory.GPW.MaxScf",
                                   default=30,
                                   label="Max. SCF Iterations:",
                                   export=1)
        scfSizer.AddWidget(self.maxScf)
        self.outerScf = EcceCheckBox(self,
                                     label=" Outer SCF loop (with OT)",
                                     name="ES.Theory.GPW.OuterScf",
                                     default=True,
                                     export=1)
        scfSizer.AddWidget(self.outerScf)
        self.outerMaxScf = EcceSpinCtrl(self,
                                        hardRange="[1..)",
                                        softRange="[1..100]",
                                        name="ES.Theory.GPW.OuterMaxScf",
                                        default=10,
                                        label="Max. outer cycles:",
                                        export=1)
        scfSizer.AddWidget(self.outerMaxScf)
        self.smearTemp = EcceFloatInput(self,
                                        unit="K",
                                        name="ES.Theory.GPW.SmearTemperature",
                                        default=0.0,
                                        hardRange="[0..)",
                                        softRange="[0..1000]",
                                        label="Smearing (0 = off, without OT):",
                                        export=1)
        scfSizer.AddWidget(self.smearTemp)
        self.panelSizer.Add(scfSizer)

        self.AddButtons()

    def LibraryInfoText(self, directory, basisFile, potentialFile):
        basis, pot = ScanLibraryDir(directory, basisFile, potentialFile)
        if basis is None or pot is None:
            return ("   The library files cannot be read here (they may "
                    "only exist on\n   the compute host).  ai.cp2k will "
                    "refuse to write the input file\n   unless it can read "
                    "both.")
        return ("   Basis sets for %d elements, potentials for %d elements "
                "found.\n   ai.cp2k picks them for each element in the "
                "structure."
                % (len(basis), len(pot)))

    def OnChanges(self, event):
        # OT restarts in the outer loop, so 30 inner cycles suit it;
        # diagonalisation (metals, smearing) needs more.  Only a click on OT
        # swaps the limit, and only from the other method's default.
        if event is not None and event.GetEventObject() is self.useOT:
            try:
                n = int(self.maxScf.GetValue())
            except (TypeError, ValueError):
                n = None
            if self.useOT.GetValue() and n == 100:
                self.maxScf.SetValue(30)
            elif not self.useOT.GetValue() and n == 30:
                self.maxScf.SetValue(100)
        EccePanel.OnChanges(self, event)

    def CheckDependency(self):
        ot = self.useOT.GetValue()
        self.otMinimizer.Enable(ot)
        self.otPreconditioner.Enable(ot)
        self.outerScf.Enable(ot)
        self.outerMaxScf.Enable(ot and self.outerScf.GetValue())
        self.smearTemp.Enable(not ot)

        for el, orb, u in zip(self.hubbardEl, self.hubbardOrb,
                              self.hubbardU):
            used = bool(el.GetValue().strip())
            orb.Enable(used)
            u.Enable(used)

        for el, b, p in zip(self.kindEl, self.kindBasis, self.kindPot):
            used = bool(el.GetValue().strip())
            b.Enable(used)
            p.Enable(used)

        self.libInfo.SetLabel(self.LibraryInfoText(
            self.libDir.GetValue().strip(),
            self.basisFile.GetValue().strip(),
            self.potentialFile.GetValue().strip()))


frame = Cp2kTheoryFrame(None,
                        title="ECCE CP2K Editor: Theory Details",
                        app=app,
                        helpURL="")
