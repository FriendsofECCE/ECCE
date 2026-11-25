from templates import *


class VinaTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = VinaTheoryPanel(self, helpURL)
        self.Finalize()

class VinaTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # Vina 1.2's two scoring functions; keep in step with vinatheory.dlg.
        noteSizer = EcceBoxSizer(self, label="Scoring", cols=1)
        self.scoring = EcceComboBox(self, choices=["vina", "vinardo"],
                                    name="ES.Theory.Vina.Scoring", default=0,
                                    label="Scoring function:", export=1)
        noteSizer.AddWidget(self.scoring)
        noteSizer.AddWidget(wx.StaticText(self,
            label="Vina scores a pose with an empirical function.\n"
                  "The score is an estimate in kcal/mol, not an energy from\n"
                  "a quantum calculation, and is not comparable with the\n"
                  "energies of other codes."))
        self.panelSizer.Add(noteSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = VinaTheoryFrame(None,
                        title="ECCE Vina Editor: Theory Details",
                        app=app,
                        helpURL="")
