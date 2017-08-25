/////////////////////////////////////////////////////////////////////////////
// Name:        PTDataFrameGUI.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PTDATAFRAMEGUI_H_
#define _PTDATAFRAMEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PTDataFrameGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/notebook.h"
#include "wx/statline.h"
#include "wx/grid.h"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxStaticText;
class wxBoxSizer;
class ewxGrid;
class ewxButton;
class ewxTextCtrl;
class ewxGrid;
class ewxButton;
class ewxStaticText;
class ewxFrame;
class ewxTextCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_PTDATAFRAMEGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_PTDATAFRAMEGUI_TITLE _("ECCE Periodic Table: Atomic Data")
#define SYMBOL_PTDATAFRAMEGUI_IDNAME ID_PTDATAFRAMEGUI
#define SYMBOL_PTDATAFRAMEGUI_SIZE wxSize(550, 420)
#define SYMBOL_PTDATAFRAMEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PTDataFrameGUI class declaration
 */

class PTDataFrameGUI: public ewxFrame
{    
    DECLARE_CLASS( PTDataFrameGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PTDataFrameGUI( );
    PTDataFrameGUI( wxWindow* parent, wxWindowID id = SYMBOL_PTDATAFRAMEGUI_IDNAME, const wxString& caption = SYMBOL_PTDATAFRAMEGUI_TITLE, const wxPoint& pos = SYMBOL_PTDATAFRAMEGUI_POSITION, const wxSize& size = SYMBOL_PTDATAFRAMEGUI_SIZE, long style = SYMBOL_PTDATAFRAMEGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PTDATAFRAMEGUI_IDNAME, const wxString& caption = SYMBOL_PTDATAFRAMEGUI_TITLE, const wxPoint& pos = SYMBOL_PTDATAFRAMEGUI_POSITION, const wxSize& size = SYMBOL_PTDATAFRAMEGUI_SIZE, long style = SYMBOL_PTDATAFRAMEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin PTDataFrameGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_PTDATAFRAMEGUI
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTEDITCOLOR_BUTTON
    virtual    void OnEditColorClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETCOLOR_BUTTON
    virtual    void OnResetColorClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLCOLOR_BUTTON
    virtual    void OnResetAllColorClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COVRAD_INPUT
    virtual    void OnRadiiUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETRADII_BUTTON
    virtual    void OnResetRadiiClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLRADII_BUTTON
    virtual    void OnResetAllRadiiClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTDATA_CLOSE
    virtual    void OnCloseClicked( wxCommandEvent& event );

////@end PTDataFrameGUI event handler declarations

////@begin PTDataFrameGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PTDataFrameGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PTDataFrameGUI member variables
    wxPanel* p_dataPanel;
    ewxStaticText* p_atomicNumLabel;
    ewxStaticText* p_atomicWeightLabel;
    ewxStaticText* p_atomicSymbolLabel;
    ewxStaticText* p_electronConfigLabel;
    ewxStaticText* p_elementNameLabel;
    ewxStaticText* p_electronAffinityLabel;
    ewxStaticText* p_refLabel;
    wxBoxSizer* p_isotopeSizer;
    ewxStaticText* p_isoTableLabel;
    ewxGrid* p_ionPotentGrid;
    ewxStaticText* p_ref2Label;
    wxPanel* p_preferencePanel;
    wxPanel* p_customColorPanel;
    ewxButton* p_editColorButton;
    wxPanel* p_defaultColorPanel;
    ewxButton* p_resetColorButton;
    ewxButton* p_resetAllColorButton;
    ewxStaticText* p_covRadLabel;
    ewxTextCtrl* p_covRadInput;
    ewxStaticText* p_defCovRadLabel;
    ewxStaticText* p_vdwRadLabel;
    ewxTextCtrl* p_vdwRadInput;
    ewxStaticText* p_defVdwRadLabel;
    ewxStaticText* p_bftLabel;
    ewxTextCtrl* p_bftInput;
    ewxStaticText* p_defBftLabel;
    ewxButton* p_resetRadiiButton;
    ewxButton* p_resetAllRadiiButton;
    static const wxWindowID ID_PTEDITCOLOR_BUTTON;
    static const wxWindowID ID_PTRESETALLRADII_BUTTON;
    static const wxWindowID ID_NOTEBOOK;
    static const wxWindowID ID_PTDATA_POTENTPANEL;
    static const wxWindowID ID_PTDATAFRAMEGUI;
    static const wxWindowID ID_GRID;
    static const wxWindowID ID_PANEL4;
    static const wxWindowID ID_PTDATA_COLORPANEL;
    static const wxWindowID ID_BFT_INPUT;
    static const wxWindowID ID_COVRAD_INPUT;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_CUSTOMCOLOR_PANEL;
    static const wxWindowID ID_VDWRAD_INPUT;
    static const wxWindowID ID_DEFAULTCOLOR_PANEL;
    static const wxWindowID ID_PTDATA_HELP;
    static const wxWindowID ID_PTDATA_REFPANEL;
    static const wxWindowID ID_PTDATA_CLOSE;
    static const wxWindowID ID_PTRESETALLCOLOR_BUTTON;
    static const wxWindowID ID_PTRESETRADII_BUTTON;
    static const wxWindowID ID_PTRESETCOLOR_BUTTON;
////@end PTDataFrameGUI member variables
};

#endif
    // _PTDATAFRAMEGUI_H_
