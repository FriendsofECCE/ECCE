/////////////////////////////////////////////////////////////////////////////
// Name:        ImageContextPanelGUI.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ImageContextPanelGUI.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxScrolledWindow.H"
////@end includes

#include "ImageContextPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID ImageContextPanelGUI::ID_TOOLBAR = wxNewId();
const wxWindowID ImageContextPanelGUI::ID_SCROLLEDWINDOW = wxNewId();

/*!
 * ImageContextPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ImageContextPanelGUI, ewxPanel )

/*!
 * ImageContextPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( ImageContextPanelGUI, ewxPanel )

////@begin ImageContextPanelGUI event table entries
    EVT_PAINT( ImageContextPanelGUI::OnPaint )
    EVT_LEFT_DOWN( ImageContextPanelGUI::OnLeftDown )
    EVT_LEFT_UP( ImageContextPanelGUI::OnLeftUp )
    EVT_MOTION( ImageContextPanelGUI::OnMotion )
    EVT_MOUSEWHEEL( ImageContextPanelGUI::OnMouseWheel )

    EVT_MENU( wxID_ZOOM_FIT, ImageContextPanelGUI::OnZoomFitClick )

    EVT_MENU( wxID_ZOOM_100, ImageContextPanelGUI::OnZoom100Click )

    EVT_MENU( wxID_ZOOM_IN, ImageContextPanelGUI::OnZoomInClick )

    EVT_MENU( wxID_ZOOM_OUT, ImageContextPanelGUI::OnZoomOutClick )

////@end ImageContextPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * ImageContextPanelGUI constructors
 */

ImageContextPanelGUI::ImageContextPanelGUI( )
{
}

ImageContextPanelGUI::ImageContextPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * ImageContextPanelGUI creator
 */

bool ImageContextPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ImageContextPanelGUI member initialisation
    p_topSizer = NULL;
    p_imageWindow = NULL;
    p_staticBitmap = NULL;
    p_toolBar = NULL;
////@end ImageContextPanelGUI member initialisation

////@begin ImageContextPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ImageContextPanelGUI creation
    return true;
}

/*!
 * Control creation for ImageContextPanelGUI
 */

void ImageContextPanelGUI::CreateControls()
{    
////@begin ImageContextPanelGUI content construction
    ImageContextPanelGUI* itemPanel1 = this;

    p_topSizer = new wxBoxSizer(wxHORIZONTAL);
    itemPanel1->SetSizer(p_topSizer);

    p_imageWindow = new ewxScrolledWindow( itemPanel1, ID_SCROLLEDWINDOW, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    p_topSizer->Add(p_imageWindow, 1, wxGROW, 5);
    p_imageWindow->SetScrollbars(1, 1, 0, 0);

    wxBitmap p_staticBitmapBitmap(wxNullBitmap);
    p_staticBitmap = new wxStaticBitmap( p_imageWindow, wxID_STATIC, p_staticBitmapBitmap, wxDefaultPosition, wxDefaultSize, 0 );

    p_toolBar = new wxToolBar( itemPanel1, ID_TOOLBAR, wxDefaultPosition, wxDefaultSize, wxTB_FLAT|wxTB_VERTICAL|wxNO_BORDER|wxWANTS_CHARS );
    p_toolBar->SetToolPacking(2);
    wxBitmap itemtool7Bitmap(itemPanel1->GetBitmapResource(wxT("viewmagfit.png")));
    p_toolBar->AddTool(wxID_ZOOM_FIT, _T(""), itemtool7Bitmap, _T(""), wxITEM_NORMAL);
    wxBitmap itemtool8Bitmap(itemPanel1->GetBitmapResource(wxT("viewmag1.png")));
    p_toolBar->AddTool(wxID_ZOOM_100, _T(""), itemtool8Bitmap, _T(""), wxITEM_NORMAL);
    wxBitmap itemtool9Bitmap(itemPanel1->GetBitmapResource(wxT("viewmag+.png")));
    p_toolBar->AddTool(wxID_ZOOM_IN, _T(""), itemtool9Bitmap, _T(""), wxITEM_NORMAL);
    wxBitmap itemtool10Bitmap(itemPanel1->GetBitmapResource(wxT("viewmag-.png")));
    p_toolBar->AddTool(wxID_ZOOM_OUT, _T(""), itemtool10Bitmap, _T(""), wxITEM_NORMAL);
    p_toolBar->Realize();
    p_topSizer->Add(p_toolBar, 0, wxGROW|wxALL, 0);

////@end ImageContextPanelGUI content construction
}

/*!
 * wxEVT_PAINT event handler for ID_SCROLLEDWINDOW
 */

void ImageContextPanelGUI::OnPaint( wxPaintEvent& event )
{
////@begin wxEVT_PAINT event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    wxPaintDC dc(this);
////@end wxEVT_PAINT event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI. 
}

/*!
 * wxEVT_LEFT_DOWN event handler for ID_SCROLLEDWINDOW
 */

void ImageContextPanelGUI::OnLeftDown( wxMouseEvent& event )
{
////@begin wxEVT_LEFT_DOWN event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_LEFT_DOWN event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI. 
}

/*!
 * wxEVT_LEFT_UP event handler for ID_SCROLLEDWINDOW
 */

void ImageContextPanelGUI::OnLeftUp( wxMouseEvent& event )
{
////@begin wxEVT_LEFT_UP event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_LEFT_UP event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI. 
}

/*!
 * wxEVT_MOTION event handler for ID_SCROLLEDWINDOW
 */

void ImageContextPanelGUI::OnMotion( wxMouseEvent& event )
{
////@begin wxEVT_MOTION event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_MOTION event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI. 
}

/*!
 * wxEVT_MOUSEWHEEL event handler for ID_SCROLLEDWINDOW
 */

void ImageContextPanelGUI::OnMouseWheel( wxMouseEvent& event )
{
////@begin wxEVT_MOUSEWHEEL event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_MOUSEWHEEL event handler for ID_SCROLLEDWINDOW in ImageContextPanelGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_FIT
 */

void ImageContextPanelGUI::OnZoomFitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_FIT in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_FIT in ImageContextPanelGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_100
 */

void ImageContextPanelGUI::OnZoom100Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_100 in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_100 in ImageContextPanelGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_IN
 */

void ImageContextPanelGUI::OnZoomInClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_IN in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_IN in ImageContextPanelGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_OUT
 */

void ImageContextPanelGUI::OnZoomOutClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_OUT in ImageContextPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_OUT in ImageContextPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool ImageContextPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap ImageContextPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ImageContextPanelGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("viewmagfit.png"))
    {
        ewxBitmap bitmap(_T("viewmagfit.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    else if (name == wxT("viewmag1.png"))
    {
        ewxBitmap bitmap(_T("viewmag1.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    else if (name == wxT("viewmag+.png"))
    {
        ewxBitmap bitmap(_T("viewmag+.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    else if (name == wxT("viewmag-.png"))
    {
        ewxBitmap bitmap(_T("viewmag-.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    return wxNullBitmap;
////@end ImageContextPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ImageContextPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ImageContextPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end ImageContextPanelGUI icon retrieval
}
