from templates import *


class XtbRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = XtbRunTypePanel(self, helpURL)
        self.Finalize()

class XtbRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.RunType == "Geometry":
            geometrySizer = EcceBoxSizer(self,
                                         label="Geometry Optimization",
                                         cols=1)
            # xtb's own optimization levels; ai.xtb passes the word on.
            self.level = EcceComboBox(self,
                                      choices=["crude", "sloppy", "loose",
                                               "normal", "tight",
                                               "verytight", "extreme"],
                                      name="ES.Runtype.GeomOpt.Level",
                                      default=3,
                                      label="Level:",
                                      export=1)
            geometrySizer.AddWidget(self.level)
            self.panelSizer.Add(geometrySizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = XtbRunTypeFrame(None,
                        title="ECCE xTB Editor: Runtype Details",
                        app=app,
                        helpURL="")
