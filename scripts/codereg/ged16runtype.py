from templates import *

class Ged16RunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = Ged16RunTypePanel(self, helpURL)
        self.Finalize()

class Ged16RunTypePanel(EccePanel):
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
            #  By name, and guarded: hessianChoice is "Calculate" only for
            #  the theories above, so a bare index 1 is past the end of the
            #  one-entry list for every other theory -- and wx.Choice ignores
            #  an out-of-range SetSelection() silently, leaving the combo
            #  blank and GetValue() returning "".  CheckDependency() below
            #  already carries the len()==2 guard this line was missing.
            self.hessian = EcceComboBox(self,
                                        choices = self.hessianChoice,
                                        name = "ES.Runtype.GeomOpt.InitialHessian",
                                        default = (self.hessianChoice.index("Calculate")
                                                   if "Calculate" in self.hessianChoice
                                                   else 0),
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

            #  ---------------------------------------------------------
            #  Freq= options (issue #110).  Every one of these was run
            #  through the installed Gaussian 16 before being offered
            #  here, water/B3LYP/STO-3G, and two combinations turn out to
            #  be FATAL rather than merely ignored -- see CheckDependency
            #  below, which is what stops the user asking for them:
            #
            #    Freq=(Raman,Numerical)     "Unrecognized FrqTyp= 9",
            #                               Error termination in l1.exe
            #    Freq=(Anharmonic,Projected) "Mismatch between the number
            #                               of normal modes of the current
            #                               calculation and the input
            #                               source: 3 vs 2", l717.exe
            #
            #  Everything else here ran to Normal termination, including
            #  Raman with EnOnly, Raman with HPModes, Anharmonic with
            #  Numerical, and Anharmonic with each of VibRot,
            #  SaveNormalModes and HPModes.
            #  ---------------------------------------------------------

            #  Gaussian computes Raman activities by default for HF but
            #  NOT for DFT, so for the methods most people use the
            #  VIBRAM property gaussian-16.desc has always parsed was
            #  simply never produced.  Default off: it costs extra work.
            self.raman = EcceCheckBox(self,
                                      label = " Raman Activities",
                                      name = "ES.Runtype.Vibration.ComputeRaman",
                                      default = 0)
            vibSizer.AddWidget(self.raman)

            #  Second-order perturbative anharmonic analysis.  The
            #  harmonic block is still printed and still feeds
            #  VIB/VIBFREQ/...; the fundamentals are extracted separately
            #  into ANHARMFREQ/ANHARMIR by gaussian-16.anharm.
            self.anharmonic = EcceCheckBox(self,
                                           label = " Anharmonic Frequencies",
                                           name = "ES.Runtype.Vibration.Anharmonic",
                                           default = 0)
            vibSizer.AddWidget(self.anharmonic)

            #  High-precision normal modes.  Gaussian then prints the
            #  modes TWICE under an identical header; gaussian-16.vib
            #  drops the high-precision copy, which it could not read and
            #  which used to make VIBFREQ come out with twice as many
            #  values as row labels.
            self.hpModes = EcceCheckBox(self,
                                        label = " High-Precision Modes",
                                        name = "ES.Runtype.Vibration.HighPrecisionModes",
                                        default = 0)
            vibSizer.AddWidget(self.hpModes)

            self.projected = EcceCheckBox(self,
                                          label = " Project Out Translation/Rotation",
                                          name = "ES.Runtype.Vibration.Projected",
                                          default = 0)
            vibSizer.AddWidget(self.projected)

            self.vibRot = EcceCheckBox(self,
                                       label = " Vibration-Rotation Coupling",
                                       name = "ES.Runtype.Vibration.VibRot",
                                       default = 0)
            vibSizer.AddWidget(self.vibRot)

            self.saveNormalModes = EcceCheckBox(self,
                                                label = " Save Normal Modes To Checkpoint",
                                                name = "ES.Runtype.Vibration.SaveNormalModes",
                                                default = 0)
            vibSizer.AddWidget(self.saveNormalModes)

            #  Thermochemistry conditions.  Same checkbox-enables-a-value
            #  shape as Max Steps above, so that a deck that does not ask
            #  for them carries no Temperature=/Pressure= at all and
            #  Gaussian keeps its own 298.15 K / 1 atm defaults.
            self.thermoCheckBox = EcceCheckBox(self,
                                               label = " Thermochemistry At:",
                                               name = "ES.Runtype.Vibration.UseThermochemistry",
                                               default = 0)
            vibSizer.AddWidget(self.thermoCheckBox)

            self.temperature = EcceFloatInput(self,
                                              unit = "Kelvin",
                                              name = "ES.Runtype.Vibration.Temperature",
                                              default = 298.15,
                                              hardRange = "(0..)",
                                              softRange = "(0..5000]",
                                              label = "Temperature:",
                                              export = 1)
            vibSizer.AddWidget(self.temperature)

            self.pressure = EcceFloatInput(self,
                                           unit = "Atmosphere",
                                           name = "ES.Runtype.Vibration.Pressure",
                                           default = 1.0,
                                           hardRange = "(0..)",
                                           softRange = "(0..1000]",
                                           label = "Pressure:",
                                           export = 1)
            vibSizer.AddWidget(self.pressure)

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

            #  Both of these combinations are fatal in Gaussian 16, not
            #  merely ignored -- see the comment where the controls are
            #  built.  Disabling rather than warning keeps them
            #  unaskable; ai.gauss16 refuses them again in its main flow,
            #  because a calculation saved before this dialog existed can
            #  still carry the offending pair.
            ramanOK = (self.vibMethod.GetValue() != "Numerical 2nd Derivative")
            self.raman.Enable(ramanOK)
            if not ramanOK:
                self.raman.SetValue(0)

            self.projected.Enable(not self.anharmonic.GetValue())
            if self.anharmonic.GetValue():
                self.projected.SetValue(0)

            self.temperature.Enable(self.thermoCheckBox.GetValue())
            self.pressure.Enable(self.thermoCheckBox.GetValue())


frame = Ged16RunTypeFrame(None,
                          title = "ECCE Gaussian-16 Editor: Runtype Details",
                          app = app,
                          helpURL = "")
