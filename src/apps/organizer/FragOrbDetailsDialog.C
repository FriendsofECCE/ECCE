#include "FragOrbDetailsDialog.H"

#include <cstdio>
#include <fstream>
#include <sstream>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/font.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "FragOrbBox.H"
#include "XYPlotCanvas.H"

using std::string;
using std::vector;

namespace {
wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }
}

FragOrbDetailsDialog::FragOrbDetailsDialog(wxWindow *parent, const string& complex,
                                           const vector<string>& fragments,
                                           const vector<string>& names, const string& heading)
  : wxDialog(parent, wxID_ANY, _("Fragment orbitals"), wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_names(names)
{
  p_result = runFragOrb(complex, fragments);
  p_dos = runFragDos(complex, fragments);

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxStaticText *title = new wxStaticText(this, wxID_ANY, U(heading));
  wxFont bold = title->GetFont();
  bold.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(bold);
  top->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, 10);

  wxBoxSizer *body = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);
  left->Add(new wxStaticText(this, wxID_ANY,
      _("Complex MOs, highest first: energy, occupation, the part outside the span of the FOs,\n"
        "and the Mulliken share of each FO level (degenerate FOs of a fragment together)")),
      0, wxBOTTOM, 4);
  p_text = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(620, 520),
                          wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_text->SetFont(wxFont(wxFontInfo(9).Family(wxFONTFAMILY_TELETYPE)));
  left->Add(p_text, 1, wxEXPAND);
  body->Add(left, 1, wxEXPAND | wxRIGHT, 8);

  wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);
  p_pdos = new XYPlotCanvas(this);
  p_pdos->SetMinSize(wxSize(560, 250));
  p_opdos = new XYPlotCanvas(this);
  p_opdos->SetMinSize(wxSize(560, 250));
  right->Add(p_pdos, 1, wxEXPAND | wxBOTTOM, 6);
  right->Add(p_opdos, 1, wxEXPAND);
  body->Add(right, 1, wxEXPAND);
  top->Add(body, 1, wxEXPAND | wxALL, 10);

  // Status in its own line, under the content and above the buttons.
  p_status = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_status, 0, wxLEFT | wxRIGHT, 10);

  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *csv = new wxButton(this, wxID_ANY, _("Export CSV..."));
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportCsv(); });
  csv->Enable(p_result.ok);
  buttons->Add(csv, 0);
  buttons->AddStretchSpacer();
  buttons->Add(new wxButton(this, wxID_CLOSE, _("Close")));
  top->Add(buttons, 0, wxEXPAND | wxALL, 10);

  string status;
  if (p_result.ok) {
    const string text = fragOrbDetailsText(p_result, p_names);
    p_text->SetValue(U(text));
    // The list runs from the highest virtual down; open it with the first
    // occupied MO mid-window. ShowPosition scrolls just far enough to show a
    // position, so aim at a line some way below it.
    size_t lineStart = 0, at = 0;
    int below = -1;
    for (size_t i = 0; i <= text.size(); i++) {
      if (i < text.size() && text[i] != '\n') continue;
      int n; double e, occ;
      if (below < 0 && sscanf(text.c_str() + lineStart, "%d %lf %lf", &n, &e, &occ) == 3 && occ > 0)
        below = 0;
      if (below >= 0 && below++ == 14) { at = lineStart; break; }
      lineStart = i + 1;
    }
    if (at > 0) p_text->ShowPosition(U(text.substr(0, at)).length());
  }
  else status = "Composition: " + p_result.error;

  // A name of the CLI (its folder name) as the box shows it.
  auto shown = [this](const string& raw) {
    for (size_t k = 0; k < p_result.fragments.size() && k < p_names.size(); k++)
      if (p_result.fragments[k] == raw) return p_names[k];
    return raw;
  };
  if (p_dos.ok) {
    p_pdos->addCurve(p_dos.energy, p_dos.tdos, _("total"), (int)p_dos.sets.size(),
                     XYPlotCanvas::LINE);
    // The total in the text colour, so the fragments keep the palette's first colours.
    p_pdos->setCurveColour(0, p_pdos->palette().text);
    for (size_t s = 0; s < p_dos.sets.size(); s++)
      p_pdos->addCurve(p_dos.energy, p_dos.pdos[s], U(shown(p_dos.sets[s])), (int)s,
                       XYPlotCanvas::LINE);
    p_pdos->setTitle(_("Partial density of states (PDOS)"));
    p_pdos->setShowKey(true);
    for (size_t p = 0; p < p_dos.pairs.size(); p++) {
      string a = p_dos.pairs[p], b;
      for (size_t i = 0; i < p_dos.sets.size(); i++)
        for (size_t k = 0; k < p_dos.sets.size(); k++)
          if (i != k && a == p_dos.sets[i] + "-" + p_dos.sets[k]) {
            b = shown(p_dos.sets[i]) + " \xE2\x80\x93 " + shown(p_dos.sets[k]);
          }
      p_opdos->addCurve(p_dos.energy, p_dos.opdos[p], U(b.empty() ? a : b), (int)(p_dos.sets.size() + 1 + p),
                        XYPlotCanvas::LINE);
    }
    p_opdos->setTitle(_("Overlap population DOS (OPDOS): > 0 bonding, < 0 antibonding"));
    p_opdos->setShowKey(true);
    p_opdos->setZeroLine(true);
    for (XYPlotCanvas *c : {p_pdos, p_opdos}) {
      c->setXAxisLabel(_("Energy / eV"));
      c->setYAxisLabel(_("DOS / eV\xE2\x81\xBB\xC2\xB9"));
      c->addVLine(p_dos.fermi, _("HOMO/LUMO midpoint"));
    }
    char line[160];
    string ov;
    for (size_t p = 0; p < p_dos.pairs.size(); p++) {
      snprintf(line, sizeof line, "%.3f", p_dos.occupiedOverlap[p]);
      ov += (p ? ", " : "") + string(line);
    }
    status += (status.empty() ? "" : "  ") + string("Gaussian broadening, FWHM 0.3 eV; occupied overlap population ") + ov + ".";
  } else {
    p_pdos->setMessage(U("No density of states: " + p_dos.error));
    p_opdos->setMessage(U("No density of states: " + p_dos.error));
  }
  p_status->SetLabel(U(status));

  SetSizerAndFit(top);
  SetEscapeId(wxID_CLOSE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (IsModal()) EndModal(wxID_CLOSE); else Destroy();
  }, wxID_CLOSE);
}

string FragOrbDetailsDialog::report() const
{
  std::ostringstream o;
  o << "table-lines=" << (p_text->GetNumberOfLines()) << " pdos-curves=" << p_pdos->curveCount()
    << " opdos-curves=" << p_opdos->curveCount() << " points="
    << (p_pdos->curveCount() ? p_pdos->pointCount(0) : 0)
    << " status=" << string(p_status->GetLabel().ToUTF8());
  return o.str();
}

bool FragOrbDetailsDialog::saveCsv(const string& file) const
{
  if (!p_result.ok) return false;
  std::ofstream f(file.c_str());
  f << fragOrbCsv(p_result, p_names);
  return (bool)f;
}

void FragOrbDetailsDialog::exportCsv()
{
  wxFileDialog d(this, _("Export fragment-orbital shares"), "", "fragment-orbitals.csv", "*.csv",
                 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (d.ShowModal() != wxID_OK) return;
  saveCsv(string(d.GetPath().ToUTF8()));
}
