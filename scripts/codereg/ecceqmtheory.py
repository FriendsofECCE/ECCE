from templates import *

# ECCE-QM "Theory Details" dialog.  Deliberately almost empty: the engine is
# for students, so there are no convergence, grid, memory or thread settings.
# The DFT functional list is the one place to add a functional (the keyword
# it maps to is in ai.ecceqm's %XCKeyword).

XC_FUNCTIONALS = ["SVWN", "PBE", "B3LYP", "PBE0"]
XC_DEFAULT = "B3LYP"


class EcceUnrestrictedRadio(wx.RadioButton):
    """The "Unrestricted" radio button as a stored setting, true when chosen.

    Behaves like EcceCheckBox (same stored value) so the generated deck is
    unchanged; its partner radio is the restricted choice."""

    def __init__(self, parent, partner, label, name):
        wx.RadioButton.__init__(self, parent, -1, label, name=name)
        self.parent = parent
        self.partner = partner
        self.save = self.default = False
        self.export = 1
        self.parent.Settings.append(self)
        self.partner.SetValue(True)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnChanges)
        self.partner.Bind(wx.EVT_RADIOBUTTON, self.OnChanges)

    def SetValue(self, value):
        wx.RadioButton.SetValue(self, bool(value))
        self.partner.SetValue(not value)

    def Reset(self):
        if self.NotDefault():
            self.SetValue(self.default)
            e = wx.CommandEvent()
            e.SetEventObject(self)
            self.parent.OnChanges(e)

    def Save(self):
        self.save = self.GetValue()

    def NotDefault(self):
        return self.default != self.GetValue()

    def NotSaved(self):
        return self.save != self.GetValue()

    def OnChanges(self, event):
        self.parent.OnChanges(event)

    def ExportSetting(self):
        needExport = 1
        return self.GetName() + "|" + str(int(self.GetValue())) + "||" + \
               str(int(self.IsEnabled())) + "|" + str(needExport) + \
               "|toggle_input"


class EcceqmTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = EcceqmTheoryPanel(self, helpURL)
        self.Finalize()


class EcceqmTheoryPanel(EccePanel):
    def __init__(self, parent, helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        if EcceGlobals.Category == "DFT":
            dftSizer = EcceBoxSizer(self, label="DFT Functional", cols=1)
            self.xcFunc = EcceComboBox(self,
                                       choices=XC_FUNCTIONALS,
                                       name="ES.Theory.DFT.XCFunctionals",
                                       default=XC_FUNCTIONALS.index(XC_DEFAULT),
                                       label="Functional:",
                                       export=1)
            dftSizer.AddWidget(self.xcFunc)
            self.panelSizer.Add(dftSizer)

        # Open-shell systems use restricted open-shell orbitals (ROHF/ROKS)
        # unless "Unrestricted" is chosen; a closed shell has no choice.
        # Two radio buttons so the default is visible, not an unticked box.
        orbSizer = EcceBoxSizer(self, label="Open shells", cols=1)
        self.restricted = wx.RadioButton(self,
                                         label="Restricted open-shell (ROHF/ROKS)",
                                         style=wx.RB_GROUP)
        self.unrestricted = EcceUnrestrictedRadio(self, self.restricted,
                                                  label="Unrestricted (UHF/UKS)",
                                                  name="ES.Theory.ECCEQM.Unrestricted")
        self.closedNote = wx.StaticText(self,
                                        label="Closed shell: RHF/RKS, no choice.")
        orbSizer.AddWidget(self.restricted)
        orbSizer.AddWidget(self.unrestricted)
        orbSizer.AddWidget(self.closedNote)
        self.panelSizer.Add(orbSizer)
        # Mulliken charges are always printed; these add to them.
        chgSizer = EcceBoxSizer(self, label="Extra atomic charges", cols=1)
        for label, name in ((" Loewdin", "ES.Theory.ECCEQM.Loewdin"),
                            (" CHELPG (fitted to the electrostatic potential)",
                             "ES.Theory.ECCEQM.CHELPG"),
                            (" Merz-Kollman (fitted to the electrostatic potential)",
                             "ES.Theory.ECCEQM.MK")):
            chgSizer.AddWidget(EcceCheckBox(self, label=label, name=name,
                                            default=False, export=1))
        self.panelSizer.Add(chgSizer)
        self.panelSizer.SetMinSize(wx.Size(380, -1))

        self.AddButtons()
        # No input here can be invalid, so the empty message box is dropped.
        self.msg.Hide()

    def CheckDependency(self):
        openShell = EcceGlobals.SpinMultiplicity > 1
        self.restricted.Enable(openShell)
        self.unrestricted.Enable(openShell)
        self.closedNote.Show(not openShell)
        self.Layout()


frame = EcceqmTheoryFrame(None,
                          title="ECCE-QM Editor: Theory Details",
                          app=app,
                          helpURL="")
