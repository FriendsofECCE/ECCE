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
#
# 2026-09-21 session (Priority 2, issue #38 follow-up): added SCF
# convergence tolerance, RIJCOSX (+ auxiliary basis), and widened the
# DFT functional list -- every new "! <keyword>" route-card token added
# here was round-tripped against a real ORCA 6.1.1 install first (small
# water-molecule single points/opts; see scratch test notes in the PR/
# issue discussion). Geometry/Frequency runtype-specific options went
# into orcaruntype.py instead, matching Gaussian-16's split.

class OrcaTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = OrcaTheoryPanel(self, helpURL)
        self.Finalize()

class OrcaTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # SCF CONVERGENCE -- ConvergenceTolerance maps to one of ORCA's
        # own simple-input convergence keywords (e.g. "! TightSCF") on
        # the route line; "Normal" emits nothing (ORCA's own default).
        # Verified against real ORCA 6.1.1: LooseSCF/TightSCF/
        # VeryTightSCF all accepted and visibly changed the printed
        # "Convergence Tolerance" TolE/TolErr values.
        scfSizer = EcceBoxSizer(self, label="SCF Convergence", cols=2)
        self.maxIterationsSpin = EcceSpinCtrl(self,
                                              hardRange="[0..)",
                                              name="ES.Theory.SCF.ConvergenceIterations",
                                              default=64,
                                              label="Max. Iterations:")
        scfSizer.AddWidget(self.maxIterationsSpin)

        scfConvChoice = ["Normal",
                         "Loose",
                         "Tight",
                         "Very Tight"]
        self.scfConv = EcceComboBox(self,
                                    choices=scfConvChoice,
                                    name="ES.Theory.SCF.ConvergenceTolerance",
                                    default=0,
                                    label="Convergence:",
                                    export=1)
        scfSizer.AddWidget(self.scfConv)
        self.panelSizer.Add(scfSizer)

        # RIJCOSX -- RI-J + "chain of spheres" approximate exchange, the
        # single most common speed knob in real-world ORCA input files.
        # Verified against real ORCA 6.1.1 (both "RKS B3LYP ... RIJCOSX
        # def2/J" and "HF ... RIJCOSX def2/J") -- COSX grid generation
        # and "Your calculation utilizes the auxiliary basis: def2/J"
        # both confirmed in the output, for DFT and HF alike.
        riSizer = EcceBoxSizer(self, label="RI Approximation", cols=2)
        self.useRIJCOSX = EcceCheckBox(self,
                                       label=" Use RIJCOSX",
                                       name="ES.Theory.SCF.UseRIJCOSX",
                                       default=False,
                                       export=1)
        riSizer.AddWidget(self.useRIJCOSX)

        auxBasisChoice = ["def2/J",
                          "def2/JK",
                          "SARC/J"]
        self.auxBasis = EcceComboBox(self,
                                     choices=auxBasisChoice,
                                     name="ES.Theory.SCF.RIJCOSXAuxBasis",
                                     default=0,
                                     label="Auxiliary Basis:",
                                     export=1)
        riSizer.AddWidget(self.auxBasis)
        self.panelSizer.Add(riSizer)

        # MEMORY -- ORCA's "%maxcore" is memory PER CORE, in MB (confirmed
        # against a real ORCA 6.1.1 run: reported "Max core memory ... N MB"
        # matches the per-process setting, not a job total) -- same
        # convention as NWChem's "Gigabytes / core" MemorySize widget in
        # nedtheory.py. Entered value is per-core GB; ai.orca's
        # ORCABlocks converts it to the MB/core ORCA's own %maxcore
        # directive actually needs.
        memSizer = EcceBoxSizer(self, label="Memory", cols=1)
        # Upper bound of 512 GB/core: nothing real needs more, and it
        # stops a legacy MB-era stored value (this field meant MB until
        # 78bb8d0, with default=1000) from being accepted as gigabytes.
        # ai.orca has a matching guard for values that slip past this.
        self.memSize = EcceSpinCtrl(self,
                                    hardRange="[0..512]",
                                    unit="Gigabytes / core",
                                    name="ES.Theory.SCF.MemorySize",
                                    default=1,
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

            # Every entry verified as a real ORCA 6.1.1 route-card
            # keyword (small water-molecule RKS single points, no
            # "ERROR"/"not recognized" and a normal SCF run for each) --
            # keep in lockstep with ai.orca's DFTXCFun map.
            xcFuncChoice = ["B3LYP",
                            "PBE0",
                            "PBE",
                            "revPBE",
                            "RPBE",
                            "BP86",
                            "BLYP",
                            "B97-D3",
                            "TPSS",
                            "TPSSh",
                            "M06L",
                            "M06",
                            "M062X",
                            "CAM-B3LYP",
                            "wB97X-D3"]
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
        self.auxBasis.Enable(self.useRIJCOSX.GetValue())


frame = OrcaTheoryFrame(None,
                        title="ECCE ORCA Editor: Theory Details",
                        app=app,
                        helpURL="")
