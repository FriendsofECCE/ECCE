#ifndef _SLICE_H_
#define _SLICE_H_

#include "util/Event.H"
  using ecce::Event;
#include "util/Listener.H"
  using ecce::Listener;

#include "wxviz/WxVizTool.H"

#include "SliceGUI.H"

class Slice: public SliceGUI, public WxVizTool, public Listener
{    
    DECLARE_DYNAMIC_CLASS( Slice)

public:
    Slice( );
    Slice( wxWindow* parent, wxWindowID id = SYMBOL_SLICEGUI_IDNAME, const wxPoint& pos = SYMBOL_SLICEGUI_POSITION, const wxSize& size = SYMBOL_SLICEGUI_SIZE, long style = SYMBOL_SLICEGUI_STYLE );

    virtual ~Slice( );

    // Listener virtuals
    virtual void eventMCB(const Event& event);

    //int getSelectedPlane() const;
    void generateHKLPlane();
    void generateHKLPlane(const string& name,
                          const wxColour& color,
                          bool transparent,
                          int h,int k, int l, 
                          float displacement);

    void showPlanes(bool flag);
    void getHKL(int& h, int& k, int& l);
    void setHKL(int h, int k, int l);
    void generatePlane1();
    void generatePlane2();
    void resetUI();
    void updateReadOnly();

    void OnSpinctrlSliceHUpdated( wxSpinEvent& event );
    void OnSpinctrlSliceKUpdated( wxSpinEvent& event );
    void OnSpinctrlSliceLUpdated( wxSpinEvent& event );
    void OnCheckboxShowplaneClick( wxCommandEvent& event );

    // Plane 1
    void OnButtonSliceColor1Click( wxCommandEvent& event );
    void OnCheckboxSliceTransparent1Click( wxCommandEvent& event );
    void OnSpinctrlSliceOffset1Updated( wxSpinEvent& event );

    // Plane 2
    void OnButtonSliceColor2Click( wxCommandEvent& event );
    void OnCheckboxSliceTransparent2Click( wxCommandEvent& event );
    void OnSpinctrlSliceOffset2Updated( wxSpinEvent& event );

    void OnButtonSlicerXformClick( wxCommandEvent& event );
    void OnButtonSlicerSliceClick( wxCommandEvent& event );
};

#endif
