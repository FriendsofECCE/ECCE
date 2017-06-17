from templates import *

class Ged98TheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = Ged98TheoryPanel(self, helpURL)
        self.Finalize()
            
class Ged98TheoryPanel(EccePanel):
    def __init__(self,parent,helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # SCF SETTINGS
        generalSizer = EcceBoxSizer(self, style = 1, cols = 2)
        
        scfComputedChoice = ["Direct",
                             "In Core",
                             "To Disk"]
        self.scfComputed = EcceComboBox(self,
                                        choices = scfComputedChoice,
                                        name = "ES.Theory.SCF.Direct",
                                        default = 0,
                                        label = "SCF Computed:")
        generalSizer.AddWidget(self.scfComputed)

        if EcceGlobals.Category != "SE":
            initGuessChoice = ["Gaussian-98 Guess",
                               "Core Hamiltonian",
                               "Extended Huckel"]
        else:
            initGuessChoice = ["Gaussian-98 Guess",
                               "Extended Huckel"]
        self.initGuess = EcceComboBox(self,
                                      choices = initGuessChoice,
                                      name = "ES.Theory.SCF.InitialGuess",
                                      default = 0,
                                      label = "Orbital Initial Guess:")
        generalSizer.AddWidget(self.initGuess)
        self.panelSizer.Add(generalSizer)

        # GENERAL OPTIONS
        genopsSizer = EcceBoxSizer(self, "General Options", 2)
        self.checkPointBox = EcceCheckBox(self,
                                          label = " Create Checkpoint File",
                                          name = "ES.Theory.CreateCheckpoint",
                                          default = True,
                                          export = 1)
        genopsSizer.AddWidget(self.checkPointBox)

        self.useSymmetryBox = EcceCheckBox(self,
                                           label = " Use Available Symmetry",
                                           name = "ES.Theory.UseSymmetry",
                                           default = True,
                                           export = 1)
        genopsSizer.AddWidget(self.useSymmetryBox)
        self.panelSizer.Add(genopsSizer)

        # MEMEORY/DISK LIMIT
        if EcceGlobals.Category == "SCF":
            memTitle = "Memory/Disk Limits"
        else:
            memTitle = "Memory Limit"
        memorySizer = EcceBoxSizer(self,
                                   label = memTitle,
                                   cols = 4)
        
        self.memoryBox = EcceCheckBox(self,
                                      label = " Memory:",
                                      name = "ES.Theory.SCF.Memory",
                                      default = False)
        memorySizer.AddWidget(self.memoryBox)
        
        self.memorySpin = EcceSpinCtrl(self,
                                       hardRange = "[0..)",
                                       unit = "Megawords",
                                       name = "ES.Theory.SCF.MemorySize",
                                       default = 4,
                                       export = 1)
        memorySizer.AddWidget(self.memorySpin)

        if EcceGlobals.Category == "SCF":
            self.diskBox = EcceCheckBox(self,
                                        label = " Disk:",
                                        name = "ES.Theory.SCF.Disk",
                                        default = False)
            memorySizer.AddWidget(self.diskBox)
        
            self.diskSpin = EcceSpinCtrl(self,
                                         hardRange = "[0..)",
                                         unit = "Megawords",
                                         name = "ES.Theory.SCF.DiskSize",
                                         default = 64,
                                         export = 1)
            memorySizer.AddWidget(self.diskSpin)
        self.panelSizer.Add(memorySizer)

        # SCF CONVERGENCE
        scfSizer = EcceBoxSizer(self,
                                label = "SCF Convergence",
                                cols = 2)
        
        if (EcceGlobals.RunType != "Energy" or
            EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC" or
            EcceGlobals.Category == "CI"):
            densityDefault = 1.0e-08
        else:
            densityDefault = 0.0001
        self.densityInput = EcceExpInput(self,
                                         default = densityDefault,
                                         name = "ES.Theory.SCF.ConvergenceDensity.Value",
                                         softRange = "[1e-10..1e-2]",
                                         hardRange = "(0..)",
                                         label = "Delta Density:")
        scfSizer.AddWidget(self.densityInput)

        algorithmDefault = 0
        if ((EcceGlobals.Category == "SCF" and
             EcceGlobals.Theory != "ROHF") or
            EcceGlobals.Category == "DFT" or
            EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC" or
            EcceGlobals.Category == "CI"):
            algorithmChoice = ["DIIS",
                               "Iterative SCF",
                               "Quadratic"]
        elif (EcceGlobals.Category == "SCF" and
              EcceGlobals.Theory == "ROHF"):
            algorithmChoice = ["DIIS",
                               "Iterative SCF"]
        elif (EcceGlobals.Category == "SE"):
            if (EcceGlobals.Theory == "RMINDO3" or
                EcceGlobals.Theory == "ROMINDO3" or
                EcceGlobals.Theory == "UMINDO3" or
                EcceGlobals.Theory == "RMNDO" or
                EcceGlobals.Theory == "ROMNDO" or
                EcceGlobals.Theory == "UMNDO" or
                EcceGlobals.Theory == "RAM1" or
                EcceGlobals.Theory == "UAM1"):
                algorithmChoice = ["DIIS",
                                   "Iterative SCF"]
                algorithmDefault = 1
            else:
                algorithmChoice = ["Iterative SCF"]
        self.algorithm = EcceComboBox(self,
                                      choices = algorithmChoice,
                                      name = "ES.Theory.SCF.ConvergenceAlgorithm",
                                      default = algorithmDefault,
                                      label = "Algorithm:")
        scfSizer.AddWidget(self.algorithm)

        self.maxIterationsSpin = EcceSpinCtrl(self,
                                              hardRange = "[0..)",
                                              name = "ES.Theory.SCF.ConvergenceIterations",
                                              default = 64,
                                              label = "Max. Iterations:")
        scfSizer.AddWidget(self.maxIterationsSpin)

        levelSizer = EcceHBoxSizer()
        self.levelBox = EcceCheckBox(self,
                                     label = " Level Shift Size: ",
                                     name = "ES.Theory.SCF.LevelShift",
                                     default = False)
        levelSizer.AddWidget(self.levelBox)
        
        self.levelInput = EcceFloatInput(self,
                                         unit = "Hartree",
                                         name = "ES.Theory.SCF.LevelShiftSize",
                                         hardRange = "[0..)",
                                         default = 0.1,
                                         export = 1)
        levelSizer.Add(self.levelInput)
        scfSizer.AddWidget(levelSizer)
        self.panelSizer.Add(scfSizer)
        
        # DFT Options
        if EcceGlobals.Category == "DFT":
            dftSizer = EcceBoxSizer(self,
                                    label = "DFT Convergence",
                                    cols = 1)
            
            exchangeSizer = EcceLineLabelVBoxSizer(self,
                                         label = "Exchange-Correlation Functionals")

            xcFuncChoice = ["None",
                            "SVWN 5 (local)",
                            "SVWN 1/RPA (local)",
                            "BLYP (nonlocal)",
                            "B3LYP (hybrid)",
                            "Becke Half and Half (hybrid)"]
            xcFuncDefault = 0
            if EcceGlobals.ReactionStudyFlag != 0:
                xcFuncDefault = 5

            self.xcFunc = EcceComboBox(self,
                                       choices = xcFuncChoice,
                                       name = "ES.Theory.DFT.XCFunctionals",
                                       default = xcFuncDefault,
                                       label = "Combined XC:")
            if xcFuncDefault != 0:
                self.xcFunc.export = 1
            exchangeSizer.AddWidget(self.xcFunc)

            exFuncChoice = ["Slater (local)",
                            "X-Alpha (local)",
                            "Becke88 (nonlocal)"]
            self.exFunc = EcceComboBox(self,
                                       choices = exFuncChoice,
                                       name = "ES.Theory.DFT.ExchangeFunctionals",
                                       default = 0,
                                       label = "Exchange:",
                                       export = 1)
            exchangeSizer.AddWidget(self.exFunc)
            
            corrFuncChoice = ["VWN 5 (local)",
                              "VWN 1/RPA (local)",
                              "Perdew 1981 (local)",
                              "Lee-Yang-Parr (nonlocal)",
                              "Perdew 1986 (nonlocal)",
                              "Perdew 1991 (nonlocal)"]
            self.corrFunc = EcceComboBox(self,
                                         choices = corrFuncChoice,
                                         name = "ES.Theory.DFT.CorrelationFunctionals",
                                         default = 0,
                                         label = "Correlation:",
                                         export = 1)
            exchangeSizer.AddWidget(self.corrFunc)

            dftSizer.AddWidget(exchangeSizer,
                               flag=EcceGlobals.FlagDefault|wx.EXPAND)
            
            gridOptChoice = ["Ultra Fine",
                             "Fine",
                             "Coarse"]
            self.gridOpt = EcceComboBox(self,
                                        choices = gridOptChoice,
                                        name = "ES.Theory.DFT.GridDensity",
                                        default = 1,
                                        label = "Grid Quality:")
            dftSizer.AddWidget(self.gridOpt)

            self.panelSizer.Add(dftSizer)

        # THEORY OPTIONS MP
        if EcceGlobals.Category == "MP":
            mpSizer = EcceBoxSizer(self,
                                   label = "Theory Options - MP",
                                   cols = 2)

            mpOptChoice = ["Direct",
                           "Full-Direct",
                           "Semi-Direct",
                           "In Core"]
            self.mpOpt = EcceComboBox(self,
                                      choices = mpOptChoice,
                                      name = "ES.Theory.MP.Direct",
                                      default = 0,
                                      label = "Computed:")
            mpSizer.AddWidget(self.mpOpt)
            self.panelSizer.Add(mpSizer)
            
        # THEORY OPTIONS CC
        if EcceGlobals.Category == "CC":
            ccSizer = EcceBoxSizer(self,
                                   label = "Theory Options - CC",
                                   cols = 2)

            if (EcceGlobals.RunType == "Energy" or
                EcceGlobals.RunType == "Vibration" or
                EcceGlobals.RunType == "Polarizability" or
                EcceGlobals.RunType == "Magnetic"):
                ccDEDefault = 1.0e-7
            else:
                ccDEDefault = 1.0e-8
            self.ccDEInput = EcceFloatInput(self,
                                            unit = "Hartree",
                                            name = "ES.Theory.CC.ConvergenceEnergy.Value",
                                            default = ccDEDefault,
                                            hardRange = "(0.0..)",
                                            softRange = "[1e-12..1e-2]",
                                            label = "Delta Energy:")
            ccSizer.AddWidget(self.ccDEInput)

            self.ccMaxSpin = EcceSpinCtrl(self,
                                          hardRange = "[0..)",
                                          name = "ES.Theory.CC.ConvergenceIterations",
                                          default = 50,
                                          label = "Max. Iterations:")
            ccSizer.AddWidget(self.ccMaxSpin)
            self.panelSizer.Add(ccSizer)

        # THEORY OPTIONS CI
        if EcceGlobals.Category == "CI":
            ciSizer = EcceBoxSizer(self,
                                   label = "Theory Options - CI",
                                   cols = 2)
            
            if (EcceGlobals.RunType == "Energy" or
                EcceGlobals.RunType == "Vibration" or
                EcceGlobals.RunType == "Polarizability" or
                EcceGlobals.RunType == "Magnetic"):
                ciDEDefault = 1.0e-7
            else:
                ciDEDefault = 1.0e-8
            self.ciDEInput = EcceFloatInput(self,
                                            unit = "Hartree",
                                            name = "ES.Theory.CI.ConvergenceEnergy.Value",
                                            default = ciDEDefault,
                                            hardRange = "(0.0..)",
                                            softRange = "[1e-12..1e-2]",
                                            label = "Delta Energy:")
            ciSizer.AddWidget(self.ciDEInput)

            self.ciMaxSpin = EcceSpinCtrl(self,
                                          hardRange = "[0..)",
                                          name = "ES.Theory.CI.ConvergenceIterations",
                                          default = 50,
                                          label = "Max. Iterations:")
            ciSizer.AddWidget(self.ciMaxSpin)

            self.ciStateSpin = EcceSpinCtrl(self,
                                            hardRange = "[1..)",
                                            name = "ES.Theory.CI.ConvergenceStates",
                                            default = 1,
                                            label = "States to Converge:")
            ciSizer.AddWidget(self.ciStateSpin)
            self.panelSizer.Add(ciSizer)

        # FROZEN CORE OPTIONS
        if (EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC" or
            EcceGlobals.Category == "CI"):
            fzSizer = EcceBoxSizer(self,
                                   label = "Frozen Core Options",
                                   cols = 2)

            if (EcceGlobals.Theory != "RMP4(DQ)" and
                EcceGlobals.Theory != "UMP4(DQ)" and
                EcceGlobals.NumFrozenOrbs != -1):
                fzTogDefault = 1
            else:
                fzTogDefault = 0
            self.fzTog = EcceCheckBox(self,
                                      label = " Use Frozen Core Orbitals",
                                      name = "ES.Theory.UseFrozenCores",
                                      default = fzTogDefault,
                                      export = 1)
            fzSizer.AddWidget(self.fzTog)
            
            if (EcceGlobals.NumFrozenOrbs < 0):
              defFrozen = 0
            else:
              defFrozen = EcceGlobals.NumFrozenOrbs
            self.fzSpin = EcceSpinCtrl(self,
                                       hardRange = "[0..%i]" %
                                       EcceGlobals.NumOccupiedOrbs,
                                       name = "ES.Theory.CorrelOrb.FrozenCoreValue",
                                       default = defFrozen,
                                       label = "Freeze:",
                                       export = 1)
            fzSizer.AddWidget(self.fzSpin)

            self.exTog = EcceCheckBox(self,
                                      label = " Number to Exclude:",
                                      name = "ES.Theory.CorrelOrb.ExcludedVirtual",
                                      default = 0)
            fzSizer.AddWidget(self.exTog)

            if (EcceGlobals.NumVirtualOrbs < 0):
              maxExclude = 0
            else:
              maxExclude = EcceGlobals.NumVirtualOrbs
            self.exSpin = EcceSpinCtrl(self,
                                       hardRange = "[0..%i]" % maxExclude,
                                       name = "ES.Theory.CorrelOrb.ExcludedVirtualValue",
                                       default = 0,
                                       export = 1)
            fzSizer.AddWidget(self.exSpin)
            self.panelSizer.Add(fzSizer)
            
        self.AddButtons()
        
    def CheckDependency(self):
        try:
            self.memorySpin.Enable(self.memoryBox.GetValue())
        except wx.PyDeadObjectError:
            print "Strange Error"
        if EcceGlobals.Category == "SCF":
            self.diskSpin.Enable(self.diskBox.GetValue())
        self.levelInput.Enable(self.levelBox.GetValue())
        if EcceGlobals.Category == "DFT":
            self.exFunc.Enable(not self.xcFunc.GetSelection())
            self.corrFunc.Enable(not self.xcFunc.GetSelection())

        if (EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC" or
            EcceGlobals.Category == "CI"):
            self.fzTog.Enable(EcceGlobals.Theory != "RMP4(DQ)" and
                              EcceGlobals.Theory != "UMP4(DQ)" and
                              EcceGlobals.NumFrozenOrbs != -1)
            self.fzSpin.Enable(self.fzTog.GetValue())
            self.exSpin.Enable(self.exTog.GetValue())


frame = Ged98TheoryFrame(None,
                         title = "ECCE Gaussian-98 Editor: Theory Details",
                         app = app,
                         helpURL = "")
