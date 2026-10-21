#ifndef IRCPANEL__H
#define IRCPANEL__H

#include <string>
#include <vector>
#include "VizPropertyPanel.H"

class wxChoice;
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
      int clickPoint(int index) { return showPoint(index); }

      /** The path as CSV; false if the file cannot be written. */
      bool exportCsv(const wxString& path, bool kcal);

   protected:
      void fill();
      void OnUnit(wxCommandEvent& event);
      void OnExport(wxCommandEvent& event);
      int showPoint(int index);

   private:
      std::vector<double> p_coord;
      std::vector<double> p_hartree;
      std::string p_coordUnit;
      std::vector<int> p_frame; // plot point -> IRCGEOM frame, -1 if none
      wxChoice *p_unit;
      XYPlotCanvas *p_plot;
      wxGrid *p_grid;
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
