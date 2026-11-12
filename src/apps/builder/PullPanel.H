#ifndef PULLPANEL__H
#define PULLPANEL__H

#include <string>
#include "PropertyPanel.H"
#include "PullAnalysis.H"
#include "util/MiniJson.H"

class wxStaticText;
class wxSimplebook;
class wxToggleButton;
class wxTextCtrl;
class wxCheckBox;
class wxDialog;
class wxListCtrl;
class XYPlotCanvas;

/**
 * Pull run: force against time and the centre-of-mass distance against
 * time (two XYPlotCanvas), a one-line summary and Export .csv.
 */
class PullPanel : public PropertyPanel
{
   DECLARE_DYNAMIC_CLASS(PullPanel)

   public:
      PullPanel();
      PullPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PullPanel");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PullPanel");
      virtual ~PullPanel();

      int points() const { return (int)p_series.size(); }
      const PullSeries& series() const { return p_series; }

      /** The run as CSV; false if the file cannot be written. */
      bool exportCsv(const wxString& path);

      //  Umbrella sampling (#762); each is a button, and a script hook.
      bool umbrellaFind(double spacing, const wxString& from, const wxString& to);
      bool umbrellaCreate(int eqSteps, int prodSteps);
      bool umbrellaWham(bool jacobian);
      void clipAudit();
      bool umbrellaDetails(bool show);   // the Details... window
      wxString umbrellaDetailsText() const { return p_details; }   // both pages, under ECCE_CLIP_AUDIT
      void setNoise(const std::string& kj) { if (p_noise) p_noise->SetValue(wxString::FromUTF8(kj.c_str())); }
      bool umbrellaExport(const wxString& path);
      /** The PMF plot as a PNG on a white background. */
      bool umbrellaShot(const wxString& path, int width = 900, int height = 600);
      wxString umbrellaState() const;
      void showUmbrella();
      /** Open MD Analysis on this run with analysis `key` (rms, gyrate). */
      bool openAnalysis(const std::string& key);

   protected:
      void OnExport(wxCommandEvent& event);
      void OnFind(wxCommandEvent& event);
      void OnCreate(wxCommandEvent& event);
      void OnWham(wxCommandEvent& event);
      void OnUmbrellaExport(wxCommandEvent& event);
      void OnTab(wxCommandEvent& event);
      void OnAnalysis(wxCommandEvent& event);
      void showPage(int n);

   private:
      PullSeries p_series;
      XYPlotCanvas *p_force;
      XYPlotCanvas *p_dist;
      wxStaticText *p_text;
      wxSimplebook *p_book;
      wxToggleButton *p_tab[2];
      wxTextCtrl *p_spacing, *p_from, *p_to, *p_eq, *p_prod, *p_noise;
      wxDialog *p_detailsDlg;      // Details...: the full WHAM text
      wxTextCtrl *p_detailsText, *p_info;
      wxString p_details;
      wxCheckBox *p_jacobian;
      double noiseValue() const;  // 0 = kT
      wxListCtrl *p_windows;
      XYPlotCanvas *p_pmf, *p_hist;
      MiniJson p_wham;
      bool p_haveWham;
      void umbrellaSay(const wxString& text);
};

#endif
