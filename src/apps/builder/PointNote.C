#include <wx/sizer.h>
#include <wx/stattext.h>

#include "dsm/IPropCalculation.H"
#include "tdat/PropVector.H"
#include "PointNote.H"

std::string orbitalPointNote(IPropCalculation *calc)
{
  if (!calc) return std::string();
  const bool irc = calc->getProperty("IRCCOORD") != 0;
  if (!irc && !calc->getProperty("SCANCOORD")) return std::string();
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
  wxStaticText *label = new wxStaticText(parent, wxID_ANY,
                                         wxString::FromUTF8(text.c_str()));
  label->Wrap(480);
  sizer->Insert(0, label, 0, wxEXPAND | wxALL, 4);
  return label;
}
