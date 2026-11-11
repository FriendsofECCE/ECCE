from templates import *


class VinaRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = VinaRunTypePanel(self, helpURL)
        self.Finalize()

class VinaRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        # The split keys are the shared docking layer's (docking.pl), so they
        # are the same as xtb's.  Empty means the smallest molecule.
        dockSizer = EcceBoxSizer(self, label="Receptor and Ligand", cols=1)
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
        self.panelSizer.Add(dockSizer)

        boxSizer = EcceBoxSizer(self, label="Search Box", cols=1)
        # ai.vina computes the box from the ligand atoms when this is on.
        self.auto = EcceCheckBox(self,
                                 label=" Centre the box on the ligand",
                                 name="ES.Runtype.Vina.BoxAuto",
                                 default=True,
                                 export=1)
        boxSizer.AddWidget(self.auto)
        # Also the margin of the Builder's Tools > Docking Box Around
        # Selection, which writes the centre and size below.
        self.margin = EcceFloatInput(self,
                                     unit="A",
                                     name="ES.Runtype.Vina.BoxMargin",
                                     default=5.0,
                                     hardRange="[0..)",
                                     softRange="[2..10]",
                                     label="Margin around the atoms:",
                                     export=1)
        boxSizer.AddWidget(self.margin)
        self.center = []
        self.size = []
        for axis in ("X", "Y", "Z"):
            c = EcceFloatInput(self,
                               unit="A",
                               name="ES.Runtype.Vina.BoxCenter" + axis,
                               default=0.0,
                               label="Centre %s:" % axis,
                               export=1)
            boxSizer.AddWidget(c)
            self.center.append(c)
        for axis in ("X", "Y", "Z"):
            s = EcceFloatInput(self,
                               unit="A",
                               name="ES.Runtype.Vina.BoxSize" + axis,
                               default=20.0,
                               hardRange="(0..)",
                               softRange="[8..60]",
                               label="Size %s:" % axis,
                               export=1)
            boxSizer.AddWidget(s)
            self.size.append(s)
        self.panelSizer.Add(boxSizer)

        searchSizer = EcceBoxSizer(self, label="Search", cols=1)
        self.exh = EcceSpinCtrl(self,
                                name="ES.Runtype.Vina.Exhaustiveness",
                                default=8,
                                hardRange="[1..]",
                                label="Exhaustiveness:",
                                export=1)
        searchSizer.AddWidget(self.exh)
        self.nposes = EcceSpinCtrl(self,
                                   name="ES.Runtype.Vina.NumPoses",
                                   default=9,
                                   hardRange="[1..100]",
                                   label="Number of poses:",
                                   export=1)
        searchSizer.AddWidget(self.nposes)
        self.erange = EcceFloatInput(self,
                                     unit="kcal/mol",
                                     name="ES.Runtype.Vina.EnergyRange",
                                     default=3.0,
                                     hardRange="(0..)",
                                     softRange="[1..10]",
                                     label="Energy range:",
                                     export=1)
        searchSizer.AddWidget(self.erange)
        self.seed = EcceSpinCtrl(self,
                                 name="ES.Runtype.Vina.Seed",
                                 default=0,
                                 hardRange="[0..]",
                                 label="Random seed (0: random):",
                                 export=1)
        searchSizer.AddWidget(self.seed)
        self.panelSizer.Add(searchSizer)

        prepSizer = EcceBoxSizer(self, label="Preparation", cols=1)
        self.amides = EcceCheckBox(self,
                                   label=" Amide bonds of the ligand may rotate",
                                   name="ES.Runtype.Vina.FlexibleAmides",
                                   default=False,
                                   export=1)
        prepSizer.AddWidget(self.amides)
        self.panelSizer.Add(prepSizer)

        self.AddButtons()

    def CheckDependency(self):
        manual = not self.auto.GetValue()
        for c in self.center + self.size:
            c.Enable(manual)


frame = VinaRunTypeFrame(None,
                         title="ECCE Vina Editor: Runtype Details",
                         app=app,
                         helpURL="")
