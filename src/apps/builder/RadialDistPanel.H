#ifndef RADIALDISTPANEL__H
#define RADIALDISTPANEL__H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "tdat/DensityPlane.H"
#include "tdat/RadialDist.H"
#include "PropertyPanel.H"

class wxButton;
class wxCheckBox;
class wxCheckListBox;
class wxChoice;
class wxStaticText;
class wxTextCtrl;
class XYPlotCanvas;

/**
 * Radial distributions about an atom or a point: P(r) of chosen
 * orbitals and of the total density, overlaid, so that shells, penetration
 * and shielding can be read off.  The numbers come from RadialDist, from
 * the stored MO coefficients and basis, so it works for every code whose
 * MOs ECCE reads; this class only gathers the options and draws.
 */
class RadialDistPanel : public PropertyPanel
{
   DECLARE_DYNAMIC_CLASS(RadialDistPanel)

   public:
      RadialDistPanel();
      RadialDistPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Radial Distribution");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Radial Distribution");
      virtual ~RadialDistPanel();

      virtual bool isRelevant(IPropCalculation *propCalc) const;
      virtual int minimumHeight() const { return 300; }
      virtual int openingHeight() const { return 520; }

      /** What the controls do, for a script or a test; false if refused. */
      bool setCentre(int atom);                       // 1-based, as labelled
      bool setOrbitals(const std::vector<int>& mos);  // exactly these are ticked
      bool setTotal(bool on, bool perElectron = false);
      bool setBeta(bool beta);
      bool setBohr(bool bohr);

      bool savePng(const wxString& path, int width, int height);
      bool exportCsv(const wxString& path);

      /** "key value" lines for headless checks. */
      std::string state() const;

   protected:
      virtual void initialize();

   private:
      struct Row { int mo; std::string label; };

      void build();
      void fillOrbitals();
      void chooseDefaults();
      void recompute();
      void onControl(wxCommandEvent& event);
      std::vector<RadialDist::Spec> wanted() const;
      std::string key(const RadialDist::Spec& s) const;
      void centreOf(double c[3]) const;
      std::vector<std::string> shellLabels(bool beta) const;

      std::unique_ptr<DensityPlane::Evaluator> p_eval;
      std::vector<std::string> p_sym, p_symBeta;   // from the code
      std::vector<std::string> p_shell, p_shellBeta;   // "2s" of an atom's occupied MOs
      std::vector<Row> p_rows;                  // the list, in order
      std::map<std::string, RadialDist::Curve> p_cache;
      std::vector<RadialDist::Curve> p_shown;
      std::vector<double> p_r;
      bool p_ok, p_busy;

      wxChoice *p_centre, *p_spin, *p_unit;
      wxCheckListBox *p_list;
      wxCheckBox *p_total, *p_perElectron;
      wxTextCtrl *p_text;
      XYPlotCanvas *p_plot;
};

#endif
