#ifndef FOLDERCHOOSERDIALOG_HH
#define FOLDERCHOOSERDIALOG_HH

#include <functional>
#include <string>

#include <wx/dialog.h>
#include <wx/treectrl.h>

#include "util/EcceURL.H"

class WxResourceTreeCtrl;
class wxButton;
class wxStaticText;

/// Folders only, of every tree the Organizer shows, for Move to... and
/// Copy to... (#929).  OK is enabled only where `refusal` says "".
class FolderChooserDialog : public wxDialog
{
public:
  typedef std::function<std::string(const EcceURL&)> RefusalFn;

  FolderChooserDialog(wxWindow *parent, const wxString& title,
                      const wxString& prompt, WxResourceTreeCtrl *source,
                      const EcceURL& current, RefusalFn refusal);

  EcceURL selectedUrl() const;

private:
  void fill(const wxTreeItemId& item);
  void expandTo(const EcceURL& url);
  void onExpanding(wxTreeEvent& e);
  void onSelChanged(wxTreeEvent& e);
  void onActivated(wxTreeEvent& e);
  void update();

  WxResourceTreeCtrl *p_source;
  RefusalFn p_refusal;
  wxTreeCtrl *p_tree;
  wxStaticText *p_note;
  wxButton *p_ok;
};

#endif
