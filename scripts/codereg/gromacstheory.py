from templates import *

################################################################################
#
# GROMACS Theory Details dialog.
#
# STATUS: groundwork. GROMACS.edml is not referenced by either
# ResourceDescriptor file, so no GROMACS calculation can be created and
# this dialog is not reachable from the GUI. See docs/GROMACS_ROADMAP.md
# and issue #106, which holds the scope decision that gates registering
# it.
#
# WHAT IS DELIBERATELY ABSENT: the force field. GROMACS's force field is
# chosen when the topology is built (pdb2gmx and friends), not at run
# time, and under the import-only scope the user brings a .top that
# already embeds that choice. Offering a force-field picker here would
# suggest ECCE can change something it cannot. Adding one is option 2 in
# the roadmap and means driving pdb2gmx, which is a different piece of
# work.
#
# Everything here maps to an .mdp setting that ai.gromacs writes. Keys
# must match the names ai.gromacs reads EXACTLY -- a mismatch produces a
# missing line in the .mdp and GROMACS silently applies its own default
# instead of what the user chose, which is the single most repeated bug
# shape in this codebase.
#
################################################################################


class GromacsTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = GromacsTheoryPanel(self, helpURL)
        self.Finalize()


class GromacsTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        # ---- Electrostatics -------------------------------------------
        #
        # PME is the default and is what almost every modern condensed
        # phase simulation wants. Cut-off is offered because it is
        # legitimate for gas-phase or coarse-grained work, and
        # Reaction-Field because it remains in use for some force
        # fields.
        elecSizer = EcceBoxSizer(self, label="Electrostatics", cols=2)

        self.coulombtype = EcceComboBox(self,
                                        choices=["PME",
                                                 "Cut-off",
                                                 "Reaction-Field",
                                                 "PME-Switch"],
                                        name="ES.Theory.MD.Coulombtype",
                                        default=0,
                                        label="Method:",
                                        export=1)
        elecSizer.AddWidget(self.coulombtype)

        # rcoulomb / rvdw, in NANOMETRES -- GROMACS's own unit. No
        # conversion is done anywhere in this integration: unlike
        # Quantum ESPRESSO, whose Rydberg has no entry in ECCE's
        # EnergyConverter at all, GROMACS's units are used as they come.
        self.coulombCutoff = EcceFloatInput(self,
                                            unit="nm",
                                            name="ES.Theory.MD.CoulombCutoff",
                                            default=0.9,
                                            hardRange="(0..)",
                                            label="Coulomb cutoff:",
                                            export=1)
        elecSizer.AddWidget(self.coulombCutoff)

        self.vdwCutoff = EcceFloatInput(self,
                                        unit="nm",
                                        name="ES.Theory.MD.VdwCutoff",
                                        default=0.9,
                                        hardRange="(0..)",
                                        label="Van der Waals cutoff:",
                                        export=1)
        elecSizer.AddWidget(self.vdwCutoff)
        self.panelSizer.Add(elecSizer)

        # ---- Neighbour searching --------------------------------------
        #
        # The Verlet scheme is not offered as a choice because it is the
        # only one modern GROMACS has: the group scheme was removed in
        # 2019. ai.gromacs writes it unconditionally rather than
        # pretending there is a decision to make.
        nbSizer = EcceBoxSizer(self, label="Neighbour Searching", cols=1)

        self.nstlist = EcceSpinCtrl(self,
                                    hardRange="[1..)",
                                    softRange="[1..100]",
                                    name="ES.Runtype.MD.NeighbourListInterval",
                                    default=10,
                                    label="Update interval (steps):",
                                    export=1)
        nbSizer.AddWidget(self.nstlist)
        self.panelSizer.Add(nbSizer)

        self.AddButtons()

    def CheckDependency(self):
        # rcoulomb is meaningless without a cutoff-based method, but
        # GROMACS still requires the value to be present and consistent
        # with rvdw under the Verlet scheme, so nothing is disabled here.
        pass


frame = GromacsTheoryFrame(None,
                           title="ECCE GROMACS Editor: Theory Details",
                           app=app,
                           helpURL="")
