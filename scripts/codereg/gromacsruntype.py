from templates import *

################################################################################
#
# GROMACS Runtype Details dialog.
#
# STATUS: groundwork, and not reachable -- see gromacstheory.py.
#
# Three runtypes, mapping onto mdrun integrators in ai.gromacs:
#
#   Energy    -> integrator = md, nsteps = 0
#                GROMACS has no dedicated single-point integrator; zero
#                steps evaluates the input configuration once.
#   Geometry  -> integrator = steep     (energy minimisation)
#   Dynamics  -> integrator = md
#
# Every key here must match what ai.gromacs reads exactly. A mismatch
# leaves the line out of the .mdp and GROMACS applies its own default
# silently, which looks like the setting was ignored rather than lost.
#
################################################################################


class GromacsRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = GromacsRunTypePanel(self, helpURL)
        self.Finalize()


class GromacsRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.RunType == "Geometry":

            # Energy minimisation. emtol is the force tolerance that
            # decides convergence; nsteps is a cap, and a run that hits
            # it stops WITHOUT converging while still writing a final
            # structure -- which looks like success, so the value is
            # worth having in front of the user.
            minSizer = EcceBoxSizer(self, label="Minimisation", cols=2)

            self.emtol = EcceFloatInput(self,
                                        unit="kJ/mol/nm",
                                        name="ES.Runtype.MD.EnergyTolerance",
                                        default=1000.0,
                                        hardRange="(0..)",
                                        label="Force tolerance:",
                                        export=1)
            minSizer.AddWidget(self.emtol)

            self.emstep = EcceFloatInput(self,
                                         unit="nm",
                                         name="ES.Runtype.MD.MinimizerStep",
                                         default=0.01,
                                         hardRange="(0..)",
                                         label="Initial step size:",
                                         export=1)
            minSizer.AddWidget(self.emstep)

            self.maxSteps = EcceSpinCtrl(self,
                                         hardRange="[1..)",
                                         softRange="[1..100000]",
                                         name="ES.Runtype.MD.MaxSteps",
                                         default=5000,
                                         label="Maximum steps:",
                                         export=1)
            minSizer.AddWidget(self.maxSteps)
            self.panelSizer.Add(minSizer)

        elif EcceGlobals.RunType == "Dynamics":

            # The integration timestep and run length. dt is in
            # PICOSECONDS: 0.002 ps is 2 fs, which is the usual choice
            # and is only safe with bonds to hydrogen constrained --
            # hence the constraints control below, which defaults to
            # h-bonds to match.
            mdSizer = EcceBoxSizer(self, label="Integration", cols=2)

            self.timestep = EcceFloatInput(self,
                                           unit="ps",
                                           name="ES.Runtype.MD.TimeStep",
                                           default=0.002,
                                           hardRange="(0..)",
                                           label="Timestep:",
                                           export=1)
            mdSizer.AddWidget(self.timestep)

            self.nsteps = EcceSpinCtrl(self,
                                       hardRange="[1..)",
                                       softRange="[1..10000000]",
                                       name="ES.Runtype.MD.NumberOfSteps",
                                       default=5000,
                                       label="Number of steps:",
                                       export=1)
            mdSizer.AddWidget(self.nsteps)

            self.constraints = EcceComboBox(self,
                                            choices=["h-bonds",
                                                     "all-bonds",
                                                     "none",
                                                     "h-angles",
                                                     "all-angles"],
                                            name="ES.Runtype.MD.Constraints",
                                            default=0,
                                            label="Constraints:",
                                            export=1)
            mdSizer.AddWidget(self.constraints)
            self.panelSizer.Add(mdSizer)

            # Thermostat. Offered only for dynamics: temperature
            # coupling on a minimisation or a single point is accepted
            # by grompp but meaningless, and ai.gromacs omits the block
            # entirely for those rather than writing settings that do
            # nothing.
            tempSizer = EcceBoxSizer(self, label="Temperature Coupling",
                                     cols=2)

            self.thermostat = EcceComboBox(self,
                                           choices=["v-rescale",
                                                    "berendsen",
                                                    "nose-hoover",
                                                    "no"],
                                           name="ES.Runtype.MD.Thermostat",
                                           default=0,
                                           label="Thermostat:",
                                           export=1)
            tempSizer.AddWidget(self.thermostat)

            self.refT = EcceFloatInput(self,
                                       unit="K",
                                       name="ES.Runtype.MD.Temperature",
                                       default=300.0,
                                       hardRange="(0..)",
                                       label="Temperature:",
                                       export=1)
            tempSizer.AddWidget(self.refT)

            self.tauT = EcceFloatInput(self,
                                       unit="ps",
                                       name="ES.Runtype.MD.CouplingTime",
                                       default=0.1,
                                       hardRange="(0..)",
                                       label="Coupling time:",
                                       export=1)
            tempSizer.AddWidget(self.tauT)
            self.panelSizer.Add(tempSizer)

        # ---- Output, for every runtype --------------------------------
        #
        # nstlog is what gromacs.desc parses: mdrun writes an energy
        # block to the .log every nstlog steps, and those blocks are the
        # ONLY source of TE/TEVEC. Setting it to 0 would leave ECCE with
        # no progress and no energy trace at all -- an empty result that
        # looks like a failure rather than a choice.
        outSizer = EcceBoxSizer(self, label="Output", cols=2)

        self.nstlog = EcceSpinCtrl(self,
                                   hardRange="[1..)",
                                   softRange="[1..100000]",
                                   name="ES.Runtype.MD.LogInterval",
                                   default=100,
                                   label="Log interval (steps):",
                                   export=1)
        outSizer.AddWidget(self.nstlog)

        self.nstenergy = EcceSpinCtrl(self,
                                      hardRange="[1..)",
                                      softRange="[1..100000]",
                                      name="ES.Runtype.MD.EnergyInterval",
                                      default=100,
                                      label="Energy file interval:",
                                      export=1)
        outSizer.AddWidget(self.nstenergy)
        self.panelSizer.Add(outSizer)

        self.AddButtons()

    def CheckDependency(self):
        # The thermostat's parameters mean nothing with coupling off.
        if hasattr(self, "thermostat"):
            coupled = self.thermostat.GetValue() != "no"
            self.refT.Enable(coupled)
            self.tauT.Enable(coupled)


frame = GromacsRunTypeFrame(None,
                            title="ECCE GROMACS Editor: Runtype Details",
                            app=app,
                            helpURL="")
