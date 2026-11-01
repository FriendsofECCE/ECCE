from templates import *


class CrestRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = CrestRunTypePanel(self, helpURL)
        self.Finalize()

class CrestRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        confSizer = EcceBoxSizer(self, label="Conformer Search", cols=1)
        self.ewin = EcceFloatInput(self,
                                   unit="kcal/mol",
                                   name="ES.Runtype.Conformers.EnergyWindow",
                                   default=6.0,
                                   hardRange="(0..)",
                                   softRange="[1..30]",
                                   label="Energy window:",
                                   export=1)
        confSizer.AddWidget(self.ewin)
        self.nci = EcceCheckBox(self,
                                label=" Non-covalent complex (NCI mode)",
                                name="ES.Runtype.Conformers.NCI",
                                default=False,
                                export=1)
        confSizer.AddWidget(self.nci)
        self.noreftopo = EcceCheckBox(self,
                                      label=" Skip the reference-topology check",
                                      name="ES.Runtype.Conformers.NoRefTopo",
                                      default=False,
                                      export=1)
        confSizer.AddWidget(self.noreftopo)
        self.panelSizer.Add(confSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = CrestRunTypeFrame(None,
                          title="ECCE CREST Editor: Runtype Details",
                          app=app,
                          helpURL="")
