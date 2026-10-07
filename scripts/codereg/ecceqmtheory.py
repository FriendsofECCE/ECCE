from templates import *

# ECCE-QM "Theory Details" dialog.  Deliberately almost empty: the engine is
# for students, so there are no convergence, grid, memory or thread settings.
# The DFT functional list is the one place to add a functional (the keyword
# it maps to is in ai.ecceqm's %XCKeyword).

XC_FUNCTIONALS = ["SVWN", "PBE", "B3LYP", "PBE0"]
XC_DEFAULT = "B3LYP"


class EcceqmTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = EcceqmTheoryPanel(self, helpURL)
        self.Finalize()


class EcceqmTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.Category == "DFT":
            dftSizer = EcceBoxSizer(self, label="DFT Functional", cols=1)
            self.xcFunc = EcceComboBox(self,
                                       choices=XC_FUNCTIONALS,
                                       name="ES.Theory.DFT.XCFunctionals",
                                       default=XC_FUNCTIONALS.index(XC_DEFAULT),
                                       label="Functional:",
                                       export=1)
            dftSizer.AddWidget(self.xcFunc)
            self.panelSizer.Add(dftSizer)

        # Open-shell systems use restricted open-shell orbitals (ROHF/ROKS)
        # unless this is ticked; it changes nothing for a closed shell.
        orbSizer = EcceBoxSizer(self, label="Orbitals", cols=1)
        self.unrestricted = EcceCheckBox(self,
                                         label=" Unrestricted (UHF/UKS) for open shells",
                                         name="ES.Theory.ECCEQM.Unrestricted",
                                         default=False,
                                         export=1)
        orbSizer.AddWidget(self.unrestricted)
        self.panelSizer.Add(orbSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = EcceqmTheoryFrame(None,
                          title="ECCE-QM Editor: Theory Details",
                          app=app,
                          helpURL="")
