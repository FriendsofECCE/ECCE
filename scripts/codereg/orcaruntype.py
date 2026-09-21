from templates import *

# ECCE ORCA "Runtype Details" dialog (issue #38, reduced scope).
#
# Originally only exposed Max Steps for geometry optimization
# (ES.Runtype.GeomOpt.MaximumStepsValue -> %geom MaxIter block).
# Energy/Gradient/Magnetic still have no configurable options in the
# reduced-scope input generator, so they show an empty dialog (same
# pattern ged16runtype.py uses for RunType=="Energy"). ORCA's "! NMR"
# keyword needs no method selector (unlike Gaussian's GIAO/CSGT/etc.
# choice) -- it always uses GIAO by default.
#
# 2026-09-21 session (Priority 2, issue #38 follow-up): added Geometry
# Optimization convergence tightness and Vibration analytic-vs-numeric
# method, both wired through to ai.orca as "! <keyword>" route-card
# tokens and verified against a real ORCA 6.1.1 install first (see
# orcatheory.py's header note for the general verification approach --
# LooseOpt/TightOpt/VeryTightOpt and NumFreq all confirmed to actually
# change ORCA's own reported convergence tolerances / calculation
# type, not just accepted-without-error).

class OrcaRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = OrcaRunTypePanel(self, helpURL)
        self.Finalize()

class OrcaRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            geometrySizer = EcceBoxSizer(self,
                                         label="Geometry Optimization",
                                         cols=2)

            self.stepCheckBox = EcceCheckBox(self,
                                             label=" Max Steps:",
                                             name="ES.Runtype.GeomOpt.MaximumSteps",
                                             default=0)
            geometrySizer.AddWidget(self.stepCheckBox)

            self.stepSpin = EcceSpinCtrl(self,
                                        hardRange="[1..)",
                                        softRange="[1..100]",
                                        name="ES.Runtype.GeomOpt.MaximumStepsValue",
                                        default=20,
                                        export=1)
            geometrySizer.AddWidget(self.stepSpin)

            self.panelSizer.Add(geometrySizer)

            convergenceSizer = EcceBoxSizer(self,
                                            label="Convergence",
                                            cols=1)
            gradientChoice = ["Loose",
                              "Normal",
                              "Tight",
                              "Very Tight"]
            self.gradient = EcceComboBox(self,
                                         choices=gradientChoice,
                                         name="ES.Runtype.GeomOpt.ConvergenceGradient",
                                         default=1,
                                         label="Gradient:",
                                         export=1)
            convergenceSizer.AddWidget(self.gradient)
            self.panelSizer.Add(convergenceSizer)

        if (EcceGlobals.RunType == "Vibration" or
            EcceGlobals.RunType == "GeoVib"):
            vibSizer = EcceBoxSizer(self, label="Frequencies", cols=1)

            vibMethodChoice = ["Analytic",
                               "Numerical"]
            self.vibMethod = EcceComboBox(self,
                                          choices=vibMethodChoice,
                                          name="ES.Runtype.Vibration.Method",
                                          default=0,
                                          label="Method:",
                                          export=1)
            vibSizer.AddWidget(self.vibMethod)
            self.panelSizer.Add(vibSizer)

        self.AddButtons()

    def CheckDependency(self):
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            self.stepSpin.Enable(self.stepCheckBox.GetValue())


frame = OrcaRunTypeFrame(None,
                         title="ECCE ORCA Editor: Runtype Details",
                         app=app,
                         helpURL="")
