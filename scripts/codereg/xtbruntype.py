from templates import *


class XtbRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = XtbRunTypePanel(self, helpURL)
        self.Finalize()

class XtbRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.RunType in ("Geometry", "GeoVib"):
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

        if EcceGlobals.RunType == "Molecular Dynamics":
            mdSizer = EcceBoxSizer(self, label="Molecular Dynamics", cols=1)
            self.temp = EcceFloatInput(self,
                                       unit="K",
                                       name="ES.Runtype.MD.Temperature",
                                       default=300.0,
                                       hardRange="(0..)",
                                       softRange="[10..2000]",
                                       label="Temperature:",
                                       export=1)
            mdSizer.AddWidget(self.temp)
            self.time = EcceFloatInput(self,
                                       unit="ps",
                                       name="ES.Runtype.MD.Time",
                                       default=1.0,
                                       hardRange="(0..)",
                                       softRange="[0.1..1000]",
                                       label="Time:",
                                       export=1)
            mdSizer.AddWidget(self.time)
            self.step = EcceFloatInput(self,
                                       unit="fs",
                                       name="ES.Runtype.MD.Step",
                                       default=1.0,
                                       hardRange="(0..)",
                                       softRange="[0.1..5]",
                                       label="Time step:",
                                       export=1)
            mdSizer.AddWidget(self.step)
            self.dump = EcceFloatInput(self,
                                       unit="fs",
                                       name="ES.Runtype.MD.Dump",
                                       default=20.0,
                                       hardRange="(0..)",
                                       softRange="[1..1000]",
                                       label="Dump interval:",
                                       export=1)
            mdSizer.AddWidget(self.dump)
            # xtb's GFN-FF crashes with SHAKE; ai.xtb refuses that pair.
            self.shake = EcceComboBox(self,
                                      choices=["off", "X-H bonds",
                                               "all bonds"],
                                      name="ES.Runtype.MD.Shake",
                                      default=0,
                                      label="SHAKE:",
                                      export=1)
            mdSizer.AddWidget(self.shake)
            self.panelSizer.Add(mdSizer)

        self.AddButtons()

    def CheckDependency(self):
        if EcceGlobals.RunType == "Molecular Dynamics":
            self.shake.Enable(EcceGlobals.Theory != "GFN-FF")


frame = XtbRunTypeFrame(None,
                        title="ECCE xTB Editor: Runtype Details",
                        app=app,
                        helpURL="")
