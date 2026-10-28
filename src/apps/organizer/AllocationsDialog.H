#ifndef ALLOCATIONSDIALOG_HH
#define ALLOCATIONSDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

#include "comm/AllocationQuery.H"

class wxListCtrl;
class wxMouseEvent;

/**
 * Tools > Allocations (#613): each cluster account of the user's machine
 * profiles and how much of it is used. The answers are read in the
 * background over logins that are already open.
 */
class AllocationsDialog : public wxDialog
{
public:
  explicit AllocationsDialog(wxWindow *parent);

  /** Asks again (force) or shows what is known; never blocks. */
  void refresh(bool force);

  /** True while any row is waiting for an answer. */
  bool checking() const;

  /** "machine|account|used|checked", one per row, as shown. */
  std::vector<std::string> rowTexts() const;

private:
  void update(bool force);
  void onMotion(wxMouseEvent& event);

  wxListCtrl *p_list;
  std::vector<AllocationQuery::Target> p_targets;
  std::vector<AllocationQuery::Result> p_results;
  std::vector<std::string> p_tips;
};

#endif
