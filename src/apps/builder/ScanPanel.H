#ifndef SCANPANEL__H
#define SCANPANEL__H

#include <string>
#include <vector>
#include "PropertyPanel.H"

class wxChoice;
class wxGrid;
class XYPlotCanvas;

/**
 * Relaxed scan (#393): energy of each optimised scan point, relative to the
 * lowest, against the scanned coordinate.
 */
class ScanPanel : public PropertyPanel
{
   DECLARE_DYNAMIC_CLASS(ScanPanel)

   public:
      ScanPanel();
      ScanPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ScanPanel");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ScanPanel");
      virtual ~ScanPanel();

      virtual int minimumHeight() const { return 320; }

      /** Number of scan points shown. */
      int points() const { return (int)p_coord.size(); }

      /** The scan as CSV; false if the file cannot be written. */
      bool exportCsv(const wxString& path, bool kcal);

   protected:
      void fill();
      void OnUnit(wxCommandEvent& event);
      void OnExport(wxCommandEvent& event);

   private:
      std::vector<double> p_coord;
      std::vector<double> p_hartree;
      std::string p_coordUnit;
      wxChoice *p_unit;
      XYPlotCanvas *p_plot;
      wxGrid *p_grid;
};

/**
 * CSV text: '#' header lines, then coordinate, absolute energy in hartree
 * and relative energy in kJ/mol or kcal/mol.  Numbers use the C locale.
 */
std::string scanCsv(const std::vector<double>& coord,
                    const std::vector<double>& hartree,
                    const std::string& coordUnit, bool kcal,
                    const std::string& calculation);

#endif
