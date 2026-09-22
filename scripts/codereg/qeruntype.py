from templates import *

# ECCE Quantum ESPRESSO "Runtype Details" dialog (phase 1 first cut),
# modeled on qeruntype's siblings mopacruntype.py / orcaruntype.py.
#
# QuantumESPRESSO.edml offers exactly one runtype, Energy, which maps to
# pw.x's calculation='scf'.  There is no Geometry, GeoVib, Vibration,
# Gradient or Car-Parrinello entry, deliberately:
#
#   * relax / vc-relax (ECCE's Geometry) are roadmap phases 3 and 4 --
#     they need the &IONS and &CELL namelists, an ATOMIC_POSITIONS
#     geometry-trace parser and a CELL_PARAMETERS per-step lattice
#     parser, each of which is its own set of Begin/Skip/End hazards.
#
#   * Vibration would need a THREE-executable chain (pw.x, then ph.x,
#     then dynmat.x, sharing prefix/outdir), which does not fit ECCE's
#     "one calculation, one input, one binary, one output" model at all
#     -- one <Input type="primary">, one parse stream, one .desc, and one
#     registered binary path per code.  See docs/QE_ROADMAP.md section
#     3.2.  MOPAC set the precedent for shipping a code without a
#     runtype its reduced scope cannot honestly support.
#
#   * A separate Gradient runtype would add nothing: forces on an scf run
#     are a single &CONTROL flag (tprnfor), which is a checkbox here.
#
# So what is left for this dialog is the two output requests that turn an
# scf run into something ECCE can extract properties from.  Both are real
# &CONTROL logicals, both verified accepted and honoured by the real
# Quantum ESPRESSO 6.7 binary at /usr/bin/pw.x.


class QeRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = QeRunTypePanel(self, helpURL)
        self.Finalize()


class QeRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.RunType == "Energy":

            # tprnfor -- "Forces acting on atoms (cartesian axes,
            # Ry/au):".  Default ON: it costs almost nothing on top of an
            # scf run, and it is what feeds qe.desc's [EGRADVEC] entry
            # and so the gradient-vector 3-D overlay.  With it off, that
            # parse type simply never fires (which is correct, not a
            # bug -- but it means no forces anywhere in the results).
            propSizer = EcceBoxSizer(self, label="Properties", cols=1)
            self.printForces = EcceCheckBox(self,
                                            label=" Forces on atoms (tprnfor)",
                                            name="ES.Runtype.PW.PrintForces",
                                            default=True,
                                            export=1)
            propSizer.AddWidget(self.printForces)

            # tstress -- "total   stress  (Ry/bohr**3) ... P= ...".
            # Default OFF, and not because it is expensive (it is not for
            # an scf run) but because NOTHING IN ECCE CAN SHOW IT:
            # data/client/config/properties has no stress or pressure
            # property at all, so qe.desc declares no entry for the
            # block and the numbers would only ever be read by eye in the
            # output file.  The checkbox exists because reading them by
            # eye is a legitimate thing to want; adding a real stress
            # property is roadmap phase 4.
            self.printStress = EcceCheckBox(self,
                                            label=" Stress tensor (tstress)",
                                            name="ES.Runtype.PW.PrintStress",
                                            default=False,
                                            export=1)
            propSizer.AddWidget(self.printStress)
            self.panelSizer.Add(propSizer)

        elif EcceGlobals.RunType == "Geometry":

            # calculation='relax'.  Forces are NOT offered as a choice
            # here: pw.x needs them to move the ions, so ai.qe forces
            # tprnfor on for a relax whatever a checkbox said.  Offering
            # a control that is then overridden would be worse than
            # offering none.
            convSizer = EcceBoxSizer(self, label="Convergence", cols=1)

            # forc_conv_thr, in Ry/bohr.  pw.x's own default is 1.0d-3,
            # stated explicitly so the deck does not change meaning if
            # that default ever does.  Exposed because it is the single
            # knob that decides whether an optimisation finishes in six
            # steps or sixty.
            self.forceConv = EcceExpInput(self,
                                          unit="Ry/bohr",
                                          name="ES.Runtype.PW.ForcConvThr",
                                          default=1e-3,
                                          hardRange="(0..)",
                                          label="Force Convergence:",
                                          export=1)
            convSizer.AddWidget(self.forceConv)

            # nstep: pw.x defaults to 50 for relax.  A run that hits the
            # cap stops WITHOUT converging and still writes a final
            # geometry, which looks like success, so the value is worth
            # having in front of the user.
            self.maxSteps = EcceSpinCtrl(self,
                                         hardRange="[1..)",
                                         softRange="[1..500]",
                                         name="ES.Runtype.PW.NStep",
                                         default=50,
                                         label="Maximum Steps:",
                                         export=1)
            convSizer.AddWidget(self.maxSteps)
            self.panelSizer.Add(convSizer)

        self.AddButtons()

    def CheckDependency(self):
        pass


frame = QeRunTypeFrame(None,
                       title="ECCE Quantum ESPRESSO Editor: Runtype Details",
                       app=app,
                       helpURL="")
