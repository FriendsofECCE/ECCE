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

        # MEMORY -- ORCA's "%maxcore" is memory PER CORE, in MB (confirmed
        # against a real ORCA 6.1.1 run: reported "Max core memory ... N MB"
        # matches the per-process setting, not a job total) -- same
        # convention as NWChem's "Gigabytes / core" MemorySize widget in
        # nedtheory.py, not Gaussian's job-total "Gigabytes" one. Uses
        # ORCA's own native unit (MB) directly rather than converting
        # through GB.
        memSizer = EcceBoxSizer(self, label="Memory", cols=1)
        self.memSize = EcceSpinCtrl(self,
                                    hardRange="[0..)",
                                    unit="Megabytes / core",
                                    name="ES.Theory.SCF.MemorySize",
                                    default=1000,
                                    label="Memory Per Core:",
                                    export=1)
        memSizer.AddWidget(self.memSize)
        self.panelSizer.Add(memSizer)

        # PROCESSORS -- lets a job's "%pal nprocs N end" block be set
        # explicitly at edit time (e.g. for a manually-run input file),
        # independent of whatever processor count Launch later requests.
        # gensub's orca() sub only injects its own %pal block if the
        # input file doesn't already have one from here, so an explicit
        # choice here always wins over the launch-time queue setting --
        # leave at 1 (the default) to let Launch's queue choice decide.
        procSizer = EcceBoxSizer(self, label="Parallel", cols=1)
        self.numProcs = EcceSpinCtrl(self,
                                     hardRange="[1..)",
                                     name="ES.Theory.SCF.NumProcessors",
                                     default=1,
                                     label="Processors:",
                                     export=1)
        procSizer.AddWidget(self.numProcs)
        self.panelSizer.Add(procSizer)

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
