from templates import *

NEB_RUNTYPES = ("Transition State (NEB-TS)",
                "Transition State (NEB-TS) + Frequencies")
FREQ_RUNTYPES = ("Vibration", "GeoVib", NEB_RUNTYPES[1])

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

            # %geom Calc_Hess / Recalc_Hess; the default writes neither, so
            # ORCA keeps its own model Hessian.
            hessSizer = EcceBoxSizer(self, label="Hessian", cols=1)
            self.hessian = EcceComboBox(self,
                                        choices=["Default", "Calculate"],
                                        name="ES.Runtype.GeomOpt.InitialHessian",
                                        default=0,
                                        label="Initial Hessian:",
                                        export=1)
            hessSizer.AddWidget(self.hessian)
            self.recalc = EcceSpinCtrl(self,
                                       hardRange="[0..)",
                                       softRange="[0..50]",
                                       name="ES.Runtype.GeomOpt.RecalcFC",
                                       default=0,
                                       label="Recalculate every N steps (0 = never):",
                                       export=1)
            hessSizer.AddWidget(self.recalc)
            self.panelSizer.Add(hessSizer)

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

        # Reactant, product and TS guess are picked in the Calculation Editor.
        if EcceGlobals.RunType in NEB_RUNTYPES:
            nebSizer = EcceBoxSizer(self, label="Transition State (NEB-TS)",
                                    cols=1)
            self.nebImages = EcceSpinCtrl(self,
                                          hardRange="[2..)",
                                          softRange="[2..32]",
                                          name="ES.Runtype.NEB.Images",
                                          default=8,
                                          label="Images:",
                                          export=1)
            nebSizer.AddWidget(self.nebImages)
            # Automatic = yes when the end points come from calculations at
            # another level than this one (the Calculation Editor decides).
            self.nebPreopt = EcceComboBox(self,
                                          choices=["Automatic", "Yes", "No"],
                                          name="ES.Runtype.NEB.PreoptEnds",
                                          default=0,
                                          label="Re-optimise the end points at this level:",
                                          export=1)
            nebSizer.AddWidget(self.nebPreopt)
            self.panelSizer.Add(nebSizer)

        if EcceGlobals.RunType in FREQ_RUNTYPES:
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

            # Raman activities (issue #173), requested by "%elprop
            # Polar 1". Offered for every DFT/HF theory the dialog can
            # reach; ai.orca's verifyRamanRequest warns about the
            # combinations ORCA 6.1.1 actually cannot do this for
            # (double hybrids outright; plain B88-exchange functionals
            # -- BLYP/BP86/B3LYP -- unless Method above is Numerical),
            # with a message naming the fix, rather than silently
            # emitting a keyword that dies or a deck missing a property.
            self.useRaman = EcceCheckBox(self,
                                         label=" Raman activities",
                                         name="ES.Runtype.Vibration.UseRaman",
                                         default=False,
                                         export=1)
            vibSizer.AddWidget(self.useRaman)
            self.panelSizer.Add(vibSizer)

        # IRC from a transition state (issue #417). The input geometry must
        # already be the TS; the initial Hessian is computed in the same job.
        if EcceGlobals.RunType == "IRC":
            ircSizer = EcceBoxSizer(self, label="Reaction Path", cols=1)
            self.ircSteps = EcceSpinCtrl(self,
                                         hardRange="[1..)",
                                         softRange="[1..200]",
                                         name="ES.Runtype.IRC.MaximumStepsValue",
                                         default=20,
                                         label="Max Steps per direction:",
                                         export=1)
            ircSizer.AddWidget(self.ircSteps)
            self.ircDirection = EcceComboBox(self,
                                             choices=["Both", "Forward", "Backward"],
                                             name="ES.Runtype.IRC.Direction",
                                             default=0,
                                             label="Direction:",
                                             export=1)
            ircSizer.AddWidget(self.ircDirection)
            self.ircHess = EcceComboBox(self,
                                        choices=["Analytic", "Numerical"],
                                        name="ES.Runtype.IRC.InitHess",
                                        default=0,
                                        label="Initial Hessian:",
                                        export=1)
            ircSizer.AddWidget(self.ircHess)
            self.panelSizer.Add(ircSizer)

        # Mulliken and Loewdin come with every ORCA job; these must be asked
        # for. ORCA computes them from the converged density of whatever job
        # ran, so they are offered for every run type. NBO needs NBOEXE in
        # the machine's ORCA environment.
        chargeSizer = EcceBoxSizer(self, label="Partial Charges", cols=2)
        self.useChelpg = EcceCheckBox(self,
                                      label=" CHELPG charges (ESP fit)",
                                      name="ES.Runtype.ORCA.UseCHELPG",
                                      default=False,
                                      export=1)
        chargeSizer.AddWidget(self.useChelpg)
        self.useHirshfeld = EcceCheckBox(self,
                                         label=" Hirshfeld charges",
                                         name="ES.Runtype.ORCA.UseHirshfeld",
                                         default=False,
                                         export=1)
        chargeSizer.AddWidget(self.useHirshfeld)
        self.useMbis = EcceCheckBox(self,
                                    label=" MBIS charges",
                                    name="ES.Runtype.ORCA.UseMBIS",
                                    default=False,
                                    export=1)
        chargeSizer.AddWidget(self.useMbis)
        self.useNbo = EcceCheckBox(self,
                                   label=" NBO analysis (natural charges)",
                                   name="ES.Runtype.ORCA.UseNBO",
                                   default=False,
                                   export=1)
        chargeSizer.AddWidget(self.useNbo)
        self.panelSizer.Add(chargeSizer)

        self.AddButtons()

    def CheckDependency(self):
        if (EcceGlobals.RunType == "Geometry" or
            EcceGlobals.RunType == "GeoVib"):
            self.stepSpin.Enable(self.stepCheckBox.GetValue())


frame = OrcaRunTypeFrame(None,
                         title="ECCE ORCA Editor: Runtype Details",
                         app=app,
                         helpURL="")
