#ifndef GMXANALYSISPANEL__H
#define GMXANALYSISPANEL__H

#include <memory>
#include <string>
#include <vector>

#include "PropertyPanel.H"
#include "util/MiniJson.H"

class wxButton;
class wxCheckListBox;
class wxChoice;
class wxComboBox;
class wxFlexGridSizer;
class wxGauge;
class wxStaticText;
class wxTextCtrl;
class wxTimer;
class wxTimerEvent;
class XYPlotCanvas;

/**
 * MD Analysis of a GROMACS calculation (#877): gmx rdf, energy, rms, gyrate,
 * density and hbond on the run's stored traj.xtc, topol.tpr and ener.edr.
 * The numbers are ecce-gmxanalysis's (scripts/ecce_gmxanalysis.py, --json);
 * the panel picks the groups, runs the command in the background and draws.
 */
class GmxAnalysisPanel : public PropertyPanel
{
   DECLARE_DYNAMIC_CLASS(GmxAnalysisPanel)

   public:
      GmxAnalysisPanel();
      GmxAnalysisPanel(IPropCalculation *calculation, wxWindow *parent,
                 wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
                 const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
                 const wxString& name="GmxAnalysisPanel");
      virtual ~GmxAnalysisPanel();
      virtual bool Create(IPropCalculation *calculation, wxWindow *parent,
                 wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
                 const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
                 const wxString& name="GmxAnalysisPanel");
      virtual bool isRelevant(IPropCalculation *propCalc) const;

      //  Script and test hooks (BuilderScript "gmxa ...").
      /** rdf, energy, rms, gyrate, density or hbond; false for another. */
      bool setAnalysis(const std::string& key);
      std::string analysis() const;
      /** ref, sel, group, fit, rmax, bin, axis, slices, dens, terms, plot,
          begin, end: "ok" or why not. */
      std::string setField(const std::string& name, const std::string& value);
      /** Starts the command; false (status says why) if it cannot. */
      bool run();
      bool busy() const { return (bool)p_job; }
      bool exportCsv(const wxString& path);
      /** The curves drawn: one line per curve, "name|x y;x y;...". */
      std::string plotDump() const;
      wxString status() const;
      wxString resultsText() const;
      /** The --json the last run printed. */
      const std::string& lastJson() const { return p_lastJson; }
      /** The choices the group pickers offer, "|"-separated. */
      std::string groupChoices() const;
      std::string energyTerms() const;
      XYPlotCanvas *plot() const { return p_plot; }

      struct Job;

   private:
      void build();
      std::string calcFolder();
      std::string command(const std::vector<std::string>& args) const;
      void startJob(const std::vector<std::string>& args, const wxString& what,
                    int kind);
      static void deliver(std::shared_ptr<GmxAnalysisPanel*> alive,
                          std::shared_ptr<Job> job);
      void finish(std::shared_ptr<Job> job);
      void fillPickers(const MiniJson& groups);
      void showFields();
      void redraw();
      std::vector<std::string> runArgs(std::string& problem);
      void setStatus(const wxString& text);
      void onAnalysis(wxCommandEvent&);
      void onRun(wxCommandEvent&);
      void onExport(wxCommandEvent&);
      void onPlotChoice(wxCommandEvent&);
      void onTimer(wxTimerEvent&);

      wxChoice *p_analysis;
      wxFlexGridSizer *p_fields;
      wxComboBox *p_ref, *p_sel, *p_group;
      wxTextCtrl *p_rmax, *p_bin, *p_slices, *p_begin, *p_end;
      wxChoice *p_axis, *p_dens, *p_show;
      wxCheckListBox *p_terms;
      wxButton *p_run, *p_export;
      XYPlotCanvas *p_plot;
      wxTextCtrl *p_results;
      wxStaticText *p_status, *p_showLabel;
      wxGauge *p_gauge;
      std::unique_ptr<wxTimer> p_timer;
      std::shared_ptr<Job> p_job;
      std::shared_ptr<GmxAnalysisPanel*> p_alive;
      std::string p_folder, p_scratch;
      std::string p_lastJson;
      MiniJson p_result;
      bool p_haveResult;
      long p_started;
};

#endif
