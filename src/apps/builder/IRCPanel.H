#ifndef IRCPANEL__H
#define IRCPANEL__H

#include <string>
#include <vector>
#include "VizPropertyPanel.H"

class wxChoice;
class wxGridEvent;
class wxRadioBox;
class wxStaticText;
class wxGrid;
class XYPlotCanvas;

/**
 * IRC path (#417): energy relative to the TS against the reaction
 * coordinate; a click on a point shows its structure in the viewer.
 */
class IRCPanel : public VizPropertyPanel
{
   DECLARE_DYNAMIC_CLASS(IRCPanel)

   public:
      IRCPanel();
      IRCPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="IRCPanel");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="IRCPanel");
      virtual ~IRCPanel();

      virtual int minimumHeight() const { return 320; }
      virtual int openingHeight() const { return 400; }

      /** Number of scan points shown. */
      int points() const { return (int)p_coord.size(); }

      /**
       * What a click on plot point <index> does; returns the IRCGEOM frame
       * shown, or -1 if the point has none (the TS, or no usable frames).
       * For scripts and tests.
       */
      int clickPoint(int index) { return selectPoint(index); }

      /** The status line under the plot. For scripts and tests. */
      std::string statusText() const;

      /** Graph or table, as the Show choice does. For scripts and tests. */
      void setTableView(bool table) { showView(table); }

      /** New calculation from point <index> (default: the clicked one); URL or "". */
      std::string newCalcFromPoint(int index = -1);

      /** The path as CSV; false if the file cannot be written. */
      bool exportCsv(const wxString& path, bool kcal);

      /** Rebuilds from the calculation's current IRC properties. */
      virtual void refresh();

   protected:
      void fill();
      void OnUnit(wxCommandEvent& event);
      void OnExport(wxCommandEvent& event);
      void OnNewCalc(wxCommandEvent& event);
      void OnRow(wxGridEvent& event);
      void OnView(wxCommandEvent& event);
      int selectPoint(int index);
      int showPoint(int index);
      void showView(bool table);

   private:
      std::vector<double> p_coord;
      std::vector<double> p_hartree;
      std::string p_coordUnit;
      int p_selected = -1;      // last clicked point
      std::vector<int> p_frame; // plot point -> IRCGEOM frame, -1 if none
      wxChoice *p_unit;
      XYPlotCanvas *p_plot;
      wxGrid *p_grid;
      wxRadioBox *p_view;
      wxStaticText *p_status;
      bool p_syncing = false;   // plot/table selection echo
};

/**
 * CSV text: '#' header lines, then coordinate, absolute energy in hartree
 * and energy relative to the TS (the point at coordinate 0) in kJ/mol or kcal/mol.  Numbers use the C locale.
 */
std::string ircCsv(const std::vector<double>& coord,
                   const std::vector<double>& hartree,
                   const std::string& coordUnit, bool kcal,
                   const std::string& calculation);

/**
 * Plot point -> IRCGEOM frame for a Gaussian path (frames: FORWARD points
 * nearest the TS first, then REVERSE).  All -1 unless the frame count is
 * the number of points off the TS; the TS point never has a frame.
 */
std::vector<int> ircFrameMap(const std::vector<double>& coord, int frames);

#endif
