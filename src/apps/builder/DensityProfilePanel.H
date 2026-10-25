#ifndef DENSITYPROFILEPANEL__H
#define DENSITYPROFILEPANEL__H

#include <memory>
#include <string>
#include <vector>

#include "tdat/DensityPlane.H"
#include "tdat/LineProfile.H"
#include "tdat/RadialDist.H"
#include "PropertyPanel.H"

class wxCheckBox;
class wxCheckListBox;
class wxChoice;
class wxSpinCtrlDouble;
class wxStaticText;
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
class DensityProfilePanel : public PropertyPanel
{
   DECLARE_DYNAMIC_CLASS(DensityProfilePanel)

   public:
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
      virtual int minimumHeight() const { return 300; }
      virtual int openingHeight() const { return 540; }

      enum Mode { LINE, ATOM };
      enum Show { TOTAL_ALL, TOTAL_SELECTED, SELECTED, TOTAL_ONLY };

      /**
       * What the controls do, for a script or a test; false if refused.
       * Quantity names: density psi2 psi (line), radial avg (atom).
       * Show names: all totalselected selected total.
       */
      bool setMode(bool atomMode);
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

      bool savePng(const wxString& path, int width, int height);
      bool exportCsv(const wxString& path);

      /** "key value" lines for headless checks. */
      std::string state() const;

   protected:
      virtual void initialize();

   private:
      void build();
      void fillOrbitals();
      void chooseDefaults();
      void fillQuantities();
      void recompute();
      void onControl(wxCommandEvent& event);
      void syncControls();
      bool densityLike() const;

      std::unique_ptr<DensityPlane::Evaluator> p_eval;
      std::vector<std::string> p_shell;          // "2s" of an atom's occupied MOs
      std::vector<int> p_occupied;               // 1-based
      std::vector<double> p_axis;
      std::vector<RadialDist::Curve> p_shown;
      std::vector<LineProfile::Mark> p_marks;
      double p_sumError;                         // negative: not applicable
      double p_sumPeak;
      bool p_ok, p_busy, p_atomsOnly;

      wxChoice *p_mode, *p_atomA, *p_atomB, *p_centre, *p_quantity, *p_show,
               *p_spin, *p_unit;
      wxSpinCtrlDouble *p_margin;
      wxStaticText *p_labelA, *p_labelB, *p_labelCentre, *p_labelMargin;
      wxCheckListBox *p_list;
      wxCheckBox *p_combine, *p_clip;
      wxTextCtrl *p_text;
      XYPlotCanvas *p_plot;
};

#endif
