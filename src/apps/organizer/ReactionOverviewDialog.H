#ifndef REACTIONOVERVIEWDIALOG_HH
#define REACTIONOVERVIEWDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

#include "util/RateRun.H"
#include "EnergyProfile.H"

class wxChoice;
class wxStaticText;
class wxTextCtrl;

/**
 * Compare Energies > Reaction overview (#709): dE_el and dG per reaction as
 * copyable text, and an energy profile for each reaction with a [ts].  The
 * numbers are those of `ecce-thermo --reactions` and `ecce-rate`; the
 * dialog only lays them out.
 */
class ReactionOverviewDialog : public wxDialog
{
public:
  /** lines: one reaction per line, "a, 2*b -> c" with calculation URLs. */
  ReactionOverviewDialog(wxWindow *parent, const std::string& lines);

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
  std::string reactionText(int n) const;
  void showProfiles();

  wxTextCtrl *p_input, *p_detail;
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
