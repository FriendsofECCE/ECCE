#ifndef REACTIONOVERVIEWDIALOG_HH
#define REACTIONOVERVIEWDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

#include "util/RateRun.H"

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
  /** One line per transition-state reaction: its barrier and k at the dialog's T. */
  std::string rateText() const;
  void setTemperature(const std::string& kelvin);

private:
  struct Mode { std::string species; double freq, S, Sshare, G, Gshare; };
  struct Row
  {
    std::string text, status, reason;
    double dE = 0, dG = 0, spread = 0, lo = 0, hi = 0;
    std::vector<Mode> modes;
    // A reaction written 'a -> [ts] -> c': the barrier from the second thermo
    // row 'a -> ts', the rate constants from ecce-rate.
    bool hasTs = false;
    double dEact = 0, dGact = 0;
    RateResult rate;
    std::string rateError, thermoText, rawCsv;
  };
  void update();
  void showReaction(int n);

  wxTextCtrl *p_input, *p_detail;
  wxChoice *p_treatment;
  wxTextCtrl *p_T;
  XYPlotCanvas *p_plot;
  std::vector<Row> p_rows;
  std::vector<int> p_rowOfPoint;   ///< chart point -> p_rows index
  std::string p_csv, p_error;
  bool p_anyTs = false;
  std::string rateLines(int n) const;
};

#endif
