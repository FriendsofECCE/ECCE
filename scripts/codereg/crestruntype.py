from templates import *


class CrestRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = CrestRunTypePanel(self, helpURL)
        self.Finalize()

class CrestRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        runtype = EcceGlobals.RunType
        if runtype == "Conformer Search":
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

        elif runtype == "Solvation Shell":
            qcgSizer = EcceBoxSizer(self, label="Solvation Shell (QCG)", cols=1)
            # ai.crest holds a geometry for each of these.
            self.solvent = EcceComboBox(self,
                                        choices=["water", "methanol", "acetonitrile"],
                                        name="ES.Runtype.QCG.Solvent",
                                        default=0,
                                        label="Solvent molecule:",
                                        export=1)
            qcgSizer.AddWidget(self.solvent)
            self.nsolv = EcceSpinCtrl(self,
                                      name="ES.Runtype.QCG.Number",
                                      default=10,
                                      hardRange="[1..]",
                                      label="Number of solvent molecules:",
                                      export=1)
            qcgSizer.AddWidget(self.nsolv)
            # CREST 3.0.2 crashes in CREGEN on this step, so it is opt-in.
            self.ensemble = EcceCheckBox(self,
                                         label=" Ensemble of clusters",
                                         name="ES.Runtype.QCG.Ensemble",
                                         default=False,
                                         export=1)
            qcgSizer.AddWidget(self.ensemble)
            self.panelSizer.Add(qcgSizer)
        elif runtype in ("Protonation Sites", "Tautomers"):
            siteSizer = EcceBoxSizer(self, label=runtype, cols=1)
            # CREST's own window is 30 kcal/mol.
            self.sitewin = EcceFloatInput(self,
                                          unit="kcal/mol",
                                          name="ES.Runtype.Sites.EnergyWindow",
                                          default=30.0,
                                          hardRange="(0..)",
                                          softRange="[5..100]",
                                          label="Energy window:",
                                          export=1)
            siteSizer.AddWidget(self.sitewin)
            self.panelSizer.Add(siteSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = CrestRunTypeFrame(None,
                          title="ECCE CREST Editor: Runtype Details",
                          app=app,
                          helpURL="")
