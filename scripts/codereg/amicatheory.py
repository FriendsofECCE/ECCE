from templates import *

class AmicaTheoryFrame(EcceFrame):
    def __init__(self, parent, title, app, helpURL=""):
        EcceFrame.__init__(self, parent, title)
        panel = AmicaTheoryPanel(self, helpURL)
        self.Finalize()
            
class AmicaTheoryPanel(EccePanel):
    def __init__(self,parent,helpURL=""):
        EccePanel.__init__(self, parent, helpURL)

        mainNB = wx.Notebook(self, style = wx.TAB_TRAVERSAL)

        irrep = []
        if (EcceGlobals.Category[0:2]=="HF" and
            EcceGlobals.Category.find("CC")==-1):
            
            hfPanel = EcceTabPanel(mainNB)
            shellSizer = EcceHBoxSizer()
            shellChoice = ["Closed Shell",
                           "Open Shell"]
            self.shell = EcceComboBox(hfPanel,
                                      choices = shellChoice,
                                      name = "ES.Theory.HF.OpenClosedShell",
                                      default = 0)
            shellSizer.AddWidget(self.shell)
            shellSizer.AddWidget(wx.StaticText(hfPanel, -1,
                                               "   Occupation Numbers:"))
            hfPanel.AddWidget(shellSizer)
            hfPanel.AddSpace()
            
            self.irrepNum = EcceTextInput(hfPanel,
                                          name="ES.Theory.HF.ShellOccupation",
                                          size = (150, -1),
                                          default="",
                                          label="No. of Orbitals in Each Irrep:")
            hfPanel.AddWidget(self.irrepNum)
            hfPanel.AddSpace()

            self.irrepLabel = wx.StaticText(hfPanel, -1,
                                            "Fractional Occupations for Each Irrep:")
            hfPanel.AddWidget(self.irrepLabel)
            hfPanel.AddSpace()

            self.irrepInput = {}

            if (EcceGlobals.SymmetryGroup == "D2h"):
                irrep = ["ag", "b3u", "b2u", "b1g", "b1u", "b2g", "b3g"]
            elif (EcceGlobals.SymmetryGroup == "C2h"):
                irrep = ["ag", "bu", "au", "bg"]
            elif (EcceGlobals.SymmetryGroup == "C2v"):
                irrep = ["a1", "b1", "a2", "b2"]
            elif (EcceGlobals.SymmetryGroup == "D2"):
                irrep = ["a1", "b1", "b2", "b3"]
            elif (EcceGlobals.SymmetryGroup == "C2"):
                irrep = ["a", "b"]
            elif (EcceGlobals.SymmetryGroup == "Cs"):
                irrep = ["a\'", "a\""]
            elif (EcceGlobals.SymmetryGroup == "Ci"):
                irrep = ["ag", "au"]
            else:
                EcceGlobals.SymmetryGroup = "C1"
                irrep = ["a"]

            for irrepItem in irrep:
                self.irrepInput[irrepItem] = EcceTextInput(hfPanel,
                                                           name="ES.Theory.Occupation" + str(irrep.index(irrepItem)),
                                                           size = (150, -1),
                                                           default="",
                                                           label=(" "+irrepItem+":")[-3:])
                hfPanel.AddWidget(self.irrepInput[irrepItem])
            if len(irrep)%2:
                hfPanel.AddSpace()

            couplingSizer = EcceHBoxSizer()
            hfPanel.AddWidget(couplingSizer)

            self.cplngLabel = wx.StaticText(hfPanel,
                                            label = "Open Shell Coupling Parameters:")
            couplingSizer.AddWidget(self.cplngLabel)
            
            self.cplngA = EcceTextInput(hfPanel,
                                        name = "ES.Theory.HF.CouplingA",
                                        label = " A:",
                                        size = (50, -1))
            couplingSizer.AddWidget(self.cplngA)

            self.cplngB = EcceTextInput(hfPanel,
                                        name = "ES.Theory.HF.CouplingB",
                                        label = " B:",
                                        size = (50, -1))
            couplingSizer.AddWidget(self.cplngB)

            self.scfEconv = EcceFloatInput(hfPanel,
                                           name = "ES.Theory.HF.EnergyConvergence",
                                           label = "Energy Convergence:",
                                           default = 1e-10,
                                           hardRange = "[0..)")
            hfPanel.AddSpace()
            hfPanel.AddWidget(self.scfEconv)
            
            hfPanel.SetAutoLayout(True)
            hfPanel.Fit()
            mainNB.AddPage(hfPanel, "Hartree-Fock")

        if (EcceGlobals.Category.find("MCSCF")!=-1 or
            EcceGlobals.Category.find("CI")!=-1):
            
            psiPanel = EcceTabPanel(mainNB)

            self.state = EcceSpinCtrl(psiPanel,
                                      name = "ES.Theory.PSI.State",
                                      label = "State:",
                                      default = 1,
                                      hardRange = "[1..)",
                                      unit = "1=ground state")

            psiPanel.AddWidget(self.state)

            irrepCap = []
            for each in irrep:
                irrepCap.append(each.capitalize())
            self.irrepSelect = EcceComboBox(psiPanel,
                                            name = "ES.Theory.PSI.Irrep",
                                            label = "Symmetry:",
                                            choices = irrepCap,
                                            default = len(irrepCap)-1)
            psiPanel.AddWidget(self.irrepSelect)

            self.interact = EcceCheckBox(psiPanel,
                                         name = "ES.Theory.PSI.Interact",
                                         label = "Interacting Space Restrictions",
                                         default = True)
            psiPanel.AddWidget(self.interact)

            self.asym = EcceCheckBox(psiPanel,
                                     name = "ES.Theory.PSI.Asym",
                                     label = "Use References of All Symmetries",
                                     default = False)
            psiPanel.AddWidget(self.asym)

            psiPanel.AddWidget(wx.StaticText(psiPanel, -1, "Orbital List:"))

            self.frozen = EcceTextInput(psiPanel,
                                        name = "ES.Theory.PSI.FrozenCore",
                                        label = "Frozen Core:",
                                        size = (150, -1))
            psiPanel.AddWidget(self.frozen)
            psiPanel.AddSpace()
                                        
            self.double = EcceTextInput(psiPanel,
                                        name = "ES.Theory.PSI.DoublyOcc",
                                        label = "Doubly Occupied:",
                                        size = (150, -1))
            psiPanel.AddWidget(self.double)
            psiPanel.AddSpace()

            self.active = EcceTextInput(psiPanel,
                                        name = "ES.Theory.PSI.ActiveOrbs",
                                        label = "Active:",
                                        size = (150, -1))
            psiPanel.AddWidget(self.active)

            psiPanel.AddWidget(wx.StaticText(psiPanel, -1,
                                             "Occupations of Active Levels:"))

            self.occMin = EcceTextInput(psiPanel,
                                        name = "ES.Theory.PSI.OccMin",
                                        label = "Minimum:",
                                        size = (150, -1))
            psiPanel.AddWidget(self.occMin)
            psiPanel.AddSpace()

            self.occMax = EcceTextInput(psiPanel,
                                        name = "ES.Theory.PSI.OccMax",
                                        label = "Maximum:",
                                        size = (150, -1))
            psiPanel.AddWidget(self.occMax)

            psiPanel.SetAutoLayout(True)
            psiPanel.Fit()
            mainNB.AddPage(psiPanel, "Reference Space")

        if (EcceGlobals.Category.find("MCSCF")!=-1):

            mcscfPanel = EcceTabPanel(mainNB)

            self.mcscfIter = EcceSpinCtrl(mcscfPanel,
                                          name = "ES.Theory.MCSCF.Iter",
                                          label = "Max. No. of Iterations:",
                                          default = 100,
                                          hardRange = "[1..)")
            mcscfPanel.AddWidget(self.mcscfIter)
            mcscfPanel.AddSpace()

            self.mcscfShift = EcceFloatInput(mcscfPanel,
                                             name = "ES.Theory.MCSCF.Shift",
                                             label = "PSCI Emergency Shift:",
                                             default = 0.0,
                                             hardRange = "[0..)")
            mcscfPanel.AddWidget(self.mcscfShift)
            mcscfPanel.AddSpace()

            self.mcscfWnorm = EcceFloatInput(mcscfPanel,
                                             name = "ES.Theory.MCSCF.Wnorm",
                                             label = "Orbital-State Coupling Tolerance:",
                                             default = 1e-3,
                                             hardRange = "[0..)")
            mcscfPanel.AddWidget(self.mcscfWnorm)
            mcscfPanel.AddSpace()

            self.mcscfFcilev = EcceTextInput(mcscfPanel,
                                             name = "ES.Theory.MCSCF.Fcilev",
                                             label = "Orbital Resolution Mask:",
                                             size = (150, -1))
            mcscfPanel.AddWidget(self.mcscfFcilev)
            mcscfPanel.AddSpace()

            self.mcscfFreeze = EcceTextInput(mcscfPanel,
                                             name = "ES.Theory.MCSCF.Freeze",
                                             label = "Frozen Core Orbitals:",
                                             size = (150, -1))
            mcscfPanel.AddWidget(self.mcscfFreeze)

            mcscfPanel.SetAutoLayout(True)
            mcscfPanel.Fit()
            mainNB.AddPage(mcscfPanel, "MCSCF")

        if (EcceGlobals.Category.find("CI")!=-1):
            ciPanel = EcceTabPanel(mainNB, column = 1)

            toggleSizer = wx.GridSizer(0, 7, EcceGlobals.BorderDefault,
                                       EcceGlobals.BorderDefault)
            toggleSizer.Add((0,0))

            self.acpf = EcceCheckBox(ciPanel,
                                     name = "ES.Theory.CI.ACPF",
                                     label = "ACPF",
                                     default = False)
            toggleSizer.Add(self.acpf, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.acpf2 = EcceCheckBox(ciPanel,
                                      name = "ES.Theory.CI.ACPF2",
                                      label = "ACPF2",
                                      default = True)
            toggleSizer.Add(self.acpf2, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.aqcc = EcceCheckBox(ciPanel,
                                     name = "ES.Theory.CI.AQCC",
                                     label = "AQCC",
                                     default = False)
            toggleSizer.Add(self.aqcc, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.cepa0 = EcceCheckBox(ciPanel,
                                      name = "ES.Theory.CI.CEPA0",
                                      label = "CEPA-0",
                                      default = False)
            toggleSizer.Add(self.cepa0, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.ci = EcceCheckBox(ciPanel,
                                   name = "ES.Theory.CI.CI",
                                   label = "CI",
                                   default = False)
            toggleSizer.Add(self.ci, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.cpf = EcceCheckBox(ciPanel,
                                    name = "ES.Theory.CI.CPF",
                                    label = "CPF",
                                    default = False)
            toggleSizer.Add(self.cpf, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.r12 = EcceCheckBox(ciPanel,
                                    name = "ES.Theory.CI.R12",
                                    label = "r12-",
                                    default = False)
            toggleSizer.Add(self.r12, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)
            self.r12.Bind(wx.EVT_CHECKBOX, self.R12Clicked)

            self.acpfR12 = EcceCheckBox(ciPanel,
                                        name = "ES.Theory.CI.R12ACPF",
                                        label = "ACPF",
                                        default = False)
            toggleSizer.Add(self.acpfR12, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.acpf2R12 = EcceCheckBox(ciPanel,
                                         name = "ES.Theory.CI.R12ACPF2",
                                         label = "ACPF2",
                                         default = True)
            toggleSizer.Add(self.acpf2R12, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.aqccR12 = EcceCheckBox(ciPanel,
                                        name = "ES.Theory.CI.R12AQCC",
                                        label = "AQCC",
                                        default = False)
            toggleSizer.Add(self.aqccR12, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.cepa0R12 = EcceCheckBox(ciPanel,
                                         name = "ES.Theory.CI.R12CEPA0",
                                         label = "CEPA-0",
                                         default = False)
            toggleSizer.Add(self.cepa0R12, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)

            self.ciR12 = EcceCheckBox(ciPanel,
                                      name = "ES.Theory.CI.R12CI",
                                      label = "CI",
                                      default = False)
            toggleSizer.Add(self.ciR12, 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)
            ciPanel.AddWidget(toggleSizer)
            self.line1 = [self.acpf, self.acpf2, self.aqcc,
                          self.cepa0, self.ci, self.cpf]
            self.line2 = [self.acpfR12, self.acpf2R12,
                          self.aqccR12, self.cepa0R12, self.ciR12]

            self.ciR12Orb = EcceSpinCtrl(ciPanel,
                                         name = "ES.Theory.CI.R12Orbs",
                                         label = "No. of Int. Orbs. Used for r12-Terms:",
                                         default = 0,
                                         hardRange = "[0..)",
                                         unit = "0=all")
            ciPanel.AddWidget(self.ciR12Orb)

            self.ciNElec = EcceSpinCtrl(ciPanel,
                                        name = "ES.Theory.CI.NELEC",
                                        label = "No. of Corr. Elec. in ACPF:",
                                        default = 0,
                                        hardRange = "[0..)",
                                        unit = "0=all")
            ciPanel.AddWidget(self.ciNElec)

            self.ciR12Shell = EcceTextInput(ciPanel,
                                            name = "ES.Theory.CI.R12shell",
                                            label = "Invariant Subspace Masks for r12-Terms:",
                                            size = (150, -1))
            ciPanel.AddWidget(self.ciR12Shell)
            
            ciSizer = wx.FlexGridSizer(0, 2)

            self.ciNIter = EcceSpinCtrl(ciPanel,
                                        name = "ES.Theory.CI.Iterations",
                                        label = "Max. No. of Iterations:",
                                        default = 30,
                                        hardRange = "[1..)")
            ciSizer.Add(self.ciNIter, 0, EcceGlobals.FlagDefault,
                        EcceGlobals.BorderDefault)

            self.ciEConv = EcceFloatInput(ciPanel,
                                          name = "ES.Theory.CI.EnergyConvergence",
                                          label = "Energy Convergence:",
                                          default = 1e-6,
                                          hardRange = "[0..)")
            ciSizer.Add(self.ciEConv, 0, EcceGlobals.FlagDefault,
                        EcceGlobals.BorderDefault)

            self.ciNoHess = EcceCheckBox(ciPanel,
                                         name = "ES.Theory.CI.NOHESS",
                                         label = "Ignore Indefinite Hessian",
                                         default = False)

            ciSizer.Add(self.ciNoHess, 0, EcceGlobals.FlagDefault,
                        EcceGlobals.BorderDefault)

            ciPropChoices = ["No Density/Property",
                             "1e-Density Matrix",
                             "1th-order Rel. Energy"]
            self.ciProp = EcceComboBox(ciPanel,
                                       name = "ES.Theory.CI.Properties",
                                       choices = ciPropChoices,
                                       default = 0)
            ciSizer.Add(self.ciProp, 0, EcceGlobals.FlagDefault,
                        EcceGlobals.BorderDefault)

            self.nvcimx = EcceSpinCtrl(ciPanel,
                                       name = "ES.Theory.CI.NVCIMX",
                                       label = "Max. No. of Kept CI Vectors:",
                                       default = 0,
                                       hardRange = "[0..)",
                                       unit = "0=all")
            ciSizer.Add(self.nvcimx, 0, EcceGlobals.FlagDefault,
                        EcceGlobals.BorderDefault)

            ciPanel.AddWidget(ciSizer)

            for each in self.line1 + self.line2:
                each.Bind(wx.EVT_CHECKBOX, self.ToggleClicked)
            
            ciPanel.SetAutoLayout(True)
            ciPanel.Fit()
            mainNB.AddPage(ciPanel, "(R12-)CI")

        if (EcceGlobals.Category.find("HF,CC")!=-1):
            ccPanel = EcceTabPanel(mainNB)

            self.ccR12Choices = ["Traditional", "r12", "r12/Extr. Pairs"]
            self.ccR12Default = 0
            self.ccR12 = EcceComboBox(ccPanel,
                                      name = "ES.Theory.CC.R12_CC",
                                      choices = self.ccR12Choices,
                                      default = self.ccR12Default,
                                      export = 1)
            ccPanel.AddWidget(self.ccR12)

            self.ccDirectChoices = ["None", "HF", "R12", "U12", "HF+R12",
                                    "HF+U12", "R12+U12", "HF+R12+U12"]
            self.ccDirectDefault = 0
            self.ccDirect = EcceComboBox(ccPanel,
                                         name = "ES.Theory.CC.Direct",
                                         label = "Integral Direct:",
                                         choices = self.ccDirectChoices,
                                         default = self.ccDirectDefault,
                                         export = 1)
            ccPanel.AddWidget(self.ccDirect)

            self.ccMenuChoices = ["ACCD", "CCD", "CCSD", "CCSD[T]", "CCSD(T)",
                                  "CCSDT-1A", "LCCD", "MP4"]
            self.ccMenuDefault = 4
            self.ccMenu = EcceComboBox(ccPanel,
                                       name = "ES.Theory.CC.CC",
                                       choices = self.ccMenuChoices,
                                       default = self.ccMenuDefault,
                                       export = 1)
            ccPanel.AddWidget(self.ccMenu)
            ccPanel.AddSpace()
            
            ccPanel.AddWidget(wx.StaticText(ccPanel, -1, "No. of Orbs.:"))
                              
            self.ccNFrz = EcceSpinCtrl(ccPanel,
                                       name = "ES.Theory.CC.FROZEN",
                                       label = "Frozen Core:",
                                       hardRange = "[0..)",
                                       default = 0) 
            ccPanel.AddWidget(self.ccNFrz)
            ccPanel.AddSpace()

            self.ccNOcc = EcceSpinCtrl(ccPanel,
                                       name = "ES.Theory.CC.OCCUPIED",
                                       label = "Occupied:",
                                       hardRange = "[0..)",
                                       default = 0)
            ccPanel.AddWidget(self.ccNOcc)
            ccPanel.AddSpace()
            
            self.ccFrzVirt = EcceSpinCtrl(ccPanel,
                                          name = "ES.Theory.CC.FRZVIRT",
                                          label = "Frozen Virtual:",
                                          hardRange = "[0..)",
                                          default = 0) 
            ccPanel.AddWidget(self.ccFrzVirt)
            
            ccPanel.AddWidget(wx.StaticText(ccPanel, -1, "Energy Convergence:"))
            
            self.ccHFConv = EcceFloatInput(ccPanel,
                                           name = "ES.Theory.CC.HF_Convergence",
                                           label = "HF:",
                                           default = 1e-10,
                                           hardRange = "[0..)")
            ccPanel.AddWidget(self.ccHFConv)
            ccPanel.AddSpace()

            self.ccCCConv = EcceFloatInput(ccPanel,
                                           name = "ES.Theory.CC.CC_Convergence",
                                           label = "CC:",
                                           default = 1e-8,
                                           hardRange = "[0..)")
            ccPanel.AddWidget(self.ccCCConv)

            self.ccPrint = EcceCheckBox(ccPanel,
                                        name = "ES.Theory.CC.DETAILED",
                                        label = "Detailed Output",
                                        default = False)
            ccPanel.AddWidget(self.ccPrint)

            ccPanel.SetAutoLayout(True)
            ccPanel.Fit()
            mainNB.AddPage(ccPanel, "HF + Coupled Cluster (-R12)")

        mainNB.SetAutoLayout(True)
        mainNB.Fit()
        miscPanel = EcceTabPanel(mainNB)
        
        self.diatomic = EcceCheckBox(miscPanel,
                                     name = "ES.Theory.MISC.DIATOM",
                                     label = "Diatomic",
                                     default = False)
        miscPanel.AddWidget(self.diatomic)
        
        self.bond = EcceCheckBox(miscPanel,
                                 name = "ES.Theory.MISC.BOND",
                                 label = "Bond Functions",
                                 default = False)
        miscPanel.AddWidget(self.bond)

        self.dist = EcceTextInput(miscPanel,
                                  name = "ES.Theory.MISC.DIST",
                                  label = "Loop over Distances:",
                                  size = (150, -1))

        miscPanel.AddWidget(self.dist)

        self.unitChoices = ["Bohr", "Angstrom"]
        self.unit = EcceComboBox(miscPanel,
                                 name = "ES.Theory.MISC.UNIT",
                                 label = "in",
                                 choices = self.unitChoices,
                                 default = 0)
        miscPanel.AddWidget(self.unit)

        self.memory = EcceSpinCtrl(miscPanel,
                                   name = "ES.Theory.MISC.MEMORY",
                                   label = "Max. Memory:",
                                   default = 1024,
                                   unit = "MBytes",
                                   hardRange = "[128..)")
        miscPanel.AddWidget(self.memory)

        self.noRun = EcceCheckBox(miscPanel,
                                  name = "ES.Theory.MISC.NORUN",
                                  label = "Run Calculation",
                                  default = True)
        miscPanel.AddWidget(self.noRun)
        
        miscPanel.SetAutoLayout(True)
        miscPanel.Fit()
        mainNB.AddPage(miscPanel, "Miscellaneous Options")

        self.panelSizer.Add(mainNB)


        self.AddButtons()
        
    def CheckDependency(self):
        if (EcceGlobals.Category[0:2]=="HF" and
            EcceGlobals.Category.find("CC")==-1):
            
            isOpenShell = (self.shell.GetValue() == "Open Shell")
            self.irrepNum.Enable(not isOpenShell)
            self.irrepLabel.Enable(isOpenShell)
            for irrepItem in self.irrepInput.values():
                irrepItem.Enable(isOpenShell)
                self.cplngLabel.Enable(isOpenShell)
                self.cplngA.Enable(isOpenShell)
                self.cplngB.Enable(isOpenShell)

        if (EcceGlobals.Category.find("MCSCF")!=-1 or
            EcceGlobals.Category.find("CI")!=-1):
            
            self.interact.Enable(not self.asym.GetValue())
            self.asym.Enable(not self.interact.GetValue())

        if (EcceGlobals.Category.find("CI")!=-1):
            if self.r12.GetValue():
                for each in self.line2:
                    each.Enable(True)
                self.ciR12Orb.Enable(True)
                self.ciR12Shell.Enable(True)
                self.ciProp.SetSelection(0)
                self.ciProp.Enable(False)
            else:
                for each in self.line2:
                    each.Enable(False)
                self.ciR12Orb.Enable(False)
                self.ciR12Shell.Enable(False)
                self.ciProp.Enable(True)
            self.ciNElec.Enable(self.acpf.GetValue() or
                                self.acpf2.GetValue() or
                                (self.r12.GetValue() and
                                 (self.acpfR12.GetValue() or
                                  self.acpf2R12.GetValue())))

        self.bond.Enable(self.diatomic.GetValue())
        self.dist.Enable(self.diatomic.GetValue())
        self.unit.Enable(self.diatomic.GetValue())

    def R12Clicked(self, event):
        if self.r12.GetValue():
            for each in self.line2:
                each.Enable(True)
            self.ciR12Orb.Enable(True)
            self.ciR12Shell.Enable(True)
            self.ciProp.SetSelection(0)
            self.ciProp.Enable(False)
        else:
            checkedCount1 = False
            for each in self.line1:
                if each.GetValue():
                    checkedCount1 = checkedCount1 + 1
            if checkedCount1:
                for each in self.line2:
                    each.Enable(False)
                self.ciR12Orb.Enable(False)
                self.ciR12Shell.Enable(False)
                self.ciProp.Enable(True)
            else:
                self.r12.SetValue(True)
                self.SetStatusText("Error: Need to check at least one box!",
                                   EcceGlobals.ERROR)
        self.ciNElec.Enable(self.acpf.GetValue() or
                            self.acpf2.GetValue() or
                            (self.r12.GetValue() and
                             (self.acpfR12.GetValue() or
                              self.acpf2R12.GetValue())))
        event.Skip()

    def ToggleClicked(self, event):
        if self.r12.GetValue():
            checkableSet = self.line2
        else:
            checkableSet = self.line1
        for each in checkableSet:
            if each.GetValue():
                break
        else:
            event.GetEventObject().SetValue(True)
            self.SetStatusText("Error: Need to check at least one box!",
                               EcceGlobals.ERROR)

        self.ciNElec.Enable(self.acpf.GetValue() or
                            self.acpf2.GetValue() or
                            (self.r12.GetValue() and
                             (self.acpfR12.GetValue() or
                              self.acpf2R12.GetValue())))
        event.Skip()

frame = AmicaTheoryFrame(None,
                         title = "ECCE Amica Editor: Theory Details",
                         app = app,
                         helpURL = "http://www.hec.utah.edu/gdanitz/amica/manual.pdf")
