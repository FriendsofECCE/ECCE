#ifndef EWXPLOTCTRL_H_
#define EWXPLOTCTRL_H_

#include <map>
  using std::map;

#include "wxgui/PlotCtrl.H"

class wxMenu;


struct wxPoint2DDoubleCmp {
  bool operator () (wxPoint2DDouble p1, wxPoint2DDouble p2) const {
    if (p1.m_x == p2.m_x) {
      return p1.m_y < p2.m_y;
    }
    return p1.m_x < p2.m_x;
  }
};


typedef map<wxPoint2DDouble, wxString, wxPoint2DDoubleCmp> AxisMap;


class ewxPlotCtrl : public wxPlotCtrl
{
  public:

    static const wxWindowID ID_PLOT_MENU_CLONE;
    static const wxWindowID ID_PLOT_MENU_ZOOM_TO_FIT;
    static const wxWindowID ID_PLOT_MENU_SHOW_CROSSHAIR;
    static const wxWindowID ID_PLOT_MENU_DRAW_SYMBOLS;
    static const wxWindowID ID_PLOT_MENU_DRAW_GRID;
    static const wxWindowID ID_PLOT_MENU_SHOW_POINT_LABEL;
    static const wxWindowID ID_PLOT_MENU_SHOW_AXIS_X;
    static const wxWindowID ID_PLOT_MENU_SHOW_AXIS_Y;
    static const wxWindowID ID_PLOT_MENU_SHOW_AXIS_LABEL_X;
    static const wxWindowID ID_PLOT_MENU_SHOW_AXIS_LABEL_Y;
    static const wxWindowID ID_PLOT_MENU_SHOW_TITLE;
    static const wxWindowID ID_PLOT_MENU_SHOW_KEY;

    ewxPlotCtrl();
    ewxPlotCtrl( wxWindow *parent, wxWindowID win_id = wxID_ANY,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxSize(300,200),
                 wxPlotCtrlAxis_Type flags = wxPLOTCTRL_DEFAULT,
                 const wxString& name = wxT("wxPlotCtrl") );
    virtual ~ewxPlotCtrl();

    bool Create( wxWindow *parent, wxWindowID id = wxID_ANY,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 wxPlotCtrlAxis_Type flags = wxPLOTCTRL_DEFAULT,
                 const wxString& name = wxT("wxPlotCtrl") );

    virtual ewxPlotCtrl * Clone(wxWindow * parent);

    // Override these from wxPlotCtrl for better functionality.
    // Note that since it's not virtual, internal methods won't call it as
    // necessary.  Clients must call this manually as needed.
    void CalcBoundingPlotRect();
    // Not part of original API - know what you're doing before calling this
    void SetCurveBoundingRect(const wxRect2DDouble& rect);

    // New functionality.
    int FindMarker(const wxPlotMarker& marker);

    bool GetShowPointLabel() const { return p_showPointLabel; }
    void SetShowPointLabel(bool show);

    AxisMap GetXValueMap() const { return p_xValueMap; }
    void SetXValueMap(const AxisMap& axisMap);
    AxisMap GetYValueMap() const { return p_yValueMap; }
    void SetYValueMap(const AxisMap& axisMap);

  protected:
    void OnClick( wxPlotCtrlEvent &event );
    void OnRightClick( wxMouseEvent &event );

    void OnMenuClone( wxCommandEvent &event );
    void OnMenuZoomToFit( wxCommandEvent &event );
    void OnMenuShowCrosshair( wxCommandEvent &event );
    void OnMenuDrawSymbols( wxCommandEvent &event );
    void OnMenuDrawGrid( wxCommandEvent &event );
    void OnMenuShowPointLabel( wxCommandEvent &event );
    void OnMenuShowAxisX( wxCommandEvent &event );
    void OnMenuShowAxisY( wxCommandEvent &event );
    void OnMenuShowAxisLabelX( wxCommandEvent &event );
    void OnMenuShowAxisLabelY( wxCommandEvent &event );
    void OnMenuShowTitle( wxCommandEvent &event );
    void OnMenuShowKey( wxCommandEvent &event );

    void UpdateUIShowCrosshair( wxUpdateUIEvent &event );
    void UpdateUIDrawSymbols( wxUpdateUIEvent &event );
    void UpdateUIDrawGrid( wxUpdateUIEvent &event );
    void UpdateUIShowPointLabel( wxUpdateUIEvent &event );
    void UpdateUIShowAxisX( wxUpdateUIEvent &event );
    void UpdateUIShowAxisY( wxUpdateUIEvent &event );
    void UpdateUIShowAxisLabelX( wxUpdateUIEvent &event );
    void UpdateUIShowAxisLabelY( wxUpdateUIEvent &event );
    void UpdateUIShowTitle( wxUpdateUIEvent &event );
    void UpdateUIShowKey( wxUpdateUIEvent &event );

    virtual void ProcessAreaEVT_MOUSE_EVENTS( wxMouseEvent &event );
    virtual void ProcessAxisEVT_MOUSE_EVENTS( wxMouseEvent &event );

  private:
    void Init();
    DECLARE_CLASS(ewxPlotCtrl)
    DECLARE_EVENT_TABLE()

    wxMenu *p_menu;

    bool p_showPointLabel;
    wxPlotMarker p_pointMarker;

    AxisMap p_xValueMap;
    AxisMap p_yValueMap;
};

#endif // EWXPLOTCTRL_H_
