// Tools > Batch Add from SMILES... (#961): a table of names, SMILES and
// measured values, its columns chosen, previewed row by row.  The
// calculations are made by SmilesBatch::make (CalcMgr::batchSmiles).
#ifndef BATCHSMILESDIALOG_H
#define BATCHSMILESDIALOG_H

#include <string>
#include <utility>
#include <vector>
#include <wx/dialog.h>

#include "dsm/SmilesBatch.H"

class wxButton;
class wxCheckBox;
class wxCheckListBox;
class wxChoice;
class wxListCtrl;
class wxStaticText;
class wxTextCtrl;

class BatchSmilesDialog : public wxDialog
{
public:
  /// templates: (label, calculation URL) of the folder's calculations.
  BatchSmilesDialog(wxWindow *parent, const std::string& folderName,
                    const std::vector<std::pair<std::string, std::string> >& templates);

  /// Load a table file into the text box (Open File... does this).
  bool loadFile(const std::string& path, std::string& why);
  void setText(const std::string& text);

  /// The rows as Create would make them; empty URL: the default template.
  std::vector<SmilesBatch::Row> rows() const { return p_rows; }
  std::string templateUrl() const;
  /// Make again the calculations of the same name that were never launched.
  bool replace() const;
  SmilesBatch::Columns columns() const;
  /// "name=... smiles=... responses=a,b rows=N problems=M" for the tests.
  std::string describe() const;

private:
  void OnOpen(wxCommandEvent&);
  void OnTextChanged(wxCommandEvent&);
  void OnColumnChanged(wxCommandEvent&);
  void parse();
  void fillColumnChoices();
  void refreshPreview();

  wxTextCtrl *p_text;
  wxChoice *p_name, *p_smiles, *p_template;
  wxCheckListBox *p_responses;
  wxCheckBox *p_replace;
  wxListCtrl *p_list;
  wxStaticText *p_summary;
  wxButton *p_create;
  SmilesBatch::Table p_table;
  bool p_parsed = false;
  std::vector<SmilesBatch::Row> p_rows;
  std::vector<std::pair<std::string, std::string> > p_templates;
};

#endif
