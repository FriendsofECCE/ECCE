from templates import *
import os

class MetaTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = MetaTheoryPanel(self, helpURL)
        self.Finalize()
            
class MetaTheoryPanel(EccePanel):
    def __init__(self,parent,helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # GENERAL SETTING
        generalSizer = EcceBoxSizer(self, style = 1, cols = 2)

        self.SCFDirect = EcceComboBox(self,
                                      choices = ["Restricted", "Unrestricted"],
                                      name = "ES.Theory.NWPW.Spintype",
                                      default = 0,
                                      label = "Spin Type:")
        generalSizer.AddWidget(self.SCFDirect)

        self.panelSizer.Add(generalSizer)

        # MEMORY/DISK LIMIT
        memTitle = "Memory Limit"
        memorySizer = EcceBoxSizer(self, memTitle, 4)

        self.memTog = EcceCheckBox(self,
                                   label = " Memory:",
                                   name = "ES.Theory.SCF.Memory",
                                   default = True,
                                   export = 1)
        memorySizer.AddWidget(self.memTog)

        self.memSize = EcceSpinCtrl(self,
                                    hardRange = "[0..)",
                                    unit = "Megawords",
                                    name = "ES.Theory.SCF.MemorySize",
                                    default = 400,
                                    export = 1)
        memorySizer.AddWidget(self.memSize)

        self.panelSizer.Add(memorySizer)

        # SCF CONVERGENCE
        scfSizer = EcceBoxSizer(self, "SCF Settings", 2)
        scfLeftSizer = EcceVBoxSizer()
        scfRightSizer = EcceVBoxSizer()

        self.scfTog = EcceCheckBox(self,
                                   label = " Apply SCF Settings",
                                   name = "ES.Theory.SCF.SCFTogValue",
                                   default = False)
        scfLeftSizer.AddWidget(self.scfTog)
        
        self.algorithm = EcceComboBox(self,
                                      choices = ["CG", "RMM-DIIS"],
                                      name = "ES.Theory.SCF.LinearAlgorithm",
                                      default = 0,
                                      label = "Linear Solver Algorithm:")
        scfLeftSizer.AddWidget(self.algorithm,
                                border = EcceGlobals.BorderDefault)

        self.mixing = EcceComboBox(self,
                                   choices = ["Anderson","Simple","Broyden"],
                                   name = "ES.Theory.SCF.MixingAlgorithm",
                                   default = 0,
                                   label = "Mixing Algorithm:")
        scfLeftSizer.AddWidget(self.mixing,
                               border = EcceGlobals.BorderDefault)

        self.mixingField = EcceComboBox(self,
                                        choices = ["Density","Potential"],
                                        name = "ES.Theory.SCF.MixingField",
                                        default = 0,
                                        label = "Mixing Field:")
        scfLeftSizer.AddWidget(self.mixingField,
                               border = EcceGlobals.BorderDefault)

        self.maxInnerIter = EcceSpinCtrl(self,
                                    hardRange = "[0..)",
                                    name = "ES.Theory.SCF.InnerIterations",
                                    default = 20,
                                    label = "Max. Linear Solver Iterations:")
        scfRightSizer.AddWidget(self.maxInnerIter,
                               border = EcceGlobals.BorderDefault)
        self.maxOuterIter = EcceSpinCtrl(self,
                                    hardRange = "[0..)",
                                    name = "ES.Theory.SCF.OuterIterations",
                                    default = 0,
                                    label = "Max. Reorthogonalization Iterations:")
        scfRightSizer.AddWidget(self.maxOuterIter,
                               border = EcceGlobals.BorderDefault)

        self.dampingSize = EcceFloatInput(self,
                                          default = 0.25,
                                          name = "ES.Theory.SCF.DampingSize",
                                          hardRange = "[0..)",
                                          unit = "Hartree",
                                          label = "SCF Damping",
                                          export = 0)
        scfRightSizer.AddWidget(self.dampingSize,
                                border = EcceGlobals.BorderDefault)
        
        self.kerkerSize = EcceFloatInput(self,
                                         default = 0.0,
                                         name = "ES.Theory.SCF.KerkerSize",
                                         hardRange = "[0..)",
                                         unit = "Hartree",
                                         label = "SCF Kerker Cutoff",
                                         export = 0)
        scfRightSizer.AddWidget(self.kerkerSize,
                                border = EcceGlobals.BorderDefault)
        
        scfSizer.AddWidget(scfLeftSizer,
                           flag = wx.ALL)
        scfSizer.AddWidget(scfRightSizer,
                           flag = wx.ALL)
        self.panelSizer.Add(scfSizer)

        # THEORY OPTIONS NWPW
        nwpwSizer = EcceBoxSizer(self,
                                label = "Theory Options - NWPW",
                                cols = 2)

        nwpwLeftSizer = EcceVBoxSizer()
        nwpwRightSizer = EcceVBoxSizer()

        xcFuncChoice = ["Vosko",
                        "lda",
                        "pbe96",
                        "revpbe",
                        "hf",
                        "lda-sic",
                        "lda-sic/2",
                        "lda-0.4sic",
                        "lda-sic/4",
                        "lda-0.2sic",
                        "pbe96-sic",
                        "pbe96-sic/2",
                        "pbe96-0.4sic",
                        "pbe96-sic/4",
                        "pbe96-0.2sic",
                        "revpbe-sic",
                        "revpbe-sic/2",
                        "revpbe-0.4sic",
                        "revpbe-sic/4",
                        "revpbe-0.2sic",
                        "pbe0",
                        "revpbe0"]
        xcFuncDefault = 0
        self.xcFunc = EcceComboBox(self,
                                   choices = xcFuncChoice,
                                   name = "ES.Theory.NWPW.XCFunctionals",
                                   default = xcFuncDefault,
                                   label = "Exchange-Correlation:")
        nwpwLeftSizer.AddWidget(self.xcFunc)

        cutoffSizer = EcceHBoxSizer()
        self.usecutoff = EcceCheckBox(self,
                                    label = "Cutoff Energy:",
                                    name = "ES.Theory.NWPW.UseCutoff",
                                    default = False)
        cutoffSizer.AddWidget(self.usecutoff)
        self.cutoff = EcceFloatInput(self,
                                     default = 30.0,
                                     name = "ES.Theory.NWPW.Cutoff",
                                     unit = "Hartree",
                                     hardRange = "[0..)",
                                     label = "",
                                     export = 1)
        cutoffSizer.AddWidget(self.cutoff)
        nwpwRightSizer.AddWidget(cutoffSizer, border=0)

        npDimensionsSizer = EcceHBoxSizer()
        self.useNpDimensions = EcceCheckBox(self,
                                    label = "np_dimensions",
                                    name = "ES.Theory.NWPW.UseNpDimensions",
                                    default = False)
        npDimensionsSizer.AddWidget(self.useNpDimensions)
        self.npDimensionsRows = EcceSpinCtrl(self,
                                   hardRange = "[-1..)",
                                   name = "ES.Theory.NWPW.NpDimensionsRows",
                                   default = -1,
                                   label = "Rows:",
                                   export = 1)
        npDimensionsSizer.AddWidget(self.npDimensionsRows)
        nwpwLeftSizer.AddWidget(npDimensionsSizer)
        
        self.npDimensionsCols = EcceSpinCtrl(self,
                                   hardRange = "[-1..)",
                                   name = "ES.Theory.NWPW.NpDimensionsCols",
                                   default = -1,
                                   label = "Columns:",
                                   export = 1)
        nwpwRightSizer.AddWidget(self.npDimensionsCols)

        rcutSizer = EcceHBoxSizer()
        self.usercut = EcceCheckBox(self,
                                    label = "Ewald rcut:",
                                    name = "ES.Theory.NWPW.UseRcut",
                                    default = False)
        rcutSizer.AddWidget(self.usercut)
        self.rcut = EcceFloatInput(self,
                                   default = 0.0,
                                   name = "ES.Theory.NWPW.EwaldRcut",
                                   hardRange = "[0..)",
                                   label = "")
        rcutSizer.AddWidget(self.rcut)
        nwpwLeftSizer.AddWidget(rcutSizer)

        ncutSizer = EcceHBoxSizer()
        self.usencut = EcceCheckBox(self,
                                    label = "Ewald ncut:",
                                    name = "ES.Theory.NWPW.UseNcut",
                                    default = False)
        ncutSizer.AddWidget(self.usencut)
        self.ncut = EcceSpinCtrl(self,
                                 hardRange = "[0..)",
                                 name = "ES.Theory.NWPW.EwaldNcut",
                                 default = 1,
                                 label = "")
        ncutSizer.AddWidget(self.ncut)
        nwpwRightSizer.AddWidget(ncutSizer)

        self.mapping = EcceSpinCtrl(self,
                                 hardRange = "[1..2]",
                                 name = "ES.Theory.NWPW.mapping",
                                 default = 1,
                                 label = "Parallel Mapping")
        nwpwLeftSizer.AddWidget(self.mapping)

        self.mulliken = EcceCheckBox(self,
                                     label = "Mulliken Analysis",
                                     name = "ES.Theory.NWPW.Mulliken",
                                     default = False)
        nwpwRightSizer.AddWidget(self.mulliken)

        nwpwSizer.AddWidget(nwpwLeftSizer,
                            flag = wx.ALL)
        nwpwSizer.AddWidget(nwpwRightSizer,
                            flag = wx.ALL)
        self.panelSizer.Add(nwpwSizer)

        self.AddButtons()
        
    def CheckDependency(self):
        self.memSize.Enable(self.memTog.GetValue())

        # Toggle entire SCF Panel
        self.algorithm.Enable(self.scfTog.GetValue())
        self.mixing.Enable(self.scfTog.GetValue())
        self.mixingField.Enable(self.scfTog.GetValue())
        self.maxInnerIter.Enable(self.scfTog.GetValue())
        self.maxOuterIter.Enable(self.scfTog.GetValue())
        self.dampingSize.Enable(self.scfTog.GetValue())
        self.kerkerSize.Enable(self.scfTog.GetValue())

        self.cutoff.Enable(self.usecutoff.GetValue())
        self.rcut.Enable(self.usercut.GetValue())
        self.ncut.Enable(self.usencut.GetValue())
        self.npDimensionsRows.Enable(self.useNpDimensions.GetValue())
        self.npDimensionsCols.Enable(self.useNpDimensions.GetValue())

frame = MetaTheoryFrame(None,
                        title = "ECCE Metadynamics Editor: Theory Details",
                        app = app,
                        helpURL = "")
