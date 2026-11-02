from templates import *


class CrestTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = CrestTheoryPanel(self, helpURL)
        self.Finalize()

class CrestTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # A solvation shell has one solvent: the molecule chosen in the
        # Runtype dialog, which the stored values carry (restore.in).
        self.qcg = EcceGlobals.RunType == "Solvation Shell"
        self.qcgSolvent = "water"
        if self.qcg and EcceGlobals.RestoreFlag == "GUIValues":
            try:
                for line in open(EcceGlobals.FdIn):
                    f = line.split("|")
                    if f[0] == "ES.Runtype.QCG.Solvent" and f[1]:
                        self.qcgSolvent = f[1].strip()
            except IOError:
                pass

        # The method (GFN2-xTB, GFN-FF, GFN2//GFN-FF) is the calculation's
        # Theory, chosen in the editor.  Charge and multiplicity come from the
        # calculation; ai.crest turns them into --chrg and --uhf.
        solvSizer = EcceBoxSizer(self, label="Implicit Solvent", cols=1)
        self.solvModel = EcceComboBox(self,
                                      choices=["None", "ALPB", "GBSA"],
                                      name="ES.Theory.Solvent.Model",
                                      default=1 if self.qcg else 0,
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
        if self.qcg:
            # Shown but not editable (and so left out of the summary).
            self.solvName.SetValue(self.qcgSolvent)
            self.solvName.Enable(False)
        else:
            self.solvName.Enable(self.solvModel.GetValue() != "None")


frame = CrestTheoryFrame(None,
                         title="ECCE CREST Editor: Theory Details",
                         app=app,
                         helpURL="")
