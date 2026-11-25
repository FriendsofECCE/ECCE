from templates import *
import os
import sys


def trace(msg):
    # How the default ligand was chosen (#1244); ECCE_VINA_TRACE=1 prints it.
    if os.environ.get("ECCE_VINA_TRACE"):
        sys.stderr.write("VINADLG: %s\n" % msg)
        sys.stderr.flush()


class VinaRunTypeFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = VinaRunTypePanel(self, helpURL)
        self.Finalize()

class VinaRunTypePanel(EccePanel):
    def __init__(self, parent, helpURL):
        EccePanel.__init__(self, parent, helpURL)

        # The split keys are the shared docking layer's (docking.pl), so they
        # are the same as xtb's.  Empty means the smallest molecule.
        dockSizer = EcceBoxSizer(self, label="Receptor and Ligand", cols=1)
        # The editor lists the structure's ligand candidates in a side file
        # (<out>.molecules, label TAB atom range); without it (older editor,
        # no residues) the list stays hidden and only the field is shown.
        self.molecules = []
        self.defaulted = False
        self.chooser = wx.Choice(self, -1, choices=[])
        self.chooser.Bind(wx.EVT_CHOICE, self.OnChoose)
        self.chooserLabel = wx.StaticText(self, label="Ligand molecule:")
        self.chooserRow = wx.BoxSizer(wx.HORIZONTAL)
        self.chooserRow.Add(self.chooserLabel, 0, EcceGlobals.FlagDefault, 5)
        self.chooserRow.Add(self.chooser, 0, EcceGlobals.FlagDefault, 0)
        dockSizer.Add(self.chooserRow)
        self.ligand = EcceTextInput(self,
                                    name="ES.Runtype.Docking.LigandAtoms",
                                    size=(160, -1),
                                    default="",
                                    label="Other (atom numbers):",
                                    tip="Atom numbers, for example 1-5,9",
                                    export=1)
        dockSizer.AddWidget(self.ligand)
        # What the generator chose while Ligand atoms is empty.  Not a
        # setting: the editor writes it to a side file after each Save.
        self.autoNote = wx.StaticText(self, label="")
        dockSizer.AddWidget(self.autoNote)
        self.autoNoteText = ""
        self.noteTimer = wx.Timer(self, 4711)
        self.Bind(wx.EVT_TIMER, self.OnNoteTimer, self.noteTimer, 4711)
        self.noteTimer.Start(1000)
        self.ReadMolecules()
        self.ShowAutoNote()
        self.lcharge = EcceSpinCtrl(self,
                                    name="ES.Runtype.Docking.LigandCharge",
                                    default=0,
                                    hardRange="[-10..10]",
                                    label="Ligand charge:",
                                    export=1)
        dockSizer.AddWidget(self.lcharge)
        self.panelSizer.Add(dockSizer)

        boxSizer = EcceBoxSizer(self, label="Search Box", cols=1)
        # ai.vina computes the box from the ligand atoms when this is on.
        self.auto = EcceCheckBox(self,
                                 label=" Centre the box on the ligand",
                                 name="ES.Runtype.Vina.BoxAuto",
                                 default=True,
                                 export=1)
        boxSizer.AddWidget(self.auto)
        # Also the margin of the Builder's Tools > Docking Box Around
        # Selection, which writes the centre and size below.
        self.margin = EcceFloatInput(self,
                                     unit="A",
                                     name="ES.Runtype.Vina.BoxMargin",
                                     default=5.0,
                                     hardRange="[0..)",
                                     softRange="[2..10]",
                                     label="Margin around the atoms:",
                                     export=1)
        boxSizer.AddWidget(self.margin)
        self.center = []
        self.size = []
        for axis in ("X", "Y", "Z"):
            c = EcceFloatInput(self,
                               unit="A",
                               name="ES.Runtype.Vina.BoxCenter" + axis,
                               default=0.0,
                               label="Centre %s:" % axis,
                               export=1)
            boxSizer.AddWidget(c)
            self.center.append(c)
        for axis in ("X", "Y", "Z"):
            s = EcceFloatInput(self,
                               unit="A",
                               name="ES.Runtype.Vina.BoxSize" + axis,
                               default=20.0,
                               hardRange="(0..)",
                               softRange="[8..60]",
                               label="Size %s:" % axis,
                               export=1)
            boxSizer.AddWidget(s)
            self.size.append(s)
        self.panelSizer.Add(boxSizer)

        searchSizer = EcceBoxSizer(self, label="Search", cols=1)
        self.exh = EcceSpinCtrl(self,
                                name="ES.Runtype.Vina.Exhaustiveness",
                                default=8,
                                hardRange="[1..]",
                                label="Exhaustiveness:",
                                export=1)
        searchSizer.AddWidget(self.exh)
        self.nposes = EcceSpinCtrl(self,
                                   name="ES.Runtype.Vina.NumPoses",
                                   default=9,
                                   hardRange="[1..100]",
                                   label="Number of poses:",
                                   export=1)
        searchSizer.AddWidget(self.nposes)
        self.erange = EcceFloatInput(self,
                                     unit="kcal/mol",
                                     name="ES.Runtype.Vina.EnergyRange",
                                     default=3.0,
                                     hardRange="(0..)",
                                     softRange="[1..10]",
                                     label="Energy range:",
                                     export=1)
        searchSizer.AddWidget(self.erange)
        self.seed = EcceSpinCtrl(self,
                                 name="ES.Runtype.Vina.Seed",
                                 default=0,
                                 hardRange="[0..]",
                                 label="Random seed (0: random):",
                                 export=1)
        searchSizer.AddWidget(self.seed)
        self.panelSizer.Add(searchSizer)

        prepSizer = EcceBoxSizer(self, label="Preparation", cols=1)
        self.amides = EcceCheckBox(self,
                                   label=" Amide bonds of the ligand may rotate",
                                   name="ES.Runtype.Vina.FlexibleAmides",
                                   default=False,
                                   export=1)
        prepSizer.AddWidget(self.amides)
        self.panelSizer.Add(prepSizer)

        self.AddButtons()

    def ReadMolecules(self):
        mols = []
        try:
            with open(EcceGlobals.FdIn + ".molecules", encoding="utf-8") as f:
                for line in f:
                    parts = line.rstrip("\n").split("\t")
                    if len(parts) == 2 and parts[0] and parts[1]:
                        mols.append((parts[0], parts[1]))
        except (OSError, UnicodeDecodeError) as e:
            mols = []
            if not self.molecules:
                trace("no candidates from %s.molecules: %s" % (EcceGlobals.FdIn, e))
        if mols != self.molecules:
            trace("candidates from %s.molecules (%s, loop %s): %r"
                  % (EcceGlobals.FdIn, EcceGlobals.RestoreFlag,
                     EcceGlobals.EventLoopStarted, mols))
            self.molecules = mols
            self.chooser.Set([m[0] for m in mols] + ["Other (atom numbers)"])
            self.chooserRow.ShowItems(bool(mols))
            self.Layout()
        self.DefaultLigand()
        self.SyncChooser()

    def DefaultLigand(self):
        # An empty key means the first candidate; a stored one is kept.
        if not self.molecules or self.defaulted or self.ligand.GetValue().strip():
            return
        value = self.molecules[0][1]
        if EcceGlobals.RestoreFlag == "NO_GUIValues":
            # Values-only run (the run type was just chosen): no window and
            # no event loop; FinalizeSetting sends every widget's value and
            # closes, so the field only has to hold it by then.
            self.defaulted = True
            self.ligand.SetValue(value)
            trace("default %s set for the values-only run" % value)
        elif EcceGlobals.EventLoopStarted:
            # Shown dialog: only after the stored values are restored and the
            # editor has #INITIALIZED, so it is sent as an edit.
            self.defaulted = True
            trace("default %s sent as an edit" % value)
            self.SetLigandAtoms(value)

    def SetLigandAtoms(self, value):
        # The same path as a typed edit (EcceTextInput.OnChanges).
        self.ligand.SetValue(value)
        e = wx.CommandEvent()
        e.SetEventObject(self.ligand)
        self.OnChanges(e)
        trace("sent %s" % self.ligand.ExportSetting())

    def SyncChooser(self):
        if not self.molecules:
            return
        current = self.ligand.GetValue().strip()
        sel = len(self.molecules)
        for i, m in enumerate(self.molecules):
            if m[1] == current:
                sel = i
                break
        if self.chooser.GetSelection() != sel:
            self.chooser.SetSelection(sel)

    def OnChoose(self, event):
        i = self.chooser.GetSelection()
        if 0 <= i < len(self.molecules):
            self.SetLigandAtoms(self.molecules[i][1])
        event.Skip()

    def OnNoteTimer(self, event):
        if not getattr(self, "ticked", False):
            self.ticked = True
            trace("first timer tick, loop %s" % EcceGlobals.EventLoopStarted)
        self.ReadMolecules()
        try:
            with open(EcceGlobals.FdIn + ".note", encoding="utf-8") as f:
                self.autoNoteText = f.readline().strip()
        except (OSError, UnicodeDecodeError):
            self.autoNoteText = ""
        self.ShowAutoNote()

    def ShowAutoNote(self):
        # Shown only while the field is empty; a typed list is the ligand.
        text = self.autoNoteText
        if self.ligand.GetValue().strip():
            text = ""
        if self.autoNote.GetLabel() != text:
            self.autoNote.SetLabel(text)
            self.Layout()

    def CheckDependency(self):
        self.SyncChooser()
        self.ShowAutoNote()
        manual = not self.auto.GetValue()
        for c in self.center + self.size:
            c.Enable(manual)


frame = VinaRunTypeFrame(None,
                         title="ECCE Vina Editor: Runtype Details",
                         app=app,
                         helpURL="")
