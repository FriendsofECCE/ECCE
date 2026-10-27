#ifndef COMPAREENERGIESDIALOG_HH
#define COMPAREENERGIESDIALOG_HH

#include <string>
#include <utility>
#include <vector>

#include <wx/dialog.h>

class wxBoxSizer;
class wxButton;
class wxChoice;
class wxPanel;
class wxSpinCtrl;
class wxTextCtrl;

/**
 * Tools > Compare Energies (#573, #612).  Side A and side B are each a list
 * of calculations with + or - and a coefficient; the dialog gathers them and
 * shows what sumEnergies() makes of them.
 */
class CompareEnergiesDialog : public wxDialog
{
public:
  typedef std::vector<std::pair<std::string, int> > Side;  ///< url, signed coefficient

  /** calcs: the project's calculations (name, url); a and b: the first terms. */
  CompareEnergiesDialog(wxWindow *parent,
                        const std::vector<std::pair<std::string, std::string> >& calcs,
                        const Side& a, const Side& b);
  std::string resultText() const;
  std::string notesText() const;
  /** What Copy puts on the clipboard: full precision. */
  std::string copyText() const;

private:
  struct Row
  {
    wxPanel *panel;
    wxChoice *sign, *calc;
    wxSpinCtrl *count;
  };
  struct SideBox
  {
    wxWindow *host;
    wxBoxSizer *rows;
    std::vector<Row> items;
  };

  void addRow(int side, const std::string& url, int coeff);
  void removeRow(int side, wxPanel *panel);
  Side termsOf(int side) const;
  void update();

  std::vector<std::pair<std::string, std::string> > p_calcs;
  SideBox p_side[2];
  wxTextCtrl *p_results, *p_notes;
  std::string p_copy;
};

#endif
