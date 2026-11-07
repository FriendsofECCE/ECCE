#include "FragOrbDetailsDialog.H"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/wrapsizer.h>
#include <wx/font.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "FragOrbBox.H"
#include "FragOrbDiagram.H"
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
    p_complex(complex), p_fragments(fragments), p_names(names)
{
  p_result = runFragOrb(complex, fragments);
  p_dos = runFragDos(complex, fragments);

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxStaticText *title = new wxStaticText(this, wxID_ANY, U(heading));
  wxFont bold = title->GetFont();
  bold.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(bold);
  top->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, 10);

  // Table and pages share the width through a sash, so the window fits a
  // 1366-pixel screen; the table and the diagram scroll inside their panes.
  wxSplitterWindow *split = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                 wxSP_LIVE_UPDATE | wxSP_3DSASH);
  wxPanel *lpane = new wxPanel(split);
  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);
  wxStaticText *caption = new wxStaticText(lpane, wxID_ANY,
      _("Complex MOs, highest first: energy, occupation, the part outside the span of the FOs, "
        "and the Mulliken share of each FO level (degenerate FOs of a fragment together)"));
  caption->Wrap(340);
  left->Add(caption, 0, wxBOTTOM, 4);
  p_text = new wxTextCtrl(lpane, wxID_ANY, "", wxDefaultPosition, wxSize(340, 400),
                          wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_text->SetFont(wxFont(wxFontInfo(9).Family(wxFONTFAMILY_TELETYPE)));
  left->Add(p_text, 1, wxEXPAND);
  lpane->SetSizer(left);

  // Diagram and densities of states as pages beside the table.
  p_book = new wxNotebook(split, wxID_ANY);
  wxPanel *dpage = new wxPanel(p_book);
  wxBoxSizer *dsizer = new wxBoxSizer(wxVERTICAL);
  wxWrapSizer *controls = new wxWrapSizer(wxHORIZONTAL);
  auto labelled = [&](const wxString& label, wxWindow *w) {
    wxBoxSizer *pair = new wxBoxSizer(wxHORIZONTAL);
    pair->Add(new wxStaticText(dpage, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    pair->Add(w, 0);
    controls->Add(pair, 0, wxRIGHT | wxBOTTOM, 8);
  };
  p_occupied = new wxSpinCtrl(dpage, wxID_ANY, "4", wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 1, 99, 4);
  labelled(_("Occupied levels"), p_occupied);
  p_virtual = new wxSpinCtrl(dpage, wxID_ANY, "4", wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 0, 99, 4);
  labelled(_("Empty levels"), p_virtual);
  p_threshold = new wxSpinCtrl(dpage, wxID_ANY, "10", wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 1, 50, 10);
  labelled(wxString::FromUTF8("Line at \xE2\x89\xA5 (%)"), p_threshold);
  p_spin = new wxChoice(dpage, wxID_ANY);
  p_spin->Append(wxString::FromUTF8("\xCE\xB1 orbitals"));
  p_spin->Append(wxString::FromUTF8("\xCE\xB2 orbitals"));
  p_spin->SetSelection(0);
  controls->Add(p_spin, 0, wxRIGHT | wxBOTTOM, 8);
  wxButton *savePng = new wxButton(dpage, wxID_ANY, _("Save PNG..."));
  controls->Add(savePng, 0, wxBOTTOM, 8);
  dsizer->Add(controls, 0, wxEXPAND | wxALL, 4);
  p_canvas = new FragDiagramCanvas(dpage);
  p_canvas->SetMinSize(wxSize(420, 300));
  dsizer->Add(p_canvas, 1, wxEXPAND);
  wxStaticText *hint = new wxStaticText(dpage, wxID_ANY,
      _("Point at or click a level for its composition (the text can be copied):"));
  dsizer->Add(hint, 0, wxTOP | wxLEFT, 4);
  p_level = new wxTextCtrl(dpage, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 130),
                           wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_level->SetFont(wxFont(wxFontInfo(9).Family(wxFONTFAMILY_TELETYPE)));
  dsizer->Add(p_level, 0, wxEXPAND | wxTOP, 2);
  dpage->SetSizer(dsizer);
  p_book->AddPage(dpage, _("Diagram"), true);

  wxPanel *spage = new wxPanel(p_book);
  wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);
  p_pdos = new XYPlotCanvas(spage);
  p_pdos->SetMinSize(wxSize(400, 200));
  p_opdos = new XYPlotCanvas(spage);
  p_opdos->SetMinSize(wxSize(400, 200));
  right->Add(p_pdos, 1, wxEXPAND | wxBOTTOM, 6);
  right->Add(p_opdos, 1, wxEXPAND);
  spage->SetSizer(right);
  p_book->AddPage(spage, _("PDOS and OPDOS"));
  split->SetMinimumPaneSize(260);
  split->SetSashGravity(0.35);
  split->SplitVertically(lpane, p_book, 380);
  top->Add(split, 1, wxEXPAND | wxALL, 10);

  // Status in its own line, under the content and above the buttons.
  p_status = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                              wxST_ELLIPSIZE_END);
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

  p_canvas->onLevel([this](int level, bool) { showLevel(level); });
  for (wxSpinCtrl *c : {p_occupied, p_virtual, p_threshold})
    c->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { runDiagram(); });
  p_spin->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { showDiagram(); });
  savePng->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { saveDiagramPng(); });
  runDiagram();

  SetSizerAndFit(top);
  // Open at most 1200 x 800, smaller on a smaller screen.
  const wxSize screen = wxGetDisplaySize();
  SetSize(std::min(1200, screen.x - 40), std::min(800, screen.y - 60));
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


void FragOrbDetailsDialog::runDiagram()
{
  p_diagram = runFragDiagram(p_complex, p_fragments, p_names, p_occupied->GetValue(),
                             p_virtual->GetValue(), p_threshold->GetValue() / 100.0);
  p_spin->Show(p_diagram.ok && p_diagram.channels.size() > 1);
  if (p_spin->GetSelection() >= (int)p_diagram.channels.size()) p_spin->SetSelection(0);
  p_spin->GetParent()->Layout();
  showDiagram();
}


void FragOrbDetailsDialog::showDiagram()
{
  FragDiagramView v;
  for (size_t k = 0; k < p_names.size(); k++) v.heading += (k ? " | " : "") + p_names[k];
  const size_t ch = p_spin->GetSelection() < 0 ? 0 : (size_t)p_spin->GetSelection();
  p_canvas->setDiagram(p_diagram, ch, v);
  p_level->SetValue(p_diagram.ok ? "" : U("No diagram: " + p_diagram.error));
}


void FragOrbDetailsDialog::showLevel(int level)
{
  const size_t ch = p_spin->GetSelection() < 0 ? 0 : (size_t)p_spin->GetSelection();
  p_level->SetValue(U(fragDiagramLevelText(p_diagram, ch, level)));
}


void FragOrbDetailsDialog::saveDiagramPng()
{
  wxFileDialog d(this, _("Save the diagram"), "", "fragment-orbital-diagram.png", "*.png",
                 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (d.ShowModal() != wxID_OK) return;
  FragDiagramView v;
  for (size_t k = 0; k < p_names.size(); k++) v.heading += (k ? " | " : "") + p_names[k];
  const size_t ch = p_spin->GetSelection() < 0 ? 0 : (size_t)p_spin->GetSelection();
  saveFragDiagramPng(p_diagram, ch, v, string(d.GetPath().ToUTF8()));
}


string FragOrbDetailsDialog::diagramReport(int occupied, int virtuals, int click, const string& png)
{
  p_book->SetSelection(0);
  if (occupied > 0 || virtuals >= 0) {
    if (occupied > 0) p_occupied->SetValue(occupied);
    if (virtuals >= 0) p_virtual->SetValue(virtuals);
    runDiagram();
  }
  for (int i = 0; i < 6; i++) wxYield();
  string out = p_diagram.ok ? "ok" : "error: " + p_diagram.error;
  const size_t ch = p_spin->GetSelection() < 0 ? 0 : (size_t)p_spin->GetSelection();
  out += "\n" + p_canvas->layout().dump(p_diagram);
  if (click >= 0) {
    showLevel(click);
    out += "click " + std::to_string(click) + "\n" + string(p_level->GetValue().ToUTF8()) + "\n";
  }
  if (!png.empty()) {
    FragDiagramView v;
    for (size_t k = 0; k < p_names.size(); k++) v.heading += (k ? " | " : "") + p_names[k];
    const FragDiagramLayout e = exportFragDiagram(p_diagram, ch, v);
    std::istringstream in(e.dump(p_diagram));
    for (string l; std::getline(in, l); ) out += "export " + l + "\n";
    out += saveFragDiagramPng(p_diagram, ch, v, png) ? "png saved\n" : "png failed\n";
  }
  return out;
}


string FragOrbDetailsDialog::fitReport() const
{
  // Every shown control must lie inside the window's client area.
  std::ostringstream o;
  const wxSize c = GetClientSize();
  o << "size=" << GetSize().x << "x" << GetSize().y << " min=" << GetMinSize().x << "x"
    << GetMinSize().y << " best=" << GetBestSize().x << "x" << GetBestSize().y;
  int outside = 0;
  std::function<void(const wxWindow*)> walk = [&](const wxWindow *w) {
    for (const wxWindow *k : w->GetChildren()) {
      if (!k->IsShownOnScreen() || k->IsTopLevel()) continue;
      const wxPoint p = ScreenToClient(k->GetScreenPosition());
      const wxSize s = k->GetSize();
      if (p.x < 0 || p.x + s.x > c.x + 1) {
        outside++;
        o << " outside=" << string(wxString(k->GetClassInfo()->GetClassName()).ToUTF8()) << "@" << p.x << "+" << s.x;
      }
      walk(k);
    }
  };
  walk(this);
  o << " cut=" << outside;
  return o.str();
}
