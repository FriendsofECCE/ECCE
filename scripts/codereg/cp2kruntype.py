from templates import *

# ECCE CP2K "Runtype Details" dialog.  Energy maps to RUN_TYPE ENERGY (or
# ENERGY_FORCE with the Forces box, which also prints the forces) and
# Geometry to GEO_OPT; ai.cp2k writes the &MOTION section from the second.


class Cp2kRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = Cp2kRunTypePanel(self, helpURL)
        self.Finalize()


class Cp2kRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.RunType == "Energy":
            propSizer = EcceBoxSizer(self, label="Properties", cols=1)
            self.printForces = EcceCheckBox(self,
                                            label=" Forces on atoms",
                                            name="ES.Runtype.GPW.PrintForces",
                                            default=True,
                                            export=1)
            propSizer.AddWidget(self.printForces)
            self.panelSizer.Add(propSizer)

        elif EcceGlobals.RunType == "Geometry":
            optSizer = EcceBoxSizer(self, label="Optimizer", cols=1)
            self.optimizer = EcceComboBox(self,
                                          choices=["BFGS", "LBFGS", "CG"],
                                          name="ES.Runtype.GPW.Optimizer",
                                          default=0,
                                          label="Method:",
                                          export=1)
            optSizer.AddWidget(self.optimizer)
            self.maxIter = EcceSpinCtrl(self,
                                        hardRange="[1..)",
                                        softRange="[1..1000]",
                                        name="ES.Runtype.GPW.MaxIter",
                                        default=100,
                                        label="Maximum Steps:",
                                        export=1)
            optSizer.AddWidget(self.maxIter)
            self.panelSizer.Add(optSizer)

            # CP2K stops only when the maximum force, the maximum
            # displacement and their RMS values are all below their
            # limits; ai.cp2k sets the two RMS limits from these two at
            # CP2K's own ratios.
            convSizer = EcceBoxSizer(self, label="Convergence", cols=1)
            self.maxForce = EcceFloatInput(self,
                                           unit="Ha/bohr",
                                           name="ES.Runtype.GPW.MaxForce",
                                           default=0.00045,
                                           hardRange="(0..)",
                                           softRange="[0.00001..0.01]",
                                           label="Maximum Force:",
                                           export=1)
            convSizer.AddWidget(self.maxForce)
            self.maxDR = EcceFloatInput(self,
                                        unit="bohr",
                                        name="ES.Runtype.GPW.MaxDR",
                                        default=0.003,
                                        hardRange="(0..)",
                                        softRange="[0.0001..0.05]",
                                        label="Maximum Displacement:",
                                        export=1)
            convSizer.AddWidget(self.maxDR)
            self.panelSizer.Add(convSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = Cp2kRunTypeFrame(None,
                         title="ECCE CP2K Editor: Runtype Details",
                         app=app,
                         helpURL="")
