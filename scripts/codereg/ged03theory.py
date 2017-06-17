from templates import *

class Ged03TheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = Ged03TheoryPanel(self, helpURL)
        self.Finalize()
        
class Ged03TheoryPanel(EccePanel):
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
            initGuessChoice = ["Gaussian Guess",
                               "Core Hamiltonian",
                               "Extended Huckel"]
        else:
            initGuessChoice = ["Gaussian Guess",
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
        
        self.useCartesianBox = EcceCheckBox(self,
                                           label = " Use Cartesian Coordinates",
                                           name = "ES.Theory.UseCartesian",
                                           default = False,
                                           export = 1)
        genopsSizer.AddWidget(self.useCartesianBox)
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
                                         default = 0.1,
                                         hardRange = "[0..)",
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
                            "M06 (hybrid)",
                            "X3LYP (hybrid)",
                            "PBE0 (hybrid)",
                            "BP86 (hybrid)",
                            "SVWN 5 (local)",
                            "SVWN 1/RPA (local)",                            
                            "BLYP (nonlocal)",
                            "VSXC (nonlocal)",
                            "HCTH (nonlocal)",
                            "HCTH/407 (nonlocal)",
                            "HCTH/147 (nonlocal)",
                            "HCTH/93 (nonlocal)",
                            "tHCTH (nonlocal)",
                            "M06L (nonlocal)",
                            "B97D (nonlocal)",
                            "B97D3 (nonlocal)",
                            "SOGGA11 (nonlocal)",
                            "M11L (nonlocal)",
                            "N12 (nonlocal)",
                            "MN12L (nonlocal)",
                            "B3LYP (hybrid)",
                            "B3P86 (hybrid)",
                            "B3PW91 (hybrid)",
                            "B1B95 (hybrid)",
                            "B1LYP (hybrid)",
                            "BB95 (hybrid)",
                            "BB1K (hybrid)",
                            "MPW1PW91 (hybrid)",
                            "MPWPW91 (hybrid)",
                            "MPW1K (hybrid)",
                            "MPW1LYP (hybrid)",
                            "MPW1PBE (hybrid",
                            "MPW3PBE (hybrid)",
                            "O3LYP (hybrid)",
                            "Becke98 (hybrid)",
                            "Becke97 (1) (hybrid)",
                            "Becke97 (2) (hybrid)",
                            "PBE1PBE (hybrid)",
                            "Becke Half and Half (hybrid)",
                            "Becke Half and LYP Half (hybrid)",
                            "BMK (hybrid)",
                            "MO6HF (hybrid)",
                            "M062X (hybrid)",
                            "APFD (hybrid)",
                            "APF (hybrid)",
                            "SOGGA11X (hybrid)",
                            "TPSSh (hybrid)",
                            "tHCTHhyb (hybrid)",
                            "HSEH1PBE (range)",
                            "OHSE2PBE (range)",
                            "OHSE1PBE (range)",
                            "wB97XD (range)",
                            "wB97 (range)",
                            "wB97X (range)",
                            "LC-wPBE (range)",
                            "CAM-B3LYP (range)",
                            "HISSbPBE (range)",
                            "M11 (range)",
                            "N12SX (range)"
                            "MN12SX (range)"
                            ]
            xcFuncDefault = 0
            if EcceGlobals.ReactionStudyFlag != 0:
                xcFuncDefault = 17

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
                            "Becke88 (nonlocal)",
                            "Perdew-Wang 91",
                            "Modified Perdew-Wang 91",
                            "Gill 96",
                            "PBE 96",
                            "OPTX",
                            "TPSS",
#                            "REvTPSS",
                            "Becke 89", #BRx
                            "PKZB",
                            "wPBEh", #aka HSE
                            "PBEh" #1998 rev of PBE
                            ]
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
                              "Perdew 1991 (nonlocal)",
                              "Becke95 (nonlocal)",
                              "PBE 96 (nonlocal)",
                              "TPSS",
#                              "RevTPSS",
                              "KCIS",
                              "BRC",
                              "PKZB"
                              ]
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

            # Theory options solvation -CAO
            sovSizer = EcceBoxSizer(self,
                                    label = "Solvation",
                                    cols = 2)
            sovLeftSizer = EcceVBoxSizer()
            sovRightSizer = EcceVBoxSizer()


            # Use solvation
            self.useSCRF = EcceCheckBox(self, #useCosmo
                                         label = " Use SCRF",
                                         name = "ES.Theory.SCF.UseSCRF",
                                         default = False)
            sovLeftSizer.AddWidget(self.useSCRF,
                                   border = EcceGlobals.BorderDefault)
            # SCRF type
            scrfChoice = ["PCM",
                          "CPCM",
                          "IPCM",
                          "SCIPCM",
                          "SMD",
                          "Dipole"]
            self.scrf = EcceComboBox(self,
                                        choices = scrfChoice,
                                        name = "ES.Theory.SCF.SCRF",
                                        label = "SCRF type:",
                                        default = 0)
            sovRightSizer.AddWidget(self.scrf,
                                    border = EcceGlobals.BorderDefault)
            # Solvent type 
            solventChoice = ["Water",
                             "Acetonitrile",
                             "Methanol",
                             "Ethanol",
                             "Manual",
                             "Benzene",
                             "Chloroform",
                             "Diethylether",
                             "Dichloromethane",
                             "Dichloroethane",
                             "Carbontetrachloride",
                             "Toluene",
                             "Chlorobenzene",
                             "Nitromethane",
                             "Heptane",
                             "Aniline",
                             "Acetone",
                             "Tetrahydrofuran",
                             "Dimethylsulfoxide",
                             "Argon",
                             "Krypton",
                             "Xenon",
                             "n-Octanol",
                             "1-Butanol"
                             "Cyclohexane",
                             "Isoquinoline",
                             "Quinoline",
                             ]
            self.solvent = EcceComboBox(self,
                                        choices = solventChoice,
                                        name = "ES.Theory.SCF.Solvent",
                                        label = "Solvent:",
                                        default = 0)
            sovRightSizer.AddWidget(self.solvent,
                                    border = EcceGlobals.BorderDefault)

            self.scrfDielec = EcceFloatInput(self,
                                              default = 78.4,
                                              name = "ES.Theory.SCF.Dielectric",
                                              label = "Dielectric Constant:",
                                              hardRange = "(0..)",
                                              unit = "Debye")
            sovRightSizer.AddWidget(self.scrfDielec,
                                    border = EcceGlobals.BorderDefault)

            sovSizer.AddWidget(sovLeftSizer,
                               flag = wx.ALL)
            sovSizer.AddWidget(sovRightSizer,
                               flag = wx.ALL)
            self.panelSizer.Add(sovSizer)
            # End theory options solvation -CAO

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

        # SCRF solvation -- CAO
        if EcceGlobals.Category == "DFT" or EcceGlobals.Category == "SCF":   
            self.solvent.Enable(self.useSCRF.GetValue())
            self.scrf.Enable(self.useSCRF.GetValue())
            self.scrfDielec.Enable(self.useSCRF.GetValue() and
                                   self.solvent.GetValue() == "Manual")
        # end SCRF

        if (EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC" or
            EcceGlobals.Category == "CI"):
            self.fzTog.Enable(EcceGlobals.Theory != "RMP4(DQ)" and
                              EcceGlobals.Theory != "UMP4(DQ)" and
                              EcceGlobals.NumFrozenOrbs != -1)
            self.fzSpin.Enable(self.fzTog.GetValue())
            self.exSpin.Enable(self.exTog.GetValue())


frame = Ged03TheoryFrame(None,
                         title = "ECCE Gaussian-03 Editor: Theory Details",
                         app = app,
                         helpURL = "")
