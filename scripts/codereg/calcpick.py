"""Pick a calculation of the project in a Runtype Details dialog: the shared
part of the Gaussian QST and ORCA NEB-TS pickers (#419)."""
import os
from templates import *

def qstCandidates():
    """Calculations of this project that have a structure, as CalcEd lists
    them in the file named by ECCE_QST_CANDIDATES: name, URL, kind per line
    (kind "optimised" or "input").  The dialog cannot see the project."""
    out = []
    try:
        with open(os.environ.get("ECCE_QST_CANDIDATES", "")) as f:
            for line in f:
                parts = line.rstrip("\n").split("\t")
                if len(parts) == 3:
                    out.append(parts)
    except OSError:
        pass
    return out


class CalcChoice(EcceComboBox):
    """A choice that shows calculation names and stores their URLs."""

    def __init__(self, parent, entries, **kw):
        self.urls = [""] + [e[1] for e in entries]
        labels = ["(none)"] + ["%s (%s structure)" % (e[0], e[2])
                               for e in entries]
        EcceComboBox.__init__(self, parent, choices=labels, **kw)

    def GetValue(self):
        sel = self.combo.GetSelection()
        return self.urls[sel] if sel >= 0 else ""

    def SetValue(self, val):
        self.combo.SetSelection(self.urls.index(val)
                                if val in self.urls else 0)

    def ExportSetting(self):
        needExport = 1 if (self.export or self.NotDefault()) else 0
        return (self.combo.GetName() + "|" + self.GetValue() + "||" +
                str(int(self.combo.IsEnabled())) + "|" + str(needExport) +
                "|menu_input")

