#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wx/process.h"
#include "wx/sstream.h"
#include "wx/gbsizer.h"

#include "util/Ecce.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/AddServerDialog.H"

namespace {
const int PAD = 5;
}


AddServerDialog::AddServerDialog(wxWindow* parent, const ServerEntry* old)
  : wxDialog(parent, wxID_ANY, old ? _("Edit Server") : _("Add Server"),
             wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
    p_proc(NULL)
{
  if (old) p_old = old->name;
  wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
  p_top = top;
  wxFlexGridSizer* grid = new wxFlexGridSizer(2, PAD, PAD);
  grid->AddGrowableCol(1);

  grid->Add(new ewxStaticText(this, wxID_ANY, _("Type:")), 0, wxALIGN_CENTER_VERTICAL);
  p_type = new ewxChoice(this, wxID_ANY);
  p_type->Append(_("SSH folder (SFTP)"));
  p_type->Append(_("ECCE server (https)"));
  grid->Add(p_type, 1, wxGROW);

  p_name = new ewxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(300, -1));
  p_host = new ewxTextCtrl(this, wxID_ANY);
  p_user = new ewxTextCtrl(this, wxID_ANY);
  p_folder = new ewxTextCtrl(this, wxID_ANY);
  p_address = new ewxTextCtrl(this, wxID_ANY);
  p_host->SetHint(_("hpc.example.edu"));
  p_folder->SetHint(_("/home/me/ecce"));
  p_address->SetHint(_("https://ecce.example.edu"));

  grid->Add(new ewxStaticText(this, wxID_ANY, _("Name:")), 0, wxALIGN_CENTER_VERTICAL);
  grid->Add(p_name, 1, wxGROW);
  int n = 0, m = 0;
  struct { const wxChar* t; ewxTextCtrl* f; } ssh[] = {
    { _("Host:"), p_host }, { _("User:"), p_user }, { _("Folder:"), p_folder } };
  for (int i = 0; i < 3; i++) {
    wxWindow* l = new ewxStaticText(this, wxID_ANY, ssh[i].t);
    grid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(ssh[i].f, 1, wxGROW);
    p_sshRows[n++] = l;
    p_sshRows[n++] = ssh[i].f;
  }
  wxWindow* al = new ewxStaticText(this, wxID_ANY, _("Address:"));
  grid->Add(al, 0, wxALIGN_CENTER_VERTICAL);
  grid->Add(p_address, 1, wxGROW);
  p_eccRows[m++] = al;
  p_eccRows[m++] = p_address;
  top->Add(grid, 0, wxGROW|wxALL, PAD*2);

  p_test = new ewxButton(this, wxID_ANY, _("Test connection"));
  p_status = new ewxStaticText(this, wxID_ANY, wxEmptyString);
  wxBoxSizer* testRow = new wxBoxSizer(wxHORIZONTAL);
  testRow->Add(p_test, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, PAD);
  testRow->Add(p_status, 1, wxALIGN_CENTER_VERTICAL);
  top->Add(testRow, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, PAD*2);

  wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
  wxButton* ok = new ewxButton(this, wxID_OK, old ? _("Save") : _("Add"));
  buttons->AddButton(ok);
  buttons->AddButton(new ewxButton(this, wxID_CANCEL, _("Cancel")));
  buttons->Realize();
  ok->SetDefault();
  top->Add(buttons, 0, wxGROW|wxALL, PAD*2);
  SetSizer(top);

  int sel = 0;
  if (old) {
    sel = old->type == "ecce" ? 1 : 0;
    p_name->SetValue(wxString::FromUTF8(old->name.c_str()));
    if (sel == 1) {
      p_address->SetValue(wxString::FromUTF8(old->address.c_str()));
    } else {
      std::string u, h, f;
      if (ServerList::parseSsh(old->address, u, h, f)) {
        p_user->SetValue(wxString::FromUTF8(u.c_str()));
        p_host->SetValue(wxString::FromUTF8(h.c_str()));
        p_folder->SetValue(wxString::FromUTF8(f.c_str()));
      }
    }
  }
  p_type->SetSelection(sel);
  showType();

  p_type->Bind(wxEVT_CHOICE, &AddServerDialog::OnType, this);
  p_test->Bind(wxEVT_BUTTON, &AddServerDialog::OnTest, this);
  Bind(wxEVT_END_PROCESS, &AddServerDialog::OnTestDone, this);
  ok->Bind(wxEVT_BUTTON, &AddServerDialog::OnOk, this);
}


void AddServerDialog::showType()
{
  bool ssh = p_type->GetSelection() == 0;
  for (int i = 0; i < 6; i++) p_sshRows[i]->Show(ssh);
  for (int i = 0; i < 2; i++) p_eccRows[i]->Show(!ssh);
  // An SSH folder cannot be reached from here until the backend exists.
  p_test->Enable(!ssh);
  p_test->SetToolTip(ssh ? _("Available when SSH folders are supported.")
                         : _("Check that the server answers and its certificate."));
  p_status->SetLabel(wxEmptyString);
  Layout();
  Fit();
}


void AddServerDialog::OnType(wxCommandEvent&)
{
  showType();
}


bool AddServerDialog::collect(ServerEntry& e) const
{
  e.name = p_name->GetValue().Trim().Trim(false).ToStdString();
  if (p_type->GetSelection() == 0) {
    e.type = "ssh";
    wxString f = p_folder->GetValue().Trim().Trim(false);
    e.address = ServerList::sshAddress(
        p_user->GetValue().Trim().Trim(false).ToStdString(),
        p_host->GetValue().Trim().Trim(false).ToStdString(), f.ToStdString());
  } else {
    e.type = "ecce";
    e.address = p_address->GetValue().Trim().Trim(false).ToStdString();
  }
  return true;
}


void AddServerDialog::OnTest(wxCommandEvent&)
{
  if (p_proc) return;
  std::string a = p_address->GetValue().Trim().Trim(false).ToStdString();
  if (a.empty()) {
    p_status->SetLabel(_("Type the address of the server first."));
    return;
  }
  wxString home = wxString::FromUTF8(Ecce::ecceHome());
#ifdef __WXMSW__
  wxString cmd = "\"" + home + "/python/python3w.exe\" \"" + home +
                 "/bin/ecce-first-start\" --test-server \"" +
                 wxString::FromUTF8(a.c_str()) + "\"";
#else
  wxString cmd = "\"" + home + "/bin/ecce-first-start\" --test-server \"" +
                 wxString::FromUTF8(a.c_str()) + "\"";
#endif
  p_proc = new wxProcess(this);
  p_proc->Redirect();
  if (wxExecute(cmd, wxEXEC_ASYNC|wxEXEC_HIDE_CONSOLE, p_proc) == 0) {
    delete p_proc;
    p_proc = NULL;
    p_status->SetLabel(_("The connection test could not be started."));
    return;
  }
  p_test->Disable();
  p_status->SetLabel(_("Testing..."));
}


void AddServerDialog::OnTestDone(wxProcessEvent&)
{
  if (!p_proc) return;
  wxString out;
  wxInputStream* in = p_proc->GetInputStream();
  if (in) {
    wxStringOutputStream sos(&out);
    in->Read(sos);
  }
  delete p_proc;
  p_proc = NULL;
  p_test->Enable(p_type->GetSelection() == 1);
  p_status->SetLabel(out.Trim().IsEmpty() ? wxString(_("No answer.")) : out.Trim());
  Layout();
  Fit();
}


void AddServerDialog::OnOk(wxCommandEvent&)
{
  ServerEntry e;
  collect(e);
  std::string err = ServerList::replace(p_old, e);
  if (!err.empty()) {
    p_status->SetLabel(wxString::FromUTF8(err.c_str()));
    Layout();
    Fit();
    return;
  }
  p_entry = e;
  EndModal(wxID_OK);
}
