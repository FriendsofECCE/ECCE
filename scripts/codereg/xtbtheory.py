from templates import *


class XtbTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = XtbTheoryPanel(self, helpURL)
        self.Finalize()

class XtbTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # The method (GFN2-xTB, GFN1-xTB, GFN-FF) is the calculation's Theory,
        # chosen in the editor, not here.  Charge and multiplicity come from
        # the calculation; ai.xtb turns them into --chrg and --uhf.
        solvSizer = EcceBoxSizer(self, label="Implicit Solvent", cols=1)
        self.solvModel = EcceComboBox(self,
                                      choices=["None", "ALPB", "GBSA"],
                                      name="ES.Theory.Solvent.Model",
                                      default=0,
                                      label="Model:",
                                      export=1)
        solvSizer.AddWidget(self.solvModel)

        # xtb's ALPB list.  GBSA knows a subset (ai.xtb refuses the rest).
        solvents = ["water", "acetone", "acetonitrile", "aniline",
                    "benzaldehyde", "benzene", "ch2cl2", "chcl3", "cs2",
                    "dioxane", "dmf", "dmso", "ether", "ethylacetate",
                    "furane", "hexadecane", "hexane", "methanol",
                    "nitromethane", "octanol", "woctanol", "phenol",
                    "toluene", "thf"]
        self.solvName = EcceComboBox(self,
                                     choices=solvents,
                                     name="ES.Theory.Solvent.Name",
                                     default=0,
                                     label="Solvent:",
                                     export=1)
        solvSizer.AddWidget(self.solvName)
        self.panelSizer.Add(solvSizer)

        scfSizer = EcceBoxSizer(self, label="SCF", cols=1)
        self.etemp = EcceFloatInput(self,
                                    unit="K",
                                    name="ES.Theory.SCF.ElectronicTemperature",
                                    default=300.0,
                                    hardRange="(0..)",
                                    softRange="[100..5000]",
                                    label="Electronic temperature:",
                                    export=1)
        scfSizer.AddWidget(self.etemp)
        self.accuracy = EcceFloatInput(self,
                                       name="ES.Theory.SCF.Accuracy",
                                       default=1.0,
                                       hardRange="(0..)",
                                       softRange="[0.0001..1000]",
                                       label="Accuracy (lower is tighter):",
                                       export=1)
        scfSizer.AddWidget(self.accuracy)
        self.panelSizer.Add(scfSizer)

        self.AddButtons()

    def CheckDependency(self):
        self.solvName.Enable(self.solvModel.GetValue() != "None")


frame = XtbTheoryFrame(None,
                       title="ECCE xTB Editor: Theory Details",
                       app=app,
                       helpURL="")
