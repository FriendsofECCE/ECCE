#ifndef MOPANEL__H
#define MOPANEL__H

#include "wxgui/TearableContentProvider.H"

#include "MoGUI.H"

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
     double getTransparency();
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
