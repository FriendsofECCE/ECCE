from templates import *

# MIT first; the ASL models get a licence notice when chosen.
MODELS = ["medium-mpa-0", "medium-0b3", "small", "medium", "large",
          "medium-omat-0 (ASL)", "small-omat-0 (ASL)",
          "mace-matpes-pbe-0 (ASL)", "mace-matpes-r2scan-0 (ASL)"]


class MlpTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = MlpTheoryPanel(self, helpURL)
        self.Finalize()

class MlpTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # The weights are downloaded on first use; ai.mlp drops the
        # " (ASL)" tag from the menu entry.
        modelSizer = EcceBoxSizer(self, label="Model", cols=1)
        self.model = EcceComboBox(self,
                                  choices=MODELS,
                                  name="ES.Theory.MLP.Model",
                                  default=0,
                                  label="MACE model:",
                                  export=1)
        modelSizer.AddWidget(self.model)
        self.aslNote = wx.StaticText(self,
            label="This model is under the Academic Software License:\n"
                  "no commercial use.")
        modelSizer.AddWidget(self.aslNote)
        self.precision = EcceComboBox(self,
                                      choices=["float32", "float64"],
                                      name="ES.Theory.MLP.Precision",
                                      default=0,
                                      label="Precision:",
                                      export=1)
        modelSizer.AddWidget(self.precision)
        self.dispersion = EcceCheckBox(self,
                                       label=" D3 dispersion (needs torch-dftd)",
                                       name="ES.Theory.MLP.Dispersion",
                                       default=False,
                                       export=1)
        modelSizer.AddWidget(self.dispersion)
        self.panelSizer.Add(modelSizer)

        self.AddButtons()

    def CheckDependency(self):
        self.aslNote.Show("ASL" in self.model.GetValue())
        self.Layout()


frame = MlpTheoryFrame(None,
                       title="ECCE MLP (MACE) Editor: Theory Details",
                       app=app,
                       helpURL="")
