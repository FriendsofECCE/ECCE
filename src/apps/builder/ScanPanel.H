#ifndef SCANPANEL__H
#define SCANPANEL__H

#include <string>
#include <vector>
#include "VizPropertyPanel.H"

class wxChoice;
class wxGrid;
class wxGridEvent;
class wxStaticText;
class PlaybackControl;
class PlaybackExtras;
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
      int clickPoint(int index) { return selectPoint(index); }

      /** Step the playback control: "first", "prev", "next", "last"; the point now selected. */
      int step(const std::string& which);

      /** Playback delay (ms per step) and Loop, as the controls do. For scripts and tests. */
      void setPlaybackSpeed(const std::string& name);
      std::string playbackSpeed() const;
      void setPlaybackLoop(bool on);
      bool playbackLoop() const;

      /** The points shown by <ticks> playback ticks from the first, without timers. */
      std::vector<int> playOrder(int ticks);

      /** The selected point (plot marker, table row, playback), -1 if none. */
      int selectedPoint() const { return p_selected; }

      /** What a click on table row <index> does. */
      void selectRow(int index);

      /** The plot marker's point, -1 if none. */
      int markerPoint() const;

      /** The row selected in the table, -1 if none. */
      int selectedRow() const;

      /** The status line next to the playback buttons. */
      wxString status() const;

      /** New calculation from point <index> (default: the clicked one); URL or "". */
      std::string newCalcFromPoint(int index = -1);

      /** The scan as CSV; false if the file cannot be written. */
      bool exportCsv(const wxString& path, bool kcal);

      /** Rebuilds from the calculation's current scan properties. */
      virtual void refresh();

   protected:
      void fill();
      void OnUnit(wxCommandEvent& event);
      void OnExport(wxCommandEvent& event);
      void OnNewCalc(wxCommandEvent& event);
      void OnRow(wxGridEvent& event);
      void OnPlayback(wxCommandEvent& event);
      virtual void receiveFocus();
      virtual void loseFocus();
      int showPoint(int index);
      /** Show point <index> and move the plot marker, table row and playback to it. */
      int selectPoint(int index);
      void setStatus(const wxString& text);

      DECLARE_EVENT_TABLE()

   private:
      std::vector<double> p_coord;
      std::vector<double> p_hartree;
      std::string p_coordUnit;
      std::vector<int> p_frame; // plot point -> SCANGEOM frame, -1 if none
      int p_selected;           // last clicked point, -1 if none
      wxChoice *p_unit;
      XYPlotCanvas *p_plot;
      wxGrid *p_grid;
      wxStaticText *p_status;
      PlaybackControl *p_playback;
      PlaybackExtras *p_extras;
      int p_delay;
      bool p_syncing;           // selectPoint is moving the row; ignore its event
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
