#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <wx/event.h>

#include "tdat/BondDistort.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "DistortBondsDialog.H"

namespace {
const int ID_ALL = wxID_HIGHEST + 6191, ID_JT = wxID_HIGHEST + 6192;

double distance(const Fragment& f, int a, int b)
{
  const double *p = f.atomRef(a)->coordinates(), *q = f.atomRef(b)->coordinates();
  return std::sqrt((p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) +
                   (p[2]-q[2])*(p[2]-q[2]));
}
}

DistortBondsDialog::DistortBondsDialog(wxWindow *parent, const Fragment& frag,
                                       int centre, std::function<void()> onApply)
  : wxDialog(parent, wxID_ANY, _("Distort Bonds"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_frag(&frag), p_sym(frag.atomRef(centre)->atomicSymbol()), p_centre(centre), p_ok(true),
    p_apply(onApply)
{
  p_nb = BondDistort::neighbours(frag, centre);
  for (size_t i = 0; i < p_nb.size(); i++) p_nbSyms.push_back(frag.atomRef(p_nb[i])->atomicSymbol());
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(new wxStaticText(this, wxID_ANY,
      wxString::Format(wxString::FromUTF8("Change in each bond to %s%d, in \xC3\x85 "
                         "(positive stretches, negative shrinks):"),
                       frag.atomRef(centre)->atomicSymbol(), centre + 1)),
      0, wxALL, 8);

  wxScrolledWindow *sc = new wxScrolledWindow(this, wxID_ANY);
  sc->SetScrollRate(0, 10);
  wxFlexGridSizer *grid = new wxFlexGridSizer(3, 6, 10);
  for (size_t i = 0; i < p_nb.size(); i++) {
    p_name.push_back(new wxStaticText(sc, wxID_ANY, ""));
    grid->Add(p_name.back(), 0, wxALIGN_CENTER_VERTICAL);
    p_len.push_back(new wxStaticText(sc, wxID_ANY, ""));
    grid->Add(p_len.back(), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(sc, wxID_ANY, "0.0");
    p_dr.push_back(t);
    grid->Add(t);
  }
  sc->SetSizer(grid);
  sc->SetMinSize(wxSize(300, std::min<int>(260, 40 + 30 * (int)p_nb.size())));
  top->Add(sc, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(new wxStaticText(this, wxID_ANY, _("All the same:")), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_all = new wxTextCtrl(this, wxID_ANY, "0.1", wxDefaultPosition, wxSize(70, -1));
  row->Add(p_all, 0, wxRIGHT, 6);
  row->Add(new wxButton(this, ID_ALL, _("Fill")));
  top->Add(row, 0, wxALL, 8);

  wxBoxSizer *jt = new wxBoxSizer(wxHORIZONTAL);
  jt->Add(new wxStaticText(this, wxID_ANY, _("Jahn-Teller (six bonds): axial")), 0,
          wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_delta = new wxTextCtrl(this, wxID_ANY, "0.1", wxDefaultPosition, wxSize(70, -1));
  jt->Add(p_delta, 0, wxRIGHT, 6);
  jt->Add(new wxStaticText(this, wxID_ANY, _("equatorial -half, axis")), 0,
          wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_axis = new wxChoice(this, wxID_ANY);
  p_axis->Append("longest"); p_axis->Append("x"); p_axis->Append("y");
  p_axis->Append("z");
  p_axis->SetSelection(0);
  jt->Add(p_axis, 0, wxRIGHT, 6);
  jt->Add(new wxButton(this, ID_JT, _("Fill")));
  top->Add(jt, 0, wxALL, 8);

  p_note = new wxStaticText(this, wxID_ANY, "");
  p_note->SetForegroundColour(wxColour(150, 0, 0));
  top->Add(p_note, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

  wxBoxSizer *btns = new wxBoxSizer(wxHORIZONTAL);
  btns->AddStretchSpacer(1);
  p_applyBtn = new wxButton(this, wxID_APPLY, _("Apply"));
  btns->Add(p_applyBtn, 0, wxRIGHT, 8);
  btns->Add(new wxButton(this, wxID_CLOSE, _("Close")));
  top->Add(btns, 0, wxEXPAND | wxALL, 8);
  setNames();
  SetSizerAndFit(top);
  Bind(wxEVT_BUTTON, &DistortBondsDialog::onApplyBtn, this, wxID_APPLY);
  Bind(wxEVT_BUTTON, &DistortBondsDialog::onCloseBtn, this, wxID_CLOSE);
  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { Destroy(); });
  Bind(wxEVT_BUTTON, &DistortBondsDialog::onAll, this, ID_ALL);
  Bind(wxEVT_BUTTON, &DistortBondsDialog::onJT, this, ID_JT);
}

// Labels hold the atom names and current bond lengths of the centre's bonds.
void DistortBondsDialog::setNames()
{
  for (size_t i = 0; i < p_nb.size(); i++) {
    p_name[i]->SetLabel(wxString::Format("%s%d",
        p_frag->atomRef(p_nb[i])->atomicSymbol(), p_nb[i] + 1));
    p_len[i]->SetLabel(wxString::Format("%.4f A",
        distance(*p_frag, p_centre, p_nb[i])));
  }
}

void DistortBondsDialog::refresh(const Fragment *frag, const std::vector<int>& sel)
{
  p_frag = frag;
  wxString why;
  // Undo rebuilds the atoms, so the centre is tracked by number and element.
  int idx = (frag && p_centre >= 0 && p_centre < (int)frag->numAtoms() &&
             p_sym == frag->atomRef(p_centre)->atomicSymbol()) ? p_centre : -1;
  if (idx < 0) {
    why = _("The atom this dialog was opened for no longer exists.");
  } else if (std::find(sel.begin(), sel.end(), idx) == sel.end()) {
    why = _("The atom this dialog was opened for is no longer selected.");
  } else {
    std::vector<int> nb = BondDistort::neighbours(*frag, idx);
    std::vector<std::string> atoms;
    for (size_t i = 0; i < nb.size(); i++) atoms.push_back(frag->atomRef(nb[i])->atomicSymbol());
    if (atoms != p_nbSyms) {
      why = _("The bonds of this atom have changed; open the dialog again.");
    } else {
      p_nb = nb;
      p_centre = idx;
      setNames();
    }
  }
  p_ok = why.empty();
  p_applyBtn->Enable(p_ok);
  p_note->SetLabel(why);
  Layout();
}

void DistortBondsDialog::fillAll(const std::string& v)
{
  p_all->SetValue(v);
  for (size_t i = 0; i < p_dr.size(); i++) p_dr[i]->SetValue(v);
}

bool DistortBondsDialog::applyEnabled() const { return p_applyBtn->IsEnabled(); }

std::string DistortBondsDialog::note() const
{
  return std::string(p_note->GetLabel().mb_str());
}

void DistortBondsDialog::onApplyBtn(wxCommandEvent&)
{
  if (p_ok && p_apply) p_apply();
}

void DistortBondsDialog::onCloseBtn(wxCommandEvent&) { Close(); }

void DistortBondsDialog::onAll(wxCommandEvent&)
{
  for (size_t i = 0; i < p_dr.size(); i++) p_dr[i]->SetValue(p_all->GetValue());
}

void DistortBondsDialog::onJT(wxCommandEvent&)
{
  std::vector<double> dr;
  if (!BondDistort::jahnTeller(*p_frag, p_centre, p_nb,
                               atof(p_delta->GetValue().mb_str()),
                               std::string(p_axis->GetStringSelection().mb_str()),
                               dr)) {
    wxMessageBox(_("The Jahn-Teller preset needs an atom with exactly six bonds."),
                 _("Distort Bonds"), wxOK, this);
    return;
  }
  for (size_t i = 0; i < dr.size(); i++)
    p_dr[i]->SetValue(wxString::Format("%g", dr[i]));
}

std::string DistortBondsDialog::list() const
{
  std::string out;
  for (size_t i = 0; i < p_nb.size(); i++) {
    double d = atof(p_dr[i]->GetValue().mb_str());
    if (d == 0.0) continue;
    char buf[64];
    snprintf(buf, sizeof buf, "%s%d:%.10g", out.empty() ? "" : ",", p_nb[i] + 1, d);
    out += buf;
  }
  return out;
}
