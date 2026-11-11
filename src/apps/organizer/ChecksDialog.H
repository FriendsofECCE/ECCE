#ifndef CHECKSDIALOG_HH
#define CHECKSDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

#include "util/EcceURL.H"

class wxChoice;
class wxGrid;
class wxGridEvent;

/// With `qsarGroups` (a comma list for ecce-qsar table --groups) it shows the
/// QSAR descriptor table instead (#961).
/// The Checks table (#615) of the calculations under a folder: one sortable
/// row each, problems highlighted, excluded rows greyed.  The values are
/// those of ecce-si-export --checks, so the grid and the exports agree.
class ChecksDialog : public wxDialog
{
public:
  /// `rank` shows the Rank table (#703) instead: dE and dG against the lowest.
  ChecksDialog(wxWindow *parent, const std::vector<EcceURL>& calcs, bool rank = false,
               const std::string *qsarGroups = nullptr);

  /// False, with `why`, when the table could not be made.
  bool ok(std::string& why) const { why = p_why; return p_ok; }
  /// Compare Energies only: take dE and dG against calculation `name`
  /// ("" = the lowest) and refill the table; false, with `why`, on failure.
  bool setReference(const std::string& name, std::string& why);
  /// The rows as shown (current sort) in the script's CSV form.
  std::string csv() const;
  std::vector<std::string> referenceChoices() const;
  std::string selectedReference() const;

private:
  bool load(wxWindow *parent, bool withProgress);
  void fill();
  void syncReference();
  void onSort(wxGridEvent& e);
  void exportAs(const std::string& format);

  std::vector<EcceURL> p_calcs;
  std::vector<std::string> p_head;
  std::vector<std::vector<std::string>> p_rows;
  wxGrid *p_grid = nullptr;
  wxChoice *p_reference = nullptr;
  int p_sortCol = -1;
  bool p_ascending = true;
  bool p_rank = false;
  bool p_qsar = false;        ///< the QSAR descriptor table (#961)
  std::string p_groups;
  bool p_ok = false;
  std::string p_why;
  std::string p_ref;
};

/// The calculations at or below each of `roots`, in tree order.
std::vector<EcceURL> calculationsUnder(const std::vector<EcceURL>& roots);

#endif
