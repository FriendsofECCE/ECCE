from templates import *


class CrestTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = CrestTheoryPanel(self, helpURL)
        self.Finalize()

class CrestTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # The method (GFN2-xTB, GFN-FF, GFN2//GFN-FF) is the calculation's
        # Theory, chosen in the editor.  Charge and multiplicity come from the
        # calculation; ai.crest turns them into --chrg and --uhf.
        solvSizer = EcceBoxSizer(self, label="Implicit Solvent", cols=1)
        self.solvModel = EcceComboBox(self,
                                      choices=["None", "ALPB", "GBSA"],
                                      name="ES.Theory.Solvent.Model",
                                      default=0,
                                      label="Model:",
                                      export=1)
        solvSizer.AddWidget(self.solvModel)

        # xtb's ALPB list; GBSA knows a subset (ai.crest refuses the rest).
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

        self.AddButtons()

    def CheckDependency(self):
        self.solvName.Enable(self.solvModel.GetValue() != "None")


frame = CrestTheoryFrame(None,
                         title="ECCE CREST Editor: Theory Details",
                         app=app,
                         helpURL="")
