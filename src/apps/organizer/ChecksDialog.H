#ifndef CHECKSDIALOG_HH
#define CHECKSDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

#include "util/EcceURL.H"

class wxGrid;
class wxGridEvent;

/// The Checks table (#615) of the calculations under a folder: one sortable
/// row each, problems highlighted, excluded rows greyed.  The values are
/// those of ecce-si-export --checks, so the grid and the exports agree.
class ChecksDialog : public wxDialog
{
public:
  ChecksDialog(wxWindow *parent, const std::vector<EcceURL>& calcs);

  /// False, with `why`, when the table could not be made.
  bool ok(std::string& why) const { why = p_why; return p_ok; }
  /// The rows as shown (current sort) in the script's CSV form.
  std::string csv() const;

private:
  void fill();
  void onSort(wxGridEvent& e);
  void exportAs(const std::string& format);

  std::vector<EcceURL> p_calcs;
  std::vector<std::string> p_head;
  std::vector<std::vector<std::string>> p_rows;
  wxGrid *p_grid = nullptr;
  int p_sortCol = -1;
  bool p_ascending = true;
  bool p_ok = false;
  std::string p_why;
};

/// The calculations at or below each of `roots`, in tree order.
std::vector<EcceURL> calculationsUnder(const std::vector<EcceURL>& roots);

#endif
