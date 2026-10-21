#ifndef SCANPANEL__H
#define SCANPANEL__H

#include <string>
#include <vector>
#include "VizPropertyPanel.H"

class wxChoice;
class wxGrid;
class XYPlotCanvas;

/**
 * Relaxed scan (#393): energy of each optimised scan point, relative to the
 * lowest, against the scanned coordinate; a click on a point shows its
 * structure in the viewer, and a button starts a calculation from it (#410).
 */
class ScanPanel : public VizPropertyPanel
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
      virtual int openingHeight() const { return 400; }

      /** Number of scan points shown. */
      int points() const { return (int)p_coord.size(); }

      /**
       * What a click on plot point <index> does; returns the SCANGEOM frame
       * shown, or -1 if the point has no structure.
       */
      int clickPoint(int index) { return showPoint(index); }

      /** New calculation from point <index> (default: the clicked one); URL or "". */
      std::string newCalcFromPoint(int index = -1);

      /** The scan as CSV; false if the file cannot be written. */
      bool exportCsv(const wxString& path, bool kcal);

   protected:
      void fill();
      void OnUnit(wxCommandEvent& event);
      void OnExport(wxCommandEvent& event);
      void OnNewCalc(wxCommandEvent& event);
      int showPoint(int index);

   private:
      std::vector<double> p_coord;
      std::vector<double> p_hartree;
      std::string p_coordUnit;
      std::vector<int> p_frame; // plot point -> SCANGEOM frame, -1 if none
      int p_selected;           // last clicked point, -1 if none
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
