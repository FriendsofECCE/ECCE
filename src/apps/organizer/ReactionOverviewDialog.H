#ifndef REACTIONOVERVIEWDIALOG_HH
#define REACTIONOVERVIEWDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxTextCtrl;
class XYPlotCanvas;

/**
 * Compare Energies > Reaction overview (#709): dE_el and dG per reaction
 * with the spread of dG over the treatments as an error bar.  The numbers
 * are those of `ecce-thermo --reactions`; the dialog only draws them.
 */
class ReactionOverviewDialog : public wxDialog
{
public:
  /** lines: one reaction per line, "a, 2*b -> c" with calculation URLs. */
  ReactionOverviewDialog(wxWindow *parent, const std::string& lines);

  /** Test hooks. */
  void setTreatment(const std::string& cli);
  /** One line per chart point: "curve index reaction x y lo hi" (kJ/mol). */
  std::string chartText() const;
  /** What a click on reaction n (1-based) lists. */
  std::string clickText(int n);
  bool savePng(const std::string& file) const;
  bool saveCsv(const std::string& file) const;

private:
  struct Mode { std::string species; double freq, S, Sshare, G, Gshare; };
  struct Row
  {
    std::string text, status, reason;
    double dE = 0, dG = 0, spread = 0, lo = 0, hi = 0;
    std::vector<Mode> modes;
  };
  void update();
  void showReaction(int n);

  wxTextCtrl *p_input, *p_detail;
  wxChoice *p_treatment;
  XYPlotCanvas *p_plot;
  std::vector<Row> p_rows;
  std::vector<int> p_rowOfPoint;   ///< chart point -> p_rows index
  std::string p_csv, p_error;
};

#endif
