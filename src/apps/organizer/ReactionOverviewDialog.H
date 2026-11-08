#ifndef REACTIONOVERVIEWDIALOG_HH
#define REACTIONOVERVIEWDIALOG_HH

#include <map>
#include <string>
#include <vector>

#include <wx/dialog.h>

#include "util/RateRun.H"
#include "EnergyProfile.H"

class wxChoice;
class wxStaticText;
class wxTextCtrl;
class wxPanel;
class wxFlexGridSizer;
class wxCollapsiblePane;

/**
 * Compare Energies > Reaction overview (#709): dE_el and dG per reaction as
 * copyable text, and an energy profile for each reaction with a [ts].  The
 * numbers are those of `ecce-thermo --reactions` and `ecce-rate`; the
 * dialog only lays them out.
 */
class wxHtmlWindow;
class ReactionOverviewDialog : public wxDialog
{
public:
  /** lines: one reaction per line, "a, 2*b -> [ts] -> c", the stored form of
   *  the Reactions table (the user picks, never types it); species are
   *  calculation names in the folder `base` (a URL), or full URLs/paths.
   *  Blank `lines`: the table is filled from the workflow `base` is. */
  ReactionOverviewDialog(wxWindow *parent, const std::string& lines,
                         const std::string& base = std::string());

  /** Test hooks: the stored reaction lines, one per line; add a line as the
   *  picker would; remove row n (1-based); the picker's TS choices as
   *  "name:imaginary:enabled" lines. */
  std::string reactionLines() const;
  void addReactionLine(const std::string& line);
  void removeReaction(int n);
  std::string tsChoices() const;
  /** One line per stored reaction: "n balanced" or the "Not balanced: ..." message. */
  std::string balanceText() const;
  /** Test hooks. */
  void setTreatment(const std::string& cli);
  /** One line per drawn reaction: "reaction n dE dG lo hi" (kJ/mol). */
  std::string tableText() const;
  /** One line per profile level: "profile p n reactant|ts|product dE dG". */
  std::string profileText() const;
  /** Every profile's export layout, lines prefixed "layout p ". */
  std::string layoutText() const;
  /** Reaction n's (1-based) part of the text: numbers, soft modes, rates. */
  std::string clickText(int n);
  /** All profiles, one under the other; false when there is none. */
  bool savePng(const std::string& file) const;
  bool saveCsv(const std::string& file) const;
  /** One line per transition-state reaction: its barrier and k at the dialog's T. */
  std::string rateText() const;
  void setTemperature(const std::string& kelvin);
  /** Test hook: shown state and size of chart and choices, the text control's flags; "shot" gets a PNG of the dialog. */
  std::string layoutReport(const std::string& shot);

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
    std::string names[3];            ///< reactants, TS, products, shortened
  };
  void update();
  std::string p_base;
  std::string reactionText(int n) const;
  void showProfiles();
  void showResults();
  /** "" when balanced (or unknown), else "Not balanced: left ..., right ...". */
  std::string balanceOf(const std::string& line) const;
  mutable std::map<std::string, std::string> p_calcUrls;
  wxPanel *p_results;
  wxFlexGridSizer *p_resSizer;
  wxStaticText *p_units;

  void rebuildTable();
  void pick(int row);                 ///< the picker for a new reaction (-1) or row
  void autoFill();
  std::vector<std::string> p_lines;
  int p_sel = -1;
  wxPanel *p_table;
  wxFlexGridSizer *p_tableSizer;
  wxStaticText *p_note;
  wxWindow *p_editButton;
  wxCollapsiblePane *p_pane;
  wxHtmlWindow *p_detail;
  std::string p_detailHtml;
  std::string buildDetailHtml(const std::string& treatment, const std::string& tText) const;
public:
  /** The formatted Details page as HTML (display only; the plain text is reactionText). */
  std::string detailHtml() const { return p_detailHtml; }
private:
  wxChoice *p_treatment, *p_profileChoice;
  wxStaticText *p_profileLabel;
  wxWindow *p_pngButton;
  wxTextCtrl *p_T;
  EnergyProfileCanvas *p_plot;
  std::vector<Row> p_rows;
  std::vector<EnergyProfileData> p_profiles;
  std::vector<int> p_profileRow;   ///< profile -> p_rows index
  std::string p_csv, p_error;
  bool p_anyTs = false;
  std::string rateLines(int n) const;
};

#endif
