from templates import *

# ECCE MOPAC "Theory Details" dialog (reduced scope first cut).
#
# Deliberately minimal, same principle as orcatheory.py: every field
# here corresponds to an AbiDict key that scripts/parsers/ai.mopac
# actually reads and turns into a real MOPAC keyword.  Widgets whose
# value the input generator would silently drop are not added.
#
# NOTE ON THE HAMILTONIAN: there is no method/functional chooser here.
# For MOPAC the Hamiltonian *is* the theory (PM7/PM6/PM3/AM1/RM1/MNDO),
# so it is chosen in CalcEd's own Theory list from MOPAC.edml's
# <Theory> entries and arrives here as EcceGlobals.Theory.  There is
# likewise no basis set anywhere in the MOPAC path -- MOPAC.edml marks
# every theory needsBasis="false", which greys out the Basis Set Tool
# and stops ESInputController from writing a .basis file at all.
#
# NOTE ON MEMORY: CLAUDE.md's standing preference is that memory
# settings are entered and labeled in GB for every code.  MOPAC has NO
# memory keyword of any kind -- verified by dumping the complete keyword
# table compiled into the real MOPAC 23.1.2 binary (the
# " *  <KEYWORD>  - <description>" banner strings it echoes) and finding
# no memory/MB/GB/core-allocation entry at all.  So there is deliberately
# no Memory field on this dialog: the preference is about how a memory
# setting is *labeled* where one exists, not a reason to invent one that
# the code would ignore.  THREADS= (below) is MOPAC's only resource knob.
#
# Every keyword this dialog can produce was verified against the real
# binary at /usr/bin/mopac: accepted without error AND echoed in MOPAC's
# own keyword banner with the intended meaning.

class MopacTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = MopacTheoryPanel(self, helpURL)
        self.Finalize()

class MopacTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        # SCF CONVERGENCE
        #
        # ITRY=n -- MOPAC's SCF iteration limit ("DO A MAXIMUM OF n ..."
        # in the binary's own banner).  Left at 0 by default, which
        # ai.mopac takes to mean "emit no ITRY= keyword at all and let
        # MOPAC use its own default" -- MOPAC's built-in limit is large
        # and appropriate for almost every job, so imposing an ECCE
        # number here would only ever make convergence worse.
        #
        # PRECISE -- "TIGHTER CRITERIA TO BE USED".  Verified live: the
        # keyword is echoed and the job runs to a visibly tighter SCF
        # and geometry criterion.
        scfSizer = EcceBoxSizer(self, label="SCF Convergence", cols=2)
        # 0 means "emit no ITRY= keyword at all", leaving MOPAC on its own
        # built-in limit of 2000 -- ai.mopac only writes ITRY=n when n > 0.
        # Spelling that out in the label: read as a bare "0" it looks like
        # zero iterations, which is what it looked like to the first person
        # who opened this dialog.
        self.maxIterationsSpin = EcceSpinCtrl(self,
                                              hardRange="[0..)",
                                              name="ES.Theory.SCF.ConvergenceIterations",
                                              default=0,
                                              label="Max. SCF Iterations (0 = MOPAC default, 2000):",
                                              export=1)
        scfSizer.AddWidget(self.maxIterationsSpin)

        self.precise = EcceCheckBox(self,
                                    label=" Tighter criteria (PRECISE)",
                                    name="ES.Theory.SCF.Precise",
                                    default=False,
                                    export=1)
        scfSizer.AddWidget(self.precise)
        self.panelSizer.Add(scfSizer)

        # PARALLEL
        #
        # THREADS=n -- MOPAC's only parallelism control, and it is
        # shared-memory threading, not MPI, so there is no node count to
        # expose.  Left at 1 (the default) lets mopac.launchpp inject
        # whatever thread count Launch actually requested at launch
        # time; setting it explicitly here wins over that, the same way
        # orcatheory.py's Processors field wins over gensub's %pal.
        procSizer = EcceBoxSizer(self, label="Parallel", cols=1)
        # Label spells out what 1 means, because a "Threads: 1" here next
        # to a "4" in the Launcher reads as a contradiction -- it isn't,
        # 1 means "emit no THREADS= keyword and let mopac.launchpp inject
        # whatever the launcher actually requested", which is the normal
        # case. Only a value >1 pins it at edit time and overrides the
        # launcher.
        self.numProcs = EcceSpinCtrl(self,
                                     hardRange="[1..)",
                                     name="ES.Theory.SCF.NumProcessors",
                                     default=1,
                                     label="Threads (1 = use launcher's count):",
                                     export=1)
        procSizer.AddWidget(self.numProcs)
        self.panelSizer.Add(procSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = MopacTheoryFrame(None,
                         title="ECCE MOPAC Editor: Theory Details",
                         app=app,
                         helpURL="")
