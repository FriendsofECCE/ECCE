#ifndef MOPANEL__H
#define MOPANEL__H

#include "wxgui/TearableContentProvider.H"
//  Self-contained, so this header can be included from anywhere.
#include <map>
#include <utility>
#include "wxgui/SliderCombo.H"

#include "MoGUI.H"
#include <set>
#include <string>

class ewxCheckBox;
class ewxStaticText;
class ewxTextCtrl;
class Command;

class XYPlotCanvas;
class GridDlg;
class MoCoeffs;
class MoAoBasis;
class wxGrid;
class WxColorSchemeButton;
class wxStaticText;
class wxTextCtrl;


/**
 */
class MoPanel : public MoGUI,
                public TearableContentProvider
{
  DECLARE_DYNAMIC_CLASS(MoPanel)
  DECLARE_EVENT_TABLE()

   friend class Builder;   // plotshot switches the table/plot view

   enum MenuIds {SOLID=9780, MESH, CONTOUR,
                 REDGREEN, REDBLUE, BLUEYELLOW, LIGHTDARK,
                 DEFINEGRID, HIDE, 
                 COEFFS, OPAQUE, SEMITRANSPARENT,
                 TABLE, PLOT, PLOTSYM};

  static const wxWindowID ID_SLIDER;

  public:
    //  The orbital table with a dozen rows, the composition and controls.
    virtual int openingHeight() const { return 460; }
    MoPanel();
    MoPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="MoPanelVizPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="MoPanelVizPropertyPanel");
    virtual ~MoPanel();
    int getSelectedMo() const;
    string getSelectedType() const;

    virtual void receiveFocus();
    virtual void loseFocus();

    void tellDensityMap();
    bool selectRowForMo(int number);
    void OnButtonMoComputeClick( wxCommandEvent& event );
    void OnButtonMoViewcoefClick( wxCommandEvent& event );

    //  Never narrower than its own rows: the Builder gives every property
    //  panel a flat minimum, and the dock is sized from this.
    virtual wxSize GetMinSize() const override;

   protected:
     virtual void initialize() override;
     void initialize(bool resetgrids);
     void getSurfaceColors(double& posR, double& posG, double& posB,
                           double& negR, double& negG, double& negB);
     void showCoeffs(bool popup);
   public:

     /**
      * Select an orbital by its index in ORBENG and compute its surface.
      *
      * For the MO diagram, whose levels are clickable.  Takes the
      * ORBENG index (0-based) rather than the grid row, because the
      * grid is filled in reverse and may carry a Type column and beta
      * orbitals below the alphas -- a caller outside this panel has no
      * business knowing any of that.
      *
      * @return false if there is no such orbital, or the panel has no
      *         valid basis to compute one with.
      */
     bool showOrbital(int orbengIndex);

     /**
      * The composition text of alpha orbitals given by ORBENG index (0-based),
      * as the composition line under the table: one orbital, or the mean over
      * a degenerate set.  For the MO diagram's tooltip and info line.
      */
     string compositionText(const std::vector<int>& orbengIndices);

     /**
      * The table's selection and scroll state as "key value" lines, for
      * headless checks (the Builder's "motable" scene command).
      */
     string tableState() const;

     /**
      * The ESP colour bar as drawn, "key value" lines, for headless checks
      * (the Builder's "mobar" scene command).
      */
     string espBarState() const;

     /** Choose a field type as a click in the type drop-down does. */
     bool selectFieldType(const string& name);
     bool sortByHeading(const string& heading, bool ascending);

     /** The text shown under the table: the selected MO's composition. */
     string compositionText() const { return p_compSummary; }

   protected:

     double getTransparency();
     bool espFieldSelected();
     void updateIsoValue();
     void updateIsoValue(double val);
     void updateSurface();
     void updateUIOptions();
     bool rebuildTable();   // table and graphs for the canonical or localised set
     void fillUI();
     void selectMo(int index);
     void showComposition();
     void fillCharacterColumn(bool haveTypes);
     bool ensureAoBasis();
     GridDlg *getGridDlg();
     void updateGridDlg();
     void showGridDlg();
     int  fillTable(const string& type,
                    PropVector *orbEnergy,
                    PropVector *orbOcc, 
                    PropVecString *orbSym,
                    int startRow,
                    bool showType);
     void fillGraph(bool haveType);
     void resetColorMap();

     virtual wxWindow* GetTearableContent();

     void OnMenuClick( wxCommandEvent& event );
     void OnComputeButtonClick( wxCommandEvent& event );
     void OnViewCoeffsClick( wxCommandEvent& event );
     void OnMoSelection( wxGridEvent& event );
     void OnMoLabelClick( wxGridEvent& event );
     void sortTable(int col, bool ascending);
     void showMainView();
     wxString spinDensityText();
     void OnGraphSelection(bool regular, int curve, int index);
     void OnEndSliderMotion(wxScrollEvent& event);
     void OnSliderTextEnter(wxCommandEvent& event);

     void OnGridOkButton( wxCommandEvent& event );

   private:

      //  Colour range for a potential-mapped surface.  Auto measures it
      //  from the shell being drawn; unchecking it uses the two fields,
      //  which is what makes two molecules comparable -- they are only
      //  comparable on the same scale.
      void buildEspRangeControls();
      void applyEspRange(Command *cmd);
      void onEspRangeChanged(wxCommandEvent& event);
      void onEspRangeFocusLost(wxFocusEvent& event);
      void showEspRangeControls(bool show);
      void onFieldTypeChanged(wxCommandEvent& event);
      void chooseMoFieldType();
      void rebuildEspSurface();

      //  The legend for a potential-mapped surface: the surface's own ramp,
      //  the range it is stretched over, and the density isovalue.  Shown
      //  only while such a surface is the one drawn.
      void buildEspBar();
      void refreshEspBar();
      void hideEspBar();

      //  Lay the panel out again after controls came or went, keep the
      //  selected MO in view and, for the Properties list's column, ask
      //  for the height the table needs.
      void reflow();
      bool p_espBarShown = false;

      //  Bring the selected row into the middle of the table once the
      //  table has its real, on-screen size; see selectMo().
      //  Coefficients and basis disagree in size; see MoPanel.C.
      void reportBasisMismatch(int coefWidth, int basisWidth,
                               const string& code);
      std::set<string> p_mismatchReported;   // calculation URLs told

      //  The output has fewer functions than the completed stored basis,
      //  and the stored basis as it is matches it (#265).
      void reportBasisFallback(int repairedWidth, int usedWidth);
      void buildBasisWarning();
      std::set<string> p_fallbackReported;
      ewxStaticText *p_basisWarning = NULL;

      void scrollToSelection();
      void onGridWindowSize(wxSizeEvent& event);
      void onIdle(wxIdleEvent& event);
      bool p_scrollPending = false;

      wxBoxSizer  *p_espSizer;
      ewxCheckBox *p_espAuto;
      ewxTextCtrl *p_espMin;
      ewxTextCtrl *p_espMax;
      wxBoxSizer  *p_espBarSizer = 0;
      WxColorSchemeButton *p_espBar = 0;
      wxStaticText *p_espBarMin = 0;
      wxStaticText *p_espBarMax = 0;
      wxStaticText *p_espBarInfo = 0;
      string p_espDrawnType;     // field type of the surface on screen
      string p_drawnType;        // any field type drawn; empty before one


      //  The selected MO's atomic-orbital composition (#161), from the
      //  same MoAoBasis as ecce-mocomp.  The basis and its overlap matrix
      //  cost something, so they are built on the first selection.
      wxTextCtrl      *p_compText = nullptr;
      MoAoBasis       *p_aoBasis = nullptr;
      bool             p_aoTried = false;
      string           p_aoWhy;
      vector< vector<double> > p_aoRows[2];     // alpha, beta
      string           p_compSummary;

      /* The report oriented table showing MO energies */
      wxGrid *p_mogrid;  

      /* May not have enough data to use Mo panel */
      bool p_isValid; 

      bool p_interrupt;

      /* Dialog for setting size/dimensions of grid */
      GridDlg *p_griddlg;

      /* Dialog for viewing coeffs */
      MoCoeffs *p_coeffsDlg;

      SliderCombo *p_slider;

      /* currently selected row; GetSelected* methods don't work */
      int p_selectedRow;
      int p_sortCol = -1;          // column the localised table is sorted by
      bool p_sortAscending = true;
      wxTextCtrl *p_spinText = NULL;   // the spin density view
      bool p_spinShown = false;
      //  One atom: each orbital's shell ("1s", "2p", "3s (virtual)"), alpha
      //  then beta, replacing the symmetry label; empty for a molecule.
      vector<string> p_atomLabels[2];
      string p_virtNote;          // "Virtual orbitals: ..." line, for tests
      bool p_localised = false;   // the table lists the localised orbitals

      /* The first plot; all symmetries are on same x value. */
      XYPlotCanvas *p_plotReg;

      /* The second plot; all values grouped by symmetries. */
      XYPlotCanvas *p_plotSym;

      /* map from plot selections to table rows */
      std::map<std::pair<int,int>,int> p_pointToRowReg;
      std::map<std::pair<int,int>,int> p_pointToRowSym;

};

#endif // MOPANEL_H
