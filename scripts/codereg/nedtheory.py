from templates import *
import os

class NedTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = NedTheoryPanel(self, helpURL)
        self.Finalize()
            
class NedTheoryPanel(EccePanel):
    def __init__(self,parent,helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # GENERAL SETTING
        generalSizer = EcceBoxSizer(self, style = 1, cols = 2)

        SCFDirectName = "ES.Theory.SCF.Direct"
        SCFDirectLabel = "SCF Computed:"
        initGuessName = "ES.Theory.SCF.InitialGuess"
        initGuessChoice = ["NWChem Atomic Guess", "Core Hamiltonian"]
        if ((EcceGlobals.Category == "SCF") or
            (EcceGlobals.Category == "MP") or
            (EcceGlobals.Category == "CC")):
            SCFDirectChoice = ["Direct", "Semi-Direct"]
            SCFDirectDefault = 1
        elif (EcceGlobals.Category == "DFT"):
            SCFDirectChoice = ["In Core", "Direct"]
            SCFDirectDefault = 0
        # Start Eric Bylaska's planewave module integration
        elif (EcceGlobals.Category == "NWPW"):
            SCFDirectName = "ES.Theory.NWPW.Spintype"
            SCFDirectLabel = "Spin Type:"
            SCFDirectChoice = ["Restricted", "Unrestricted"]
            SCFDirectDefault = 0
            initGuessName = "ES.Theory.NWPW.InitialGuess"
            initGuessChoice = ["LCAO", "LCAO_skip"]
            # End Eric Bylaska's planewave module integration
        else:
            SCFDirectChoice = ["None"]
            SCFDirectDefault = 0

        self.SCFDirect = EcceComboBox(self,
                                      choices = SCFDirectChoice,
                                      name = SCFDirectName,
                                      default = SCFDirectDefault,
                                      label = SCFDirectLabel)
        generalSizer.AddWidget(self.SCFDirect)

        self.initGuess = EcceComboBox(self,
                                      choices = initGuessChoice,
                                      name = initGuessName,
                                      default = 0,
                                      label = "  Orbital Initial Guess:")
        generalSizer.AddWidget(self.initGuess)
        self.panelSizer.Add(generalSizer)

        # Special logic for DirDyVTST task, as determined by overloading
        # the meaning of the RunType value
        if EcceGlobals.RunType != "DirDyVTST":
            geometrySizer = EcceBoxSizer(self, "Geometry", 2)

            self.symmetryTog = EcceCheckBox(self,
                                            label = " Use Available Symmetry",
                                            name = "ES.Theory.UseSymmetry",
                                            default = True,
                                            export = 1)
            geometrySizer.AddWidget(self.symmetryTog)

            self.symmetryTol = EcceFloatInput(self,
                                              name = "ES.Theory.SymmetryTol",
                                              default = 1e-2,
                                              hardRange = "(0..)",
                                              unit = "Angstroms",
                                              label = "Tolerance:")
            geometrySizer.AddWidget(self.symmetryTol)

            self.useAutoZ = EcceCheckBox(self,
                                         label = " Use Automatic Z-matrix",
                                         name = "ES.Theory.UseAutoZ",
                                         default = True)
            geometrySizer.AddWidget(self.useAutoZ)
            self.panelSizer.Add(geometrySizer)
        
            # MEMEORY/DISK LIMIT
            if EcceGlobals.Category == "SCF":
                memTitle = "Memory/Disk Limits"
            else:
                memTitle = "Memory Limit"
            memorySizer = EcceBoxSizer(self, memTitle, 4)

            self.memTog = EcceCheckBox(self,
                                       label = " Memory:",
                                       name = "ES.Theory.SCF.Memory",
                                       default = False)
            memorySizer.AddWidget(self.memTog)

            self.memSize = EcceSpinCtrl(self,
                                        hardRange = "[0..)",
                                        unit = "Megawords",
                                        name = "ES.Theory.SCF.MemorySize",
                                        default = 2,
                                        export = 1)
            memorySizer.AddWidget(self.memSize)

            if EcceGlobals.Category == "SCF":
                self.diskTog = EcceCheckBox(self,
                                            label = " Disk:",
                                            name = "ES.Theory.SCF.Disk",
                                            default = 0)
                memorySizer.AddWidget(self.diskTog)
        
                self.diskSize = EcceSpinCtrl(self,
                                             hardRange = "[0..)",
                                             unit = "Megawords",
                                             name = "ES.Theory.SCF.DiskSize",
                                             default = 64,
                                             export = 1)
                memorySizer.AddWidget(self.diskSize)
            self.panelSizer.Add(memorySizer)

        # SCF CONVERGENCE
        scfSizer = EcceBoxSizer(self, "SCF Convergence", 2)
        scfLeftSizer = EcceVBoxSizer()
        scfRightSizer = EcceVBoxSizer()
        
        if (EcceGlobals.Category == "SCF" or
            EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC"):
            algorithmChoice = ["Quadratic", "Conjugate Gradient"]
        elif (EcceGlobals.Category == "DFT"):
            algorithmChoice = ["DIIS", "Iterative SCF"]
        # Start Eric Bylaska's planewave module integration
        elif (EcceGlobals.Category == "NWPW"):
            algorithmChoice = ["CG", "LMBFGS"]
            # End Eric Bylaska's planewave module integration
        else:
            algorithmChoice = ["None"]
        self.algorithm = EcceComboBox(self,
                                      choices = algorithmChoice,
                                      name = "ES.Theory.SCF.ConvergenceAlgorithm",
                                      default = 0,
                                      label = "Algorithm:")
        scfRightSizer.AddWidget(self.algorithm,
                                border = EcceGlobals.BorderDefault)

        if (EcceGlobals.Category == "SCF" or
            EcceGlobals.Category == "DFT"):
            if (EcceGlobals.Category == "SCF"):
                gradientDefault = 1e-4
            elif (EcceGlobals.Category == "DFT"):
                gradientDefault = 5e-4
            self.gradient = EcceFloatInput(self,
                                           default = gradientDefault,
                                           name = "ES.Theory.SCF.ConvergenceGradient.Value",
                                           softRange = "[1e-10..1e-2]",
                                           hardRange = "(0..)",
                                           unit = "Hartree",
                                           label = "Gradient:")
            scfLeftSizer.AddWidget(self.gradient,
                                   border = EcceGlobals.BorderDefault)

        if (EcceGlobals.Category == "DFT"):
            self.deltaDensity = EcceFloatInput(self,
                                               default = 1e-5,
                                               name = "ES.Theory.SCF.ConvergenceDensity.Value",
                                               softRange = "[1e-10..1e-2]",
                                               hardRange = "(0..)",
                                               label = "Delta Density:")
            scfRightSizer.AddWidget(self.deltaDensity,
                                    border = EcceGlobals.BorderDefault)
            
            self.deltaEnergy = EcceFloatInput(self,
                                              default = 1e-6,
                                              name = "ES.Theory.SCF.ConvergenceEnergy.Value",
                                              softRange = "[1e-10..1e-2]",
                                              hardRange = "(0..)",
                                              unit = "Hartree",
                                              label = "Delta Energy:")
            scfRightSizer.AddWidget(self.deltaEnergy,
                                    border = EcceGlobals.BorderDefault)

        if (EcceGlobals.Category != "DFT"):
            maxIterDefault = 20
        else:
            maxIterDefault = 30
        self.maxIter = EcceSpinCtrl(self,
                                    hardRange = "[0..)",
                                    name = "ES.Theory.SCF.ConvergenceIterations",
                                    default = maxIterDefault,
                                    label = "Max. Iterations:")
        scfLeftSizer.AddWidget(self.maxIter,
                               border = EcceGlobals.BorderDefault)
        
        # Start Eric Bylaska's planewave module integration
        if (EcceGlobals.Category == "NWPW"):
            self.deltaDensity = EcceFloatInput(self,
                                               default = 1e-7,
                                               name = "ES.Theory.NWPW.ConvergenceDensity.Value",
                                               softRange = "[1e-10..1e-2]",
                                               hardRange = "(0..)",
                                               label = "Delta Density:")
            scfLeftSizer.AddWidget(self.deltaDensity,
                                   border = EcceGlobals.BorderDefault)

            self.deltaEnergy = EcceFloatInput(self,
                                              default = 1e-7,
                                              name = "ES.Theory.NWPW.ConvergenceEnergy.Value",
                                              softRange = "[1e-10..1e-2]",
                                              hardRange = "(0..)",
                                              unit = "Hartree",
                                              label = "Delta Energy:")
            scfLeftSizer.AddWidget(self.deltaEnergy,
                                   border = EcceGlobals.BorderDefault)
            # End Eric Bylaska's planewave module integration

        if (EcceGlobals.Category == "DFT"):
            dampSizer = EcceHBoxSizer()
            
            self.dampTog = EcceCheckBox(self,
                                        label = " Damping Percentage:",
                                        name = "ES.Theory.SCF.Damping",
                                        default = False)
            dampSizer.AddWidget(self.dampTog)
            
            self.dampSize = EcceSpinCtrl(self,
                                         default = 70,
                                         name = "ES.Theory.SCF.DampingValue",
                                         hardRange = "[0..100]",
                                         export = 1)
            dampSizer.AddWidget(self.dampSize)
            scfRightSizer.AddWidget(dampSizer,
                                    border = EcceGlobals.BorderDefault)

        levelSizer = EcceHBoxSizer()
        self.levelShiftTog = EcceCheckBox(self,
                                          label = " Level Shift Size:",
                                          name = "ES.Theory.SCF.LevelShift",
                                          default = False)
        levelSizer.AddWidget(self.levelShiftTog)
        
        if (EcceGlobals.Category != "DFT"):
            levelShiftSizeDefault = 20
        else:
            levelShiftSizeDefault = 0.5
        self.levelShiftSize = EcceFloatInput(self,
                                             default = levelShiftSizeDefault,
                                             name = "ES.Theory.SCF.LevelShiftSize",
                                             hardRange = "[0..)",
                                             unit = "Hartree",
                                             label = "",
                                             export = 1)
        levelSizer.AddWidget(self.levelShiftSize)
        scfRightSizer.AddWidget(levelSizer,
                                border = EcceGlobals.BorderDefault)
        
        if (EcceGlobals.Category == "SCF" or
            EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC"):
            newLevelSizer = EcceHBoxSizer()
            self.newLevelTog = EcceCheckBox(self,
                                            label = " New Level Shift After:",
                                            name = "ES.Theory.SCF.NewLevelShift",
                                            default = False)
            newLevelSizer.AddWidget(self.newLevelTog)
            
            self.newLevelCross = EcceFloatInput(self,
                                                default = 0.5,
                                                name = "ES.Theory.SCF.NewLevelShiftCrossover",
                                                hardRange = "[0..)",
                                                unit = "Hartree",
                                                label = "",
                                                export = 1)
            newLevelSizer.AddWidget(self.newLevelCross)
            scfRightSizer.AddWidget(newLevelSizer,
                                    border = EcceGlobals.BorderDefault)
            
            self.newLevelSize = EcceFloatInput(self,
                                               default = 0,
                                               name = "ES.Theory.SCF.NewLevelShiftSize",
                                               hardRange = "[0..)",
                                               unit = "Hartree",
                                               label = "       Size:")
            scfRightSizer.AddWidget(self.newLevelSize,
                                    border = EcceGlobals.BorderDefault)

        scfSizer.AddWidget(scfLeftSizer,
                           flag = wx.ALL)
        scfSizer.AddWidget(scfRightSizer,
                           flag = wx.ALL)
        self.panelSizer.Add(scfSizer)

        # Start Eric Bylaska's planewave module integration
        if (EcceGlobals.RunType != "DirDyVTST" and
            EcceGlobals.Category != "NWPW"):
            # End Eric Bylaska's planewave module integration
            sovSizer = EcceBoxSizer(self,
                                    label = "Solvation",
                                    cols = 2)
            sovLeftSizer = EcceVBoxSizer()
            sovRightSizer = EcceVBoxSizer()

            self.useCosmo = EcceCheckBox(self,
                                         label = " Use COSMO Solvation",
                                         name = "ES.Theory.SCF.UseCosmo",
                                         default = False)
            sovLeftSizer.AddWidget(self.useCosmo,
                                   border = EcceGlobals.BorderDefault)

            self.cosmoRadius = EcceFloatInput(self,
                                              default = 0.5,
                                              name = "ES.Theory.SCF.SolventRadius",
                                              label = "Solvent Radius:",
                                              hardRange = "[0..)",
                                              unit = "Angstroms")
            sovLeftSizer.AddWidget(self.cosmoRadius,
                                   border = EcceGlobals.BorderDefault)

            solventChoice = ["Water: 78.4 Debye",
                             "Methanol: 33.0 Debye",
                             "Cyclohexane: 2.0243 Debye",
                             "Benzene: 2.2825 Debye",
                             "Acetonitrile: 36.64 Debye",
                             "Other"]
            self.solvent = EcceComboBox(self,
                                        choices = solventChoice,
                                        name = "ES.Theory.SCF.Solvent",
                                        label = "Solvent:",
                                        default = 0)
            sovRightSizer.AddWidget(self.solvent,
                                    border = EcceGlobals.BorderDefault)

            self.cosmoDielec = EcceFloatInput(self,
                                              default = 78.4,
                                              name = "ES.Theory.SCF.Dielectric",
                                              label = "Dielectric Constant:",
                                              hardRange = "(0..)",
                                              unit = "Debye")
            sovRightSizer.AddWidget(self.cosmoDielec,
                                    border = EcceGlobals.BorderDefault)

            sovSizer.AddWidget(sovLeftSizer,
                               flag = wx.ALL)
            sovSizer.AddWidget(sovRightSizer,
                               flag = wx.ALL)
            self.panelSizer.Add(sovSizer)


        # THEORY OPTIONS MP
        if EcceGlobals.Category == "MP":
            mpSizer = EcceBoxSizer(self,
                                   label = "Theory Options - MP",
                                   cols = 1)
            
            self.tightConv = EcceCheckBox(self,
                                          label = " Use Tight Convergence",
                                          name = "ES.Theory.MP.UseTightConvergence",
                                          default = False)
            mpSizer.AddWidget(self.tightConv)
            self.panelSizer.Add(mpSizer)

        # THEORY OPTIONS CC
        if EcceGlobals.Category == "CC":
            ccSizer = EcceBoxSizer(self,
                                   label = "Theory Options - CC",
                                   cols = 2)
            
            self.deltaEnergyCC = EcceFloatInput(self,
                                                default = 1e-6,
                                                name = "ES.Theory.CC.ConvergenceEnergy.Value",
                                                softRange = "[1e-12..1e-2]",
                                                hardRange = "[0..)",
                                                unit = "Hartree",
                                                label = "Delta Energy:")
            ccSizer.AddWidget(self.deltaEnergyCC)

            self.maxIterCC = EcceSpinCtrl(self,
                                          default = 20,
                                          name = "ES.Theory.CC.ConvergenceIterations",
                                          hardRange = "[0..)",
                                          label = "Max. Iterations:")
            ccSizer.AddWidget(self.maxIterCC)
            self.panelSizer.Add(ccSizer)

        # Start Eric Bylaska's planewave module integration
        # THEORY OPTIONS NWPW
        if EcceGlobals.Category == "NWPW":
            nwpwSizer = EcceBoxSizer(self,
                                    label = "Theory Options - NWPW",
                                    cols = 2)

            nwpwLeftSizer = EcceVBoxSizer()
            nwpwRightSizer = EcceVBoxSizer()

            xcFuncChoice = ["lda",
                            "pbe96",
                            "revpbe",
                            "blyp",
                            "hf",
                            "pbe0",
                            "revpbe0",
                            "lda-sic",
                            "pbe96-sic",
                            "revpbe-sic",
                            "lda-0.5sic",
                            "pbe96-0.5sic",
                            "revpbe-0.5sic",
                            "lda-0.4sic",
                            "pbe96-0.4sic",
                            "revpbe-0.4sic"]
            xcFuncDefault = 0
            self.xcFunc = EcceComboBox(self,
                                       choices = xcFuncChoice,
                                       name = "ES.Theory.NWPW.XCFunctionals",
                                       default = xcFuncDefault,
                                       label = "Exchange-Correlation:")
            nwpwLeftSizer.AddWidget(self.xcFunc)

            cutoffSizer = EcceHBoxSizer()
            self.usecutoff = EcceCheckBox(self,
                                        label = "Cutoff Energy:",
                                        name = "ES.Theory.NWPW.UseCutoff",
                                        default = False)
            cutoffSizer.AddWidget(self.usecutoff)
            self.cutoff = EcceFloatInput(self,
                                         default = 30.0,
                                         name = "ES.Theory.NWPW.Cutoff",
                                         unit = "Hartree",
                                         hardRange = "[0..)",
                                         label = "",
                                         export = 1)
            cutoffSizer.AddWidget(self.cutoff)
            nwpwRightSizer.AddWidget(cutoffSizer, border=0)

            npDimensionsSizer = EcceHBoxSizer()
            self.useNpDimensions = EcceCheckBox(self,
                                        label = "np_dimensions",
                                        name = "ES.Theory.NWPW.UseNpDimensions",
                                        default = False)
            npDimensionsSizer.AddWidget(self.useNpDimensions)
            self.npDimensionsRows = EcceSpinCtrl(self,
                                       hardRange = "[-1..)",
                                       name = "ES.Theory.NWPW.NpDimensionsRows",
                                       default = -1,
                                       label = "Rows:")
            npDimensionsSizer.AddWidget(self.npDimensionsRows)
            nwpwLeftSizer.AddWidget(npDimensionsSizer)
            
            self.npDimensionsCols = EcceSpinCtrl(self,
                                       hardRange = "[-1..)",
                                       name = "ES.Theory.NWPW.NpDimensionsCols",
                                       default = -1,
                                       label = "Columns:")
            nwpwRightSizer.AddWidget(self.npDimensionsCols)

            rcutSizer = EcceHBoxSizer()
            self.usercut = EcceCheckBox(self,
                                        label = "Ewald rcut:",
                                        name = "ES.Theory.NWPW.UseRcut",
                                        default = False)
            rcutSizer.AddWidget(self.usercut)
            self.rcut = EcceFloatInput(self,
                                       default = 0.0,
                                       name = "ES.Theory.NWPW.EwaldRcut",
                                       hardRange = "[0..)",
                                       label = "")
            rcutSizer.AddWidget(self.rcut)
            nwpwLeftSizer.AddWidget(rcutSizer)

            ncutSizer = EcceHBoxSizer()
            self.usencut = EcceCheckBox(self,
                                        label = "Ewald ncut:",
                                        name = "ES.Theory.NWPW.UseNcut",
                                        default = False)
            ncutSizer.AddWidget(self.usencut)
            self.ncut = EcceSpinCtrl(self,
                                     hardRange = "[0..)",
                                     name = "ES.Theory.NWPW.EwaldNcut",
                                     default = 1,
                                     label = "")
            ncutSizer.AddWidget(self.ncut)
            nwpwRightSizer.AddWidget(ncutSizer)

            self.mapping = EcceSpinCtrl(self,
                                     hardRange = "[1..2]",
                                     name = "ES.Theory.NWPW.mapping",
                                     default = 1,
                                     label = "Parallel Mapping")
            nwpwLeftSizer.AddWidget(self.mapping)

            self.mulliken = EcceComboBox(self,
                                       choices = ["None","LCAO","Kawai"],
                                       name = "ES.Theory.NWPW.Mulliken",
                                       default = 0,
                                       label = "Mulliken:")
            nwpwRightSizer.AddWidget(self.mulliken)

            nwpwSizer.AddWidget(nwpwLeftSizer,
                                flag = wx.ALL)
            nwpwSizer.AddWidget(nwpwRightSizer,
                                flag = wx.ALL)
            self.panelSizer.Add(nwpwSizer)

            unitcellSizer = EcceBoxSizer(self,
                                   label = "Unit Cell",
                                   cols = 1)

            self.unitMenuSizer = EcceHBoxSizer()

            #Lattice support not in current NWChem 5.1.1  GDB 4/23/09
            #cellChoice = ["None","SC","FCC","BCC", "LatticeVectors", "Lattice"]
            cellChoice = ["None","SC","FCC","BCC", "LatticeVectors"]
            self.cell = EcceComboBox(self,
                                     choices = cellChoice,
                                     name = "ES.Theory.NWPW.CellType",
                                     default = 0,
                                     label = "Cell Type:",
                                     export = 1)
            self.unitMenuSizer.AddWidget(self.cell)

            bcChoice = ["periodic","aperiodic"]
            self.bc  = EcceComboBox(self,
                                     choices = bcChoice,
                                     name = "ES.Theory.NWPW.CellBoundary",
                                     default = 0,
                                     label = "  Boundary Condition:")
            self.unitMenuSizer.AddWidget(self.bc)

            self.unitNonePanel = EcceSubPanel(self)
            unitNoneSizer = EcceVBoxSizer()

            noneSpacer = EcceSpacer(self.unitNonePanel, height = 81)
            unitNoneSizer.AddWidget(noneSpacer)
            self.unitNonePanel.SetSizer(unitNoneSizer)

            #lat input
            self.unitLPanel = EcceSubPanel(self)
            unitLSizer = EcceVBoxSizer()

            self.lat = EcceFloatInput(self.unitLPanel,
                                      default = 20.0,
                                      name = "ES.Theory.NWPW.CellL",
                                      hardRange = "[0..)",
                                      label = "L:")
            unitLSizer.AddWidget(self.lat)

            latSpacer = EcceSpacer(self.unitLPanel, height = 45)
            unitLSizer.AddWidget(latSpacer)
            self.unitLPanel.SetSizer(unitLSizer)

            #lattice vector input
            self.unitVectorPanel = EcceSubPanel(self)
            unitVectorSizer = EcceBoxSizer(self, cols = 3, style = 1)

            self.a1x = EcceFloatInput(self.unitVectorPanel,
                                      default = 20.0,
                                      name = "ES.Theory.NWPW.a1x",
                                      label = "a1:")
            unitVectorSizer.AddWidget(self.a1x)
            self.a1y = EcceFloatInput(self.unitVectorPanel,
                                      default = 0.0,
                                      name = "ES.Theory.NWPW.a1y",
                                      label = "")
            unitVectorSizer.AddWidget(self.a1y)
            self.a1z = EcceFloatInput(self.unitVectorPanel,
                                      default = 0.0,
                                      name = "ES.Theory.NWPW.a1z",
                                      label = "")
            unitVectorSizer.AddWidget(self.a1z)

            self.a2x = EcceFloatInput(self.unitVectorPanel,
                                      default = 0.0,
                                      name = "ES.Theory.NWPW.a2x",
                                      label = "a2:")
            unitVectorSizer.AddWidget(self.a2x)
            self.a2y = EcceFloatInput(self.unitVectorPanel,
                                      default = 20.0,
                                      name = "ES.Theory.NWPW.a2y",
                                      label = "")
            unitVectorSizer.AddWidget(self.a2y)
            self.a2z = EcceFloatInput(self.unitVectorPanel,
                                      default = 0.0,
                                      name = "ES.Theory.NWPW.a2z",
                                      label = "")
            unitVectorSizer.AddWidget(self.a2z)

            self.a3x = EcceFloatInput(self.unitVectorPanel,
                                      default = 0.0,
                                      name = "ES.Theory.NWPW.a3x",
                                      label = "a3:")
            unitVectorSizer.AddWidget(self.a3x)
            self.a3y = EcceFloatInput(self.unitVectorPanel,
                                      default = 0.0,
                                      name = "ES.Theory.NWPW.a3y",
                                      label = "")
            unitVectorSizer.AddWidget(self.a3y)
            self.a3z = EcceFloatInput(self.unitVectorPanel,
                                      default = 20.0,
                                      name = "ES.Theory.NWPW.a3z",
                                      label = "")
            unitVectorSizer.AddWidget(self.a3z)
            self.unitVectorPanel.SetSizer(unitVectorSizer)

            # lat_a, lat_b, ... input
            #Lattice support not in current NWChem 5.1.1  GDB 4/23/09
            #self.unitLatticePanel = EcceSubPanel(self)
            #unitLatticeSizer = EcceBoxSizer(self, cols = 3, style = 1)

            #self.lat_a = EcceFloatInput(self.unitLatticePanel,
            #                          default = 20.0,
            #                          name = "ES.Theory.NWPW.lat_a",
            #                          hardRange = "[0..)",
            #                          label = "lat_a: ")
            #unitLatticeSizer.AddWidget(self.lat_a)
            #self.lat_b = EcceFloatInput(self.unitLatticePanel,
            #                          default = 20.0,
            #                          name = "ES.Theory.NWPW.lat_b",
            #                          hardRange = "[0..)",
            #                          label = "lat_b: ")
            #unitLatticeSizer.AddWidget(self.lat_b)
            #self.lat_c = EcceFloatInput(self.unitLatticePanel,
            #                          default = 20.0,
            #                          name = "ES.Theory.NWPW.lat_c",
            #                          hardRange = "[0..)",
            #                          label = "lat_c:    ")
            #unitLatticeSizer.AddWidget(self.lat_c)

            #self.lat_alpha = EcceFloatInput(self.unitLatticePanel,
            #                          default = 20.0,
            #                          name = "ES.Theory.NWPW.lat_alpha",
            #                          hardRange = "[0..)",
            #                          label = "alpha:")
            #unitLatticeSizer.AddWidget(self.lat_alpha)
            #self.lat_beta = EcceFloatInput(self.unitLatticePanel,
            #                          default = 20.0,
            #                          name = "ES.Theory.NWPW.lat_beta",
            #                          hardRange = "[0..)",
            #                          label = "beta: ")
            #unitLatticeSizer.AddWidget(self.lat_beta)
            #self.lat_gamma = EcceFloatInput(self.unitLatticePanel,
            #                          default = 20.0,
            #                          name = "ES.Theory.NWPW.lat_gamma",
            #                          hardRange = "[0..)",
            #                          label = "gamma:")
            #unitLatticeSizer.AddWidget(self.lat_gamma)

            #latticeSpacer = EcceSpacer(self.unitLatticePanel, height = 9)
            #unitLatticeSizer.AddWidget(latticeSpacer)
            #self.unitLatticePanel.SetSizer(unitLatticeSizer)

            # ngrid input
            # Eric doesn't use this to generate an input file so I'm going to
            # comment it out for now until I hear back from him. GDB 4/22/09
            #self.unitNgridPanel = EcceSubPanel(self)
            #unitNgridSizer = EcceHBoxSizer()

            #self.usengrid = EcceCheckBox(self.unitNgridPanel,
            #                            label = "ngrid:",
            #                            name = "ES.Theory.NWPW.UseNgrid",
            #                            default = False)
            #unitNgridSizer.AddWidget(self.usengrid)
            #self.ngrid1 = EcceSpinCtrl(self.unitNgridPanel,
            #                         hardRange = "[2..)",
            #                         name = "ES.Theory.NWPW.Ngrid1",
            #                         default = 32,
            #                         label = "")
            #unitNgridSizer.AddWidget(self.ngrid1)
            #self.ngrid2 = EcceSpinCtrl(self.unitNgridPanel,
            #                         hardRange = "[2..)",
            #                         name = "ES.Theory.NWPW.Ngrid2",
            #                         default = 32,
            #                         label = "")
            #unitNgridSizer.AddWidget(self.ngrid2)
            #self.ngrid3 = EcceSpinCtrl(self.unitNgridPanel,
            #                         hardRange = "[2..)",
            #                         name = "ES.Theory.NWPW.Ngrid3",
            #                         default = 32,
            #                         label = "")
            #unitNgridSizer.AddWidget(self.ngrid3)
            #self.unitNgridPanel.SetSizer(unitNgridSizer)

            unitcellSizer.AddWidget(self.unitMenuSizer, flag = wx.ALL)
            unitcellSizer.AddWidget(self.unitNonePanel, flag = wx.ALL)
            unitcellSizer.AddWidget(self.unitLPanel, flag = wx.ALL)
            unitcellSizer.AddWidget(self.unitVectorPanel, flag = wx.ALL)
            #unitcellSizer.AddWidget(self.unitLatticePanel, flag = wx.ALL)
            #unitcellSizer.AddWidget(self.unitNgridPanel, flag = wx.ALL)
            self.panelSizer.Add(unitcellSizer)
            # End Eric Bylaska's planewave module integration

        # THEORY OPTIONS DFT
        if EcceGlobals.Category == "DFT":
            dftSizer = EcceBoxSizer(self,
                                    label = "Theory Options - DFT",
                                    cols = 2)

            exchangeSizer = EcceLineLabelVBoxSizer(self,
                                         label = "Exchange-Correlation Functionals")
            
            xcFuncChoice = ["None",
                            "HCTH (Gradient Corr.)",
                            "HCTH 120 (Gradient Corr.)",
                            "HCTH 147 (Gradient Corr.)",
                            "HCTH 407 (Gradient Corr.)",
                            "HCTHP 14 (Gradient Corr.)",
                            "BLYP (hybrid)", # added by CAO
                            "B1LYP (hybrid)", # added by CAO
                            "B3LYP (hybrid)",
                            "CAM-B3LYP (range)", # added by CAO
                            "Adiabatic Conn. (hybrid)",
                            "B3PW91 (hybrid)",   # added by CAO
                            "BP86 (hybrid)", # added by CAO
                            "B3P86 (hybrid)", # added by CAO
                            "X3LYP (hybrid)", # added by CAO
                            "mPW1PW91 (hybrid)", # added by CAO
                            "mPW1PBE (hybrid)", # added by CAO
                            "mPW3PBE (hybrid)", # added by CAO
                            "mPW1LYP (hybrid)", # added by CAO             
                            "TPSSh (hybrid)", # added by CAO
                            "VSXC (hybrid)", # added by CAO
                            "Gaussian B3 (hybrid)", # added by CAO
                            "BOP (Gradient Corr.)",# added by CAO
                            "PBEOP (Gradient Corr.)",# added by CAO
                            "M05 (hybrid)",# added by CAO
                            "M05-2X (hybrid)",# added by CAO
                            "M06 (hybrid)",# added by CAO
                            "M06-HF (hybrid)",# added by CAO
                            "M06-2X (hybrid)",# added by CAO
                            "M06-L (hybrid)",# added by CAO
                            "Becke Half and Half (hybrid)",
                            "Becke 1997 (hybrid)",
                            "Becke 1997-1 (hybrid)",
                            "Becke 1997-2 (hybrid)",
                            "Becke 1997-GGA1 (Gradient Corr.)",
                            "Becke 1998 (hybrid)",
                            "PBE0 (hybrid)",
                            "Mod. Perdew-Wang 1K (hybrid)",
                            "BB1K (hybrid)",
                            "Filatov-Thiel 1997 (Gradient Corr.)"]
            xcFuncDefault = 0

            if EcceGlobals.ReactionStudyFlag != 0:
                xcFuncDefault =27 # CAO --changed from 15 to 37 to reflect added XCs
            elif os.environ.has_key("ECCE_NWCHEM_DFT_USE_B3LYP"):
                if os.environ["ECCE_NWCHEM_DFT_USE_B3LYP"] == "true":
                    xcFuncDefault = 8 #  CAO -- changed from 7 to 8 to reflect reordering
            self.xcFunc = EcceComboBox(self,
                                       choices = xcFuncChoice,
                                       name = "ES.Theory.DFT.XCFunctionals",
                                       default = xcFuncDefault,
                                       label = "Combined XC:")
            if xcFuncDefault != 0:
                self.xcFunc.export = 1
            exchangeSizer.AddWidget(self.xcFunc)

            exFuncChoice = ["Slater (local)",
                            "Becke88 (Gradient Corr.)",
                            "Perdew 1991 (Gradient Corr.)",
                            "PBE 1996 (Gradient Corr.)",
                            "Gill 1996 (Gradient Corr.)",
                            "Optimized Exchange (Gradient Corr.)",
                            "Mod. Perdew-Wang 1991 (Gradient Corr.)",
                            "Filatov-Thiel 1997 (Gradient Corr.)",
# start -- added by CAO                            
                            "M05 (Meta-GGA)",
                            "M05-2X (Meta-GGA)",
                            "M06 (Meta-GGA)",
                            "M06-HF (Meta-GGA)",
                            "M06-2X (Meta-GGA)",
                            "M06-L (Meta-GGA)",
                            "VSXC (Meta-GGA)",
                            "rPBE (Hybr.)",
                            "revPBE (Hybr.)",
                            "PW6B95 (Hybr.)",
                            "PWB6k (Hybr.)",
                            "PKZB99 (Meta-GGA)",
                            "TPSS03 (Meta-GGA)"]
# end --added by CAO                            
            self.exFunc = EcceComboBox(self,
                                       choices = exFuncChoice,
                                       name = "ES.Theory.DFT.ExchangeFunctionals",
                                       default = 0,
                                       label = "Exchange:")
            exchangeSizer.AddWidget(self.exFunc)
            
            corrFuncChoice = ["VWN 1 (local)",
                              "VWN 2 (local)",
                              "VWN 3 (local)",
                              "VWN 4 (local)",
                              "VWN 5 (local)",
                              "VWN 1/RPA (local)",
                              "Perdew 1981 (local)",
                              "Perdew-Wang 1991 (local)",
                              "Perdew 1986 (Gradient Corr.)",
                              "Lee-Yang-Parr (Gradient Corr.)",
                              "Perdew 1991 (Gradient Corr.)",
                              "PBE 1996 (Gradient Corr.)",
                              "Filatov-Thiel 1997 (Gradient Corr.)" ,
#start -- added by CAO
                              "M05 (Meta-GGA)",
                              "M05-2X (Meta-GGA)",
                              "M06 (Meta-GGA)",
                              "M06-HF (Meta-GGA)",
                              "M06-2X (Meta-GGA)",
                              "M06-L (Meta-GGA)",
                              "OP (Gradient Corr.)",
                              "PKZB99 (Meta-GGA)",
                              "TPSS03 (Meta-GGA)",
                              "BC95 (Meta-GGA)",
                              "PW6B95 (Meta-GGA)",
                              "PWB6K (Meta-GGA)",
                              "VSXC (Meta-GGA)"]
 #end -- added by CAO 
            self.corrFunc = EcceComboBox(self,
                                         choices = corrFuncChoice,
                                         name = "ES.Theory.DFT.CorrelationFunctionals",
                                         default = 4,
                                         label = "Correlation:")
            exchangeSizer.AddWidget(self.corrFunc)

            self.coulomb = EcceExpInput(self,
                                        default = 1e-10,
                                        name = "ES.Theory.DFT.CoulombCutoff",
                                        hardRange = "[0..)",
                                        label = "Coulomb Screening Tolerance:")
            exchangeSizer.AddWidget(self.coulomb)
            
            gridSizer = EcceLineLabelVBoxSizer(self, label = "Grid Options")

            qualityChoice = ["Extra Coarse",
                             "Coarse",
                             "Medium",
                             "Fine",
                             "Extra Fine"]
            self.quality = EcceComboBox(self,
                                        choices = qualityChoice,
                                        name = "ES.Theory.DFT.GridDensity",
                                        default = 2,
                                        label = "Quality:")
            gridSizer.AddWidget(self.quality)

            angularChoice = ["Gauss-Legendre", "Lebedev"]
            self.angular = EcceComboBox(self,
                                        choices = angularChoice,
                                        name = "ES.Theory.DFT.GridAngular",
                                        default = 1,
                                        label = "Angular:")
            gridSizer.AddWidget(self.angular)

            partitionChoice = ["Becke", "Erf1", "SSF"]
            self.partition = EcceComboBox(self,
                                          choices = partitionChoice,
                                          name = "ES.Theory.DFT.GridPartition",
                                          default = 1,
                                          label = "Partition:")
            gridSizer.AddWidget(self.partition)

            radialChoice = ["Euler", "Mura", "Treutler"]
            self.radial = EcceComboBox(self,
                                       choices = radialChoice,
                                       name = "ES.Theory.DFT.GridRadial",
                                       default = 1,
                                       label = "Radial:")
            gridSizer.AddWidget(self.radial)

            dftSizer.AddWidget(exchangeSizer,
                               flag=EcceGlobals.FlagDefault|wx.EXPAND)
            dftSizer.AddWidget(gridSizer,
                               flag=EcceGlobals.FlagDefault|wx.EXPAND)
            self.panelSizer.Add(dftSizer)

        # FROZEN CORE OPTIONS
        if (EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC"):
            fzSizer = EcceBoxSizer(self,
                                   label = "Frozen Core Options",
                                   cols = 2)

            self.fzTog = EcceCheckBox(self,
                                      label = " Use Frozen Core Orbitals",
                                      name = "ES.Theory.UseFrozenCores",
                                      default = True,
                                      export = 1)
            fzSizer.AddWidget(self.fzTog)
            
            self.fzSpin = EcceSpinCtrl(self,
                                       hardRange = "[0..%i]" %
                                       EcceGlobals.NumOccupiedOrbs,
                                       name = "ES.Theory.CorrelOrb.FrozenCoreValue",
                                       default = EcceGlobals.NumFrozenOrbs,
                                       label = "Freeze:",
                                       export = 1)
            fzSizer.AddWidget(self.fzSpin)
            self.panelSizer.Add(fzSizer)

        self.AddButtons()
        
    def CheckDependency(self):
        # Special logic for DirDyVTST task, as determined by overloading
        # the meaning of the RunType value
        if (EcceGlobals.RunType != "DirDyVTST"):
            self.symmetryTol.Enable(self.symmetryTog.GetValue())
            self.memSize.Enable(self.memTog.GetValue())
            if EcceGlobals.Category == "SCF":
                if self.SCFDirect.GetValue() == "Semi-Direct":
                    self.diskTog.Enable(True)
                    self.diskSize.Enable(self.diskTog.GetValue())
                else:
                    self.diskTog.Enable(False)
                    self.diskSize.Enable(False)

        if (EcceGlobals.Category == "DFT"):
            self.gradient.Enable(self.algorithm.GetValue()!="Iterative SCF")
            self.dampSize.Enable(self.dampTog.GetValue())
        self.levelShiftSize.Enable(self.levelShiftTog.GetValue())

        if (EcceGlobals.Category == "SCF" or
            EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC"):
            self.newLevelTog.Enable(self.levelShiftTog.GetValue())
            self.newLevelCross.Enable(self.newLevelTog.GetValue() and
                                      self.levelShiftTog.GetValue())
            self.newLevelSize.Enable(self.newLevelTog.GetValue() and
                                     self.levelShiftTog.GetValue())
        
        if (EcceGlobals.Category == "DFT"):
            self.corrFunc.Enable(self.xcFunc.GetValue()=="None")
            self.exFunc.Enable(self.xcFunc.GetValue()=="None")
            exlocal = 0
            corrlocal = 0
            if self.exFunc.GetValue() == "Slater (local)":
                exlocal = 1
            if self.corrFunc.GetValue() in ["VWN 1 (local)",
                                            "VWN 2 (local)",
                                            "VWN 3 (local)",
                                            "VWN 4 (local)",
                                            "VWN 5 (local)",
                                            "VWN 1/RPA (local)",
                                            "Perdew 1981 (local)",
                                            "Perdew-Wang 1991 (local)"
                                            "Perdew-Wang 1991 (local)"]:
                corrlocal = 1
            if (exlocal != corrlocal):
                if (exlocal == 1):
                    msg = "The exchange functional is local and the correlation functional"
                else:
                    msg = "The correlation functional is local and the exchange functional"
                msg = msg + " uses the Gradient correction approximation. It is recommended"
                msg = msg + " that you use either local functionals for both exchange and correlation"
                msg = msg + " or Gradient Corrected functionals for both exchange and correlation."
                self.SetStatusText(msg, EcceGlobals.WARNING)

        if (EcceGlobals.Category == "MP" or
            EcceGlobals.Category == "CC"):
            self.fzTog.Enable(EcceGlobals.NumFrozenOrbs >= 0)
            self.fzSpin.Enable(EcceGlobals.NumFrozenOrbs >= 0 and
                               self.fzTog.GetValue())


        # Start Eric Bylaska's planewave module integration
        if (EcceGlobals.RunType != "DirDyVTST" and
            EcceGlobals.Category != "NWPW"):
           # End Eric Bylaska's planewave module integration
            self.solvent.Enable(self.useCosmo.GetValue())
            self.cosmoRadius.Enable(self.useCosmo.GetValue())
            self.cosmoDielec.Enable(self.useCosmo.GetValue() and
                                    self.solvent.GetValue() == "Other")


        # Start Eric Bylaska's planewave module integration
        if EcceGlobals.Category == "NWPW":

            self.unitNonePanel.Show(self.cell.GetValue()=="None")
            self.unitLPanel.Show(self.cell.GetValue()=="SC" or
                            self.cell.GetValue()=="FCC" or
                            self.cell.GetValue()=="BCC")
            self.unitVectorPanel.Show(self.cell.GetValue()=="LatticeVectors")
            #Lattice support not in current NWChem 5.1.1  GDB 4/23/09
            #self.unitLatticePanel.Show(self.cell.GetValue()=="Lattice")
            self.Fit()

            # The enable calls below are a hack solution to some bug that
            # hasn't been tracked down.  It shouldn't be necessary to do this,
            # but otherwise these fields will always be disabled.  The guess
            # is that there is an issue with the ECCE template classes that
            # is messing up parenting somehow causing these fields to be
            # disabled. GDB 4/22/09
            self.lat.Enable(True)
            self.a1x.Enable(True)
            self.a1y.Enable(True)
            self.a1z.Enable(True)
            self.a2x.Enable(True)
            self.a2y.Enable(True)
            self.a2z.Enable(True)
            self.a3x.Enable(True)
            self.a3y.Enable(True)
            self.a3z.Enable(True)
            #Lattice support not in current NWChem 5.1.1  GDB 4/23/09
            #self.lat_a.Enable(True)
            #self.lat_b.Enable(True)
            #self.lat_c.Enable(True)
            #self.lat_alpha.Enable(True)
            #self.lat_beta.Enable(True)
            #self.lat_gamma.Enable(True)

            self.cutoff.Enable(self.usecutoff.GetValue())
            self.rcut.Enable(self.usercut.GetValue())
            self.ncut.Enable(self.usencut.GetValue())
            self.npDimensionsRows.Enable(self.useNpDimensions.GetValue())
            self.npDimensionsCols.Enable(self.useNpDimensions.GetValue())
            #self.ngrid1.Enable(self.usengrid.GetValue())
            #self.ngrid2.Enable(self.usengrid.GetValue())
            #self.ngrid3.Enable(self.usengrid.GetValue())
            # End Eric Bylaska's planewave module integration


frame = NedTheoryFrame(None,
                       title = "ECCE NWChem Editor: Theory Details",
                       app = app,
                       helpURL = "")
