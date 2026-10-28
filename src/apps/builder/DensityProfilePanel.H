#ifndef DENSITYPROFILEPANEL__H
#define DENSITYPROFILEPANEL__H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "tdat/DensityPlane.H"
#include "tdat/LineProfile.H"
#include "tdat/RadialDist.H"
#include "util/Listener.H"
#include "VizPropertyPanel.H"

class wxButton;
class wxCheckBox;
class wxCheckListBox;
class wxChoice;
class wxPanel;
class wxSpinCtrlDouble;
class wxSplitterWindow;
class wxProgressDialog;
class wxStaticText;
class wxTimer;
class wxTimerEvent;
class wxTextCtrl;
class XYPlotCanvas;

/**
 * The electron density and the orbitals as a plot against one length:
 * along a line through two atoms (LineProfile), or averaged over spheres
 * about one atom (RadialDist).  The main view is the total and each
 * orbital's occupation-weighted contribution, labelled by shell for an atom;
 * the numbers come from the stored MO coefficients and basis, so it works
 * for every code whose MOs ECCE reads.  This class only gathers the options
 * and draws.
 */
class DensityProfilePanel : public VizPropertyPanel, public ecce::Listener
{
   DECLARE_DYNAMIC_CLASS(DensityProfilePanel)

   public:
      //  The scan/IRC point whose orbitals the calculation returns changed.
      void orbitalPointChanged();
      DensityProfilePanel();
      DensityProfilePanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Density Profile");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Density Profile");
      virtual ~DensityProfilePanel();

      virtual bool isRelevant(IPropCalculation *propCalc) const;
      //  Only listens to the viewer's selection; the MOs panel keeps the surface.
      virtual bool drawsInViewer() const { return false; }
      virtual int minimumHeight() const { return 400; }
      virtual int openingHeight() const { return 540; }

      enum Mode { LINE, ATOM };
      enum Show { TOTAL_ALL, TOTAL_SELECTED, SELECTED, TOTAL_ONLY };

      /**
       * What the controls do, for a script or a test; false if refused.
       * Quantity names: density psi2 psi (line), radial avg (atom).
       * Show names: all totalselected selected total.
       */
      bool setMode(bool atomMode);
      /** Atoms picked in the viewer: two = a line, one = about that atom. */
      bool pickAtoms(const std::vector<int>& atoms);
      bool setAtoms(int a, int b);                    // the line, 1-based
      bool setMargin(double angstrom);
      bool setCentre(int atom);                       // 1-based
      bool setQuantity(const std::string& name);
      bool setShow(const std::string& name);
      bool setOrbitals(const std::vector<int>& mos);  // exactly these ticked
      bool setSpin(const std::string& name);          // alpha beta both
      bool setBohr(bool bohr);
      bool setCombine(bool on);
      bool setClip(bool on);                          // clip the nuclear peaks
      bool setXAxis(const std::string& name);         // linear sqrt log (atom mode)
      bool setYAxis(const std::string& name);         // linear log
      bool setShift(double angstrom, bool outOfPlane);  // sideways, line mode
      bool zoom(const std::string& what);             // whole fit in out
      bool setMoreOpen(bool open);                    // the folded controls

      /** The orbital chosen in the MO list or MO Diagram: shown alone. */
      void followMo(int mo, bool beta);

      bool savePng(const wxString& path, int width, int height);
      bool exportCsv(const wxString& path);

      /** Blocks until a running average has been drawn (hooks and tests only). */
      void waitForCompute();

      /** "key value" lines for headless checks. */
      std::string state() const;

   protected:
      virtual void initialize();
      virtual void eventMCB(const ecce::Event& event);

   private:
      void build();
      void fillOrbitals();
      void chooseDefaults();
      void fillQuantities(bool resetShow = false);
      void recompute();
      //  The average about an atom runs on a worker thread; its result is
      //  drawn only while its job is still the current one.
      struct AverageJob;
      void startAverage(const double centre[3], const RadialDist::Options& opt,
            const std::vector<RadialDist::Spec>& specs,
            std::function<void(std::vector<RadialDist::Curve>&)> finish);
      void cancelAverage();
      void finishAverage(const std::shared_ptr<AverageJob>& job);
      static void deliver(std::shared_ptr<DensityProfilePanel*> alive,
            std::shared_ptr<AverageJob> job);
      void onTimer(wxTimerEvent& event);
      void rankLargest(bool bohr);
      void onControl(wxCommandEvent& event);
      void syncControls();
      bool densityLike() const;
      std::string showName() const;
      void selectShow(const std::string& name);
      void ensureSelection();
      void remember() const;

      std::shared_ptr<DensityPlane::Evaluator> p_eval;   // the worker keeps its own reference
      std::shared_ptr<AverageJob> p_job;
      std::shared_ptr<DensityProfilePanel*> p_alive;     // nulled in the destructor
      std::unique_ptr<wxTimer> p_timer;
      std::unique_ptr<wxProgressDialog> p_progress;
      bool p_inProgressUpdate;
      std::vector<std::string> p_shell;          // "2s" of an atom's occupied MOs
      std::vector<std::string> p_sym;            // the code's symmetry labels
      bool tagsUnique() const { return !p_shell.empty(); }
      const std::vector<std::string>& tags() const { return p_shell.empty() ? p_sym : p_shell; }
      std::vector<int> p_occupied;               // 1-based
      std::vector<double> p_axis;
      std::vector<RadialDist::Curve> p_shown;
      std::vector<LineProfile::Mark> p_marks;
      double p_sumError;                         // negative: not applicable
      double p_sumPeak;
      bool p_ok, p_busy, p_atomsOnly;
      bool p_autoActive;                         // Total + the largest contributors
      wxString p_autoNote;
      bool p_showDensityList;                    // which Show list is in the choice
      bool p_moreOpen;

      wxChoice *p_mode, *p_atomA, *p_atomB, *p_centre, *p_quantity, *p_show,
               *p_spin, *p_unit, *p_xaxis, *p_yaxis, *p_shiftDir;
      wxSpinCtrlDouble *p_margin, *p_shift;
      wxPanel *p_more;
      wxButton *p_moreButton;
      wxSplitterWindow *p_outer, *p_inner;
      wxStaticText *p_labelXAxis, *p_labelYAxis;
      wxStaticText *p_labelA, *p_labelB, *p_labelCentre, *p_labelMargin;
      wxCheckListBox *p_list;
      wxCheckBox *p_combine, *p_clip;
      wxTextCtrl *p_text;
      XYPlotCanvas *p_plot;
};

#endif
