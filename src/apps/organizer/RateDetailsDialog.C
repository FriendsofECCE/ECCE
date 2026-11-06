#include "RateDetailsDialog.H"

#include <fstream>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/font.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "RateBox.H"

using std::string;

RateDetailsDialog::RateDetailsDialog(wxWindow *parent, const RateRequest& req)
  : wxDialog(parent, wxID_ANY, _("Rate constants over temperature"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_req(req)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  auto field = [&](const wxString& label, const char *value) {
    bar->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(70, -1));
    bar->Add(t, 0, wxRIGHT, 12);
    return t;
  };
  p_low = field(_("From T / K"), "250");
  p_high = field(_("to"), "400");
  p_step = field(_("step"), "25");
  wxButton *go = new wxButton(this, wxID_ANY, _("Update"));
  go->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { update(); });
  bar->Add(go, 0);
  top->Add(bar, 0, wxALL, 8);
  p_text = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(760, 380),
                          wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_text->SetFont(wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE)));
  top->Add(p_text, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *csv = new wxButton(this, wxID_ANY, _("Export CSV..."));
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportCsv(); });
  buttons->Add(csv, 0);
  buttons->AddStretchSpacer();
  buttons->Add(new wxButton(this, wxID_CLOSE, _("Close")));
  top->Add(buttons, 0, wxEXPAND | wxALL, 8);
  SetSizerAndFit(top);
  SetEscapeId(wxID_CLOSE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
  update();
}

void RateDetailsDialog::update()
{
  p_req.temperatures = string(p_low->GetValue().ToUTF8()) + ":" +
                       string(p_high->GetValue().ToUTF8()) + ":" +
                       string(p_step->GetValue().ToUTF8());
  p_result = runRate(p_req);
  p_text->SetValue(wxString::FromUTF8(
      (p_result.ok ? rateDetailsText(p_result) : p_result.error + "\n").c_str()));
}

void RateDetailsDialog::exportCsv()
{
  if (!p_result.ok) return;
  wxFileDialog d(this, _("Export rate constants"), "", "rates.csv", "*.csv",
                 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (d.ShowModal() != wxID_OK) return;
  std::ofstream f(string(d.GetPath().ToUTF8()).c_str());
  f << rateDetailsCsv(p_result);
}
