#ifndef MOPANEL__H
#define MOPANEL__H

#include "wxgui/TearableContentProvider.H"
//  Self-contained, so this header can be included from anywhere.
#include <map>
#include <utility>
#include "wxgui/SliderCombo.H"
#include "tdat/SpinTables.H"
#include "tdat/MoAoBasis.H"
#include "tdat/ShellLabel.H"

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
     /// Menu ids of the colour presets and surface types, for SurfaceStyle.
     static int presetId(int i) { return REDGREEN + i; }
     static int surfaceId(int i) { return SOLID + i; }
     /// The global surface settings changed (Display panel): redraw.
     void surfaceStyleChanged();
    //  The scan/IRC point whose orbitals the calculation returns changed.
    void orbitalPointChanged();
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
    /** ORBENG index (0-based) of the selected alpha orbital, -1 if none or not alpha. */
    int selectedAlphaIndex() const;
    string getSelectedType() const;

    virtual void receiveFocus();
    virtual void loseFocus();

    void tellDensityMap();
    bool selectRowForMo(int number);
    void OnButtonMoComputeClick( wxCommandEvent& event );
    void OnButtonMoViewcoefClick( wxCommandEvent& event );

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
     /** Is MO `orbeng` (0-based) a core orbital?  Its label ("F 1s") and atom (#827). */
     bool coreOrbital(int orbeng, string& label, int& atom);

     /**
      * The table's selection and scroll state as "key value" lines, for
      * headless checks (the Builder's "motable" scene command).
      */
     string tableState() const;
     //  Show on structure for the spin density view (the hook's entry).
     void paintSpin(bool on);
     std::vector<std::string> atomColourRows() const;

     /**
      * The ESP colour bar as drawn, "key value" lines, for headless checks
      * (the Builder's "mobar" scene command).
      */
     string espBarState() const;
     string surfaceState() const;

     /** The isovalue (linear) the grid's outer faces were last checked at; 0 before a surface. */
     double facesCheckedIso() const { return p_facesCheckedIso; }
     /** Choose a field type as a click in the type drop-down does. */
     bool selectFieldType(const string& name);
     struct SurfaceEntry { string label; string description; };
     std::vector<SurfaceEntry> surfaceEntries() const;
     /** "Surfaces" while the panel shows the density surfaces, else "MOs". */
     string selectedSurface() const;
     /** Surfaces entry vs MOs entry: the same panel, with its own drop-down and controls. */
     void setSurfaceMode(bool on);
     bool surfaceMode() const { return p_surfaceMode; }

     /** The table lists the localised orbitals ("Localised MO" chosen). */
     bool localisedShown() const { return p_localised; }
     bool sortByHeading(const string& heading, bool ascending);

     /** The text shown under the table: the selected MO's composition. */
     string compositionText() const { return p_compSummary; }

   protected:

     double getTransparency();
     bool espFieldSelected();
     /** ELF, LOL or NCI chosen; `nci` says which kind. */
     bool fieldViewSelected(bool *nci = 0);
     void updateIsoValue();
     void updateIsoValue(double val);
     void updateSurface();
     void updateUIOptions();
     void populateTypeChoice();
     void showOrbitalControls();
     bool rebuildTable();   // table and graphs for the canonical or localised set
     void setLocalised(bool localised);  // switch the table's set, tell the MO diagram
     void fillUI();
     void selectMo(int index);
     void showComposition();
     void fitComposition();
     void fillCharacterColumn(bool haveTypes);
     bool ensureAoBasis();
     GridDlg *getGridDlg();
     void updateGridDlg();
     void showGridDlg();
     /** Sides of the drawn grid where |field| exceeds `iso` (bit 0..5: -x +x -y +y -z +z). */
     int gridFacesCut(double iso) const;
     /** Re-check the faces at a lowered isovalue; recompute with a larger box if cut. */
     void recheckGridFaces(double isoLog);
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
     struct SpinData {
        struct Somo { int mo; bool beta; double energy; std::string main;
                     std::string full; };
        std::string message;            // set when there is nothing to tabulate
        std::vector<Somo> somos;
        bool haveBasis = false;
        std::string why;                // why the populations are missing
        std::vector<SpinTables::Population> populations;
        std::vector<std::string> atomElement;   // per atom, all of them
        std::vector<double> atomSpin;
     };
     SpinData spinDensityData();
     void fillSpinView();
     void layoutSpinView();
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
      string selectedFieldKey() const;
      void updateFieldNote();
      wxStaticText *p_fieldNote = nullptr;
      wxString p_fieldDescription;
      void rebuildEspSurface();
      void dropDrawnSurface();
      void buildFieldViewControls();
      void showFieldViewControls(bool show, bool nci);
      void onFieldViewChanged(wxCommandEvent& event);
      void onFieldViewFocusLost(wxFocusEvent& event);
      /** The row's isovalue and (NCI) density cut-off; defaults when blank. */
      double fieldViewIsovalue(const string& type) const;
      double fieldViewCutoff() const;

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
      wxBoxSizer  *p_fieldSizer = 0;
      ewxTextCtrl *p_fieldIso = 0;
      wxStaticText *p_fieldCutLabel = 0;
      ewxTextCtrl *p_fieldCut = 0;
      string p_espDrawnType;     // field type of the surface on screen
      string p_drawnType;        // any field type drawn; empty before one


      //  The selected MO's atomic-orbital composition (#161), from the
      //  same MoAoBasis as ecce-mocomp.  The basis and its overlap matrix
      //  cost something, so they are built on the first selection.
      wxPanel         *p_compBox = nullptr;     // thin-bordered box round the text
      wxStaticText    *p_compText = nullptr;
      wxString         p_compFull;              // the text before wrapping
      MoAoBasis       *p_aoBasis = nullptr;
      bool             p_aoTried = false;
      string           p_aoWhy;
      vector< vector<double> > p_aoRows[2];     // alpha, beta
      string           p_compSummary;
      //  Core orbitals for the "F 1s" labels (#827), per spin, from the deepest MOs.
      vector<ShellLabel::Core> p_coreInfo[2];
      bool             p_coreDone[2] = { false, false };
      /** comp.types with labels "F 1s" / "H(1) 1s": n from the element, core for a core MO. */
      vector<MoAoBasis::Share> namedTypes(const vector<MoAoBasis::Share>& types,
                                          int mo, bool beta);

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
      wxScrolledWindow *p_spinView = NULL;   // the spin density view
      wxStaticText *p_spinNote = NULL, *p_spinPopHead = NULL;
      wxGrid *p_spinSomo = NULL, *p_spinPop = NULL;
      std::vector<std::string> p_spinSomoFull;   // tooltip per SOMO row
      bool p_spinShown = false;
      wxStaticText *p_spinLargest = NULL, *p_spinElemHead = NULL,
                   *p_spinLegend = NULL;
      wxGrid *p_spinElem = NULL;
      wxCheckBox *p_spinPaint = NULL;
      wxPanel *p_spinBar = NULL;
      std::vector<float> p_spinSaved;       // atom colours before painting
      int p_spinSavedBinding = 0;
      bool p_spinPainted = false;
      //  One atom: each orbital's shell ("1s", "2p", "3s (virtual)"), alpha
      //  then beta, replacing the symmetry label; empty for a molecule.
      vector<string> p_atomLabels[2];
      string p_virtNote;          // "Virtual orbitals: ..." line, for tests
      bool p_surfaceMode = false; // the Surfaces entry: no orbital table or coefficient controls
      bool p_localised = false;   // the table lists the localised orbitals
      //  Grid box growth (Angstrom per side) the face check asked for, the
      //  rounds spent, and the isovalue the faces were last checked at.
      float p_gridGrow[6] = {0, 0, 0, 0, 0, 0};
      int p_gridRound = 0;
      double p_facesCheckedIso = 0;
      bool p_keepIso = false;     // a recompute keeps the slider's value
      double p_keepIsoLog = 0;

      /* The first plot; all symmetries are on same x value. */
      XYPlotCanvas *p_plotReg;

      /* The second plot; all values grouped by symmetries. */
      XYPlotCanvas *p_plotSym;

      /* map from plot selections to table rows */
      std::map<std::pair<int,int>,int> p_pointToRowReg;
      std::map<std::pair<int,int>,int> p_pointToRowSym;

};

#endif // MOPANEL_H
