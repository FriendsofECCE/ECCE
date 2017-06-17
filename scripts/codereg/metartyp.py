from templates import *

class MetaRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = MetaRunTypePanel(self, helpURL)
        self.Finalize()

class MetaRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        # Steepest Descent options
        sdSizer = EcceBoxSizer(self,
                               label = "Steepest Descent",
                               cols = 2)
        sdVSizer = EcceVBoxSizer()
        self.doSDMinimize = EcceCheckBox(self,
                                     name = "ES.Runtype.SD.DoMinimize",
                                     default = False,
                                     label = "Perform SD Minimization",
                                     export = 1)
        sdVSizer.AddWidget(self.doSDMinimize)

        self.sdInnerItr = EcceSpinCtrl(self,
                                  hardRange = "[1..)",
                                  name = "ES.Runtype.SD.InnerIterations",
                                  default = 10,
                                  label = "Number of Iterations between Prints",
                                  export = 1)
        sdVSizer.AddWidget(self.sdInnerItr)
        self.sdOuterItr = EcceSpinCtrl(self,
                                  hardRange = "[1..)",
                                  name = "ES.Runtype.SD.OuterIterations",
                                  default = 10,
                                  label = "Maximum Print Iterations",
                                  export = 1)
        sdVSizer.AddWidget(self.sdOuterItr)
        sdSizer.AddWidget(sdVSizer, flag=wx.ALL)
        self.panelSizer.Add(sdSizer)

        # Car-Parrinello MD options
        cpmdSizer = EcceBoxSizer(self,
                                     label = "Car-Parrinello",
                                     cols = 2)
        cpmdAlgChoice = ["Constant Energy", "Constant Temperature"]
        self.cpmdAlg = EcceComboBox(self,
                                      choices = cpmdAlgChoice,
                                      name = "ES.Runtype.CPMD.Algorithm",
                                      default = 0,
                                      label = "CPMD Algorithm:",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdAlg)
        cpmdSizer.AddWidget(EcceSpacer(self, width = 10))
        self.cpmdtimestep = EcceFloatInput(self,
                                      default = 5.0,
                                      name = "ES.Runtype.CPMD.TimeStep",
                                      softRange = "[1e-5..20]",
                                      hardRange = "[1e-5..)",
                                      unit = "Atomic",
                                      label = "Time Step: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdtimestep)
        self.cpmdfakemass = EcceFloatInput(self,
                                      default = 500.0,
                                      name = "ES.Runtype.CPMD.FakeMass",
                                      hardRange = "[0..)",
                                      unit = "Atomic",
                                      label = "Fake Mass: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdfakemass)
        self.cpmdinnerIt = EcceSpinCtrl(self,
                                  hardRange = "[1..)",
                                  name = "ES.Runtype.CPMD.InnerIterations",
                                  default = 10,
                                  label = "Inner Iterations:",
                                  export = 1)
        cpmdSizer.AddWidget(self.cpmdinnerIt)
        self.cpmdouterIt = EcceSpinCtrl(self,
                                  hardRange = "[1..)",
                                  name = "ES.Runtype.CPMD.OuterIterations",
                                  default = 100,
                                  label = "Outer Iterations:",
                                  export = 1)
        cpmdSizer.AddWidget(self.cpmdouterIt)

        self.cpmdtotalsteps = EcceSpinCtrl(self,
                                      default = 1000,
                                      name = "ES.Runtype.CPMD.TotalSteps",
                                      label = "Total Simulation Steps: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdtotalsteps)

        self.cpmdtotaltime = EcceFloatInput(self,
                                      default = 1000,
                                      name = "ES.Runtype.CPMD.TotalTime",
                                      label = "Total Simulation Time: ",
                                      unit = "Atomic",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdtotaltime)
        self.cpmdtotaltime.SetBackgroundColour(EcceGlobals.ReadonlyColour)

        self.cpmdelectronscaling = EcceFloatInput(self,
                                      default = 1.0,
                                      name = "ES.Runtype.CPMD.electronscaling",
                                      hardRange = "[1..)",
                                      unit = "Atomic",
                                      label = "Electronic scaling: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdelectronscaling)
        self.cpmdionscaling = EcceFloatInput(self,
                                      default = 1.0,
                                      name = "ES.Runtype.CPMD.ionscaling",
                                      hardRange = "[1..)",
                                      unit = "Atomic",
                                      label = "Ionic scaling: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdionscaling)


        self.cpmdelectronPeriod = EcceFloatInput(self,
                                      default = 1000.0,
                                      name = "ES.Runtype.CPMD.electronPeriod",
                                      hardRange = "[1..)",
                                      unit = "Atomic",
                                      label = "Electron Period: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdelectronPeriod)
        self.cpmdelectronTemperature = EcceFloatInput(self,
                                      default = 298.15,
                                      name = "ES.Runtype.CPMD.electronTemperature",
                                      hardRange = "[1..)",
                                      unit = "Atomic",
                                      label = "Electron Temperature: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdelectronTemperature)

        self.cpmdionPeriod = EcceFloatInput(self,
                                      default = 1000.0,
                                      name = "ES.Runtype.CPMD.ionPeriod",
                                      hardRange = "[1..)",
                                      unit = "Atomic",
                                      label = "Ionic Period: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdionPeriod)
        self.cpmdionTemperature = EcceFloatInput(self,
                                      default = 298.15,
                                      name = "ES.Runtype.CPMD.ionTemperature",
                                      hardRange = "[1..)",
                                      unit = "Atomic",
                                      label = "Ionic Temperature: ",
                                      export = 1)
        cpmdSizer.AddWidget(self.cpmdionTemperature)
        self.cpmdTranslation = EcceCheckBox(self,
                                     name = "ES.Runtype.CPMD.Translation",
                                     default = True,
                                     label = "Constrain Translation",
                                     export = 1)
        cpmdSizer.AddWidget(self.cpmdTranslation)
        self.cpmdRotation = EcceCheckBox(self,
                                     name = "ES.Runtype.CPMD.Rotation",
                                     default = False,
                                     label = "Constrain Rotation",
                                     export = 1)
        cpmdSizer.AddWidget(self.cpmdRotation)
        self.panelSizer.Add(cpmdSizer)
        self.AddButtons()

    def CheckDependency(self):
        if (EcceGlobals.ChainedMetadynamics != 0):
          self.doSDMinimize.SetValue(False)
          self.doSDMinimize.Enable(False)
          self.sdInnerItr.Enable(False)
          self.sdOuterItr.Enable(False)

        self.cpmdtotalsteps.SetBackgroundColour(EcceGlobals.ReadonlyColour)
        inner = self.cpmdinnerIt.GetValue()
        outer = self.cpmdouterIt.GetValue()
        totalSteps = inner * outer
        self.cpmdtotalsteps.SetValue(totalSteps)
        # Magic to trigger summary field change to a field not set by the user
        self.cpmdtotalsteps.timer.Start(0, True)

        self.cpmdtotaltime.SetBackgroundColour(EcceGlobals.ReadonlyColour)
        step = float(self.cpmdtimestep.GetValue())
        totalTime = step * float(inner) * float(outer)
        self.cpmdtotaltime.SetValue(totalTime)
        # Magic to trigger summary field change to a field not set by the user
        self.cpmdtotaltime.timer.Start(0, True)


frame = MetaRunTypeFrame(None,
                        title = "ECCE Metadynamics Editor: Runtype Details",
                        app = app,
                        helpURL = "")
