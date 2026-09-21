from templates import *

# ECCE MOPAC "Runtype Details" dialog (reduced scope first cut),
# modeled on orcaruntype.py.
#
# Energy and Gradient have no configurable options in the reduced-scope
# input generator, so they show an empty dialog (the same pattern
# ged16runtype.py and orcaruntype.py use for RunType=="Energy").
#
# NO TRANSITION-STATE SEARCH OPTION, deliberately.  MOPAC does have a
# TS keyword ("USE EF ROUTINE FOR TS SEARCH", present in the real
# binary's keyword table), and ai.mopac still has the code path to emit
# it, but running it for real is what settled this: MOPAC's TS search
# CANNOT operate on Cartesian coordinates, and ai.mopac writes
# Cartesians.  A generated "PM7 TS" water job terminates immediately
# with
#     Too many variables. By definition, at least one force constant is
#     exactly zero and the lowest force constant is not negative.
#     Number of variables = 9   Number of atoms = 3
#     (If coordinates are Cartesian, convert to internal coordinates and
#     re-run.)
# for any nonlinear molecule -- 3N Cartesian variables where the EF TS
# algorithm needs 3N-6 internal ones.  Exposing a "Search for:
# Transition State" choice would therefore be a button that always
# fails.  Bringing it back means teaching ai.mopac to emit a Z-matrix,
# which is out of scope for this pass.
#
# Every keyword this dialog can produce was verified against the real
# MOPAC 23.1.2 binary: accepted, echoed in MOPAC's own keyword banner
# with the intended meaning, and run to completion.

class MopacRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = MopacRunTypePanel(self, helpURL)
        self.Finalize()

class MopacRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):

            # CYCLES=n -- "DO A MAXIMUM OF n" geometry optimization
            # cycles.  Gated on its own checkbox so the common case
            # emits no keyword and MOPAC uses its own limit.
            geometrySizer = EcceBoxSizer(self,
                                         label="Geometry Optimization",
                                         cols=2)
            self.stepCheckBox = EcceCheckBox(self,
                                             label=" Max Steps:",
                                             name="ES.Runtype.GeomOpt.MaximumSteps",
                                             default=0,
                                             export=1)
            geometrySizer.AddWidget(self.stepCheckBox)

            self.stepSpin = EcceSpinCtrl(self,
                                         hardRange="[1..)",
                                         softRange="[1..200]",
                                         name="ES.Runtype.GeomOpt.MaximumStepsValue",
                                         default=50,
                                         export=1)
            geometrySizer.AddWidget(self.stepSpin)
            self.panelSizer.Add(geometrySizer)

            # GNORM=x -- "EXIT WHEN GRADIENT NORM DROPS BELOW x".
            # MOPAC's gradients are in kcal/(mol*Angstrom), which is
            # what this number is in; it is NOT the dimensionless
            # "Loose/Normal/Tight" tier other codes' dialogs offer,
            # because MOPAC has no such tiers -- there is one numeric
            # threshold and that is the whole mechanism.
            convergenceSizer = EcceBoxSizer(self,
                                            label="Convergence",
                                            cols=2)
            self.gnormCheckBox = EcceCheckBox(self,
                                              label=" Gradient Norm:",
                                              name="ES.Runtype.GeomOpt.ConvergenceGradient",
                                              default=0,
                                              export=1)
            convergenceSizer.AddWidget(self.gnormCheckBox)

            self.gnorm = EcceFloatInput(self,
                                        unit="kcal/mol/Angstrom",
                                        name="ES.Runtype.GeomOpt.ConvergenceGradient.Value",
                                        default=1.0,
                                        hardRange="(0..)",
                                        softRange="[0.001..10]",
                                        export=1)
            convergenceSizer.AddWidget(self.gnorm)
            self.panelSizer.Add(convergenceSizer)

        if (EcceGlobals.RunType == "Vibration" or
            EcceGlobals.RunType == "GeoVib"):

            # THERMO -- "THERMODYNAMIC QUANTITIES TO BE CALCULATED".
            # Only affects the extra temperature-dependent
            # thermochemistry table MOPAC appends; the frequencies,
            # normal modes and zero point energy this integration
            # actually parses are printed either way.
            vibSizer = EcceBoxSizer(self, label="Frequencies", cols=1)
            self.thermo = EcceCheckBox(self,
                                       label=" Thermochemistry (THERMO)",
                                       name="ES.Runtype.Vibration.Thermo",
                                       default=False,
                                       export=1)
            vibSizer.AddWidget(self.thermo)
            self.panelSizer.Add(vibSizer)

        self.AddButtons()

    def CheckDependency(self):
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            self.stepSpin.Enable(self.stepCheckBox.GetValue())
            self.gnorm.Enable(self.gnormCheckBox.GetValue())


frame = MopacRunTypeFrame(None,
                          title="ECCE MOPAC Editor: Runtype Details",
                          app=app,
                          helpURL="")
