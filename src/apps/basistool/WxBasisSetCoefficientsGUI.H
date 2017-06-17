/////////////////////////////////////////////////////////////////////////////
// Name:        WxBasisSetCoefficientsGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXBASISSETCOEFFICIENTSGUI_H_
#define _WXBASISSETCOEFFICIENTSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxBasisSetCoefficientsGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wxgui/ewxDialog.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxDialog;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXBASISSETCOEFFICIENTSGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXBASISSETCOEFFICIENTSGUI_TITLE _("ECCE Basis Set Exponents & Coefficients")
#define SYMBOL_WXBASISSETCOEFFICIENTSGUI_IDNAME ID_DIALOG_WXBASISSETCOEFFICIENTS
#define SYMBOL_WXBASISSETCOEFFICIENTSGUI_SIZE wxSize(400, 300)
#define SYMBOL_WXBASISSETCOEFFICIENTSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxBasisSetCoefficientsGUI class declaration
 */

class WxBasisSetCoefficientsGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxBasisSetCoefficientsGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxBasisSetCoefficientsGUI( );
    WxBasisSetCoefficientsGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXBASISSETCOEFFICIENTSGUI_IDNAME, const wxString& caption = SYMBOL_WXBASISSETCOEFFICIENTSGUI_TITLE, const wxPoint& pos = SYMBOL_WXBASISSETCOEFFICIENTSGUI_POSITION, const wxSize& size = SYMBOL_WXBASISSETCOEFFICIENTSGUI_SIZE, long style = SYMBOL_WXBASISSETCOEFFICIENTSGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXBASISSETCOEFFICIENTSGUI_IDNAME, const wxString& caption = SYMBOL_WXBASISSETCOEFFICIENTSGUI_TITLE, const wxPoint& pos = SYMBOL_WXBASISSETCOEFFICIENTSGUI_POSITION, const wxSize& size = SYMBOL_WXBASISSETCOEFFICIENTSGUI_SIZE, long style = SYMBOL_WXBASISSETCOEFFICIENTSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxBasisSetCoefficientsGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE
    virtual    void closeButtonClickCB( wxCommandEvent& event );

////@end WxBasisSetCoefficientsGUI event handler declarations

////@begin WxBasisSetCoefficientsGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxBasisSetCoefficientsGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxBasisSetCoefficientsGUI member variables
    static const wxWindowID ID_TEXTCTRL_WXBASISSETCOEFFICIENTS_DETAIL;
    static const wxWindowID ID_DIALOG_WXBASISSETCOEFFICIENTS;
    static const wxWindowID ID_BUTTON_WXBASISSETCOEFFICIENTS_HELP;
    static const wxWindowID ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE;
    static const wxWindowID ID_STATIC_WXBASISSETCOEFFICIENTS_HEADER;
////@end WxBasisSetCoefficientsGUI member variables
};

#endif
    // _WXBASISSETCOEFFICIENTSGUI_H_
