#ifndef COMPAREFORCESDIALOG_H
#define COMPAREFORCESDIALOG_H

#include <wx/dialog.h>
#include "tdat/ForceComparison.H"

class wxListCtrl;
class wxStaticText;

class CompareForcesDialog : public wxDialog {
public:
  CompareForcesDialog(wxWindow *parent, const std::string& a, const std::string& b);
  std::string resultText() const;
  std::string copyText() const { return p_text; }
private:
  std::string p_text;
  wxListCtrl *p_table;
  wxStaticText *p_notes;
};

#endif
