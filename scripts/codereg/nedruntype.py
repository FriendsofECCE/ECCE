from templates import *

class NedRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = NedRunTypePanel(self, helpURL)
        self.Finalize()

class NedRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        # GEOMETRY OPTIMIZATION
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            if EcceGlobals.RunType == "Geometry":
                label = "Geometry Optimization"
            else:
                label = "Geometry Optimization + Vibration"

            geometrySizer = EcceBoxSizer(self,
                                         label = label,
                                         cols = 2)
            
            searchAlgChoice = ["NWChem Driver", "NWChem Stepper"]
            self.searchAlg = EcceComboBox(self,
                                          choices = searchAlgChoice,
                                          name = "ES.Runtype.GeomOpt.SearchAlgorithm",
                                          default = 0,
                                          label = "Search Algorithm:",
                                          export = 1)
            geometrySizer.AddWidget(self.searchAlg)

            searchForChoice = ["Minimum", "Transition State"]
            searchDefault = 0
            if (EcceGlobals.ReactionStudyFlag!=0 and
                EcceGlobals.CalculationName=="Transition-State"):
                searchDefault = 1
            self.searchFor = EcceComboBox(self,
                                          choices = searchForChoice,
                                          name = "ES.Runtype.GeomOpt.SearchFor",
                                          default = searchDefault,
                                          label = "Find:")
            if searchDefault != 0:
                self.searchFor.export = 1
            geometrySizer.AddWidget(self.searchFor)
            geometrySizer.AddSpace()

            self.uphillMode = EcceSpinCtrl(self,
                                           hardRange = "[1..%i]" %
                                           EcceGlobals.NumNormalModes,
                                           name = "ES.Runtype.GeomOpt.TransitionMode", 
                                           default = 1,
                                           label = "Uphill Mode:")
            geometrySizer.AddWidget(self.uphillMode)
            self.panelSizer.Add(geometrySizer)

            cnvSizer = EcceBoxSizer(self,
                                    label = "Convergence",
                                    cols = 1)

            tolSizer = EcceHBoxSizer()
            cnvSizer.AddWidget(tolSizer)

            cnvOptChoice = ["Tight", "Default", "Loose", "Explicit"]
            self.cnvOpt = EcceComboBox(self,
                                       choices = cnvOptChoice,
                                       name = "ES.Runtype.GeomOpt.Convergence",
                                       default = 1,
                                       label = "Tolerance:",
                                       export = 1)
            tolSizer.AddWidget(self.cnvOpt, flag = wx.ALIGN_TOP|wx.ALL)

            explicitSizer = EcceVBoxSizer()
            tolSizer.AddWidget(explicitSizer)

            self.cnvGMax = EcceFloatInput(self,
                                          default = 0.00045,
                                          name = "ES.Runtype.GeomOpt.GradientMax",
                                          softRange = "[1e-5..1e-2]",
                                          hardRange = "[0..)",
                                          unit = "Hartree/Bohr",
                                          label = "Max. Gradient Value: ",
                                          export = 1)
            explicitSizer.AddWidget(self.cnvGMax)

            self.cnvGRMS = EcceFloatInput(self,
                                          default =  0.0003,
                                          name = "ES.Runtype.GeomOpt.GradientRMS",
                                          softRange = "[1e-5..1e-2]",
                                          hardRange = "[0..)",
                                          unit = "Hartree/Bohr",
                                          label = "Max. RMS Gradient:  ",
                                          export = 1)
            explicitSizer.AddWidget(self.cnvGRMS)

            self.cnvXMax = EcceFloatInput(self,
                                          default =  0.0018,
                                          name = "ES.Runtype.GeomOpt.StepSizeMax",
                                          softRange = "[1e-5..1e-2]",
                                          hardRange = "[0..)",
                                          unit = "Bohr",
                                          label = "Max. Step Size:         ",
                                          export = 1)
            explicitSizer.AddWidget(self.cnvXMax)
            
            self.cnvXRMS = EcceFloatInput(self,
                                          default = 0.0012,
                                          name = "ES.Runtype.GeomOpt.StepSizeRMS",
                                          softRange = "[1e-5..1e-2]",
                                          hardRange = "[0..)",
                                          unit = "Bohr",
                                          label = "Max. RMS Step Size:",
                                          export = 1)
            explicitSizer.AddWidget(self.cnvXRMS)

            self.gradTol = EcceFloatInput(self,
                                          default = 0.0008,
                                          name = "ES.Runtype.GeomOpt.ConvergenceGradient.Value",
                                          softRange = "[1e-5..1e-2]",
                                          hardRange = "[0..)",
                                          unit = "Hartree/Bohr",
                                          label = "Stepper Gradient Max.:")
            cnvSizer.AddWidget(self.gradTol)

            self.maxSteps = EcceSpinCtrl(self,
                                         hardRange = "[0..)",
                                         softRange = "[1..100]",
                                         name = "ES.Runtype.GeomOpt.MaximumStepsValue",
                                         default = 20,
                                         label = "Maximum Steps:")
            cnvSizer.AddWidget(self.maxSteps)
            self.panelSizer.Add(cnvSizer)

        if (EcceGlobals.RunType == "Property" and
            (EcceGlobals.Category == "SCF" or
             EcceGlobals.Category == "DFT")):
            propertySizer = EcceBoxSizer(self,
                                         label = "Properties",
                                         cols = 1)

            self.efieldTog = EcceCheckBox(self,
                                          name = "ES.Runtype.Property.Efield",
                                          default = True,
                                          label = "Electric Field at Nuclei",
                                          export = 1)
            propertySizer.AddWidget(self.efieldTog)

            self.efieldGradTog = EcceCheckBox(self,
                                              name = "ES.Runtype.Property.EfieldGrad",
                                              default = True,
                                              label = "Electric Field Gradient at Nuclei",
                                              export = 1)
            propertySizer.AddWidget(self.efieldGradTog)

            self.eDensTog = EcceCheckBox(self,
                                         name = "ES.Runtype.Property.EDens",
                                         default = True,
                                         label = "Electron Density at Nuclei",
                                         export = 1)
            propertySizer.AddWidget(self.eDensTog)

            if (EcceGlobals.Theory == "RHF" or
                EcceGlobals.Theory == "RDFT"):
                self.giaoTog = EcceCheckBox(self,
                                            name = "ES.Runtype.Property.GIAO",
                                            default = True,
                                            label = "NMR Shielding (GIAO Method)",
                                            export = 1)
                propertySizer.AddWidget(self.giaoTog)
            elif (EcceGlobals.Theory != "ROHF"):
                if (EcceGlobals.SpinMultiplicity > 1):
                    self.hyperTog = EcceCheckBox(self,
                                                 name = "ES.Runtype.Property.Hyperfine",
                                                 default = True,
                                                 label = "NMR Hyperfine Coupling (Fermi-Contact and Spin-Dipole Expectation Values)",
                                                 export = 1)
                    propertySizer.AddWidget(self.hyperTog)

                if (EcceGlobals.SpinMultiplicity == 1):
                    self.spspTog =  EcceCheckBox(self,
                                                 name = "ES.Runtype.Property.SpinSpin",
                                                 default = True,
                                                 label = "NMR Indirect Spin-Spin Coupling",
                                                 export = 1)
                    propertySizer.AddWidget(self.spspTog)
            
            self.panelSizer.Add(propertySizer)

        if (EcceGlobals.RunType == "ESP"):
            gridSizer = EcceBoxSizer(self,
                                     label = "ESP Charge Fitting Grid parameters",
                                     cols = 2)

            self.grdRange = EcceFloatInput(self,
                                           default = 0.4,
                                           name = "ES.Runtype.ESP.GridRange",
                                           softRange = "[0.25..1]",
                                           hardRange = "(0..)",
                                           unit = "nm",
                                           label = "Grid Range:",
                                           export = 1)
            gridSizer.AddWidget(self.grdRange)
            
            self.grdSpace = EcceFloatInput(self,
                                           default = 0.035,
                                           name = "ES.Runtype.ESP.GridSpacing",
                                           softRange = "[0.01..0.1]",
                                           hardRange = "(0..)",
                                           unit = "nm",
                                           label = "Grid Spacing:",
                                           export = 1)
            gridSizer.AddWidget(self.grdSpace)

            self.probeRad = EcceFloatInput(self,
                                           default = 0.07,
                                           name = "ES.Runtype.ESP.ProbeRadius",
                                           softRange = "[0.05..0.15]",
                                           hardRange = "(0..)",
                                           unit = "nm",
                                           label = "Probe Radius:")
            gridSizer.AddWidget(self.probeRad)

            self.probeScl = EcceFloatInput(self,
                                           default = 1,
                                           name = "ES.Runtype.ESP.ProbeScaleFactor",
                                           hardRange = "(0..)",
                                           label = "Probe Scale Factor:")
            gridSizer.AddWidget(self.probeScl)

            screenSizer = EcceHBoxSizer()
            self.screenTog = EcceCheckBox(self,
                                          name = "ES.Runtype.ESP.Screening",
                                          default = False,
                                          label = "Screening Tolerance: ")
            screenSizer.AddWidget(self.screenTog)

            self.screenTol = EcceFloatInput(self,
                                            default = 0.00001,
                                            name = "ES.Runtype.ESP.ScreeningTolerance",
                                            hardRange = "(0..)")
            screenSizer.AddWidget(self.screenTol)
            gridSizer.AddWidget(screenSizer)
            self.panelSizer.Add(gridSizer)

            rstrntSizer = EcceBoxSizer(self,
                                       label = "ESP Restraint Settings",
                                       cols = 2)

            self.useRstrnt = EcceCheckBox(self,
                                          default = True,
                                          name = "ES.Runtype.ESP.UseRestraints",
                                          label = "Use Charge Restraining Potential",
                                          export = 1)
            rstrntSizer.AddWidget(self.useRstrnt)
            rstrntSizer.AddSpace()
            
            self.hydFree = EcceCheckBox(self,
                                        default = False,
                                        name = "ES.Runtype.ESP.HydrogenFree",
                                        label = "Do Not Restrain Hydrogens")
            rstrntSizer.AddWidget(self.hydFree)
            rstrntSizer.AddSpace()

            rstrntAlgChoice = ["Harmonic", "Hyperbolic"]
            self.rstrntAlg = EcceComboBox(self,
                                          choices = rstrntAlgChoice,
                                          default = 1,
                                          name = "ES.Runtype.ESP.RestraintAlgorithm",
                                          label = "Restraint Algorithm:",
                                          export = 1)
            rstrntSizer.AddWidget(self.rstrntAlg)
            rstrntSizer.AddSpace()

            self.constantLabel = wx.StaticText(self,
                                               label = "Force Constant:")
            rstrntSizer.AddWidget(self.constantLabel)
            rstrntSizer.AddSpace()
            
            self.hafrcCnst = EcceFloatInput(self,
                                            default =  0.005,
                                            name = "ES.Runtype.ESP.HarmForceConstant",
                                            label = "Harmonic:",
                                            unit = "au",
                                            hardRange = "(0..)")
            rstrntSizer.AddWidget(self.hafrcCnst)
            rstrntSizer.AddSpace()

            self.hyfrcCnst = EcceFloatInput(self,
                                            default = 0.001,
                                            name = "ES.Runtype.ESP.HypForceConstant",
                                            label = "Hyperbolic:",
                                            unit = "au",
                                            hardRange = "(0..)")
            rstrntSizer.AddWidget(self.hyfrcCnst)

            
            self.hypTght = EcceFloatInput(self,
                                          default =  0.1,
                                          name = "ES.Runtype.ESP.Tightness",
                                          label = "Tightness:",
                                          unit = "e",
                                          hardRange = "(0..)")
            rstrntSizer.AddWidget(self.hypTght)

            self.maxIt = EcceSpinCtrl(self,
                                      hardRange = "[1..)",
                                      name = "ES.Runtype.ESP.MaxIterations",
                                      default = 25,
                                      label = "Max. Iterations:")
            rstrntSizer.AddWidget(self.maxIt)
            
            self.hypTol = EcceFloatInput(self,
                                         default = 0.001,
                                         name = "ES.Runtype.ESP.HypTolerance",
                                         label = "Tightness:",
                                         unit = "e",
                                         hardRange = "(0..)")
            rstrntSizer.AddWidget(self.hypTol)
            self.panelSizer.Add(rstrntSizer)

        # Start Eric Bylaska's planewave module integration
        if (EcceGlobals.RunType == "Car-Parrinello" and
            (EcceGlobals.Category == "NWPW")):
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
                                      label = "Inner Iterations:")
            cpmdSizer.AddWidget(self.cpmdinnerIt)
            self.cpmdouterIt = EcceSpinCtrl(self,
                                      hardRange = "[1..)",
                                      name = "ES.Runtype.CPMD.OuterIterations",
                                      default = 100,
                                      label = "Outer Iterations:")
            cpmdSizer.AddWidget(self.cpmdouterIt)
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
            self.cpmdRestart = EcceCheckBox(self,
                                         name = "ES.Runtype.CPMD.Restart",
                                         default = False,
                                         label = "Restart",
                                         export = 1)
            cpmdSizer.AddWidget(self.cpmdRestart)
            self.panelSizer.Add(cpmdSizer)
            # End Eric Bylaska's planewave module integration
        self.AddButtons()

    def CheckDependency(self):
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            self.uphillMode.Enable(self.searchFor.GetValue() != "Minimum")
            self.cnvOpt.Enable(self.searchAlg.GetValue() == "NWChem Driver")
            self.cnvGMax.Enable(self.cnvOpt.GetValue() == "Explicit" and
                                self.searchAlg.GetValue() == "NWChem Driver")
            self.cnvGRMS.Enable(self.cnvOpt.GetValue() == "Explicit" and
                                self.searchAlg.GetValue() == "NWChem Driver")
            self.cnvXMax.Enable(self.cnvOpt.GetValue() == "Explicit" and
                                self.searchAlg.GetValue() == "NWChem Driver")
            self.cnvXRMS.Enable(self.cnvOpt.GetValue() == "Explicit" and
                                self.searchAlg.GetValue() == "NWChem Driver")
            self.gradTol.Enable(self.searchAlg.GetValue() == "NWChem Stepper")
        if (EcceGlobals.RunType == "ESP"):
            self.screenTol.Enable(self.screenTog.GetValue())
            if self.useRstrnt.GetValue():
                if self.rstrntAlg.GetValue() == "Harmonic":
                    self.hafrcCnst.Enable(True)
                    self.hyfrcCnst.Enable(False)
                    self.hypTght.Enable(False)
                    self.maxIt.Enable(False)
                    self.hypTol.Enable(False)
                else:
                    self.hafrcCnst.Enable(False)
                    self.hyfrcCnst.Enable(True)
                    self.hypTght.Enable(True)
                    self.maxIt.Enable(True)
                    self.hypTol.Enable(True)
            else:
                self.hafrcCnst.Enable(False)
                self.hyfrcCnst.Enable(False)
                self.hypTght.Enable(False)
                self.maxIt.Enable(False)
                self.hypTol.Enable(False)
            self.hydFree.Enable(self.useRstrnt.GetValue())
            self.rstrntAlg.Enable(self.useRstrnt.GetValue())
            if (self.hyfrcCnst.NotDefault()):
                self.hyfrcCnst.export = 1
            else:
                self.hyfrcCnst.export = 0


frame = NedRunTypeFrame(None,
                        title = "ECCE NWChem Editor: Runtype Details",
                        app = app,
                        helpURL = "")
