#include <map>

#include <wx/sizer.h>
#include <wx/stattext.h>

#include "dsm/IPropCalculation.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "DensityMapPanel.H"
#include "DensityProfilePanel.H"
#include "MoDiagramPanel.H"
#include "MoPanel.H"
#include "PropertyPanel.H"
#include "PointNote.H"

static const char *NOTE_NAME = "orbitalPointNote";

// The panel's row of the selected point, per calculation: the calculation
// holds the table index, which need not equal the row.
static std::map<std::string, int>& rowOf()
{
  static std::map<std::string, int> rows;
  return rows;
}

std::string orbitalPointNote(IPropCalculation *calc)
{
  if (!calc) return std::string();
  const bool irc = calc->getProperty("IRCCOORD") != 0;
  if (!irc && !calc->getProperty("SCANCOORD")) return std::string();
  const char *kind = irc ? "IRC" : "scan";
  if (calc->hasPointOrbitals()) {
    std::map<std::string, int>::const_iterator r =
        rowOf().find(calc->getURL().toString());
    if (calc->orbitalPoint() >= 0 && r != rowOf().end())
      return std::string("Orbitals and density of ") + kind + " point " +
             std::to_string(r->second + 1) + ".";
    PropVector *c = dynamic_cast<PropVector*>(
        calc->getProperty(irc ? "IRCCOORD" : "SCANCOORD"));
    const std::string last = !irc && c && !c->values().empty()
        ? " (point " + std::to_string(c->values().size()) + ")" : "";
    return std::string("Orbitals and density of the last computed point of this ") +
           kind + last + ". Select a point in the " +
           (irc ? "IRC Path" : "Relaxed Scan") + " panel to see its orbitals.";
  }
  PropVector *c = dynamic_cast<PropVector*>(
      calc->getProperty(irc ? "IRCCOORD" : "SCANCOORD"));
  const std::string last = c && !c->values().empty()
      ? " (point " + std::to_string(irc ? 0 : c->values().size()) + ")" : "";
  // An IRC's last computed point is the end of its second direction, so its
  // number in the table is not known here.
  return std::string("Orbitals and density belong to the last computed point of this ") +
         (irc ? "IRC" : "scan") + (irc ? "" : last) +
         "; they are not stored for the other points. The structure shown is that point's.";
}

wxStaticText *addOrbitalPointNote(wxWindow *parent, wxSizer *sizer,
                                  IPropCalculation *calc)
{
  const std::string text = orbitalPointNote(calc);
  if (text.empty() || !sizer) return 0;
  // A panel that rebuilds its controls keeps its one note.  The parent
  // argument matters: FindWindowByName alone searches every window.
  if (wxStaticText *old = dynamic_cast<wxStaticText*>(
          wxWindow::FindWindowByName(NOTE_NAME, parent))) {
    old->SetLabel(wxString::FromUTF8(text.c_str()));
    old->Wrap(480);
    return old;
  }
  wxStaticText *label = new wxStaticText(parent, wxID_ANY,
                                         wxString::FromUTF8(text.c_str()),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         NOTE_NAME);
  label->Wrap(480);
  sizer->Insert(0, label, 0, wxEXPAND | wxALL, 4);
  return label;
}

void selectOrbitalPoint(IPropCalculation *calc, int tableIndex, int row)
{
  if (!calc || !calc->hasPointOrbitals()) return;
  const std::string url = calc->getURL().toString();
  if (calc->orbitalPoint() == tableIndex && rowOf()[url] == row) return;
  rowOf()[url] = row;
  calc->orbitalPoint(tableIndex);
  const std::string text = orbitalPointNote(calc);
  for (PropertyPanel *p : PropertyPanel::getPanels(url)) {
    wxStaticText *note = dynamic_cast<wxStaticText*>(
        wxWindow::FindWindowByName(NOTE_NAME, p));
    if (note) {
      note->SetLabel(wxString::FromUTF8(text.c_str()));
      note->Wrap(480);
    }
    if (MoPanel *m = dynamic_cast<MoPanel*>(p)) m->orbitalPointChanged();
    else if (DensityMapPanel *d = dynamic_cast<DensityMapPanel*>(p)) d->orbitalPointChanged();
    else if (DensityProfilePanel *f = dynamic_cast<DensityProfilePanel*>(p)) f->orbitalPointChanged();
    else if (MoDiagramPanel *g = dynamic_cast<MoDiagramPanel*>(p)) g->refresh();
  }
}
