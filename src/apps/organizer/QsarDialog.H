// Tools > QSAR Analysis... (#961): PCA (and PLS) of the QSAR descriptor table
// of a folder, run by ecce-qsar; the dialog only chooses, runs and draws.
#ifndef QSARDIALOG_H
#define QSARDIALOG_H

#include <string>
#include <vector>
#include <wx/dialog.h>

#include "util/EcceURL.H"

class wxChoice;
class wxSpinCtrl;
class wxTextCtrl;
class QsarPlot;

class QsarDialog : public wxDialog
{
public:
  QsarDialog(wxWindow *parent, const std::vector<EcceURL>& calcs);
  ~QsarDialog();

  /// False, with `why`, when the descriptor table could not be made.
  bool ok(std::string& why) const { why = p_why; return p_ok; }

  /// PCA with `scaling` (uv, pareto, ctr, none) and `components` (0: by
  /// cross-validation); the text report and plot are refreshed.
  bool runPca(const std::string& scaling, int components, std::string& why);

  /// ecce-qsar's report of the last run.
  std::string summary() const { return p_summary; }
  /// Directory with pca_scores.csv, pca_loadings.csv, pca_components.csv.
  std::string resultDir() const { return p_dir; }
  /// Copy the result files and the table into `dir`.
  bool saveResults(const std::string& dir, std::string& why) const;
  std::vector<std::string> measuredColumns() const { return p_measured; }

private:
  void OnRun(wxCommandEvent&);
  void OnSave(wxCommandEvent&);
  std::string run(const std::string& args, int& status) const;

  std::vector<EcceURL> p_calcs;
  std::string p_dir, p_table, p_summary, p_why;
  std::vector<std::string> p_measured;
  bool p_ok = false;
  wxChoice *p_scale = nullptr;
  wxSpinCtrl *p_comps = nullptr;
  wxTextCtrl *p_text = nullptr;
  QsarPlot *p_plot = nullptr;
};

#endif
