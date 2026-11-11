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
            # The cell is relaxed as by ASE's FrechetCellFilter; a structure
            # without a cell is refused by ai.mlp.
            self.relaxCell = EcceCheckBox(self,
                                          label=" Relax the cell as well as the atoms",
                                          name="ES.Runtype.GeomOpt.RelaxCell",
                                          default=False,
                                          export=1)
            optSizer.AddWidget(self.relaxCell)
            # Enthalpy E + pV is minimised; used only with the cell relaxed.
            self.pressure = EcceFloatInput(self,
                                           unit="GPa",
                                           name="ES.Runtype.GeomOpt.Pressure",
                                           default=0.0,
                                           softRange="[-10..100]",
                                           label="External pressure:",
                                           export=1)
            optSizer.AddWidget(self.pressure)
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

        if EcceGlobals.RunType == "Molecular Dynamics":
            mdSizer = EcceBoxSizer(self, label="Molecular Dynamics", cols=1)
            self.ensemble = EcceComboBox(self,
                                         choices=["NVT (Bussi thermostat)", "NVE"],
                                         name="ES.Runtype.MD.Ensemble",
                                         default=0,
                                         label="Ensemble:",
                                         export=1)
            mdSizer.AddWidget(self.ensemble)
            self.temperature = EcceFloatInput(self, unit="K",
                                              name="ES.Runtype.MD.Temperature",
                                              default=300.0, hardRange="[0..)",
                                              softRange="[1..3000]",
                                              label="Temperature:", export=1)
            mdSizer.AddWidget(self.temperature)
            self.timeStep = EcceFloatInput(self, unit="fs",
                                           name="ES.Runtype.MD.TimeStep",
                                           default=1.0, hardRange="(0..10]",
                                           softRange="[0.2..2]",
                                           label="Time step:", export=1)
            mdSizer.AddWidget(self.timeStep)
            self.steps = EcceSpinCtrl(self, hardRange="[1..)",
                                      softRange="[1..1000000]",
                                      name="ES.Runtype.MD.Steps", default=1000,
                                      label="Steps:", export=1)
            mdSizer.AddWidget(self.steps)
            self.tau = EcceFloatInput(self, unit="fs", name="ES.Runtype.MD.Tau",
                                      default=100.0, hardRange="(0..)",
                                      softRange="[10..10000]",
                                      label="Thermostat time constant:", export=1)
            mdSizer.AddWidget(self.tau)
            self.every = EcceSpinCtrl(self, hardRange="[1..)",
                                      softRange="[1..100000]",
                                      name="ES.Runtype.MD.Every", default=10,
                                      label="Save a frame every (steps):",
                                      export=1)
            mdSizer.AddWidget(self.every)
            self.seed = EcceSpinCtrl(self, hardRange="[0..)",
                                     softRange="[0..1000000]",
                                     name="ES.Runtype.MD.Seed", default=1,
                                     label="Random seed:", export=1)
            mdSizer.AddWidget(self.seed)
            self.mdTime = EcceFloatInput(self, unit="minutes",
                                         name="ES.Runtype.MD.TimeLimit",
                                         default=0.0, hardRange="[0..)",
                                         softRange="[0..1440]",
                                         label="Time limit (0 = none):",
                                         export=1)
            mdSizer.AddWidget(self.mdTime)
            self.panelSizer.Add(mdSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = MlpRunTypeFrame(None,
                        title="ECCE MLP (MACE) Editor: Runtype Details",
                        app=app,
                        helpURL="")
