from templates import *

# ECCE-QM "Runtype Details" dialog.  Every run type has the Localised
# orbitals tick (it is about what the run produces, so it is not a theory
# choice; the key keeps its ES.Theory name so stored calculations reopen with
# their setting).  The frequency run types add Raman activities, which cost
# about seven times the frequency run (the polarisability at every displaced
# geometry).  ai.ecceqm turns the settings into the engine's `raman` and
# `localise` keywords.


class EcceqmRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = EcceqmRunTypePanel(self, helpURL)
        self.Finalize()


class EcceqmRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if (EcceGlobals.RunType == "Vibration" or
                EcceGlobals.RunType == "GeoVib" or
                EcceGlobals.RunType == "Transition State (Opt-TS)"):
            vibSizer = EcceBoxSizer(self, label="Frequencies", cols=1)
            self.useRaman = EcceCheckBox(self,
                                         label=" Raman activities (about 7 times the cost)",
                                         name="ES.Runtype.Vibration.UseRaman",
                                         default=False,
                                         export=1)
            vibSizer.AddWidget(self.useRaman)
            self.panelSizer.Add(vibSizer)
        elif EcceGlobals.RunType == "IRC":
            ircSizer = EcceBoxSizer(self, label="Reaction Path", cols=1)
            self.ircSteps = EcceSpinCtrl(self,
                                         hardRange="[1..)",
                                         softRange="[1..200]",
                                         name="ES.Runtype.IRC.MaximumStepsValue",
                                         default=20,
                                         label="Max points per direction:",
                                         export=1)
            ircSizer.AddWidget(self.ircSteps)
            #  The step is a real number; a spin control takes whole numbers only.
            self.ircSize = EcceFloatInput(self,
                                        hardRange="(0..)",
                                        softRange="[0.02..0.5]",
                                        name="ES.Runtype.IRC.StepSize",
                                        default=0.1,
                                        label="Step (bohr*sqrt(amu)):",
                                        export=1)
            ircSizer.AddWidget(self.ircSize)
            self.ircDirection = EcceComboBox(self,
                                             choices=["Both", "Forward", "Backward"],
                                             name="ES.Runtype.IRC.Direction",
                                             default=0,
                                             label="Direction:",
                                             export=1)
            ircSizer.AddWidget(self.ircDirection)
            #  0 keeps the Hessian from the transition state, updated each step.
            self.ircHessEvery = EcceSpinCtrl(self,
                                             hardRange="[0..)",
                                             softRange="[0..50]",
                                             name="ES.Runtype.IRC.HessianEvery",
                                             default=0,
                                             label="New Hessian every n points (0: never):",
                                             export=1)
            ircSizer.AddWidget(self.ircHessEvery)
            self.panelSizer.Add(ircSizer)

        locSizer = EcceBoxSizer(self, label="Orbitals", cols=1)
        locSizer.AddWidget(EcceCheckBox(
            self, label=" Localised orbitals (bonds and lone pairs, same set for each spin)",
            name="ES.Theory.ECCEQM.Localise", default=True, export=1))
        self.panelSizer.Add(locSizer)

        self.AddButtons()
        self.msg.Hide()

    def CheckDependency(self):
        pass


frame = EcceqmRunTypeFrame(None,
                           title="ECCE-QM Editor: Runtype Details",
                           app=app,
                           helpURL="")
