#ifndef _PLANE_H_
#define _PLANE_H_


#include "wx/sizer.h"

#include "PlaneGUI.H"

/**
 * Implementation of Plane User interface.
 * User can create miller planes or planes based on selection.
 * This class must be aware of whether or not miller planes make sense.
 */
class Plane: public PlaneGUI
{    
    DECLARE_DYNAMIC_CLASS( Plane)

public:
    Plane( );
    Plane( wxWindow* parent, wxWindowID id = SYMBOL_PLANEGUI_IDNAME, const wxString& caption = SYMBOL_PLANEGUI_TITLE, const wxPoint& pos = SYMBOL_PLANEGUI_POSITION, const wxSize& size = SYMBOL_PLANEGUI_SIZE, long style = SYMBOL_PLANEGUI_STYLE );
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PLANEGUI_IDNAME, const wxString& caption = SYMBOL_PLANEGUI_TITLE, const wxPoint& pos = SYMBOL_PLANEGUI_POSITION, const wxSize& size = SYMBOL_PLANEGUI_SIZE, long style = SYMBOL_PLANEGUI_STYLE );

    bool   isMillerPlane() const;
    void   getMillerIndices(int& h, int& k, int& l);
    bool   isTransparent() const;
    string getName() const;
    wxColour getColor() const;
    void   generateMPName();

    virtual    void OnTextctrlEnter( wxCommandEvent& event );
    virtual    void OnButtonClick( wxCommandEvent& event );
    virtual    void OnCheckboxPlaneTransparentClick( wxCommandEvent& event );
    virtual    void OnSpinctrlPlaneHUpdated( wxSpinEvent& event );
    virtual    void OnSpinctrlPlaneKUpdated( wxSpinEvent& event );
    virtual    void OnSpinctrlPlaneLUpdated( wxSpinEvent& event );


};

#endif
