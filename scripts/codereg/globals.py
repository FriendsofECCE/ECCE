import wx
import sys
import os
import signal
import string
import socket

app = wx.PySimpleApp()

class Globals:
    # CONSTANTS
    BorderDefault = 3
    ProportionDefault = 1
    FlagDefault =  wx.ALL|wx.ALIGN_CENTER_VERTICAL
    MinFloatDefault = -1e100
    MaxFloatDefault = 1e100
    MinIntDefault = -1000000
    MaxIntDefault = 1000000
    BackgroundColour = wx.Colour(224, 225, 225) # #e0e1e1
    InputColour = wx.Colour(249, 221, 153) #f9dd99
    ReadonlyColour = wx.Colour(183, 184, 186) #b7b8ba
    
    FdIn = "restore.in"
    PortOut = 0
    RestoreFlag = "GUIValues"
    ReadOnlyFlag = "Writable"
    DebugFlag = "DebugOff"
    Signal = 0
    Category = "SCF"
    Theory = "7"
    RunType = "Geometry"
    CalculationName = "Calculation"
    ReactionStudyFlag = 0
    SymmetryGroup = 11
    NumElectrons = 12
    SpinMultiplicity = 13
    NumFrozenOrbs = 14
    NumOccupiedOrbs = 15
    NumVirtualOrbs = 16
    NumNormalModes = 17
    ChainedMetadynamics = 0

    ShowTip = True
    NameAsTip = True
    EventLoopStarted = False
    EnableDebug = False

    # For performance and appearance reasons, socket messages are batched
    # for the NO_GUIVALUES case and sent at the end of the initialization
    BatchValues = []

    Message = ""
    NORMAL = "normal"
    WARNING = "warning"
    ERROR = "error"
    ColorTimerLength = 100
    MessageTimerLength = 5000

    # Set these to False for instant validation when mouse leave events
    # and to eliminate tab traversal that isn't compatible with keying
    # validation on mouse leave events.
    TimerValidation = True
    AutoNavigation = True

    # When TimerValidation is enabled, this is the number if milliseconds
    # to wait after a key click event before performing validation
    TimerLength = 2000

    def __init__(self, values):
        self.ErrorColour = wx.Colour(255, 75, 85)
        self.WarningColour = wx.Colour(255, 255, 198)
        self.FontDefault = wx.Font(pointSize = 8,
                                   family = wx.DEFAULT,
                                   style = wx.NORMAL,
                                   weight = wx.BOLD,
                                   face = "Helvetica")
        self.ScriptFontDefault = wx.Font(pointSize = 8,
                                         family = wx.DEFAULT,
                                         style = wx.NORMAL,
                                         weight = wx.NORMAL,
                                         face = "Helvetica")

        # PARAMETER INITIALIZED CONSTANTS
        try:
            self.FdIn = values[1]
            self.PortOut = int(values[2])
            self.RestoreFlag = values[3]
            self.ReadOnlyFlag = values[4]
            self.DebugFlag = values[5]
            self.Category = values[6]
            self.Theory = values[7]
            self.RunType = values[8]
            self.CalculationName = values[9]
            self.ReactionStudyFlag = int(values[10])
            self.SymmetryGroup = values[11]
            self.NumElectrons = int(values[12])
            self.SpinMultiplicity = int(values[13])
            self.NumFrozenOrbs = int(values[14])
            self.NumOccupiedOrbs = int(values[15])
            self.NumVirtualOrbs = int(values[16])
            self.NumNormalModes = int(values[17])
            if len(values) > 18:
              self.ChainedMetadynamics = int(values[18])
        except IndexError:
            print "ERROR: wxPython code registration details dialog command line parameter missing!"

        if self.PortOut != 0:
          self.Socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
          self.Socket.connect( ('localhost', self.PortOut) )
          line = "#STARTED"
          if (self.RestoreFlag == "NO_GUIValues"):
              line = line + " 0\n"
          else:
              pid = os.getpid()
              line = line + " " + str(pid) + "\n"
          self.Socket.send(line)

EcceGlobals = Globals(sys.argv)
