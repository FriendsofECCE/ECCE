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
class wxChoice;
class wxListCtrl;
class wxSizer;
class wxSpinCtrlDouble;
class wxHtmlWindow;
class wxStaticText;
class wxTextCtrl;
class XYPlotCanvas;
class OpdosProgress;
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
      bool setLineShape(bool lorentzian);
      bool setWindow(double from, double to);                       // eV, both plots; to <= from refused
      bool resetWindow();
      bool clickEnergy(double ev);                                  // as a click on a peak
      bool showListedMo(int row);     // as a double-click on row (0-based) of the MO list
      bool savePng(const wxString& path, int width, int height);
      bool exportCsv(const wxString& path);
      /** "key value" lines for headless checks. */
      std::string state() const;

   protected:
      virtual void initialize();
      virtual void eventMCB(const ecce::Event&) {}

   private:
      void build();
      void recomputeOrbitals(OpdosProgress *progress = nullptr);   // S (once) and the per-MO numbers (slow)
      void applyWindow(bool remember);
      void showExplanation(double occOp);
      void redraw();                 // broadening and plot (fast)
      void listMos(double ev);
      void setText(const wxString& head);
      static std::string atomText(const std::vector<int>& atoms);

      std::shared_ptr<const resultsh5::Model> p_model;
      fragorb::DosResult p_all, p_part[2];
      bool p_ok = false, p_unres = false, p_haveAll = false;
      std::string p_error;
      struct ListedMo { int mo, spin; };
      std::vector<ListedMo> p_listed;   // rows of p_moList
      wxString p_drawn, p_normNote;
      std::vector<int> p_a, p_b;     // 0-based
      wxString p_head, p_clicked, p_headPlain;
      std::vector<double> p_S;       // overlap matrix, the same for every A/B choice
      bool p_haveS = false, p_busy = false, p_winOn = false;

      wxTextCtrl *p_atomsA = nullptr, *p_atomsB = nullptr, *p_text = nullptr;
      wxButton *p_useA = nullptr, *p_useB = nullptr;
      wxHtmlWindow *p_html = nullptr;
      wxSpinCtrlDouble *p_from = nullptr, *p_to = nullptr;
      wxSpinCtrlDouble *p_width = nullptr;
      wxCheckBox *p_sticks = nullptr, *p_occ = nullptr, *p_int = nullptr;
      wxChoice *p_shape = nullptr;
      wxListCtrl *p_moList = nullptr;
      wxSizer *p_listRow = nullptr;
      XYPlotCanvas *p_plot = nullptr, *p_intPlot = nullptr;
};

#endif
