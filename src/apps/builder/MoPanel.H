#ifndef MOPANEL__H
#define MOPANEL__H

#include "wxgui/TearableContentProvider.H"
//  Self-contained, so this header can be included from anywhere.  It
//  used to rely on whatever MoPanel.C happened to include ahead of it
//  -- wxPoint2DDoubleCmp, SliderCombo and wxPlotCtrlEvent all come
//  from these two -- which meant no other translation unit could
//  include it at all.
#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/SliderCombo.H"

#include "MoGUI.H"

class ewxCheckBox;
class ewxTextCtrl;
class Command;

class ewxPlotCtrl;
class GridDlg;
class MoCoeffs;
class wxGrid;


/**
 */
class MoPanel : public MoGUI,
                public TearableContentProvider
{
  DECLARE_DYNAMIC_CLASS(MoPanel)
  DECLARE_EVENT_TABLE()

   enum MenuIds {SOLID=9780, MESH, CONTOUR,
                 REDGREEN, REDBLUE, BLUEYELLOW, LIGHTDARK,
                 DEFINEGRID, HIDE, 
                 COEFFS, OPAQUE, SEMITRANSPARENT,
                 TABLE, PLOT, PLOTSYM};

  static const wxWindowID ID_SLIDER;

  public:
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

    void OnButtonMoComputeClick( wxCommandEvent& event );
    void OnButtonMoViewcoefClick( wxCommandEvent& event );

   protected:
     void initialize(bool resetgrids=true);
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

   protected:

     double getTransparency();
     bool espFieldSelected();
     void updateIsoValue();
     void updateIsoValue(double val);
     void updateSurface();
     void updateUIOptions();
     void fillUI();
     void selectMo(int index);
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
     void OnGraphSelection(wxPlotCtrlEvent& event);
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
      void rebuildEspSurface();

      wxBoxSizer  *p_espSizer;
      ewxCheckBox *p_espAuto;
      ewxTextCtrl *p_espMin;
      ewxTextCtrl *p_espMax;


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

      /* The first plot; all symmetries are on same x value. */
      ewxPlotCtrl *p_plotReg;

      /* The second plot; all values grouped by symmetries. */
      ewxPlotCtrl *p_plotSym;

      /* map from plot selections to table rows */
      map<wxPoint2DDouble,int,wxPoint2DDoubleCmp> p_pointToRowReg;
      map<wxPoint2DDouble,int,wxPoint2DDoubleCmp> p_pointToRowSym;

};

#endif // MOPANEL_H
