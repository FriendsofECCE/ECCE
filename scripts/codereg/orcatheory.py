from templates import *

# ECCE ORCA "Theory Details" dialog (issue #38, reduced scope).
#
# Deliberately minimal: every field here corresponds to an AbiDict key
# ai.orca (scripts/parsers/ai.orca) actually reads. Modeled on
# ged16theory.py, but that file exposes many SCF/DFT options ai.orca
# has no equivalent handling for yet (algorithm choice, level shift,
# grid density, ...) -- adding those widgets here without ai.orca
# consuming them would just be dead UI, so they're left out until the
# input generator grows to match. The XCFunctionals choice list below
# is kept in lockstep with ai.orca's DFTXCFun translation table -- every
# entry here must have a matching entry there.

class OrcaTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = OrcaTheoryPanel(self, helpURL)
        self.Finalize()

class OrcaTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # SCF CONVERGENCE
        scfSizer = EcceBoxSizer(self, label="SCF Convergence", cols=2)
        self.maxIterationsSpin = EcceSpinCtrl(self,
                                              hardRange="[0..)",
                                              name="ES.Theory.SCF.ConvergenceIterations",
                                              default=64,
                                              label="Max. Iterations:")
        scfSizer.AddWidget(self.maxIterationsSpin)
        self.panelSizer.Add(scfSizer)

        # DFT FUNCTIONAL -- keep in sync with ai.orca's DFTXCFun map
        if EcceGlobals.Category == "DFT":
            dftSizer = EcceBoxSizer(self, label="DFT Functional", cols=1)

            xcFuncChoice = ["B3LYP",
                            "PBE0",
                            "PBE",
                            "BP86",
                            "BLYP",
                            "TPSS",
                            "M06L",
                            "M06"]
            self.xcFunc = EcceComboBox(self,
                                       choices=xcFuncChoice,
                                       name="ES.Theory.DFT.XCFunctionals",
                                       default=0,
                                       label="Functional:",
                                       export=1)
            dftSizer.AddWidget(self.xcFunc)
            self.panelSizer.Add(dftSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = OrcaTheoryFrame(None,
                        title="ECCE ORCA Editor: Theory Details",
                        app=app,
                        helpURL="")
