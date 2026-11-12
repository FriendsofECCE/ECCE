// Tools > QSAR... (#996): one window per folder with the descriptor Table, PCA
// and PLS tabs. The computation is ecce-qsar's; the window chooses, runs,
// explains and draws.
#ifndef QSARWINDOW_H
#define QSARWINDOW_H

#include <functional>
#include <string>
#include <vector>
#include <wx/frame.h>

#include "util/EcceURL.H"

class wxButton;
class wxCheckBox;
class wxChoice;
class wxGrid;
class wxNotebook;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;
class QsarPlot;

class QsarWindow : public wxFrame
{
public:
  /// `groups`: comma list of descriptor groups (counts, topology, models,
  /// charges, vsa, qm). `chooseFolder` returns the calculations of the folder
  /// the Organizer has selected now (empty: none).
  QsarWindow(wxWindow *parent, const std::vector<EcceURL>& calcs, const std::string& groups,
             std::function<std::vector<EcceURL>()> chooseFolder,
             std::function<void(QsarWindow *)> onDestroy);
  ~QsarWindow();

  bool ok(std::string& why) const { why = p_why; return p_ok; }
  /// The folder (path of the calculations' parent), the key of "one window per folder".
  std::string folderKey() const { return p_folder; }
  /// The descriptor table as CSV.
  std::string csv() const { return p_csvText; }
  /// Gather the table again with these groups; PCA and PLS results are cleared.
  bool setGroups(const std::string& groups, std::string& why);

  /// PCA with `scaling` (uv, pareto, ctr, none) and `components` (0: by
  /// cross-validation).
  bool runPca(const std::string& scaling, int components, std::string& why);
  /// PLS of the measured column `y`; `exclude` lists descriptor columns to leave out.
  bool runPls(const std::string& y, const std::string& scaling, int components,
              int testPercent, int permutations, std::string& why,
              const std::vector<std::string>& exclude = std::vector<std::string>());

  std::string summary() const { return p_summary; }
  std::string status() const;
  std::string resultDir() const { return p_dir; }
  bool saveResults(const std::string& dir, std::string& why) const;
  std::vector<std::string> measuredColumns() const { return p_measured; }
  /// 0 Table, 1 PCA, 2 PLS.
  void showTab(int n);

private:
  bool load(const std::vector<EcceURL>& calcs, const std::string& groups, std::string& why);
  void build();
  void fillTable();
  void refreshHeader();
  void updateCircular();
  void setStatus(const std::string& s);
  std::string run(const std::string& args, int& status) const;
  std::string failure(const std::string& what) const;
  void showNotes();

  std::vector<EcceURL> p_calcs;
  std::function<std::vector<EcceURL>()> p_choose;
  std::function<void(QsarWindow *)> p_onDestroy;
  std::string p_dir, p_table, p_csvText, p_groups, p_summary, p_why, p_folder, p_name;
  std::vector<std::string> p_head, p_measured, p_notes;
  std::vector<std::vector<std::string>> p_rows;
  bool p_ok = false;

  wxStaticText *p_header = nullptr, *p_status = nullptr;
  wxNotebook *p_book = nullptr;
  std::vector<wxCheckBox *> p_groupBoxes;
  wxGrid *p_tableGrid = nullptr;

  wxChoice *p_pcaScale = nullptr, *p_pcaComps = nullptr;
  wxStaticText *p_pcaSentence = nullptr, *p_pcaConst = nullptr;
  wxButton *p_pcaShow = nullptr;
  wxGrid *p_pcaGrid = nullptr;
  QsarPlot *p_pcaPlot = nullptr;

  wxChoice *p_plsY = nullptr, *p_plsScale = nullptr, *p_plsComps = nullptr;
  wxSpinCtrl *p_plsTest = nullptr, *p_plsPerm = nullptr;
  wxStaticText *p_plsSentence = nullptr, *p_plsConst = nullptr, *p_plsWarn = nullptr;
  wxCheckBox *p_plsLeave = nullptr;
  wxButton *p_plsShow = nullptr, *p_plsRun = nullptr;
  wxGrid *p_plsGrid = nullptr, *p_vipGrid = nullptr;
  QsarPlot *p_plsPlot = nullptr;

  std::string p_circular;  ///< descriptor that estimates the chosen property, or ""
  std::string p_pcaStatus, p_plsStatus;
};

#endif
