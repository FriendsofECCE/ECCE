from templates import *

# ECCE ORCA "Runtype Details" dialog (issue #38, reduced scope).
#
# Only exposes Max Steps for geometry optimization, the one runtype
# option ai.orca (scripts/parsers/ai.orca) currently consumes
# (ES.Runtype.GeomOpt.MaximumStepsValue -> %geom MaxIter block).
# Energy/Gradient/Vibration/Magnetic have no configurable options yet
# in the reduced-scope input generator, so they show an empty dialog
# (same pattern ged16runtype.py uses for RunType=="Energy"). ORCA's
# "! NMR" keyword needs no method selector (unlike Gaussian's GIAO/
# CSGT/etc. choice) -- it always uses GIAO by default.

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

        self.AddButtons()

    def CheckDependency(self):
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            self.stepSpin.Enable(self.stepCheckBox.GetValue())


frame = OrcaRunTypeFrame(None,
                         title="ECCE ORCA Editor: Runtype Details",
                         app=app,
                         helpURL="")
