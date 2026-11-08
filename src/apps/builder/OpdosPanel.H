#ifndef OPDOSPANEL_HH
#define OPDOSPANEL_HH

#include <memory>
#include <string>
#include <vector>
#include "tdat/FragmentAnalysis.H"
#include "util/Listener.H"
#include "VizPropertyPanel.H"

class wxButton;
class wxCheckBox;
class wxSpinCtrlDouble;
class wxStaticText;
class wxTextCtrl;
class XYPlotCanvas;
namespace resultsh5 { struct Model; }

/**
 * TDOS, PDOS of two atom sets A and B and the overlap-population DOS
 * OPDOS(A,B) of one calculation (#171).  The numbers come from
 * fragorb::dosOrbitals/dosCurves (wx-free, `ecce-fragorb dos`); this class
 * collects the options and draws.
 */
class OpdosPanel : public VizPropertyPanel, public ecce::Listener
{
   DECLARE_DYNAMIC_CLASS(OpdosPanel)

   public:
      OpdosPanel();
      OpdosPanel(IPropCalculation *calculation, wxWindow *parent,
            wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
            const wxString& name="Density of States (OPDOS)");
      virtual bool Create(IPropCalculation *calculation, wxWindow *parent,
            wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
            const wxString& name="Density of States (OPDOS)");
      virtual ~OpdosPanel();

      virtual bool isRelevant(IPropCalculation *propCalc) const;
      virtual bool drawsInViewer() const { return false; }
      virtual int minimumHeight() const { return 460; }
      virtual int openingHeight() const { return 600; }

      //  What the controls do, for a script or a test; false if refused.
      bool setAtoms(const std::string& a, const std::string& b);   // "1,2" "3-8", 1-based
      bool useSelection(int which);                                 // 0 = A, 1 = B
      bool setWidth(double ev);
      bool setSticks(bool on);
      bool setOccupiedOnly(bool on);
      bool setIntegrated(bool on);
      bool clickEnergy(double ev);                                  // as a click on a peak
      bool savePng(const wxString& path, int width, int height);
      bool exportCsv(const wxString& path);
      /** "key value" lines for headless checks. */
      std::string state() const;

   protected:
      virtual void initialize();
      virtual void eventMCB(const ecce::Event&) {}

   private:
      void build();
      void recomputeOrbitals();      // S and the per-MO numbers (slow)
      void redraw();                 // broadening and plot (fast)
      void listMos(double ev);
      void setText(const wxString& head);
      static std::string atomText(const std::vector<int>& atoms);

      std::shared_ptr<const resultsh5::Model> p_model;
      fragorb::DosResult p_all, p_part[2];
      bool p_ok = false, p_unres = false, p_haveAll = false;
      std::string p_error;
      std::vector<int> p_a, p_b;     // 0-based
      wxString p_head, p_clicked;

      wxTextCtrl *p_atomsA = nullptr, *p_atomsB = nullptr, *p_text = nullptr;
      wxSpinCtrlDouble *p_width = nullptr;
      wxCheckBox *p_sticks = nullptr, *p_occ = nullptr, *p_int = nullptr;
      XYPlotCanvas *p_plot = nullptr, *p_intPlot = nullptr;
};

#endif
