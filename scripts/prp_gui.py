"""
This is a small wxPython GUI for plotting any PRP output.
"""

import glob
import os
import re
import signal
import sys
import wx

class PrpPanel(wx.Panel):
  """This panel contains what is needed to plot PRP data.

  Contains two identical listboxes showing the property names corresponding to
  the x and y axes of a plot image appearing below.

  """

  def __init__(self, parent, path, *args, **kwargs):
    """Create the PrpPanel."""

    wx.Panel.__init__(self, parent, *args, **kwargs)

    Sizer = wx.BoxSizer(wx.HORIZONTAL)

    self.ParsedDescriptors = False
    self.path = path
    self.plotFilename = os.path.join(self.path, "input.gnuplot")
    self.propName = []
    self.propUnit = []
    self.propValue = []
    self.propIndex = {}
    if not self.ParseFiles():
      print "No PRP files found\n"
      Sizer.Add(wx.StaticText(self, label="No PRP files found"))
      self.SetSizerAndFit(Sizer)
      return

    # We want the properties list to be sorted, so all parallel lists must
    # be sorted at the same time.
    allLists = zip(self.propName, self.propUnit, self.propValue)
    allLists.sort()
    self.propName, self.propUnit, self.propValue = zip(*allLists)
    for i,prop in enumerate(self.propName):
      self.propIndex[prop] = i

    xSizer = wx.BoxSizer(wx.VERTICAL)
    xSizer.Add(wx.StaticText(self, label="X Axis"), 0, wx.ALL, 5)
    self.XListBox = wx.ListBox(self, style=wx.LB_ALWAYS_SB,
                               choices=self.propName)
    self.XListBox.SetSelection(0)
    xSizer.Add(self.XListBox, 1, wx.ALL|wx.EXPAND, 5)
    filterSizer = wx.BoxSizer(wx.HORIZONTAL)
    filterSizer.Add(wx.StaticText(self, label="Filter:"),
                    0, wx.ALL|wx.ALIGN_CENTER, 5)
    self.xFilter = wx.TextCtrl(self)
    self.xFilter.myList = self.XListBox
    self.xFilter.Bind(wx.EVT_TEXT, self.OnXFilterChange)
    filterSizer.Add(self.xFilter, 1, wx.ALL|wx.EXPAND, 5)
    xSizer.Add(filterSizer, 0, wx.EXPAND)
    self.xLog = wx.CheckBox(self, label="Use Logarithmic Scale")
    xSizer.Add(self.xLog, 0, wx.ALL, 5)
    Sizer.Add(xSizer)

    ySizer = wx.BoxSizer(wx.VERTICAL)
    ySizer.Add(wx.StaticText(self, label="Y Axis"), 0, wx.ALL, 5)
    self.YListBox = wx.ListBox(self, style=wx.LB_ALWAYS_SB|wx.LB_MULTIPLE,
                               choices=self.propName)
    self.YListBox.SetSelection(0)
    ySizer.Add(self.YListBox, 1, wx.ALL|wx.EXPAND, 5)
    filterSizer = wx.BoxSizer(wx.HORIZONTAL)
    filterSizer.Add(wx.StaticText(self, label="Filter:"),
                    0, wx.ALL|wx.ALIGN_CENTER, 5)
    self.yFilter = wx.TextCtrl(self)
    self.yFilter.myList = self.YListBox
    self.yFilter.Bind(wx.EVT_TEXT, self.OnYFilterChange)
    filterSizer.Add(self.yFilter, 1, wx.ALL|wx.EXPAND, 5)
    ySizer.Add(filterSizer, 0, wx.EXPAND)
    self.yLog = wx.CheckBox(self, label="Use Logarithmic Scale")
    ySizer.Add(self.yLog, 0, wx.ALL, 5)
    Sizer.Add(ySizer)

    optionsSizer = wx.BoxSizer(wx.VERTICAL)
    optionsSizer.Add(wx.StaticText(self, label="Options"), 0, wx.ALL, 5)
    styleSizer = wx.BoxSizer(wx.HORIZONTAL)
    styleSizer.Add(wx.StaticText(self, label="Plot Style:"),
                   0, wx.ALL|wx.ALIGN_CENTER, 5)
    self.StyleChoice = wx.Choice(self, choices=["points","lines","linespoints"])
    self.StyleChoice.SetSelection(0)
    styleSizer.Add(self.StyleChoice, 0, wx.ALL|wx.ALIGN_CENTER, 5)
    optionsSizer.Add(styleSizer, 0)
    optionsSizer.Add((0,0), 1, wx.EXPAND)
    button = wx.Button(self, label="Plot");
    button.Bind(wx.EVT_BUTTON, self.OnGraphButton)
    optionsSizer.Add(button, 0, wx.ALL|wx.ALIGN_CENTER, 5)
    Sizer.Add(optionsSizer, 0, wx.EXPAND)

    self.SetSizerAndFit(Sizer)

  def OnGraphButton(self, event=None):
    """Create an input for gnuplot and run it based on GUI selections."""
    self.CreateRawDataInput()
    self.CreateGnuplotInput()
    if self.xLog.IsChecked() and self.XNegative:
      wx.MessageDialog(self,
              "x-axis: Cannot use logarithmic scale with negative values",
              "Error", wx.OK).ShowModal()
    elif self.yLog.IsChecked() and self.YNegative:
      wx.MessageDialog(self,
              "y-axis: Cannot use logarithmic scale with negative values",
              "Error", wx.OK).ShowModal()
    else:
      self.RunGnuplot()

  def CreateRawDataInput(self):
    """Create the file read by gnuplot's "plot" command."""
    x,y = self.XListBox.GetSelection(), self.YListBox.GetSelections()
    self.XNegative = False
    self.YNegative = False
    for i in range(len(y)):
      file = open(self.GetInputFilename(i), "w")
      for a,b in zip(self.propValue[x],self.propValue[y[i]]):
        file.write("%s %s\n" % (a,b))
        if float(a) < 0.0:
          self.XNegative = True
        if float(b) < 0.0:
          self.YNegative = True
      file.close()

  def CreateGnuplotInput(self):
    x = self.propIndex[self.XListBox.GetStringSelection()]
    yStrings = map(self.YListBox.GetString, self.YListBox.GetSelections())
    y = map(self.propIndex.get, yStrings)
    file = open(self.plotFilename, "w")
    title = ""
    if len(y) == 1:
      title = "%s vs %s" % (self.propName[x], self.propName[y[0]])
    else:
      title = "%s vs Multiple Properties" % (self.propName[x])
    file.write('set terminal x11 title "Gnuplot ECCE MD Property Plotter: %s"\n' % title);
    if self.xLog.IsChecked():
      file.write('set logscale x\n')
    if self.yLog.IsChecked():
      file.write('set logscale y\n')
    unit = self.propUnit[x].strip()
    if unit: unit = " (%s)" % unit
    file.write('set xlabel "%s%s"\n' % (self.propName[x].strip(), unit))
    unit = self.propUnit[y[0]].strip()
    if unit: unit = " (%s)" % unit
    file.write('plot "%s" title "%s%s" with %s' %
               (self.GetInputFilename(0), self.propName[y[0]].strip(),
                unit, self.StyleChoice.GetStringSelection()))
    for i in range(1, len(y)):
      unit = self.propUnit[y[i]].strip()
      if unit: unit = " (%s)" % unit
      file.write(',\\\n')
      file.write('     "%s" title "%s%s" with %s' %
                 (self.GetInputFilename(i), self.propName[y[i]].strip(),
                  unit, self.StyleChoice.GetStringSelection()))

  def ParseFiles(self):
    """Glob for PRP files and parse them for all of their data."""
    prpFiles = glob.glob(os.path.join(self.path, "*.prp"))
    if len(prpFiles) == 0:
      return False
    for file in sorted(prpFiles):
      self.ParseFile(file)
    return True

  def ParseFile(self, file):
    """Parse the given PRP file for its property values."""
    file = open(file, "r")
    self._ParseHeader(file)
    self._ParseDescriptors(file)
    self._ParseFrames(file)

  def _ParseHeader(self, file):
    self.numProps, self.date, self.time, n, n = file.readline().split()
    self.numProps = int(self.numProps)

  def _ParseDescriptors(self, file):
    if not self.ParsedDescriptors:
      self.ParsedDescriptors = True
      for line in file:
        if line.startswith("frame"): break
        self.propName.append(line[:-10].strip())
        self.propUnit.append(line[-10:].strip())
        self.propValue.append([])
    else:
      for line in file:
        if line.startswith("frame"): break

  def _ParseFrames(self, file):
    count = 0
    for line in file:
      if line.startswith("frame"):
        count = 0
        continue
      for value in re.findall("[ -]\d\.\d\d\d\d\d[eE][-+]\d\d", line):
        self.propValue[count].append(value)
        count += 1

  def RunGnuplot(self):
    """Call gnuplot with the input file."""
    cmd = "gnuplot -persist %s" % self.plotFilename
    os.system(cmd)

  def GetInputFilename(self, index):
    """For plotting multiple properties on same plot, need more input files."""
    return os.path.join(self.path, "input%s.data" % index)

  def OnXFilterChange(self, event):
    """Update bound listbox as text filter changes."""
    # 1) remember last selection
    # 2) start with full list, remove entries not starting with filter
    # 3) re-set list items
    # 4) restore last selection, if possible
    lastSelection = self.XListBox.GetStringSelection()
    propName = []
    for prop in self.propName:
      if prop.lower().startswith(self.xFilter.GetValue().lower()):
        propName.append(prop)
    self.XListBox.Set(propName)
    if not self.XListBox.SetStringSelection(lastSelection):
      self.XListBox.SetSelection(0)

  def OnYFilterChange(self, event):
    """Update bound listbox as text filter changes."""
    # 1) remember last selections
    # 2) start with full list, remove entries not starting with filter
    # 3) re-set list items
    # 4) restore last selections, if possible
    lastSelections = map(self.YListBox.GetString, self.YListBox.GetSelections())
    propName = []
    for prop in self.propName:
      if prop.lower().startswith(self.yFilter.GetValue().lower()):
        propName.append(prop)
    self.YListBox.Set(propName)
    needsSelection = False
    for lastSelection in lastSelections:
      if not self.YListBox.SetStringSelection(lastSelection):
        needsSelection = True
    if needsSelection:
      self.YListBox.SetSelection(0)

class PrpFrame(wx.Frame):
  """Main Frame holding the PrpPanel."""

  def __init__(self, path, *args, **kwargs):
    """Create the PmfFrame."""
    wx.Frame.__init__(self, None, title="ECCE MD Property Plotter")

    # Add the PrpPanel
    self.Panel = PrpPanel(self, path)

    self.Fit()

    signal.signal(signal.SIGTERM, self.handleSIGTERM)

  def OnQuit(self, event=None):
    """Exit application."""
    self.Close()

  def handleSIGTERM(signum, frame):
    self.OnQuit()



if __name__ == '__main__':
  app = wx.PySimpleApp()
  frame = PrpFrame(sys.argv[1])
  frame.Fit()
  frame.Show(True)
  if len(sys.argv) > 2:
    calcviewerPipe = open(sys.argv[2], "w")
    calcviewerPipe.write("#STARTED %s\n" % str(os.getpid()))
    calcviewerPipe.flush()
  app.MainLoop()

