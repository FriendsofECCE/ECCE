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

        if EcceGlobals.RunType == "Docking (aISS)":
            dockSizer = EcceBoxSizer(self, label="Docking", cols=1)
            # Empty means the smallest molecule in the system (docking.pl).
            self.ligand = EcceTextInput(self,
                                        name="ES.Runtype.Docking.LigandAtoms",
                                        size=(160, -1),
                                        default="",
                                        label="Ligand atoms (empty: smallest molecule):",
                                        tip="Atom numbers, for example 1-5,9",
                                        export=1)
            dockSizer.AddWidget(self.ligand)
            self.lcharge = EcceSpinCtrl(self,
                                        name="ES.Runtype.Docking.LigandCharge",
                                        default=0,
                                        hardRange="[-10..10]",
                                        label="Ligand charge:",
                                        export=1)
            dockSizer.AddWidget(self.lcharge)
            self.nposes = EcceSpinCtrl(self,
                                       name="ES.Runtype.Docking.NumPoses",
                                       default=4,
                                       hardRange="[1..100]",
                                       label="Poses to optimise:",
                                       export=1)
            dockSizer.AddWidget(self.nposes)
            self.fast = EcceCheckBox(self,
                                     label=" Fast search (small molecules)",
                                     name="ES.Runtype.Docking.Fast",
                                     default=True,
                                     export=1)
            dockSizer.AddWidget(self.fast)
            self.panelSizer.Add(dockSizer)

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

        if EcceGlobals.RunType == "Conformer search (simple)":
            confSizer = EcceBoxSizer(self, label="Conformer Search", cols=1)
            self.samples = EcceSpinCtrl(self,
                                            name="ES.Runtype.Conformers.Samples",
                                            default=30,
                                            hardRange="[1..]",
                                            label="Random samples:",
                                            export=1)
            confSizer.AddWidget(self.samples)
            self.ewin = EcceFloatInput(self,
                                       unit="kcal/mol",
                                       name="ES.Runtype.Conformers.EnergyWindow",
                                       default=6.0,
                                       hardRange="(0..)",
                                       softRange="[1..30]",
                                       label="Energy window:",
                                       export=1)
            confSizer.AddWidget(self.ewin)
            self.rmsd = EcceFloatInput(self,
                                       unit="A",
                                       name="ES.Runtype.Conformers.RMSD",
                                       default=0.25,
                                       hardRange="(0..)",
                                       softRange="[0.05..1]",
                                       label="Heavy-atom RMSD threshold:",
                                       export=1)
            confSizer.AddWidget(self.rmsd)
            self.seed = EcceSpinCtrl(self,
                                         name="ES.Runtype.Conformers.Seed",
                                         default=682,
                                         hardRange="[0..]",
                                         label="Random seed:",
                                         export=1)
            confSizer.AddWidget(self.seed)
            self.panelSizer.Add(confSizer)

        self.AddButtons()

    def CheckDependency(self):
        if EcceGlobals.RunType == "Molecular Dynamics":
            self.shake.Enable(EcceGlobals.Theory != "GFN-FF")


frame = XtbRunTypeFrame(None,
                        title="ECCE xTB Editor: Runtype Details",
                        app=app,
                        helpURL="")
