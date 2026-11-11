from templates import *


class VinaTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = VinaTheoryPanel(self, helpURL)
        self.Finalize()

class VinaTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # Vina has one scoring function and no theory options; this dialog
        # only says what the number is.
        noteSizer = EcceBoxSizer(self, label="Scoring", cols=1)
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
