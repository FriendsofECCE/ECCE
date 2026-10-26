#ifndef COMPAREENERGIESDIALOG_HH
#define COMPAREENERGIESDIALOG_HH

#include <string>
#include <utility>
#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxTextCtrl;

/**
 * Tools > Compare Energies (#573).  Gathers calculation A and B (preset
 * when two were selected) and shows what compareEnergies() makes of them.
 */
class CompareEnergiesDialog : public wxDialog
{
public:
  /** calcs: the project's calculations (name, url); a and b: urls or "". */
  CompareEnergiesDialog(wxWindow *parent,
                        const std::vector<std::pair<std::string, std::string> >& calcs,
                        const std::string& a, const std::string& b);
  std::string resultText() const;
  std::string notesText() const;

private:
  void update();
  void OnChoice(wxCommandEvent&) { update(); }

  std::vector<std::pair<std::string, std::string> > p_calcs;
  wxChoice *p_a, *p_b;
  wxTextCtrl *p_results, *p_notes;
};

#endif
