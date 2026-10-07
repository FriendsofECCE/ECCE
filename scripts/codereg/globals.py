import sys
import os
import signal
import string
import socket

# This whole legacy codebase combines wx.EXPAND with alignment flags
# (e.g. wx.ALIGN_CENTER_VERTICAL) on the same sizer item throughout --
# harmless (wx.EXPAND always overrides alignment in a box sizer either
# way, confirmed via the assertion's own message), but modern wxWidgets
# 3.2 raises a hard C++ assertion for it that Phoenix propagates as a
# fatal Python exception, unlike wx2.8 which just silently ignored the
# combination. Suppressing this check (wx's own documented escape
# hatch) is far lower-risk than auditing every .Add() call across
# templates.py and ~15 dialog scripts for a purely cosmetic warning.
# Must be set before `import wx`.
os.environ.setdefault("WXSUPPRESS_SIZER_FLAGS_CHECK", "1")

import wx

app = wx.App()

class Globals:
    # CONSTANTS
    BorderDefault = 3
    ProportionDefault = 1
    FlagDefault =  wx.ALL|wx.ALIGN_CENTER_VERTICAL
    MinFloatDefault = -1e100
    MaxFloatDefault = 1e100
    MinIntDefault = -1000000
    MaxIntDefault = 1000000
    # The theme draws everything except read-only fields, as the C++
    # windows do (ewxStyledWindow, #210); wx.NullColour hands a widget
    # back to the theme.
    InputColour = wx.NullColour
    ReadonlyColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_BTNFACE)
    
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

    @staticmethod
    def ThemeFont(size):
        font = wx.SystemSettings.GetFont(wx.SYS_DEFAULT_GUI_FONT)
        font.SetPointSize(size)
        return font

    @staticmethod
    def FontSizes():
        """(base point size, step) from the shared EcceGlobal preferences."""
        prefs = {}
        home = os.environ.get("ECCE_REALUSERHOME", os.path.expanduser("~"))
        try:
            with open(os.path.join(home, ".ECCE", "EcceGlobal")) as f:
                for line in f:
                    key, sep, value = line.partition(":")
                    if sep:
                        prefs[key.strip().lower()] = value.strip()
        except OSError:
            pass
        base = 10
        if prefs.get("usesystemfont", "false").lower() in ("true", "1"):
            base = wx.SystemSettings.GetFont(
                wx.SYS_DEFAULT_GUI_FONT).GetPointSize()
        try:
            step = {0: -1, 2: 2, 3: 4}.get(int(prefs.get("fontsize", 1)), 0)
        except ValueError:
            step = 0
        return base, step

    def __init__(self, values):
        # Same tints as ewxThemeColours::statusTint (BAD, UNSURE), light and
        # dark; the message box keeps the theme's text colour.
        dark = wx.SystemSettings.GetAppearance().IsDark()
        self.ErrorColour = wx.Colour("#5c1f1f" if dark else "#ffdddd")
        self.WarningColour = wx.Colour("#4d3a10" if dark else "#fff1cc")

        # The theme's font at ECCE's 10 pt, moved by the Font Size
        # preference, as ewxStyledWindow::applyFont does.
        base, step = self.FontSizes()
        self.FontDefault = self.ThemeFont(base + step)
        self.ScriptFontDefault = self.ThemeFont(base + step - 1)

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
            # EnableDebug is the flag the rest of templates.py actually tests,
            # but nothing ever set it from the command line -- so every
            # `if EcceGlobals.EnableDebug` site in this codebase was dead code
            # while calced dutifully passed DebugOn/DebugOff in argv[5].
            # Derive it from the flag that is actually delivered.
            Globals.EnableDebug = (self.DebugFlag == "DebugOn")
            if len(values) > 18:
              # This field is only meaningful to metartyp.py; every other
              # codereg script (theory dialogs especially) can end up with
              # something else entirely at this position depending on what
              # its own C++ caller appends after the required arguments
              # (e.g. display colors) -- confirmed against a real crash
              # report where this int() raised ValueError on a hex color
              # string. Not fatal to the rest of __init__: keep the class
              # default (0) rather than aborting parsing of a script that
              # never uses this value in the first place.
              try:
                self.ChainedMetadynamics = int(values[18])
              except ValueError:
                pass
        except IndexError:
            print("ERROR: wxPython code registration details dialog command line parameter missing!")

        if self.PortOut != 0:
          self.Socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
          self.Socket.connect( ('localhost', self.PortOut) )
          line = "#STARTED"
          if (self.RestoreFlag == "NO_GUIValues"):
              line = line + " 0\n"
          else:
              pid = os.getpid()
              line = line + " " + str(pid) + "\n"
          # socket.send() takes bytes in Python 3, not str -- a real
          # Python 2-ism the earlier syntax-only port (py_compile
          # doesn't catch this) missed. Same fix needed everywhere else
          # this socket is used, in templates.py.
          self.Socket.send(line.encode())

def debug(message):
    """Verbose troubleshooting output, off unless calced passed DebugOn.

    Goes to stderr on purpose: stdout of a codereg dialog is not a channel
    anything reads, but stderr lands in the gateway's log where a user can be
    asked for it.  Every line is prefixed so it can be grepped out of a mixed
    log.
    """
    if Globals.EnableDebug:
        sys.stderr.write("[ecce-dialog] %s\n" % message)
        sys.stderr.flush()


EcceGlobals = Globals(sys.argv)

debug("started: %s" % " ".join(sys.argv[1:]))
