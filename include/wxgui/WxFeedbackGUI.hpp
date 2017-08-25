/**
 * @file
 *
 *
 */

#ifndef _WXFEEDBACKGUI_H_
#define _WXFEEDBACKGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxFeedbackGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wxgui/FlatBitmapButton.H"
#include "wxgui/ewxPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXFEEDBACKGUI_STYLE 0
#define SYMBOL_WXFEEDBACKGUI_TITLE _("ECCE Feedback Area")
#define SYMBOL_WXFEEDBACKGUI_IDNAME ID_PANEL_FEEDBACK
#define SYMBOL_WXFEEDBACKGUI_SIZE wxDefaultSize
#define SYMBOL_WXFEEDBACKGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxFeedbackGUI class declaration
 */

class WxFeedbackGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( WxFeedbackGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxFeedbackGUI( );
    WxFeedbackGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXFEEDBACKGUI_IDNAME, const wxPoint& pos = SYMBOL_WXFEEDBACKGUI_POSITION, const wxSize& size = SYMBOL_WXFEEDBACKGUI_SIZE, long style = SYMBOL_WXFEEDBACKGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXFEEDBACKGUI_IDNAME, const wxPoint& pos = SYMBOL_WXFEEDBACKGUI_POSITION, const wxSize& size = SYMBOL_WXFEEDBACKGUI_SIZE, long style = SYMBOL_WXFEEDBACKGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxFeedbackGUI event handler declarations
    /// wxEVT_CONTEXT_MENU event handler for ID_PANEL_FEEDBACK
    virtual    void OnContextMenu( wxContextMenuEvent& event );

    /// ButtonClick event handler for ID_RESETSTATE
    virtual    void onResetstateClicked( wxCommandEvent& event );

    /// ButtonClick event handler for ID_SAVE
    virtual    void onSaveClicked( wxCommandEvent& event );

////@end WxFeedbackGUI event handler declarations

////@begin WxFeedbackGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxFeedbackGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxFeedbackGUI member variables
    wxBoxSizer* p_topLineSizer;
    wxPanel* p_statePanel;
    static const wxWindowID ID_PANEL_STATE;
    static const wxWindowID ID_PANEL_FEEDBACK;
    static const wxWindowID ID_TEXTCTRL_FEEDBACK_MSG;
    static const wxWindowID ID_BITMAPBUTTON_DROPICON;
    static const wxWindowID ID_RESETSTATE;
    static const wxWindowID ID_SAVE;
    static const wxWindowID wxID_STATIC_FEEDBACK_CONTEXT;
////@end WxFeedbackGUI member variables
};

#endif
    // _WXFEEDBACKGUI_H_
