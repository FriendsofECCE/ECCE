from templates import *



class GUKRunTypeFrame(EcceFrame):
  def __init__(self, parent, title, app, helpURL=""):
    EcceFrame.__init__(self, parent, title)
    panel = GUKRunTypePanel(self, helpURL)
    self.Finalize()



class GUKRunTypePanel(EccePanel):
  def __init__(self, parent, helpURL):
    EccePanel.__init__(self, parent, helpURL)

    #--GEOMETRY OPTIMAZATION---------------------------------------------------

    if EcceGlobals.RunType == "Geometry":
      self.panelSizer.Add(EcceLineLabelSeparator(self, 
                          label = "Runtype Options - Geometry Optimization"))

      self.geoLayout = EcceHBoxSizer()
      self.panelSizer.Add(self.geoLayout)

      searchAlgChoice = ["Optimise", "Optxyz", "Saddle"]
      searchAlgDefault = 1 # "Optxyz"
      if (EcceGlobals.ReactionStudyFlag!=0 and
          EcceGlobals.CalculationName=="Transition-State"):
          searchAlgDefault = 2 # "Saddle"
      self.searchAlg = EcceComboBox(self, label = "Search Algorithm:",
                                    name = "ES.Runtype.GeomOpt.SearchAlgorithm",
                                    choices = searchAlgChoice,
                                    default = searchAlgDefault,
                                    export = 1)
      self.geoLayout.AddWidget(self.searchAlg)

      #--GEOMETRY CONVERGENCE--------------------------------------------------

      self.panelSizer.Add(EcceLineLabelSeparator(self, label = "Convergence"))
    
      convergenceLayout1 = EcceHBoxSizer()
      self.panelSizer.Add(convergenceLayout1)
  
      cnvOptsChoice = ["Tight", "Default", "Loose", "Explicit"]
      cnvOptsDefault = 1 # "Default"
      self.cnvOpts = EcceComboBox(self, label = "Tolerance:" + 10*" ",
                                  name = "ES.Runtype.GeomOpt.Convergence",
                                  choices = cnvOptsChoice,
                                  default = cnvOptsDefault,
                                  export = 1)
      convergenceLayout1.AddWidget(self.cnvOpts)
  
      self.cnvXtol = EcceFloatInput(self, label = "     Max. Geometry Change:",
                                    name = "ES.Runtype.GeomOpt.StepSizeTol",
                                    unit = "Bohr",
                                    softRange = "[1e-5..1e-2]",
                                    hardRange = "[0..)",
                                    default = 0.003,
                                    export = 1)
      convergenceLayout1.AddWidget(self.cnvXtol)
  
      convergenceLayout2 = EcceHBoxSizer()
      self.panelSizer.Add(convergenceLayout2)
  
      self.maxSteps = EcceSpinCtrl(self, name = "ES.Runtype.GeomOpt.MaxSteps",
                                  label = "Maximum Steps:",
                                  softRange = "[1..100]",
                                  hardRange = "[0..)",
                                  default = 20)
      convergenceLayout2.AddWidget(self.maxSteps)
  
      self.cnvXmax = EcceFloatInput(self, label = "        Max. Step Size:",
                                    name = "ES.Runtype.GeomOpt.StepSizeMax",
                                    unit = "Bohr",
                                    softRange = "[1e-3..1]",
                                    hardRange = "[0..)",
                                    default = 0.2)
      convergenceLayout2.AddWidget(self.cnvXmax)

    self.AddButtons()


  def CheckDependency(self):
    if EcceGlobals.RunType == "Geometry":
      self.cnvXtol.Enable(self.cnvOpts.GetValue() == "Explicit")



frame = GUKRunTypeFrame(None, title = "ECCE GAMES-UK Editor: Runtype Details",
                        app = app, helpURL = "")
