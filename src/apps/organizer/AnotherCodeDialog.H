// New from This > In Another Code... (#1037): the code, a name ending and the
// folder for calculations made from the selected ones.
#ifndef ANOTHERCODEDIALOG_H
#define ANOTHERCODEDIALOG_H

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <wx/dialog.h>

#include "util/EcceURL.H"

class wxChoice;
class wxStaticText;
class wxTextCtrl;

class AnotherCodeDialog : public wxDialog
{
public:
  /// codes: (label, resource type name); chooser picks another folder and
  /// returns false when cancelled.
  AnotherCodeDialog(wxWindow *parent, int count,
                    const std::vector<std::pair<std::string, std::string> >& codes,
                    const EcceURL& folder,
                    std::function<bool(EcceURL&)> chooser);

  std::string typeName() const;
  std::string codeLabel() const;
  std::string suffix() const;
  EcceURL folder() const { return p_folder; }

private:
  void OnCode(wxCommandEvent&);
  void OnFolder(wxCommandEvent&);

  std::vector<std::pair<std::string, std::string> > p_codes;
  EcceURL p_folder;
  std::function<bool(EcceURL&)> p_chooser;
  wxChoice *p_code;
  wxTextCtrl *p_suffix;
  wxStaticText *p_where;
  bool p_suffixEdited = false;
};

#endif
