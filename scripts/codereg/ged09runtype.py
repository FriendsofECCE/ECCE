from templates import *

class Ged03RunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = Ged03RunTypePanel(self, helpURL)
        self.Finalize()

class Ged03RunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if (EcceGlobals.RunType == "Gradient"):
            gradSizer = EcceBoxSizer(self,
                                     label = "Gradient",
                                     cols = 2)
            
            if (EcceGlobals.Category == "SCF" or
                EcceGlobals.Category == "DFT" or
                (EcceGlobals.Category == "MP" and
                 (EcceGlobals.Theory == "RMP2" or
                  EcceGlobals.Theory == "UMP2" or
                  EcceGlobals.Theory == "RMP3" or
                  EcceGlobals.Theory == "UMP3" or
                  EcceGlobals.Theory == "RMP4(SDQ)" or
                  EcceGlobals.Theory == "UMP4(SDQ)")) or
                EcceGlobals.Category == "CI" or
                (EcceGlobals.Category == "CC" and
                 (EcceGlobals.Theory ==  "RQCISD" or
                  EcceGlobals.Theory == "UQCISD" or
                  EcceGlobals.Theory == "RCCD" or
                  EcceGlobals.Theory == "UCCD"))):
                gradMethodChoice = ["Analytic",
                                    "Finite Difference"]
            elif (EcceGlobals.Category == "SE"):
                gradMethodChoice = ["Analytic"]
            else:
                gradMethodChoice = ["Finite Difference"]
            self.gradMethod = EcceComboBox(self,
                                           choices = gradMethodChoice,
                                           name = "ES.Runtype.Gradient.Method",
                                           default = 0,
                                           label = "Method:")
            gradSizer.AddWidget(self.gradMethod)

            self.panelSizer.Add(gradSizer)
            
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            # GEOMETRY OPTIMIZATION
            geometrySizer = EcceBoxSizer(self,
                                         label = "Geometry Optimization",
                                         cols = 2)
            
            findChoice = ["Minimum",
                          "Transition State"]
            findDefault = 0
            if (EcceGlobals.ReactionStudyFlag!=0 and
                EcceGlobals.CalculationName=="Transition-State"):
                findDefault = 1
            self.find = EcceComboBox(self,
                                     choices = findChoice,
                                     name = "ES.Runtype.GeomOpt.SearchFor",
                                     default = findDefault,
                                     label = "Find:")
            if findDefault != 0:
                self.find.export = 1
            geometrySizer.AddWidget(self.find)
            
            # CONVERGENCE
            convergenceSizer = EcceBoxSizer(self,
                                            label = "Convergence",
                                            cols = 3)
            
            gradientChoice = ["Loose",
                              "Medium",
                              "Tight",
                              "Very Tight"]
            self.gradient = EcceComboBox(self,
                                         choices = gradientChoice,
                                         name = "ES.Runtype.GeomOpt.ConvergenceGradient",
                                         default = 1,
                                         label = "Gradient:")
            convergenceSizer.AddWidget(self.gradient)
            
            self.stepCheckBox = EcceCheckBox(self,
                                             label = " Max Steps:",
                                             name = "ES.Runtype.GeomOpt.MaximumSteps",
                                             default = 0)
            convergenceSizer.AddWidget(self.stepCheckBox)
            
            self.stepSpin = EcceSpinCtrl(self,
                                         hardRange = "[1..)",
                                         softRange = "[1..100]",
                                         name = "ES.Runtype.GeomOpt.MaximumStepsValue",
                                         default = 20,
                                         export = 1)
            convergenceSizer.AddWidget(self.stepSpin)
            
            # HESSIAN
            hessianSizer = EcceBoxSizer(self,
                                        label = "Hessian",
                                        cols = 2)
            
            if (EcceGlobals.Category == "SCF" or
                EcceGlobals.Category == "DFT" or
                (EcceGlobals.Category == "MP" and
                 (EcceGlobals.Theory == "RMP2" or
                  EcceGlobals.Theory == "UMP2" or
                  EcceGlobals.Theory == "RMP3" or
                  EcceGlobals.Theory == "UMP3" or
                  EcceGlobals.Theory == "RMP4(SDQ)" or
                  EcceGlobals.Theory == "UMP4(SDQ)")) or
                (EcceGlobals.Category == "CC" and
                 (EcceGlobals.Theory == "RQCISD" or
                  EcceGlobals.Theory == "UQCISD"))):
                self.hessianChoice = ["Valence Force Field",
                                      "Calculate"]
            else:
                self.hessianChoice = ["Valence Force Field"]
            self.hessian = EcceComboBox(self,
                                        choices = self.hessianChoice,
                                        name = "ES.Runtype.GeomOpt.InitialHessian",
                                        default = 1,
                                        label = "Initial Source:")
            hessianSizer.AddWidget(self.hessian)
            
            self.panelSizer.Add(geometrySizer)
            self.panelSizer.Add(convergenceSizer)
            self.panelSizer.Add(hessianSizer)
            
        if EcceGlobals.RunType == "Polarizability":
            polarSizer = EcceBoxSizer(self,
                                      label = "Polarizability",
                                      cols = 1)

            if (EcceGlobals.Theory == "RHF" or
                EcceGlobals.Theory == "UHF" or
                EcceGlobals.Category == "DFT" or
                EcceGlobals.Theory == "RMP2" or
                EcceGlobals.Theory == "UMP2"):
                polarMethodChoice = ["Analytic Polariz. + Hyperpol.",
                                     "Numerical Polariz."]
            elif (EcceGlobals.Theory == "ROHF" or
                  (EcceGlobals.Category == "MP" and
                   (EcceGlobals.Theory == "RMP3" or
                    EcceGlobals.Theory == "UMP3" or
                    EcceGlobals.Theory == "RMP4(SDQ)" or
                    EcceGlobals.Theory == "UMP4(SDQ)")) or
                  EcceGlobals.Category == "CI" or
                  (EcceGlobals.Category == "CC" and
                   (EcceGlobals.Theory ==  "RQCISD" or
                    EcceGlobals.Theory == "UQCISD" or
                    EcceGlobals.Theory == "RCCD" or
                    EcceGlobals.Theory == "UCCD"))):
                polarMethodChoice = ["Numerical Polariz.",
                                     "Numerical Polariz. + Hyperpol."]
            else:
                polarMethodChoice = ["Numerical Polariz."]
            self.polarMethod = EcceComboBox(self,
                                            choices = polarMethodChoice,
                                            name = "ES.Runtype.Polar.Method",
                                            default = 0,
                                            label = "Method:")
            polarSizer.AddWidget(self.polarMethod)
            
            self.efstepInput = EcceFloatInput(self,
                                              unit = "AU",
                                              name = "ES.Runtype.Polar.EFieldStepSize",
                                              default = 0.0019,
                                              hardRange = "[0..)",
                                              softRange = "[1e-5..1e-2]",
                                              label = "Electric Field Step Size:")
            polarSizer.AddWidget(self.efstepInput)

            self.panelSizer.Add(polarSizer)

        if (EcceGlobals.RunType == "Vibration" or
            EcceGlobals.RunType == "GeoVib"):
            vibSizer = EcceBoxSizer(self, "IR/Raman", 1)
            vibMethodEnable = True
            if (EcceGlobals.Theory == "RHF" or
                EcceGlobals.Theory == "UHF" or
                EcceGlobals.Category == "DFT" or
                EcceGlobals.Theory == "RMP2" or
                EcceGlobals.Theory == "UMP2" or
                EcceGlobals.Theory == "CIS"):
                vibMethodChoice = ["Analytic",
                                   "Numerical 2nd Derivative",
                                   "Numerical 1st and 2nd Derivative"]
            elif (EcceGlobals.Theory == "ROHF" or
                  EcceGlobals.Theory == "RMP3" or
                  EcceGlobals.Theory == "UMP3" or
                  EcceGlobals.Theory == "RMP4(SDQ)" or
                  EcceGlobals.Theory == "UMP4(SDQ)" or
                  EcceGlobals.Theory == "RMP4(DQ)" or
                  EcceGlobals.Theory == "RMP4(DQ)" or
                  EcceGlobals.Theory == "UMP4(DQ)" or
                  EcceGlobals.Theory ==  "RQCISD" or
                  EcceGlobals.Theory == "UQCISD" or
                  EcceGlobals.Theory == "RCCD" or
                  EcceGlobals.Theory == "UCCD" or
                  EcceGlobals.Theory == "CISD"):
                vibMethodChoice = ["Numerical 2nd Derivative",
                                   "Numerical 1st and 2nd Derivative"]
            elif (EcceGlobals.Theory == "RMP4" or
                  EcceGlobals.Theory == "UMP4" or
                  EcceGlobals.Theory == "RQCISD(T)" or
                  EcceGlobals.Theory == "UQCISD(T)" or
                  EcceGlobals.Theory == "RCCSD" or
                  EcceGlobals.Theory == "UCCSD" or
                  EcceGlobals.Theory == "RCCSD(T)" or
                  EcceGlobals.Theory == "UCCSD(T)"):
                vibMethodChoice = ["Numerical 1st and 2nd Derivative"]
            elif (EcceGlobals.Category == "SE"):
                vibMethodChoice = ["Analytic",
                                   "Numerical 2nd Derivative"]
            else:
                vibMethodChoice = ["Numerical 1st and 2nd Derivative"]
                vibMethodEnable = False

            self.vibMethod = EcceComboBox(self,
                                          choices = vibMethodChoice,
                                          name = "ES.Runtype.Vibration.Method",
                                          default = 0,
                                          label = "Method:")
            self.vibMethod.Enable(vibMethodEnable)
            vibSizer.AddWidget(self.vibMethod)

            self.fdStep = EcceFloatInput(self,
                                         unit = "Angstrom",
                                         name = "ES.Runtype.Vibration.FiniteStepSize",
                                         default = 0.001,
                                         hardRange = "(0..)",
                                         softRange = "(0..0.1]",
                                         label = "FD Step Size:")
            vibSizer.AddWidget(self.fdStep)

            self.panelSizer.Add(vibSizer)
        if EcceGlobals.RunType == "Magnetic":
            magneticSizer = EcceBoxSizer(self,
                                         label = "Magnetic/NMR",
                                         cols = 1)

            magneticMethodChoice = ["Gauge-Independent Atomic Orbitals",
                                    "Continuous Set of Gauge Transformations",
                                    "Atoms-In-Molecules Gauge",
                                    "Single Origin"]
            self.magneticMethod = EcceComboBox(self,
                                               choices = magneticMethodChoice,
                                               name = "ES.Runtype.Magnetic.Method",
                                               default = 0,
                                               label = "Method:")
            magneticSizer.AddWidget(self.magneticMethod)

            self.panelSizer.Add(magneticSizer)

        self.AddButtons()

    def CheckDependency(self):
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            self.stepSpin.Enable(self.stepCheckBox.GetValue())
            if (len(self.hessianChoice) == 2 and
                self.find.GetSelection() == 1):
                self.hessian.SetSelection(1)
        if EcceGlobals.RunType == "Polarizability":
            self.efstepInput.Enable(self.polarMethod.GetValue() !=
                                    "Analytic Polariz. + Hyperpol.")
        if (EcceGlobals.RunType == "Vibration" or
            EcceGlobals.RunType == "GeoVib"):
            self.fdStep.Enable(self.vibMethod.GetValue() != "Analytic")


frame = Ged03RunTypeFrame(None,
                          title = "ECCE Gaussian-03 Editor: Runtype Details",
                          app = app,
                          helpURL = "")
