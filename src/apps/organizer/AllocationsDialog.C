#include "AllocationsDialog.H"

#include <wx/app.h>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/weakref.h>

#include "dsm/AllocationLeft.H"
#include "wxgui/WxHelpViewer.H"

using std::string;

namespace {
const int COL_MACHINE = 0, COL_ACCOUNT = 1, COL_USED = 2, COL_CHECKED = 3;

string usedText(const AllocationQuery::Result& r)
{
  if (r.when > 0)
    return r.used.empty() ? "not recognised" : r.used;
  if (r.state == AllocationQuery::Result::Checking)
    return "checking...";
  return "not checked: not logged in to " + r.host;
}
}

AllocationsDialog::AllocationsDialog(wxWindow *parent)
  : wxDialog(parent, wxID_ANY, _("Allocations"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  p_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(720, 220),
                          wxLC_REPORT | wxLC_SINGLE_SEL);
  p_list->AppendColumn(_("Machine"), wxLIST_FORMAT_LEFT, 130);
  p_list->AppendColumn(_("Account"), wxLIST_FORMAT_LEFT, 140);
  p_list->AppendColumn(_("Used"), wxLIST_FORMAT_LEFT, 340);
  p_list->AppendColumn(_("Checked"), wxLIST_FORMAT_LEFT, 70);
  top->Add(p_list, 1, wxEXPAND | wxALL, 8);

  //  Close stays in view when the window is made small.
  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *refresh = new wxButton(this, wxID_REFRESH, _("Refresh"));
  wxButton *help = new wxButton(this, wxID_HELP, _("Help"));
  wxButton *close = new wxButton(this, wxID_CLOSE, _("Close"));
  buttons->Add(refresh, 0, wxRIGHT, 6);
  buttons->AddStretchSpacer(1);
  buttons->Add(help, 0, wxRIGHT, 6);
  buttons->Add(close, 0);
  top->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
  SetSizerAndFit(top);
  SetMinSize(wxSize(420, 200));
  close->SetDefault();

  refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->refresh(true); });
  help->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
    WxHelpViewer::show("allocations.html");
  });
  close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Destroy(); });
  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { Destroy(); });
  p_list->Bind(wxEVT_MOTION, &AllocationsDialog::onMotion, this);

  p_targets = AllocationQuery::targets();
  for (size_t i = 0; i < p_targets.size(); i++) {
    long row = p_list->InsertItem(i, wxString::FromUTF8(p_targets[i].machine.c_str()));
    p_list->SetItem(row, COL_ACCOUNT, wxString::FromUTF8(p_targets[i].account.c_str()));
  }
  if (p_targets.empty()) {
    //  Said once, in the table's place, not as a table row.
    top->Insert(0, new wxStaticText(this, wxID_ANY,
        _("No machine with an allocation command and an account was found.")),
        0, wxALL, 8);
    Layout();
  }
  update(false);
}

void AllocationsDialog::refresh(bool force)
{
  update(force);
}

void AllocationsDialog::update(bool force)
{
  wxWeakRef<AllocationsDialog> self(this);
  p_results.clear();
  p_tips.clear();
  for (size_t i = 0; i < p_targets.size(); i++) {
    AllocationQuery::Result r = AllocationQuery::check(p_targets[i], force, [self]() {
      wxTheApp->CallAfter([self]() { if (self) self->refresh(false); });
    });
    p_results.push_back(r);
    p_tips.push_back(r.when > 0 && r.used.empty() ? r.raw : string());
    p_list->SetItem(i, COL_USED, wxString::FromUTF8(usedText(r).c_str()));
    p_list->SetItem(i, COL_CHECKED, r.when > 0 ?
        wxString(AllocationLeft::checkedAt(r.when)) : wxString());
  }
}

bool AllocationsDialog::checking() const
{
  for (size_t i = 0; i < p_results.size(); i++)
    if (p_results[i].state == AllocationQuery::Result::Checking)
      return true;
  return false;
}

std::vector<string> AllocationsDialog::rowTexts() const
{
  std::vector<string> v;
  for (size_t i = 0; i < p_results.size(); i++)
    v.push_back(p_targets[i].machine + "|" + p_targets[i].account + "|" +
                usedText(p_results[i]) + "|" +
                (p_results[i].when > 0 ? AllocationLeft::checkedAt(p_results[i].when)
                                       : string()));
  return v;
}

//  The site's own line for a row ECCE could not read, instead of the table cell.
void AllocationsDialog::onMotion(wxMouseEvent& event)
{
  int flags = 0;
  long item = p_list->HitTest(event.GetPosition(), flags);
  p_list->SetToolTip(item >= 0 && (size_t)item < p_tips.size() &&
                     !p_tips[item].empty() ?
                     wxString::FromUTF8(p_tips[item].c_str()) : wxString());
  event.Skip();
}
