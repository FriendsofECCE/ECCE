"""
Simplifies the creation of Ecce-styled wx GUI objects.
Also defines some helper functions dealing with ranges.
"""



import wx
import string
import webbrowser
import os
import math
from globals import *



def GetRangeBounds(range):
    """Based on a string representation of a numeric range, a tuple is returned
    giving the lower and upper limits of the range as numeric values.  If the
    range string has omitted either limit, that limit is returned as a None
    object."""
    #check for a blank string argument, which is allowed
    if not range: return None, None

    #parse range into its different components
    #once dived in half, slice out the number portions of the range
    try: minString, maxString = range.split("..")
    except ValueError: raise "BadRangeFormat"
    minString = minString[1:]   #removes [ or ( from front of min
    maxString = maxString[:-1]  #removes ] or ) from back of max

    #this function works the same whether the range is using float or
    #integer values
    lower = None
    upper = None
    try: lower = string.atoi(minString)
    except ValueError:
        try: lower = string.atof(minString)
        except ValueError: lower = None
    try: upper = string.atoi(maxString)
    except ValueError:
        try: upper = string.atof(maxString)
        except ValueError: upper = None
    return lower, upper



def InRange(range, value):
    """Based on a string representation of a numeric range, value is checked to
    make sure it falls within range.  The range is expected to be in the format
    [lower..upper] where lower or upper can be integers, floats, or left out
    entirely.  The brackets can also be replaced with parenthesis to represent
    an open set instead of the closed set."""

    #need to preset some variables so that they are in scope for later use
    ret = True
    temp_val = None
    maxVal = None
    minVal = None

    if EcceGlobals.DebugFlag == "DebugOn":
        print "Value in check range ", value

    #clear message
    EcceGlobals.Message = ""

    #this function works the same whether the range/value is using float or
    #integer values
    try:
        temp_val = string.atoi(str(value))
    except ValueError:
        try:
            temp_val = string.atof(str(value))
        except ValueError:
            raise "BadRangeValue"

    minVal, maxVal = GetRangeBounds(range)

    if minVal <> None:
        if range[0] == "(":
            if temp_val <= minVal:
                EcceGlobals.Message = "greater than %s." % str(minVal)
                ret = False
        elif range[0] == "[":
            if temp_val < minVal:
                EcceGlobals.Message = "greater than or equal to %s."%str(minVal)
                ret = False
        else: raise "BadRangeFormat"

    if maxVal <> None:
        if range[-1] == ")":
            if temp_val >= maxVal:
                EcceGlobals.Message = "less than %s." % str(maxVal)
                ret = False
        elif range[-1] == "]":
            if temp_val > maxVal:
                EcceGlobals.Message = "less than or equal to %s." % str(maxVal)
                ret = False
        else: raise "BadRangeFormat"

    return ret




class EcceFrame(wx.Frame):
    """Ecce styled wxFrame."""

    def __init__(self, parent, title, id=-1):
        wx.Frame.__init__(self, parent, id, title,
                          style = wx.DEFAULT_FRAME_STYLE &
                          ~(wx.RESIZE_BORDER|wx.RESIZE_BOX|wx.MAXIMIZE_BOX))

        self.SetFont(EcceGlobals.FontDefault)
        wx.ToolTip.Enable(EcceGlobals.ShowTip)


    def Finalize(self):
        if EcceGlobals.RestoreFlag != "NO_GUIValues":
            self.Fit()
            self.Show(True)
            app.MainLoop()
        else:
            self.Destroy()



class EccePanelSizer(wx.BoxSizer):
    """Ecce styled wxBoxSizer for use as the main sizer of an EccePanel."""

    def __init__(self):
        wx.BoxSizer.__init__(self, wx.VERTICAL)


    def Add(self, window, flag = EcceGlobals.FlagDefault|wx.EXPAND,
            border = 1):
        wx.BoxSizer.Add(self, window, 0, flag, border)



class EcceLineSeparator(wx.BoxSizer):
    """Ecce styled line separator."""
    
    def __init__(self, parent):
        wx.BoxSizer.__init__(self, wx.HORIZONTAL)
        self.Add(wx.StaticLine(parent),
                 proportion = 1,
                 flag = wx.ALIGN_CENTER_VERTICAL|wx.TOP|wx.BOTTOM,
                 border = EcceGlobals.BorderDefault)



class EcceLineLabelSeparator(wx.BoxSizer):
    """Ecce styled line separator with a label."""

    def __init__(self, parent, label):
        wx.BoxSizer.__init__(self, wx.HORIZONTAL)
        self.Add(wx.StaticLine(parent, 
                               size = (10, wx.StaticLine_GetDefaultSize())),
                 proportion = 0,
                 flag = EcceGlobals.FlagDefault,
                 border = 0)
        self.Add(wx.StaticText(parent, label = label),
                 proportion = 0,
                 flag = EcceGlobals.FlagDefault,
                 border = EcceGlobals.BorderDefault)
        self.Add(wx.StaticLine(parent),
                 proportion = 1, #stretch to fill rest of horz. space
                 flag = EcceGlobals.FlagDefault|wx.EXPAND,
                 border = 0)



class EcceSpacer(wx.Size):
    """Ecce styled size."""

    def __init__(self, parent, width = 0, height = 0):
        wx.Size.__init__(self, width, height)



class EcceBoxSizer(wx.BoxSizer):
    """
    The style of EcceBoxSizer is defined as this:
    0       show line separator with the label in it.
    1       do not show line separator nor the label
    2       show a bounding box with label
    """

    def __init__(self, parent, label="", cols=1, style = 0):
        wx.BoxSizer.__init__(self, wx.VERTICAL)

        if (style == 0):
            titleBarSizer = wx.BoxSizer(wx.HORIZONTAL)
            if label == "":
                titleBarSizer.Add(wx.StaticLine(parent),
                                  proportion = 1)
            else:
                titleBarSizer.Add(wx.StaticLine(parent, size = (10, -1)),
                                  proportion = 0,
                                  flag = wx.ALIGN_CENTER,
                                  border = 0)
                titleBarSizer.Add(wx.StaticText(parent, label = label),
                                  proportion = 0,
                                  flag = wx.ALIGN_CENTER|wx.ALL,
                                  border = 2)
                titleBarSizer.Add(wx.StaticLine(parent),
                                  proportion = 1,
                                  flag = wx.ALIGN_CENTER,
                                  border = 0)

            wx.BoxSizer.Add(self, titleBarSizer, 0, wx.EXPAND, 0)
                
        if (style == 0 or style == 1):
            self.gridSizer = wx.FlexGridSizer(rows = 0, cols = cols, vgap = 0,
                                              hgap = EcceGlobals.BorderDefault)
            wx.BoxSizer.Add(self, self.gridSizer, proportion = 1,
                            flag = EcceGlobals.FlagDefault|wx.EXPAND,
                            border = 0)
        elif (style == 2):
            staticBox = wx.StaticBox(parent, id = -1, label = label)
            boxSizer = wx.StaticBoxSizer(staticBox, wx.VERTICAL)
            self.gridSizer = wx.FlexGridSizer(rows = 0, cols = cols, vgap = 0,
                                              hgap = EcceGlobals.BorderDefault)
            boxSizer.Add(self.gridSizer, proportion = 1,
                         flag = EcceGlobals.FlagDefault|wx.EXPAND, border = 0)
            wx.BoxSizer.Add(self, boxSizer, proportion = 1, flag = wx.EXPAND,
                            border = 0)


    def AddWidget(self, widget, flag=EcceGlobals.FlagDefault,
                  border=EcceGlobals.BorderDefault):
        self.gridSizer.Add(widget, proportion = 0, flag = flag, border = border)


    def AddSpace(self):
        self.gridSizer.Add(item = (0, 0), proportion = 0,
                           flag = EcceGlobals.FlagDefault,
                           border = EcceGlobals.BorderDefault)



class EcceHBoxSizer(wx.BoxSizer):
    """Ecce styled wxBoxSizer oriented horizontal."""

    def __init__(self):
        wx.BoxSizer.__init__(self, wx.HORIZONTAL)


    def AddWidget(self, widget, flag = EcceGlobals.FlagDefault, 
                  border = EcceGlobals.BorderDefault):
        wx.BoxSizer.Add(self, widget, proportion = 0, flag = flag, 
                        border = border)



class EcceVBoxSizer(wx.BoxSizer):
    """Ecce styled wxBoxSizer oriented vertical."""

    def __init__(self):
        wx.BoxSizer.__init__(self, wx.VERTICAL)


    def AddWidget(self, widget, flag = EcceGlobals.FlagDefault, 
                  border = EcceGlobals.BorderDefault):
        wx.BoxSizer.Add(self, widget, proportion = 0, flag = flag, 
                        border = border)



class EcceLineLabelHBoxSizer(wx.StaticBoxSizer):
    """Ecce styled wxBoxSizer with border and label oriented horizontally."""

    def __init__(self, parent, label = ""):
        staticBox = wx.StaticBox(parent, label = label)
        wx.StaticBoxSizer.__init__(self, staticBox, wx.HORIZONTAL)


    def AddWidget(self, widget, flag = EcceGlobals.FlagDefault, 
                  border = EcceGlobals.BorderDefault):
        wx.BoxSizer.Add(self, widget, proportion = 0, flag = flag, 
                        border = border)



class EcceLineLabelVBoxSizer(wx.StaticBoxSizer):
    """Ecce styled wxBoxSizer with border and label oriented horizontally."""

    def __init__(self, parent, label = ""):
        staticBox = wx.StaticBox(parent, label = label)
        wx.StaticBoxSizer.__init__(self, staticBox, wx.VERTICAL)


    def AddWidget(self, widget, flag = EcceGlobals.FlagDefault, 
                  border = EcceGlobals.BorderDefault):
        wx.BoxSizer.Add(self, widget, proportion = 0, flag = flag, 
                        border = border)


class EcceSubPanel(wx.Panel):
    """Ecce styled wxPanel for use within dialogs."""

    def __init__(self, parent):
        wx.Panel.__init__(self, parent, id = -1, style = wx.TAB_TRAVERSAL)
        self.SetFont(EcceGlobals.FontDefault)
        self.SetBackgroundColour(EcceGlobals.BackgroundColour)
        self.Settings = parent.Settings
        self.CheckButtonEnables = parent.CheckButtonEnables
        self.OnChanges = parent.OnChanges
        self.SetStatusText = parent.SetStatusText


class EccePanel(wx.Panel):
    """Ecce styled top-level wxPanel."""

    def __init__(self, parent, helpURL=""):
        wx.Panel.__init__(self, parent, id = -1, style = wx.TAB_TRAVERSAL)
        self.helpURL = helpURL
        self.panelSizer = EccePanelSizer()
        self.SetSizer(self.panelSizer)
        self.SetFont(EcceGlobals.FontDefault)
        self.SetBackgroundColour(EcceGlobals.BackgroundColour)
        self.Settings = []

        parent.Bind(wx.EVT_CLOSE, self.OnClose)
        parent.Bind(wx.EVT_SHOW, self.OnShow)


    def OnShow(self, event):
        if event.GetShow():
            self.CheckDependency()
            self.SetFocus()


    def AddButtons(self):
        # message box
        self.msg = wx.TextCtrl(self, size=wx.Size(-1, 65),
                               style = wx.TE_MULTILINE|wx.TE_WORDWRAP|
                               wx.TE_READONLY)
        self.msg.SetBackgroundColour(EcceGlobals.ReadonlyColour)
        self.colorTimer = wx.Timer(self.msg, 1)
        self.messageTimer = wx.Timer(self.msg, 2)
        self.msg.Bind(wx.EVT_TIMER, self.OnColorTimer, self.colorTimer, 1)
        self.msg.Bind(wx.EVT_TIMER, self.OnMessageTimer, self.messageTimer, 2)
        self.panelSizer.Add(self.msg)
        
        # buttons
        buttonSizer = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer.Add(item = (0,0), proportion =EcceGlobals.ProportionDefault,
                        flag = EcceGlobals.FlagDefault, border = 0)

        self.closeButton = wx.Button(self, label = "Close")
        self.closeButton.SetToolTip(wx.ToolTip("close window"))
        self.Bind(wx.EVT_BUTTON, self.OnClose, self.closeButton)

        if wx.Platform == '__WXMSW__':
            buttonSizer.Add(self.closeButton, proportion = 0,
                            flag = wx.ALL, border = EcceGlobals.BorderDefault)
        else:
            buttonSizer.Add(self.closeButton, proportion = 0, flag = wx.TOP,
                            border = EcceGlobals.BorderDefault)
            buttonSizer.Add((0,0), proportion = EcceGlobals.ProportionDefault,
                            flag = EcceGlobals.FlagDefault, border = 0)

        self.resetButton = wx.Button(self, label = "Reset")
        self.resetButton.SetToolTip(wx.ToolTip("Restore default settings"))
        self.Bind(wx.EVT_BUTTON, self.OnResetButton, self.resetButton)

        if wx.Platform == '__WXMSW__':
            buttonSizer.Add(self.resetButton, proportion = 0, flag = wx.ALL,
                            border = EcceGlobals.BorderDefault)
        else:
            buttonSizer.Add(self.resetButton, proportion = 0, flag = wx.TOP,
                            border = EcceGlobals.BorderDefault)
            buttonSizer.Add((0,0), proportion = EcceGlobals.ProportionDefault,
                            flag = EcceGlobals.FlagDefault, border = 0)
        
        if self.helpURL:
            self.helpButton = wx.Button(self, label = "Help")
            self.Bind(wx.EVT_BUTTON, self.OnHelpButton, self.helpButton)
            if wx.Platform == '__WXMSW__':
                buttonSizer.Add(self.helpButton, proportion = 0, flag = wx.ALL,
                                border = EcceGlobals.BorderDefault)
            else:
                buttonSizer.Add(self.helpButton, proportion = 0, flag = wx.TOP,
                                border = EcceGlobals.BorderDefault)
                buttonSizer.Add((0,0), 
                                proportion = EcceGlobals.ProportionDefault,
                                flag = EcceGlobals.FlagDefault, border = 0)
            
        self.panelSizer.Add(buttonSizer, border = 0)
        self.FinalizeSetting()


    def OnChanges(self, event):
        self.CheckDependency()
        if EcceGlobals.ReadOnlyFlag == "Writable":
            if EcceGlobals.PortOut != 0:
                widget = event.GetEventObject()
                if EcceGlobals.RestoreFlag == "NO_GUIValues":
                  EcceGlobals.BatchValues.append(widget.ExportSetting()+"\n")
                else:
                  EcceGlobals.Socket.send(widget.ExportSetting()+"\n")
                self.CheckButtonEnables()
                event.GetEventObject().Save()
            else:
                self.SetStatusText("Calculation is read only.  Changes cannot "
                                   "be saved!", EcceGlobals.WARNING)
        event.Skip()


    def OnClose(self, event):
        if (EcceGlobals.ReadOnlyFlag == "Writable"):
            if EcceGlobals.PortOut != 0:
                EcceGlobals.Socket.send("#CLOSING\n")
        if EcceGlobals.PortOut != 0:
            EcceGlobals.Socket.close()
        self.GetParent().Destroy()


    def OnResetButton(self, event):
        msg = wx.MessageDialog(self, "Hitting OK will set all parameters to "
                              "their default values.\nDo you want to continue?",
                              "Reset Defaults?", 
                              wx.OK|wx.CANCEL|wx.ICON_QUESTION|wx.STAY_ON_TOP)
        msg.SetFont(EcceGlobals.FontDefault)
        if msg.ShowModal() == wx.ID_OK:
            for setting in self.Settings:
                setting.Reset()
            self.CheckButtonEnables()
            self.CheckDependency()
        event.Skip()


    def OnHelpButton(self, event):
        webbrowser.open(self.helpURL)
        event.Skip()


    def OnColorTimer(self, event):
        self.msg.SetBackgroundColour(EcceGlobals.ReadonlyColour)
        event.Skip()


    def OnMessageTimer(self, event):
        self.msg.SetValue("")
        event.Skip()


    def CheckButtonEnables(self):
        if not EcceGlobals.EventLoopStarted:
            return

        for setting in self.Settings:
            if setting.NotDefault():
                self.resetButton.Enable(True)
                break
        else:
            self.resetButton.Enable(False)


    def SetStatusText(self, message, severity = EcceGlobals.NORMAL):
        self.msg.SetValue(message)
        if severity in [EcceGlobals.WARNING, EcceGlobals.ERROR]:
            if severity == EcceGlobals.WARNING:
                self.msg.SetBackgroundColour(EcceGlobals.WarningColour)
            elif severity == EcceGlobals.ERROR:
                self.msg.SetBackgroundColour(EcceGlobals.ErrorColour)
            self.colorTimer.Start(EcceGlobals.ColorTimerLength, True)
        self.messageTimer.Start(EcceGlobals.MessageTimerLength, True)


    def FinalizeSetting(self):
        if (EcceGlobals.RestoreFlag == "GUIValues"):
            f = open(EcceGlobals.FdIn, "r")
            line = f.readline()
            while line:
                name = ""
                value = ""
                unit = ""
                active = ""
                requiredOnExport = ""
                valType = ""
                i = 0
                j = 0
                while i < len(line)-1:
                    if line[i] != "|":
                        if j == 0:
                            name = name + line[i]
                        elif j == 1:
                            value = value + line[i]
                        elif j == 2:
                            unit = unit + line[i]
                        elif j == 3:
                            active = active + line[i]
                        elif j == 4:
                            requiredOnExport = requiredOnExport + line[i]
                        elif j == 5:
                            valType = valType + line[i]
                    else:
                        j = j + 1
                    i = i + 1

                # line has been parsed, assign values (if line is intelligable)
                if j == 5:
                    for setting in self.Settings:
                        if (setting.GetName() == name):
                            if valType == "integer_input":
                                setting.SetValue(int(value))
                                setting.SetUnit(unit)
                            elif valType == "float_input" or \
                                 valType == "exponential_input":
                                setting.SetValue(value)
                                setting.SetUnit(unit)
                            elif valType == "toggle_input":
                                setting.SetValue(value == "1")
                            elif valType == "text_input":
                                setting.SetValue(value)
                            elif valType == "menu_input":
                                try:
                                    setting.SetValue(value)
                                except wx.core.PyAssertionError:
                                    ()
                            setting.Enable(active == "1")
                line = f.readline()
        # Save result
        for setting in self.Settings:
            setting.Save()
            if EcceGlobals.PortOut != 0:
                if EcceGlobals.RestoreFlag == "NO_GUIValues":
                  EcceGlobals.BatchValues.append(setting.ExportSetting()+"\n")
                else:
                  EcceGlobals.Socket.send(setting.ExportSetting()+"\n")

        # Need to trigger dependencies so they are completed before the
        # initialized message.  Otherwise they can make it look like the
        # user made a change (enabling the save flag in the caller)
        # when it was really part of the initialization.
        self.CheckDependency()

        if EcceGlobals.PortOut != 0:
          if EcceGlobals.RestoreFlag == "NO_GUIValues":
            EcceGlobals.BatchValues.append("#INITIALIZED\n")
          else:
            EcceGlobals.Socket.send("#INITIALIZED\n")

        if EcceGlobals.RestoreFlag == "NO_GUIValues":
            # Send the batched list of values over the socket
            EcceGlobals.Socket.send(''.join(EcceGlobals.BatchValues))
            self.OnClose(None)
        else:
            EcceGlobals.EventLoopStarted = True
            self.CheckButtonEnables()
            self.SetAutoLayout(True)
            self.Fit()
            


class EcceCheckBox(wx.CheckBox):
    """Ecce styled wxCheckBox."""

    def __init__(self, parent, label="", id=-1, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, style=0, validator=wx.DefaultValidator,
                 name="checkBox", default=True, export=0, tip=""):
        wx.CheckBox.__init__(self, parent, id, label, pos, size,
                             style, validator, name)
        self.parent = parent

        if not tip and EcceGlobals.NameAsTip:
            tip = name
        if tip:
            self.SetToolTip(wx.ToolTip(tip))

        self.SetBackgroundColour(EcceGlobals.InputColour)
        self.Bind(wx.EVT_CHECKBOX, self.OnChanges)

        self.save = self.default = default
        self.SetValue(default)
        self.parent.Settings.append(self)
        self.export = export


    def GetDefault():
        return self.default


    def SetDefault(default):
        self.default = default


    def GetExport():
        return self.export


    def SetExport(export):
        self.export = export

        
    def Reset(self):
        if self.NotDefault:
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
        if EcceGlobals.AutoNavigation:
            if self.parent.FindFocus():
                self.parent.FindFocus().Navigate()


    def ExportSetting(self):
        needExport = self.export
        if (self.NotDefault()):
            needExport = 1
        return self.GetName() + "|" + str(int(self.GetValue())) + "||" + \
               str(int(self.IsEnabled())) + "|" + str(needExport) + \
               "|toggle_input"



class EcceFloatInput(wx.BoxSizer):
    """Ecce styled wxTextControl with a label that takes float values."""

    def __init__(self, parent, value="", id=-1, pos=wx.DefaultPosition,
                 size=(80, -1), style=wx.TE_PROCESS_ENTER,
                 validator=wx.DefaultValidator,
                 name="floatInput", default=0.0, label="", unit="",
                 softRange="", hardRange="", export=0, tip=""):
        wx.BoxSizer.__init__(self, wx.HORIZONTAL)
        self.text = wx.TextCtrl(parent, id, value, pos, size, style,
                                validator, name)
        self.parent = parent

        if not tip and EcceGlobals.NameAsTip:
            tip = name
        if tip:
            self.text.SetToolTip(wx.ToolTip(tip))

        self.SetBackgroundColour(EcceGlobals.InputColour)
        self.SetValue(str(default))
        self.save = self.default = default

        self.isInvalid = False
        self.softRange = softRange
        self.hardRange = hardRange
        self.label=0
        self.export = export
        if label:
            self.label = wx.StaticText(parent, -1, label+"   ")
            wx.BoxSizer.Add(self, self.label, 0, EcceGlobals.FlagDefault, 0)
        wx.BoxSizer.Add(self, self.text, 0, EcceGlobals.FlagDefault, 0)

        self.unitString = unit
        self.rangeString = ""
        if self.softRange:
            self.rangeString = self.softRange
        elif self.hardRange:
            self.rangeString = self.hardRange

        self.unitRange = 0
        if self.unitString or self.rangeString:
            self.unitRange = wx.StaticText(parent, -1, "  "+self.unitString
                                           + "\n   " + self.rangeString)
            self.unitRange.SetFont(EcceGlobals.ScriptFontDefault)
            wx.BoxSizer.Add(self, self.unitRange, 0, EcceGlobals.FlagDefault, 0)

        self.parent.Settings.append(self)

        self.timer = wx.Timer(self.text)
        self.text.Bind(wx.EVT_CHAR, self.OnChar)
        self.text.Bind(wx.EVT_KILL_FOCUS, self.OnKillFocus)
        self.text.Bind(wx.EVT_TIMER, self.OnTimer)
        self.text.Bind(wx.EVT_ENTER_WINDOW, self.OnMouseEnter)
        self.text.Bind(wx.EVT_LEAVE_WINDOW, self.OnMouseLeave)


    def Reset(self):
        if self.NotDefault:
            self.SetValue(self.default)
            e = wx.CommandEvent()
            e.SetEventObject(self)
            self.parent.OnChanges(e)
            self.isInvalid = False
        

    def Save(self):
        try:
            value = float(self.GetValue())
            self.save = value
        except ValueError:
            if EcceGlobals.EnableDebug:
                print "ValueError in Save"


    def NotDefault(self):
        try:
            value = float(self.GetValue())
            return self.default != value
        except ValueError:
            if EcceGlobals.EnableDebug:
                print "ValueError in Function NotDefault"
            return True


    def NotSaved(self):
        try:
            value = float(self.GetValue())
            return self.save != value
        except ValueError:
            if EcceGlobals.EnableDebug:
                print "ValueError in Function NotSaved"
            return False


    def Enable(self, enable=True):
        if self.label:
            self.label.Enable(enable)
        if self.unitRange:
            self.unitRange.Enable(enable)
        if self.text.Enable(enable):
          # was not in the state specified (i.e. work was done)
          e = wx.CommandEvent()
          e.SetEventObject(self)
          self.parent.OnChanges(e)
        if enable:
            self.SetBackgroundColour(EcceGlobals.InputColour)
        else:
            self.SetBackgroundColour(EcceGlobals.ReadonlyColour)
        

    def FloatOnly(self, key):
        return (key < wx.WXK_SPACE 
                or key == wx.WXK_DELETE 
                or key > 255 
                or chr(key).isdigit() 
                or chr(key) in "Ee" 
                and "E" not in self.GetValue() 
                and "e" not in self.GetValue() 
                or chr(key) == "." 
                and "." not in self.GetValue() 
                or chr(key) in "+-" 
                and (self.GetValue() == "" 
                     or self.GetValue().endswith("e") 
                     or self.GetValue().endswith("E") 
                     or self.text.GetInsertionPoint() == 0))


    def FloatTest(self):
        try:
            value = float(self.GetValue())
            self.SetValue(str(value))
            self.parent.CheckButtonEnables()
            if not InRange(self.hardRange, value):
                self.parent.SetStatusText("Error: Value outside allowed range--"
                                          "must be " + EcceGlobals.Message,
                                          EcceGlobals.ERROR)
                self.SetValue(str(self.save))
                self.isInvalid = True
                self.SetFocus()
            elif not InRange(self.softRange, value):
                self.parent.SetStatusText("Warning: Value outside recommended "
                                          "range--typically " + 
                                          EcceGlobals.Message,
                                          EcceGlobals.WARNING)
                self.isInvalid = False
            else:
                self.isInvalid = False
        except ValueError:
            self.parent.SetStatusText("Error: %s is not a valid floating point "
                                      "number!" % self.GetValue(),
                                      EcceGlobals.ERROR)
            self.SetValue(str(self.save))
            self.isInvalid = True
            self.SetFocus()
        except OverflowError:
            self.parent.SetStatusText("Error: Input is out of range",
                                      EcceGlobals.ERROR)
            self.SetValue(str(self.save))
            self.isInvalid = True
            self.SetFocus()
        return self.isInvalid == False and value != self.save


    def OnChar(self, event):
        key = event.GetKeyCode()
        if (self.FloatOnly(key)):
            if EcceGlobals.TimerValidation:
                self.timer.Start(EcceGlobals.TimerLength, True)
            event.Skip()


    def OnKillFocus(self, event):
        self.timer.Stop()
        if (self.FloatTest()):
            self.OnChanges(event)
        event.Skip()


    def OnMouseEnter(self, event):
        self.timer.Stop()
        self.text.SetEditable(True)
        self.SetFocus()
        event.Skip()


    def OnMouseLeave(self, event):
        if EcceGlobals.TimerValidation:
            self.timer.Start(EcceGlobals.TimerLength, True)
        else:
            self.OnKillFocus(event)
        event.Skip()


    def OnTimer(self, event):
        if (self.FloatTest()):
            self.OnChanges(event)
        event.Skip()


    def OnChanges(self, event):
        event.SetEventObject(self)
        self.parent.OnChanges(event)
        if EcceGlobals.AutoNavigation:
            if self.parent.FindFocus():
                self.parent.FindFocus().Navigate()
        else:
            self.text.SetEditable(False)


    def GetValue(self):
        return self.text.GetValue()
    

    def SetValue(self, val):
        return self.text.SetValue(str(val))


    def GetDefault():
        return self.default


    def SetDefault(default):
        self.default = default
        

    def GetExport():
        return self.export


    def SetExport(export):
        self.export = export


    def GetLabel():
        if self.label:
            return self.label.GetLabel()
        else:
            return ""


    def SetLabel(label):
        if label:
            if self.label:
                self.label.SetLabel(label)
            else:
                self.label = wx.StaticText(self.parent, -1, label+"   ")
                wx.BoxSizer.Insert(self, 0, self.label, 0, EcceGlobals.FlagDefault, 0)
        else:
            if self.label:
                wx.BoxSizer.Remove(self, 0)
                self.label = 0


    def GetName(self):
        return self.text.GetName()


    def SetBackgroundColour(self, color):
        self.text.SetBackgroundColour(color)


    def SetFocus(self):
        self.text.SetSelection(0, self.text.GetLastPosition())
        self.text.SetFocus()
        self.text.SetSelection(0, self.text.GetLastPosition())


    def SetUnit(self, unit):
        self.unitString = unit
        if self.unitString or self.rangeString:
            self.unitRange.SetLabel("  " + self.unitString + "\n   " +
                                    self.rangeString)


    def ExportSetting(self):
        needExport = self.export
        if (self.NotDefault()):
            needExport = 1
        return self.text.GetName() + "|" + str(self.text.GetValue()) + "|" + \
               self.unitString + "|" + str(int(self.text.IsEnabled())) + "|" +\
               str(needExport) + "|float_input"


class EcceExpInput(EcceFloatInput):
    def FloatTest(self):
        if not EcceFloatInput.FloatTest(self):
            return False
        value = float(self.GetValue())
        try:
            power = math.log10(value)
            if power == int(power):
                return True
            self.parent.SetStatusText("Error: Require a number that is exactly power of 10",
                                      EcceGlobals.ERROR)
        except OverflowError:
            self.parent.SetStatusText("Error: Input is out of range",
                                      EcceGlobals.ERROR)
        self.SetValue(str(self.save))
        self.isInvalid = True
        self.SetFocus()
        return False



class EcceComboBox(wx.BoxSizer):
    """Ecce style wxComboBox."""

    def __init__(self, parent, id=-1, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, choices=[], style=0,
                 validator=wx.DefaultValidator, name = "choiceBox", default=0, 
                 label="", export=0, tip=""):
        wx.BoxSizer.__init__(self, wx.HORIZONTAL)
        self.parent = parent
        self.combo = wx.Choice(parent, id, pos, size, choices,
                               style, validator, name)

        if not tip and EcceGlobals.NameAsTip:
            tip = name
        if tip:
            self.combo.SetToolTip(wx.ToolTip(tip))
        self.SetBackgroundColour(EcceGlobals.InputColour)
#        self.combo.SetSize((5*max(map(len, choices))+40, -1))

        self.SetSelection(default)
        self.save = self.default = default
        self.label = 0
        self.export = export
        if label:
            self.label = wx.StaticText(parent, -1, label+"   ")
            wx.BoxSizer.Add(self, self.label, 0, EcceGlobals.FlagDefault, 0)
        wx.BoxSizer.Add(self, self.combo, 0, EcceGlobals.FlagDefault, 0)
        self.parent.Settings.append(self)
        
        self.combo.Bind(wx.EVT_CHOICE, self.OnChanges)


    def OnChanges(self, event):
        event.SetEventObject(self)
        self.parent.OnChanges(event)
        if EcceGlobals.AutoNavigation:
            if self.parent.FindFocus():
                self.parent.FindFocus().Navigate()


    def Reset(self):
        if self.NotDefault:
            self.SetSelection(self.default)
            e = wx.CommandEvent()
            e.SetEventObject(self)
            self.parent.OnChanges(e)
        

    def Save(self):
        self.save = self.GetSelection()


    def NotDefault(self):
        return self.default != self.GetSelection()


    def NotSaved(self):
        return self.save != self.GetSelection()


    def Enable(self, enable=True):
        if self.label:
            self.label.Enable(enable)
        if self.combo.Enable(enable):
          # was not in the state specified (i.e. work was done)
          e = wx.CommandEvent()
          e.SetEventObject(self)
          self.parent.OnChanges(e)
        if enable:
            self.SetBackgroundColour(EcceGlobals.InputColour)
        else:
            self.SetBackgroundColour(EcceGlobals.ReadonlyColour)


    def GetSelection(self):
        return self.combo.GetSelection()


    def SetSelection(self, sel):
        self.combo.SetSelection(sel)


    def GetValue(self):
        return self.combo.GetStringSelection()


    def SetValue(self, val):
        self.combo.SetStringSelection(val)


    def GetDefault():
        return self.default


    def SetDefault(default):
        self.default = default
        

    def GetExport():
        return self.export


    def SetExport(export):
        self.export = export


    def GetLabel():
        if self.label:
            return self.label.GetLabel()
        else:
            return ""


    def SetLabel(label):
        if label:
            if self.label:
                self.label.SetLabel(label)
            else:
                self.label = wx.StaticText(self.parent, -1, label+"   ")
                wx.BoxSizer.Insert(self, 0, self.label, 0, EcceGlobals.FlagDefault, 0)
        else:
            if self.label:
                wx.BoxSizer.Remove(self, 0)
                self.label = 0


    def GetName(self):
        return self.combo.GetName()


    def SetBackgroundColour(self, color):
        self.combo.SetBackgroundColour(color)


    def SetFocus(self):
        self.combo.SetFocus()


    def ExportSetting(self):
        needExport = self.export
        if (self.NotDefault()):
            needExport = 1
        return self.combo.GetName() + "|" + \
               str(self.combo.GetStringSelection()) + "||" +\
               str(int(self.combo.IsEnabled())) + "|" +  str(needExport) + \
               "|menu_input"



class EcceSpinCtrl(wx.BoxSizer):
    """Ecce style wxSpinControl with a label."""

    def __init__(self, parent, value="", id=-1, pos=wx.DefaultPosition,
                 size=(80, -1), style=wx.SP_ARROW_KEYS, unit = "",
                 initial=0, name="spinCtrl", default=0, label = "",
                 softRange="", hardRange="", export=0, tip=""):
        wx.BoxSizer.__init__(self, wx.HORIZONTAL)

        #Using SetRange was causing the control to take focus at startup
        self.softRange = softRange
        self.hardRange = hardRange
        lower, upper = GetRangeBounds(self.hardRange)
        if lower == None: lower = EcceGlobals.MinIntDefault
        if upper == None: upper = EcceGlobals.MaxIntDefault

        #now initialize the base class
        self.spin=wx.SpinCtrl(parent, id, value, pos, size, style,
                              lower, upper, initial, name)
        self.parent = parent

        if not tip and EcceGlobals.NameAsTip:
            tip = name
        if tip:
            self.spin.SetToolTip(wx.ToolTip(tip))

        self.SetBackgroundColour(EcceGlobals.InputColour)
        self.SetValue(default)
        self.save = self.default = default

        self.isInvalid = False
        self.label = 0
        self.export = export
        if label:
            self.label = wx.StaticText(parent, -1, label+"   ")
            wx.BoxSizer.Add(self, self.label, 0, EcceGlobals.FlagDefault, 0)
        wx.BoxSizer.Add(self, self.spin, 0, EcceGlobals.FlagDefault, 0)
        self.unitString = unit
        self.rangeString = ""
        if self.softRange:
            self.rangeString = self.softRange
        elif self.hardRange:
            self.rangeString = self.hardRange
        self.unitRange = 0
        if self.unitString or self.rangeString:
            self.unitRange = wx.StaticText(parent, -1, "  "+self.unitString
                                           + "\n   " + self.rangeString)
            self.unitRange.SetFont(EcceGlobals.ScriptFontDefault)
            wx.BoxSizer.Add(self, self.unitRange, 0, EcceGlobals.FlagDefault, 0)
        self.parent.Settings.append(self)

        self.timer = wx.Timer(self.spin)
        self.spin.Bind(wx.EVT_CHAR, self.OnChar)
        self.spin.Bind(wx.EVT_KILL_FOCUS, self.OnKillFocus)
        self.spin.Bind(wx.EVT_TIMER, self.OnTimer)
        self.spin.Bind(wx.EVT_ENTER_WINDOW, self.OnMouseEnter)
        self.spin.Bind(wx.EVT_LEAVE_WINDOW, self.OnMouseLeave)
        self.spin.Bind(wx.EVT_SPINCTRL, self.OnSpin)


    def IntTest(self):
        value = self.GetValue()
        if not InRange(self.softRange, value):
            self.parent.SetStatusText("Warning: Value outside recommended "
                                      "range " + self.softRange + "!", 
                                      EcceGlobals.WARNING)
            self.isInvalid = False
        else:
            self.isInvalid = False
        return self.isInvalid == False and value != self.save


    def OnChar(self, event):
        if EcceGlobals.TimerValidation:
            self.timer.Start(EcceGlobals.TimerLength, True)
        event.Skip()


    def OnKillFocus(self, event):
        self.timer.Stop()
        if self.IntTest():
            self.OnChanges(event)
        event.Skip()


    def OnMouseEnter(self, event):
        self.timer.Stop()
        self.SetFocus()
        event.Skip()


    def OnMouseLeave(self, event):
        if EcceGlobals.TimerValidation:
            self.timer.Start(EcceGlobals.TimerLength, True)
        else:
            self.OnKillFocus(event)
        event.Skip()
        

    def OnTimer(self, event):
        if self.IntTest():
            self.OnChanges(event)
        event.Skip()


    def OnSpin(self, event):
        if EcceGlobals.TimerValidation:
            self.timer.Start(EcceGlobals.TimerLength, True)
        event.Skip()


    def OnChanges(self, event):
        event.SetEventObject(self)
        self.parent.OnChanges(event)
        if EcceGlobals.AutoNavigation:
            if self.parent.FindFocus():
                self.parent.FindFocus().Navigate()
        event.Skip()


    def Reset(self):
        if self.NotDefault:
            self.SetValue(self.default)
            e = wx.CommandEvent()
            e.SetEventObject(self)
            self.parent.OnChanges(e)
            self.isInvalid = False
        

    def Save(self):
        self.save = self.GetValue()


    def NotDefault(self):
        return self.default != self.GetValue()


    def NotSaved(self):
        return self.save != self.GetValue()


    def Enable(self, enable=True):
        if self.label:
            self.label.Enable(enable)
        if self.unitRange:
            self.unitRange.Enable(enable)
        if self.spin.Enable(enable):
          # was not in the state specified (i.e. work was done)
          e = wx.CommandEvent()
          e.SetEventObject(self)
          self.parent.OnChanges(e)
        if enable:
            self.SetBackgroundColour(EcceGlobals.InputColour)
        else:
            self.SetBackgroundColour(EcceGlobals.ReadonlyColour)


    def SetFocus(self):
        self.spin.SetSelection(0, len(str(self.spin.GetValue())))
        self.spin.SetFocus()


    def GetValue(self):
        return self.spin.GetValue()


    def SetValue(self, val):
        self.spin.SetValue(val)


    def GetDefault():
        return self.default


    def SetDefault(default):
        self.default = default
        

    def GetExport():
        return self.export


    def SetExport(export):
        self.export = export


    def GetLabel():
        if self.label:
            return self.label.GetLabel()
        else:
            return ""


    def SetLabel(label):
        if label:
            if self.label:
                self.label.SetLabel(label)
            else:
                self.label = wx.StaticText(self.parent, -1, label+"   ")
                wx.BoxSizer.Insert(self, 0, self.label, 0, EcceGlobals.FlagDefault, 0)
        else:
            if self.label:
                wx.BoxSizer.Remove(self, 0)
                self.label = 0


    def GetName(self):
        return self.spin.GetName()


    def SetBackgroundColour(self, color):
        self.spin.SetBackgroundColour(color)


    def SetUnit(self, unit):
        self.unitString = unit
        if self.unitString or self.rangeString:
            self.unitRange.SetLabel("  " + self.unitString + "\n   " + 
                                    self.rangeString)


    def ExportSetting(self):
        needExport = self.export
        if (self.NotDefault()):
            needExport = 1
        return self.spin.GetName() + "|" + str(self.spin.GetValue()) + "|" + \
               self.unitString + "|" + str(int(self.spin.IsEnabled())) + \
               "|" + str(needExport) + "|integer_input"



class EcceTabPanel(wx.Panel):
    """Ecce style panel with tabbed navigation."""

    def __init__(self, parent, rootPanel=0, column=2):
        wx.Panel.__init__(self, parent, -1, style = wx.TAB_TRAVERSAL)
        if not rootPanel:
            rootPanel = parent.GetParent()
        self.panelSizer = wx.FlexGridSizer(0, column, EcceGlobals.BorderDefault,
                                           EcceGlobals.BorderDefault)
        self.SetSizer(self.panelSizer)
        self.SetFont(EcceGlobals.FontDefault)
        self.SetBackgroundColour(EcceGlobals.BackgroundColour)
        self.Settings = rootPanel.Settings
        self.CheckButtonEnables = rootPanel.CheckButtonEnables
        self.OnChanges = rootPanel.OnChanges
        self.SetStatusText = rootPanel.SetStatusText


    def AddWidget(self, widget, flag=EcceGlobals.FlagDefault,
                  border=EcceGlobals.BorderDefault):
        self.panelSizer.Add(widget, 0, flag, border)
        self.Fit()


    def AddSpace(self):
        self.panelSizer.Add((0, 0), 0, EcceGlobals.FlagDefault,
                            EcceGlobals.BorderDefault)



class EcceTextInput(wx.BoxSizer):
    """Ecce style text input with label."""

    def __init__(self, parent, id=-1, pos=wx.DefaultPosition,
                 size=(80, -1), style=wx.TE_PROCESS_ENTER, 
                 validator = wx.DefaultValidator, name="textInput", default="", 
                 label="", export=0, tip=""):
        wx.BoxSizer.__init__(self, wx.HORIZONTAL)
        self.text = wx.TextCtrl(parent, id, default, pos, size, style,
                                validator, name)
        self.parent = parent

        if not tip and EcceGlobals.NameAsTip:
            tip = name
        if tip:
            self.text.SetToolTip(wx.ToolTip(tip))

        self.SetBackgroundColour(EcceGlobals.InputColour)
        self.save = self.default = default

        self.isInvalid = False
        self.label=0
        self.export = export
        if label:
            self.label = wx.StaticText(parent, -1, label+"   ")
            wx.BoxSizer.Add(self, self.label, 0, EcceGlobals.FlagDefault, 0)
        wx.BoxSizer.Add(self, self.text, 0, EcceGlobals.FlagDefault, 0)

        self.parent.Settings.append(self)

        self.timer = wx.Timer(self.text)
        self.text.Bind(wx.EVT_CHAR, self.OnChar)
        self.text.Bind(wx.EVT_KILL_FOCUS, self.OnKillFocus)
        self.text.Bind(wx.EVT_TIMER, self.OnTimer)
        self.text.Bind(wx.EVT_ENTER_WINDOW, self.OnMouseEnter)
        self.text.Bind(wx.EVT_LEAVE_WINDOW, self.OnMouseLeave)


    def OnChar(self, event):
        if EcceGlobals.TimerValidation:
            self.timer.Start(EcceGlobals.TimerLength, True)
        event.Skip()


    def OnKillFocus(self, event):
        self.timer.Stop()
        self.OnChanges(event)
        event.Skip()


    def OnMouseEnter(self, event):
        self.timer.Stop()
        self.SetFocus()
        event.Skip()


    def OnMouseLeave(self, event):
        if EcceGlobals.TimerValidation:
            self.timer.Start(EcceGlobals.TimerLength, True)
        else:
            self.OnKillFocus(event)
        event.Skip()


    def OnTimer(self, event):
        self.OnChanges(event)
        event.Skip()


    def OnChanges(self, event):
        event.SetEventObject(self)
        self.parent.OnChanges(event)
        if EcceGlobals.AutoNavigation:
            if self.parent.FindFocus():
                self.parent.FindFocus().Navigate()

        
    def Reset(self):
        if self.NotDefault:
            self.SetValue(self.default)
            e = wx.CommandEvent()
            e.SetEventObject(self)
            self.parent.OnChanges(e)
            self.SetBackgroundColour(EcceGlobals.InputColour)
        

    def Save(self):
        self.save = self.GetValue()


    def NotDefault(self):
        return self.default != self.GetValue()


    def NotSaved(self):
        return self.save != self.GetValue()


    def Enable(self, enable=True):
        if self.label:
            self.label.Enable(enable)
        if self.text.Enable(enable):
          # was not in the state specified (i.e. work was done)
          e = wx.CommandEvent()
          e.SetEventObject(self)
          self.parent.OnChanges(e)
        if enable:
            self.SetBackgroundColour(EcceGlobals.InputColour)
        else:
            self.SetBackgroundColour(EcceGlobals.ReadonlyColour)
        

    def GetValue(self):
        return self.text.GetValue()
    

    def SetValue(self, val):
        return self.text.SetValue(val)


    def GetDefault():
        return self.default


    def SetDefault(default):
        self.default = default
        

    def GetExport():
        return self.export


    def SetExport(export):
        self.export = export


    def GetLabel():
        if self.label:
            return self.label.GetLabel()
        else:
            return ""


    def SetLabel(label):
        if label:
            if self.label:
                self.label.SetLabel(label)
            else:
                self.label = wx.StaticText(self.parent, -1, label+"   ")
                wx.BoxSizer.Insert(self, 0, self.label, 0, EcceGlobals.FlagDefault, 0)
        else:
            if self.label:
                wx.BoxSizer.Remove(self, 0)
                self.label = 0


    def GetName(self):
        return self.text.GetName()


    def SetBackgroundColour(self, color):
        self.text.SetBackgroundColour(color)


    def SetFocus(self):
        self.text.SetFocus()


    def ExportSetting(self):
        needExport = self.export
        if (self.NotDefault()):
            needExport = 1
        return self.text.GetName() + "|" + self.text.GetValue() + "||" + \
               str(int(self.text.IsEnabled())) + "|" + str(needExport) + \
               "|text_input"

