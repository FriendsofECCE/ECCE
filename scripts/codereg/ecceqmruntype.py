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
