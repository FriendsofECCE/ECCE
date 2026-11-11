from templates import *


class MlpRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = MlpRunTypePanel(self, helpURL)
        self.Finalize()

class MlpRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.RunType == "Geometry":
            optSizer = EcceBoxSizer(self, label="Geometry Optimization", cols=1)
            self.optimizer = EcceComboBox(self,
                                          choices=["FIRE", "BFGS"],
                                          name="ES.Runtype.GeomOpt.Optimizer",
                                          default=0,
                                          label="Optimizer:",
                                          export=1)
            optSizer.AddWidget(self.optimizer)
            # The cell is relaxed with ASE's FrechetCellFilter; a structure
            # without a cell is refused by ai.mlp.
            self.relaxCell = EcceCheckBox(self,
                                          label=" Relax the cell as well as the atoms",
                                          name="ES.Runtype.GeomOpt.RelaxCell",
                                          default=False,
                                          export=1)
            optSizer.AddWidget(self.relaxCell)
            self.fmax = EcceFloatInput(self,
                                       unit="eV/A",
                                       name="ES.Runtype.GeomOpt.Fmax",
                                       default=0.05,
                                       hardRange="(0..)",
                                       softRange="[0.001..1]",
                                       label="Force limit:",
                                       export=1)
            optSizer.AddWidget(self.fmax)
            self.maxSteps = EcceSpinCtrl(self,
                                         hardRange="[1..)",
                                         softRange="[1..10000]",
                                         name="ES.Runtype.GeomOpt.MaxSteps",
                                         default=200,
                                         label="Maximum steps:",
                                         export=1)
            optSizer.AddWidget(self.maxSteps)
            # The last structure is kept when a limit stops the run.
            self.timeLimit = EcceFloatInput(self,
                                            unit="minutes",
                                            name="ES.Runtype.GeomOpt.TimeLimit",
                                            default=0.0,
                                            hardRange="[0..)",
                                            softRange="[0..1440]",
                                            label="Time limit (0 = none):",
                                            export=1)
            optSizer.AddWidget(self.timeLimit)
            optSizer.AddWidget(wx.StaticText(self,
                label="A run that reaches a limit keeps its last structure."))
            self.panelSizer.Add(optSizer)

        if EcceGlobals.RunType == "Vibration":
            vibSizer = EcceBoxSizer(self, label="Vibrations", cols=1)
            # Central differences: 6N force calls at +/- this step.
            self.step = EcceFloatInput(self,
                                       unit="A",
                                       name="ES.Runtype.Vibration.Step",
                                       default=0.01,
                                       hardRange="(0..0.1]",
                                       softRange="[0.001..0.05]",
                                       label="Displacement:",
                                       export=1)
            vibSizer.AddWidget(self.step)
            self.panelSizer.Add(vibSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = MlpRunTypeFrame(None,
                        title="ECCE MLP (MACE) Editor: Runtype Details",
                        app=app,
                        helpURL="")
