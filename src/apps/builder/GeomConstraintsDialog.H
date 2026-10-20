#ifndef GEOMCONSTRAINTSDIALOG_HH
#define GEOMCONSTRAINTSDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

class Builder;
class GeomConstraint;
class wxButton;
class wxListView;
class wxListEvent;
class wxStaticText;

/**
 * Geometry constraints of the structure open in the Builder (#306).
 *
 * Constraints belong to the structure, not to a run type: they are defined
 * here at any time from the atoms selected in the viewer, kept with the
 * calculation, and used by the input only when its run type can (see
 * GeomConstraintModel::usedByRunType).  The dialog says what to do next and
 * lists the constraints made.
 */
class GeomConstraintsDialog : public wxDialog
{
public:
  GeomConstraintsDialog(Builder *builder);

  /// Redisplay for the current structure and selection.
  void refresh();

  /// Test hook (ECCE_BUILDER_SCRIPT): a button, without a click.
  /// @param what "constrain", "restrain" or "fix"
  /// @param confirm the answer to a "not bonded" question
  /// @return what the user would read in the status line
  std::string press(const std::string& what, bool confirm);
  std::string selectionText() const { return p_pickText; }
  std::string noteText() const;
  /// One line per constraint, as listed.
  std::vector<std::string> rowTexts() const;

private:
  void OnConstrain(wxCommandEvent&);
  void OnRestrain(wxCommandEvent&);
  void OnFix(wxCommandEvent&);
  void OnScan(wxCommandEvent&);
  void OnSave(wxCommandEvent&);
  void OnRemove(wxCommandEvent&);
  void OnClearAll(wxCommandEvent&);
  void OnEdit(wxCommandEvent&);
  void OnListActivated(wxListEvent&);
  void OnListSelected(wxListEvent&);
  void OnClose(wxCommandEvent&);
  void OnCloseWindow(wxCloseEvent&);

  std::string addFromSelection(bool restraint, bool ask, bool confirm);
  std::string fixSelection();
  void done(const std::string& message);
  void updateButtons();
  void fillList();
  /// scanOn: open with Scan ticked (the Scan button).
  void editRow(long row, bool scanOn = false);

  Builder *p_builder;
  wxStaticText *p_pick;
  wxStaticText *p_status;
  wxStaticText *p_note;
  wxListView *p_list;
  wxButton *p_constrain;
  wxButton *p_restrain;
  wxButton *p_fix;
  wxButton *p_scan;
  wxButton *p_save;
  wxButton *p_edit;
  wxButton *p_remove;
  wxButton *p_clear;
  std::string p_pickText;
  std::vector<GeomConstraint*> p_rows;   ///< the constraint behind each row
};

#endif
