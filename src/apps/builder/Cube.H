#ifndef CUBE_H_
#define CUBE_H_

#include "wxgui/TearableContentProvider.H"

#include "VizPropertyPanel.H"

class ewxButton;
class ewxStaticText;
class ewxTextCtrl;
class SliderCombo;
class StringListCtrl;


class Cube : public VizPropertyPanel
           , public TearableContentProvider
{
  DECLARE_DYNAMIC_CLASS(Cube)
  DECLARE_EVENT_TABLE()

  public:
    static const wxWindowID ID_LIST;
    static const wxWindowID ID_TEXT_A;
    static const wxWindowID ID_TEXT_GRID_A;
    static const wxWindowID ID_TEXT_B;
    static const wxWindowID ID_TEXT_GRID_B;
    static const wxWindowID ID_BUTTON;
    static const wxWindowID ID_SLIDER;

    static const wxWindowID ID_MENU_SOLID;
    static const wxWindowID ID_MENU_MESH;
    static const wxWindowID ID_MENU_CONTOUR;
    static const wxWindowID ID_MENU_REDGREEN;
    static const wxWindowID ID_MENU_REDBLUE;
    static const wxWindowID ID_MENU_BLUEYELLOW;
    static const wxWindowID ID_MENU_LIGHTDARK;
    static const wxWindowID ID_MENU_OPAQUE;
    static const wxWindowID ID_MENU_SEMITRANSPARENT;

    Cube();
    Cube(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Cube");

    virtual bool Create(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Cube");

    virtual ~Cube();

    virtual void refresh();

  protected:
    virtual void receiveFocus();
    //  loseFocus, not lostFocus.  It was spelled lostFocus, which is not
    //  the name VizPropertyPanel declares, so it overrode nothing and was
    //  never called by anything: a Cube panel that took focus turned its
    //  isosurface on (getMORoot() -> SO_SWITCH_ALL) and left the viewer in
    //  SCREEN_DOOR transparency, and giving focus to any other panel could
    //  not turn either back off.  One more overlay the viewer accumulated
    //  and would not give back (#111).
    virtual void loseFocus();

    virtual void initialize();

    virtual wxWindow* GetTearableContent();

    void updateSurface();
    void updateIsoValue();
    void updateIsoValue(double val);
    void getSurfaceColors(double& posR, double& posG, double& posB,
                          double& negR, double& negG, double& negB);
    double getTransparency();
    int getGridIndex();

    void OnMenuClick(wxCommandEvent& event);
    void OnListLeftClick(wxListEvent& event);
    void OnListRightClick(wxListEvent& event);
    void OnSliderEndMotion(wxScrollEvent& event);
    void OnSliderTextEnter(wxCommandEvent& event);
    void OnCompute(wxCommandEvent& event);

  private:
    StringListCtrl * p_list;
    ewxStaticText  * p_labelChemSys;
    ewxTextCtrl    * p_textA;
    ewxStaticText  * p_textAGrid;
    ewxTextCtrl    * p_textB;
    ewxStaticText  * p_textBGrid;
    ewxButton      * p_button;
    SliderCombo    * p_slider;

    float p_multA;
    float p_multB;

    // each instance of this Cube panel will hold its own PropGrids
    PropGrids * p_propGrids;

    static map<wxString,wxWindowID> s_id;
    static map<wxWindowID,wxString> s_str;
};

#endif // CUBE_H_
