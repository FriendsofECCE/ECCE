from templates import *

# ECCE-QM "Runtype Details" dialog.  Only the frequency run types have an
# option: Raman activities, which cost about seven times the frequency run
# (the polarisability at every displaced geometry).  ai.ecceqm turns the
# setting into the engine's `raman` keyword.


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
            self.ircSize = EcceSpinCtrl(self,
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
            self.panelSizer.Add(ircSizer)
        else:
            note = wx.StaticText(self, label="No options for this run type.")
            self.panelSizer.Add(note)

        self.AddButtons()
        self.msg.Hide()

    def CheckDependency(self):
        pass


frame = EcceqmRunTypeFrame(None,
                           title="ECCE-QM Editor: Runtype Details",
                           app=app,
                           helpURL="")
