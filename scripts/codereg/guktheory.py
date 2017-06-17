from templates import *

class GUKTheoryFrame(EcceFrame):
  def __init__(self, parent, title, app, helpURL=""):
    EcceFrame.__init__(self, parent, title)
    panel = GUKTheoryPanel(self, helpURL)
    self.Finalize()



class GUKTheoryPanel(EccePanel):
  def __init__(self, parent, helpURL=""):
    EccePanel.__init__(self, parent, helpURL)

    #--GENERAL SETTINGS--------------------------------------------------------
    self.topLayout = EcceHBoxSizer()
    self.panelSizer.Add(self.topLayout)

    self.symmetryTog = EcceCheckBox(self, label = "Use Available Symmetry",
                                    name = "ES.Theory.UseSymmetry",
                                    default = False,
                                    export = 1)
    self.topLayout.AddWidget(self.symmetryTog)

    self.topLayout.AddWidget(EcceSpacer(self, width = 50))

    SCFDirectChoice = []
    if EcceGlobals.Category in ["SCF", "DFT", "MP"]:
      SCFDirectChoice.append("Direct")
    SCFDirectChoice.append("Conventional")
    SCFDirectDefault = 1 # "Conventional"
    self.SCFDirect = EcceComboBox(self, label = "SCF Computed:",
                                  name = "ES.Theory.SCF.Direct",
                                  choices = SCFDirectChoice,
                                  default = SCFDirectDefault)
    self.topLayout.AddWidget(self.SCFDirect)

    #--MEMORY LIMITS-----------------------------------------------------------
    self.panelSizer.Add(EcceLineLabelSeparator(self, label = "Memory Limit"))

    self.memoryLayout = EcceHBoxSizer()
    self.panelSizer.Add(self.memoryLayout)

    self.memTog = EcceCheckBox(self, label = "",
                               default = False,
                               name = "ES.Theory.Memory")
    self.memoryLayout.AddWidget(self.memTog)

    self.memSize = EcceSpinCtrl(self, label = "Memory:",
                                name = "ES.Theory.MemorySize",
                                default = 20,
                                unit = "Megawords",
                                hardRange = "[0..)",
                                export = 1)
    self.memoryLayout.AddWidget(self.memSize)

    #--MOLECULAR ORBITALS------------------------------------------------------
    self.panelSizer.Add(EcceLineLabelSeparator(self,
                                               label = "Molecular Orbitals"))

    self.guessLayout = EcceHBoxSizer()
    self.panelSizer.Add(self.guessLayout)

    initGuessChoice = ["Atomic Guess", "Core Hamiltonian", "Vectors"]
    initGuessDefault = 0 # "Atomic Guess"
    self.initGuess = EcceComboBox(self, label = "Initial Guess:",
                                  name = "ES.Theory.SCF.InitGuess",
                                  choices = initGuessChoice,
                                  default = initGuessDefault,
                                  export = 1)
    self.guessLayout.AddWidget(self.initGuess)

    self.guessLayout.AddWidget(EcceSpacer(self, width = 10))

    self.initVector1 = EcceSpinCtrl(self, label = "Initial Vectors:",
                                    unit = "",
                                    name = "ES.Theory.SCF.InitVector1",
                                    default = 1,
                                    export = 1)
    self.guessLayout.AddWidget(self.initVector1)

    if EcceGlobals.Theory in ["UHF", "UDFT", "UMP2"]:
      self.initVector2 = EcceSpinCtrl(self, label = "",
                                      unit = "",
                                      name = "ES.Theory.SCF.InitVector2",
                                      default = 2,
                                      export = 1)
      self.guessLayout.AddWidget(self.initVector2)

    #--SCF CONVERGENCE---------------------------------------------------------
    self.panelSizer.Add(EcceLineLabelSeparator(self, label = "SCF Convergence"))

    self.cnvLayout = EcceHBoxSizer()
    self.panelSizer.Add(self.cnvLayout)

    self.cnvLeftLayout = EcceVBoxSizer()
    self.cnvLayout.AddWidget(self.cnvLeftLayout)

    self.cnvRightLayout = EcceVBoxSizer()
    self.cnvLayout.AddWidget(self.cnvRightLayout)

    if EcceGlobals.RunType in ["Geometry", "Vibration", "Gradient"]:
        gradientDefault = 1.0e-7
    else:
        gradientDefault = 1.0e-5
    self.gradient = EcceFloatInput(self, label = "Gradient:          ",
                                   name = "ES.Theory.SCF.ConvGradient",
                                   unit = "Hartree",
                                   default = gradientDefault,
                                   softRange = "[1e-10..1e-02]",
                                   hardRange = "(0..)",
                                   export = 0)
    self.cnvLeftLayout.AddWidget(self.gradient)

    self.maxIter = EcceSpinCtrl(self, label = "Max. Iterations:",
                                name = "ES.Theory.SCF.ConvIterations",
                                default = 50,
                                unit = "",
                                hardRange = "[0..)")
    self.cnvLeftLayout.AddWidget(self.maxIter)

    self.diisTog = EcceCheckBox(self, label = "Use DIIS",
                                name = "ES.Theory.SCF.UseDIIS",
                                default = True)
    self.cnvLeftLayout.AddWidget(self.diisTog)

    levelHBoxLayout = EcceHBoxSizer()
    self.cnvRightLayout.AddWidget(levelHBoxLayout, border = 0)

    self.levelShiftTog = EcceCheckBox(self, default = True,
                                      label = "Use Level Shift",
                                      name = "ES.Theory.SCF.UseLevelShift")
    levelHBoxLayout.AddWidget(self.levelShiftTog)

    self.levelShiftSize1 = EcceFloatInput(self, label = "Size:",
                                          name = "ES.Theory.SCF.LevelShiftSize1",
                                          default = 2.0,
                                          unit = "Hartree",
                                          hardRange = "[0..)",
                                          export = 1)
    levelHBoxLayout.AddWidget(self.levelShiftSize1)

    self.levelShiftIter = EcceSpinCtrl(self, label = 23*" "+"Iterations:",
                                       name = "ES.Theory.SCF.LevelShiftIter",
                                       default = 3,
                                       unit = "",
                                       hardRange = "[0..)",
                                       export = 1)
    self.cnvRightLayout.AddWidget(self.levelShiftIter)

    self.levelShiftSize2 = EcceFloatInput(self,
                                          label =5*" "+"Final Level Shift Size:",
                                          name = "ES.Theory.SCF.LevelShiftSize2",
                                          default = 0.5,
                                          unit = "Hartree",
                                          hardRange = "[0..)",
                                          export = 1)
    self.cnvRightLayout.AddWidget(self.levelShiftSize2)

    #--THEORY OPTIONS----------------------------------------------------------
    if EcceGlobals.Category == "MP":
      pass
    elif EcceGlobals.Category == "DFT":
      self.panelSizer.Add(EcceLineLabelSeparator(self, 
                          label = "Theory Options - DFT"))

      self.theoryLayout = EcceVBoxSizer()
      self.panelSizer.Add(self.theoryLayout)

      self.theoryTopLayout = EcceLineLabelVBoxSizer(self,
                              label = "Exchange-Correlation Functionals")
      self.theoryLayout.AddWidget(self.theoryTopLayout)

      self.theoryBottomLayout = EcceLineLabelVBoxSizer(self,
                               label = "Grid Options")
      self.theoryLayout.AddWidget(self.theoryBottomLayout)

      combinedXCChoice = ["None", "SVWN (LDA)", "BLYP (GGA)", "EDF1 (GGA)", 
                          "Filatov-Thiel 1997 (GGA)", "HCTH93 (GGA)", 
                          "HCTH120 (GGA)", "HCTH147 (GGA)", "HCTH407 (GGA)", 
                          "PB86 (GGA)", "PBE (GGA)", "PW91 (GGA)", 
                          "B3LYP (hybrid)", "Becke 1997 (hybrid)", 
                          "Becke 1997-1 (hybrid)", "Becke 1997-2 (hybrid)", 
                          "B1B95 (meta-GGA)", "BB1K (meta-GGA)", 
                          "BB95 (meta-GGA)"]
      combinedXCDefault = 2 # "BLYP (GGA)"
      if EcceGlobals.ReactionStudyFlag != 0:
          combinedXCDefault = 17 # "BB1K (meta-GGA)"
      self.combinedXC = EcceComboBox(self, label = "Combined XC:",
                                     name = "ES.Theory.DFT.XCFunctionals",
                                     choices = combinedXCChoice,
                                     default = combinedXCDefault,
                                     export = 1)
      self.theoryTopLayout.AddWidget(self.combinedXC)

      exchangeChoice = ["None", "Hartree-Fock", "Slater (LDA)", 
                        "Becke 1988 (GGA)", "EDF1 (GGA)", 
                        "Filatov-Thiel 1997 (GGA)", 
                        "Filatov-Thiel 1997 A (GGA)", 
                        "Perdew-Burke-Ernzerhof (GGA)", 
                        "Perdew-Wang 1991 (GGA)", "Becke 3 (hybrid)", 
                        "Becke 1997 (hybrid)"]
      exchangeDefault = 3 # "Becke 1988 (GGA)"
      self.exchange = EcceComboBox(self, label = "Exchange:       ",
                                   name = "ES.Theory.DFT.XFunctionals",
                                   choices = exchangeChoice,
                                   default = exchangeDefault,
                                   export = 1)
      self.theoryTopLayout.AddWidget(self.exchange)

      correlationChoice = ["None", "Perdew-Zunger 1981 (LDA)", 
                           "Perdew-Wang 1992 (LDA)", "Vosko-Wilk-Nusair (LDA)", 
                           "Vosko-Wilk-Nusair 5-RPA (LDA)", "EDF1 (GGA)", 
                           "Filatov-Thiel 1997 (GGA)", "Lee-Yang-Parr (GGA)", 
                           "Perdew-Burke-Ernzerhof (GGA)", "Perdew 1986 (GGA)", 
                           "Perdew 1991 (GGA)", "Perdew-Wang 1991 (GGA)", 
                           "Becke 1995 (meta-GGA)"]
      correlationDefault = 7 # "Lee-Yang-Parr (GGA)"
      self.correlation = EcceComboBox(self, label = "Correlation:     ",
                                      name = "ES.Theory.DFT.CFunctionals",
                                      choices = correlationChoice,
                                      default = correlationDefault,
                                      export = 1)
      self.theoryTopLayout.AddWidget(self.correlation)

      qualityChoice = ["Low", "Medium", "High", "Very High"]
      qualityDefault = 1 # "Medium"
      self.quality = EcceComboBox(self, label = "Quality:   ",
                                  name = "ES.Theory.DFT.GridAccuracy",
                                  choices = qualityChoice,
                                  default = qualityDefault)
      self.theoryBottomLayout.AddWidget(self.quality)

#
# Changing the angular and radial grid settings is rather involved.
# If this is going to be required I need to think about this.
#

# angular = MenuInput(main)
# angular.LABEL = "Angular:"
# angular.NAME = "ES.Theory.DFT.GridAngular"
# angular.insertItem("Gauss-Legendre")
# angular.insertItem("Lebedev")
# angular.DEFAULT = "Lebedev"
# theory2_panel.addWidget(angular)

# radial = MenuInput(main)
# radial.LABEL = "Radial:"
# radial.NAME = "ES.Theory.DFT.GridRadial"
# radial.insertItem("Euler-MacLaurin")
# radial.insertItem("Mura-Knowles")
# radial.DEFAULT = "Mura-Knowles"
# theory2_panel.addWidget(radial)

      partitionChoice = ["Becke", "Stratmann-Scuseria-Frisch (SSF)", 
                         "Murray-Handy-Laming (MHL)", "MHL4SSF", "MHL8SSF"]
      partitionDefault = 4 # "MHL8SSF"
      self.partition = EcceComboBox(self, label = "Partition:",
                                    name = "ES.Theory.DFT.GridPartition",
                                    choices = partitionChoice,
                                    default = partitionDefault)
      self.theoryBottomLayout.AddWidget(self.partition)

    #--FROZEN CORE OPTIONS-----------------------------------------------------
    if EcceGlobals.Category == "MP":
      if EcceGlobals.RunType == "Energy":
        self.panelSizer.Add(EcceLineLabelSeparator(self,
                            label = "Frozen Orbital Options"))

        frozenCoreLayout = EcceHBoxSizer()
        self.panelSizer.Add(frozenCoreLayout)
  
        self.frozenCore = EcceCheckBox(self,
                                       label = "Use Frozen Core Orbitals   ",
                                       name = "ES.Theory.PostHF.UseFrozenCore",
                                       default = False,
                                       export = 1)
        frozenCoreLayout.AddWidget(self.frozenCore)
  
        freezeCoreDefault = EcceGlobals.NumFrozenOrbs
        if EcceGlobals.NumFrozenOrbs > -1: freezeCoreDefault = 1
        else: freezeCoreDefault = 0
        self.freezeCore = EcceSpinCtrl(self, label = "Freeze:",
                                       name = "ES.Theory.PostHF.NumFrozenCore",
                                       default = freezeCoreDefault,
                                       unit = "",
                                       hardRange = "[0..%i]" % (EcceGlobals.NumOccupiedOrbs),
                                       export = 1)
        frozenCoreLayout.AddWidget(self.freezeCore)
        
        #Removed since the perl script now handles passing this value rather 
        #  than hacking an unused and unseen widget
        #self.numCore = EcceSpinCtrl(self, label = "Occupied Orbs:",
                                    #name = "ES.Theory.PostHF.NumOccupiedOrbs",
                                    #default = EcceGlobals.NumOccupiedOrbs)
        #self.numCore.export = 1
        #frozenCoreLayout.AddWidget(self.numCore)
  
  
        frozenVirtPanel = EcceHBoxSizer()
        self.panelSizer.Add(frozenVirtPanel)
  
        self.frozenVirt = EcceCheckBox(self,
                                       label = "Use Frozen Virtual Orbitals",
                                       name = "ES.Theory.PostHF.UseFrozenVirt",
                                       default = False,
                                       export = 1)
        frozenVirtPanel.AddWidget(self.frozenVirt)
  
        freezeVirtDefault = EcceGlobals.NumFrozenOrbs
        if EcceGlobals.NumFrozenOrbs > -1: freezeVirtDefault = 1
        else: freezeVirtDefault = 0
        self.freezeVirt = EcceSpinCtrl(self, label = "Freeze:",
                                       name = "ES.Theory.PostHF.NumFrozenVirt",
                                       default = freezeVirtDefault,
                                       unit = "",
                                       hardRange = "[0..%i]" % (EcceGlobals.NumVirtualOrbs),
                                       export = 1)
        frozenVirtPanel.AddWidget(self.freezeVirt)
  
        #Removed since the perl script now handles passing this value rather 
        #  than hacking an unused and unseen widget
        #self.numVirt = EcceSpinCtrl(self, label = "     Virtual Orbs:",
                                    #name = "ES.Theory.PostHF.NumVirtualOrbs",
                                    #default = EcceGlobals.NumVirtualOrbs)
        #self.numVirt.export = 1
        #frozenVirtPanel.AddWidget(self.numVirt)
  
    self.AddButtons()



  def CheckDependency(self):
    """Enable or disable fields based on current settings."""

    self.memSize.Enable(self.memTog.GetValue())

    self.initVector1.Enable(self.initGuess.GetValue() == "Vectors")
    if EcceGlobals.Theory in ["UHF", "UDFT", "UMP2"]:
      self.initVector2.Enable(self.initGuess.GetValue() == "Vectors")

    self.levelShiftSize1.Enable(self.levelShiftTog.GetValue())
    self.levelShiftIter.Enable(self.levelShiftTog.GetValue())
    self.levelShiftSize2.Enable(self.levelShiftTog.GetValue())

    if EcceGlobals.Category == "DFT":
      self.exchange.Enable(self.combinedXC.GetValue() == "None")

      self.correlation.Enable(self.combinedXC.GetValue() == "None")

      exLocal = 0
      corrLocal = 0
      if self.exchange.GetValue() == "Slater (LDA)":
        exLocal = 1
      if self.correlation.GetValue() in ["Perdew-Zunger 1981 (LDA)", 
                                         "Perdew-Wang 1992 (LDA)",
                                         "Vosko-Wilk-Nusair (LDA)",
                                         "Vosko-Wilk-Nusair 5-RPA (LDA)"]:
        corrLocal = 1

      message = ""
      if exLocal == 1 and corrLocal == 0:
        message = "The exchange functional is local and the correlation "\
                  "functional"
      elif (exLocal == 0 and corrLocal == 1):
        message += "The correlation functional is local and the exchange "\
                   "functional"
      message += " uses the Gradient correction approximation. It is "\
                 "recommended that you use either local functionals for both "\
                 "exchange and correlation or Gradient Corrected functionals "\
                 "for both exchange and correlation."
      if (exLocal != corrLocal):
        self.SetStatusText(message, EcceGlobals.WARNING)

    if EcceGlobals.Category == "MP":
      if EcceGlobals.RunType == "Energy":
        self.freezeCore.Enable(self.frozenCore.GetValue())
        self.freezeVirt.Enable(self.frozenVirt.GetValue())




frame = GUKTheoryFrame(None,
                       title = "ECCE GAMESS-UK Editor: Theory Details",
                       app = app,
                       helpURL = "")
