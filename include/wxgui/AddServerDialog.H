#ifndef ADDSERVERDIALOG_H
#define ADDSERVERDIALOG_H

#include <atomic>
#include <memory>
#include <string>
#include "wx/wx.h"
#include "util/ServerList.H"

class ewxChoice;
class ewxTextCtrl;
class ewxButton;
class ewxStaticText;
class wxProcessEvent;

// Add Server... / Edit...: an SSH folder (name, host, user, folder) or an
// ECCE server (name, address).  OK stores the entry in the user's
// ServerList (replacing oldName when editing), so Preferences and the
// Organizer share one dialog.  Test connection reaches an ECCE server the
// way the start window does (certificate trusted, else pinned).
class AddServerDialog : public wxDialog
{
public:
  // oldEntry NULL adds; otherwise the entry named oldEntry->name is edited.
  AddServerDialog(wxWindow* parent, const ServerEntry* oldEntry = NULL);
  ~AddServerDialog();

  // The entry as stored; valid after ShowModal() returned wxID_OK.
  const ServerEntry& entry() const { return p_entry; }

  // Test hooks: fill the fields as typing does; press OK.
  void fillForTest(bool ssh, const std::string& name, const std::string& a,
                   const std::string& b, const std::string& c);
  void acceptForTest();

private:
  void OnType(wxCommandEvent& event);
  void OnTest(wxCommandEvent& event);
  void OnTestDone(wxProcessEvent& event);
  void OnOk(wxCommandEvent& event);
  void testSsh(bool create);
  void showType();
  bool collect(ServerEntry& e) const;

  std::string p_old;
  ServerEntry p_entry;
  ewxChoice* p_type;
  ewxTextCtrl* p_name;
  ewxTextCtrl* p_host;
  ewxTextCtrl* p_user;
  ewxTextCtrl* p_folder;
  ewxTextCtrl* p_address;
  ewxButton* p_test;
  ewxStaticText* p_status;
  wxProcess* p_proc;
  std::shared_ptr<std::atomic<bool> > p_alive;   // a login thread may outlive the dialog
  wxBoxSizer* p_top;
  wxWindow* p_sshRows[6];
  wxWindow* p_eccRows[2];
};

#endif
