"""
This is a small wxPython GUI for showing PMF output.
"""

import glob
import math
import optparse
import os
import signal
import wx

class PmfPanel(wx.Panel):
  """This Panel holds a listbox containing PMF data display options.
  
  Contains a list of pmf indices, a radio button set for picking the graph
  type, and a checkbox specifying whether the graphs should be drawn separate
  or superimposed.
  
  """

  def __init__(self, parent, path, *args, **kwargs):
    """Create the PmfPanel."""

    self.path = path
    wx.Panel.__init__(self, parent, *args, **kwargs)

    Sizer = wx.BoxSizer(wx.HORIZONTAL)

    pmfListSizer = wx.BoxSizer(wx.VERTICAL)
    pmfListSizer.Add(wx.StaticText(self,label="PMF Directive"), 0, wx.ALL, 5)
    self.pmfListBox = wx.ListBox(self, style=wx.LB_ALWAYS_SB)
    self.LoadPmfList()
    pmfListSizer.Add(self.pmfListBox, 1, wx.ALL|wx.EXPAND, 5)
    Sizer.Add(pmfListSizer, 1, wx.ALL|wx.EXPAND, 5)

    fileListSizer = wx.BoxSizer(wx.VERTICAL)
    fileListSizer.Add(wx.StaticText(self, label="PMF Frame"), 0, wx.ALL, 5)
    self.fileListBox = wx.ListBox(self, style=wx.LB_ALWAYS_SB|wx.LB_MULTIPLE)
    self.LoadFileList()
    fileListSizer.Add(self.fileListBox, 1, wx.ALL|wx.EXPAND, 5)
    button = wx.Button(self, label="Select All")
    button.Bind(wx.EVT_BUTTON, self.OnSelectAllButton)
    fileListSizer.Add(button, 0, wx.ALL|wx.EXPAND, 5)
    Sizer.Add(fileListSizer, 1, wx.ALL|wx.EXPAND, 5)

    optionsSizer = wx.BoxSizer(wx.VERTICAL)
    optionsSizer.Add(wx.StaticText(self, label="Plot Options"), 0, wx.ALL, 5)
    self.radiobox = wx.RadioBox(self, label="Plot Type:",
            choices=["Energy vs Coordinate",
                     "Coordinate Distribution",
                     "Energy Distribution"],
            style=wx.RA_SPECIFY_ROWS)
    optionsSizer.Add(self.radiobox, 0, wx.ALL|wx.EXPAND, 5)
    self.superImposeCheck = wx.CheckBox(self, label="Overlay Plots")
    optionsSizer.Add(self.superImposeCheck, 0, wx.ALL|wx.EXPAND, 5)
    button = wx.Button(self, label="Plot");
    button.Bind(wx.EVT_BUTTON, self.OnGraphButton)
    optionsSizer.Add(button, 0, wx.ALL|wx.ALIGN_CENTER, 5)
    Sizer.Add(optionsSizer, 1, wx.ALL|wx.EXPAND, 5)

    self.SetSizerAndFit(Sizer)

  def OnGraphButton(self, event=None):
    """Create an input file for gnuplot and run it based on GUI selections."""

    files = [self.fileListBox.GetClientData(n)
             for n in self.fileListBox.GetSelections()]

    if self.radiobox.GetSelection() == 0:
      self.PlotHarmonic(files)
    else:
      self.BinFiles(files)
      files = map(lambda x: x+".bin", files)
      if self.radiobox.GetSelection() == 1:
        self.PlotBinCol2(files)
      if self.radiobox.GetSelection() == 2:
        self.PlotBinCol3(files)

  def OnSelectAllButton(self, event=None):
    """Select all PMF frames."""

    for i in range(self.fileListBox.GetCount()):
      self.fileListBox.Select(i)

  def _Plot(self, files, plotCmd, plotFile):
    """Uses given plotCmd to write single or multiplot output to plotFile."""

    if (self.superImposeCheck.IsChecked()):
      plotFile.write("plot ")
      for i in range(len(files)):
        plotFile.write(plotCmd % (files[i],
                                  files[i].split("-")[1].split(".")[0]))
        if i < len(files) - 1:
          plotFile.write(",\\\n")
    else:
      width,height = 1,1
      for i in range(len(files)+1):
        if width*height < i:
          ratio = 1.25
          if abs(((width+1)/height)-ratio) <= abs((width/(height+1))-ratio):
            width+=1
          else:
            height+=1
      plotFile.write("set size 1,1\n")
      plotFile.write("set origin 0,0\n")
      plotFile.write("set multiplot\n")
      i = 0
      sizeW, sizeH = 1.0/width, 1.0/height
      for row in range(height):
        for col in range(width):
          plotFile.write("set size %f,%f\n" % (sizeW,sizeH))
          plotFile.write("set origin %f,%f\n" % (sizeW*col,1.0-sizeH*(row+1)))
          plotFile.write("plot " + plotCmd % (files[i],
                         files[i].split("-")[1].split(".")[0]) + "\n")
          i+=1
          if i >= len(files):
            break;
        if i >= len(files):
          break;
      plotFile.write("set nomultiplot\n")
    plotFile.close()
    self.RunGnuplot(plotFile.name)


  def PlotHarmonic(self, files):
    """Plot column 2 vs column 3."""

    pmfOpt = int(self.pmfListBox.GetStringSelection())
    plotCmd = '"%%s" ' \
              'using 2:($1 == %i ? $3 : 1/0) ' \
              'every ::%d ' \
              'title "frame %%s"' % (pmfOpt, self.pmfListBox.GetCount())
    gnuplotHarmonicFileName = os.path.join(self.path, "pmf_harmonic.gnuplot")
    plotFile = open(gnuplotHarmonicFileName, 'w')
    plotFile.write('set terminal x11 title "Gnuplot ECCE MD PMF Property Plotter: PMF Option %i - Energy vs Coordinate"\n' % pmfOpt)
    plotFile.write('set xlabel "Coordinate"\n')
    plotFile.write('set ylabel "Energy"\n')
    self._Plot(files, plotCmd, plotFile)

  def PlotBinCol2(self, files):
    """Plot column 2 binned."""

    pmfOpt = int(self.pmfListBox.GetStringSelection())
    plotCmd = '"%%s" using 3:($1 == %i ? $2 == 2 ? $4 : 1/0 : 1/0) ' \
              'title "frame %%s" with histeps' % pmfOpt
    gnuplotBinFileName = os.path.join(self.path, "pmf_bin2.gnuplot")
    plotFile = open(gnuplotBinFileName, 'w')
    plotFile.write('set terminal x11 title "Gnuplot ECCE MD PMF Property Plotter: PMF Option %i - Coordinate Distribution"\n' % pmfOpt)
    plotFile.write('set xlabel "Coordinate Bins"\n')
    plotFile.write('set ylabel "Count"\n')
    self._Plot(files, plotCmd, plotFile)

  def PlotBinCol3(self, files):
    """Plot column 3 binned."""

    pmfOpt = int(self.pmfListBox.GetStringSelection())
    plotCmd = '"%%s" using 3:($1 == %i ? $2 == 3 ? $4 : 1/0 : 1/0) ' \
              'title "frame %%s" with histeps' % pmfOpt
    gnuplotBinFileName = os.path.join(self.path, "pmf_bin3.gnuplot")
    plotFile = open(gnuplotBinFileName, 'w')
    plotFile.write('set terminal x11 title "Gnuplot ECCE MD PMF Property Plotter: PMF Option %i - Energy Distribution"\n' % pmfOpt)
    plotFile.write('set xlabel "Energy Bins"\n')
    plotFile.write('set ylabel "Count"\n')
    self._Plot(files, plotCmd, plotFile)

  def RunGnuplot(self, filename):
    """Call gnuplot with the given file."""

    cmd = "gnuplot -persist %s" % filename
    os.system(cmd)

  def LoadPmfList(self):
    """Read a random PMF file to initialize the PMF option listbox."""

    self.pmfListBox.Clear()
    pmfFiles = glob.glob(os.path.join(self.path, "*.pmf"))
    if len(pmfFiles) <= 0: return #bail!
    pmfNumbers = []
    for line in open(pmfFiles[0]):
      num = line.split()[0]
      if num not in pmfNumbers:
        pmfNumbers.append(num)
    pmfNumbers.sort()
    for num in pmfNumbers:
      self.pmfListBox.Append(num)
    self.pmfListBox.SetSelection(0)

  def LoadFileList(self):
    """Determine *.pmf files and list their indices in the listbox."""

    self.fileListBox.Clear()
    files = glob.glob(os.path.join(self.path, "*.pmf"))
    files.sort()
    for file in files:
      self.fileListBox.Select(
      self.fileListBox.Append(file[:-4].split("-")[-1], file))

  def BinFiles(self, files):
    """Preprocess *.pmf files for input into gnuplot as a distribution."""

    for file in files:
      outfile = file + ".bin"
      if os.path.exists(outfile): continue

      data = {}
      file = open(file, 'r')
      for opt in range(self.pmfListBox.GetCount()):
        file.readline()
      for line in file:
        colVal = {}
        pmfOption,colVal["col2"],colVal["col3"] = line.split()
        for col in ["col2","col3"]: colVal[col] = float(colVal[col])
        if pmfOption not in data:
          data[pmfOption] = {}
          for col in ["col2","col3"]:
            data[pmfOption][col] = {"values":[colVal[col]],"bins":{}}
        else:
          for col in ["col2","col3"]:
            data[pmfOption][col]["values"].append(colVal[col])
      for pmfOption in data:
        for col in ["col2","col3"]:
          minVal = min(data[pmfOption][col]["values"])
          maxVal = max(data[pmfOption][col]["values"])
          for bin in frange(minVal,maxVal,(maxVal-minVal)/20):
            data[pmfOption][col]["bins"][bin] = 0
          for value in data[pmfOption][col]["values"]:
            for bin in frange(minVal,maxVal,(maxVal-minVal)/20):
              if value < bin:
                data[pmfOption][col]["bins"][bin]+=1
                break
      outfile = open(outfile, 'w')
      for pmfOption in data:
        for col in ["col2","col3"]:
          for bin in sorted(data[pmfOption][col]["bins"].keys()):
            outfile.write("%s %s %f %i\n" %
            (pmfOption, col[3], bin, data[pmfOption][col]["bins"][bin]))


class PmfFrame(wx.Frame):
  """Main Frame holding the PmfPanel."""

  def __init__(self, path, *args, **kwargs):
    """Create the PmfFrame."""
    wx.Frame.__init__(self, None, title="ECCE MD PMF Property Plotter")

    # Add the PmfPanel
    self.Panel = PmfPanel(self, path)

    self.Fit()

    signal.signal(signal.SIGTERM, self.handleSIGTERM)

  def OnQuit(self, event=None):
    """Exit application."""
    self.Close()

  def handleSIGTERM(signum, frame):
    self.OnQuit()


def frange(limit1, limit2=None, increment=1.):
  """Range function that accepts floats (and integers). numpy would be nice.
  
  Usage:
    frange(-2,2,0.1)
    frange(10)
    frange(10, increment=0.5)
    
  The returned value is an iterator.  Use list(frange(start,...)) for a list.
  """

  if limit1 == limit2:
    return (limit1 for n in xrange(1))
  if limit2 is None:
    limit2,limit1 = limit1,0.
  else:
    limit1 = float(limit1)

  count = int(math.ceil((limit2-limit1)/increment))
  return ((limit1 + n*increment) for n in xrange(count))


def sorted(sortable):
  sortable.sort()
  return sortable


if __name__ == '__main__':
  app = wx.PySimpleApp()
  (options, args) = optparse.OptionParser().parse_args()
  frame = PmfFrame(args[0])
  frame.Fit()
  frame.Show(True)
  if len(args) > 1:
    calcviewerPipe = open(args[1], "w")
    calcviewerPipe.write("#STARTED %s\n" % str(os.getpid()))
    calcviewerPipe.flush()
  app.MainLoop()

