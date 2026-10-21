#ifndef PULLPANEL__H
#define PULLPANEL__H

#include <string>
#include "PropertyPanel.H"
#include "PullAnalysis.H"

class wxStaticText;
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

   protected:
      void OnExport(wxCommandEvent& event);

   private:
      PullSeries p_series;
      XYPlotCanvas *p_force;
      XYPlotCanvas *p_dist;
      wxStaticText *p_text;
};

#endif
