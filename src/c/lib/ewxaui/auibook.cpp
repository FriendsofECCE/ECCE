///////////////////////////////////////////////////////////////////////////////
// Name:        src/ewxaui/auibook.cpp
// Purpose:     wxaui: wx advanced user interface - notebook
// Author:      Benjamin I. Williams
// Modified by:
// Created:     2006-06-28
// Copyright:   (C) Copyright 2006, Kirix Corporation, All Rights Reserved
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_AUI

#include "wx/ewxaui/auibook.hpp"

#ifndef WX_PRECOMP
    #include "wx/settings.h"
    #include "wx/image.h"
    #include "wx/menu.h"
#endif

#include "wx/ewxaui/tabmdi.hpp"
#include "wx/dcbuffer.h"
#include "wx/log.h"

#ifdef __WXMSW__
#include  "wx/msw/private.h"
#endif

#ifdef __WXMAC__
#include "wx/mac/carbon/private.h"
#endif

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include "wx/gtk/win_gtk.h"
#endif

#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(ewxAuiNotebookPageArray)
WX_DEFINE_OBJARRAY(ewxAuiTabContainerButtonArray)

DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_BUTTON)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_END_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_DRAG_MOTION)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_BG_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_DOWN)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_DOWN)


IMPLEMENT_CLASS(ewxAuiNotebook, wxControl)
IMPLEMENT_CLASS(ewxAuiTabCtrl, wxControl)
IMPLEMENT_DYNAMIC_CLASS(ewxAuiNotebookEvent, wxEvent)




// these functions live in dockart.cpp -- they'll eventually
// be moved to a new utility cpp file

wxColor ewxAuiStepColour(const wxColor& c, int percent);

wxBitmap ewxAuiBitmapFromBits(const unsigned char bits[], int w, int h,
                             const wxColour& color);

wxString ewxAuiChopText(wxDC& dc, const wxString& text, int max_size);

static void DrawButtons(wxDC& dc,
                        const wxRect& _rect,
                        const wxBitmap& bmp,
                        const wxColour& bkcolour,
                        int button_state)
{
    wxRect rect = _rect;

    if (button_state == ewxAUI_BUTTON_STATE_PRESSED)
    {
        rect.x++;
        rect.y++;
    }

    if (button_state == ewxAUI_BUTTON_STATE_HOVER ||
        button_state == ewxAUI_BUTTON_STATE_PRESSED)
    {
        dc.SetBrush(wxBrush(ewxAuiStepColour(bkcolour, 120)));
        dc.SetPen(wxPen(ewxAuiStepColour(bkcolour, 75)));

        // draw the background behind the button
        dc.DrawRectangle(rect.x, rect.y, 15, 15);
    }

    // draw the button itself
    dc.DrawBitmap(bmp, rect.x, rect.y, true);
}

static void IndentPressedBitmap(wxRect* rect, int button_state)
{
    if (button_state == ewxAUI_BUTTON_STATE_PRESSED)
    {
        rect->x++;
        rect->y++;
    }
}

static void DrawFocusRect(wxWindow* win, wxDC& dc, const wxRect& rect, int flags)
{
#ifdef __WXMSW__
    wxUnusedVar(win);
    wxUnusedVar(flags);

    RECT rc;
    wxCopyRectToRECT(rect, rc);

    ::DrawFocusRect(GetHdcOf(dc), &rc);

#elif defined(__WXGTK20__)
    GdkWindow* gdk_window = dc.GetGDKWindow();
    wxASSERT_MSG( gdk_window,
                  wxT("cannot draw focus rectangle on wxDC of this type") );

    GtkStateType state;
    //if (flags & wxCONTROL_SELECTED)
    //    state = GTK_STATE_SELECTED;
    //else
        state = GTK_STATE_NORMAL;

    gtk_paint_focus( win->m_widget->style,
                     gdk_window,
                     state,
                     NULL,
                     win->m_wxwindow,
                     NULL,
                     dc.LogicalToDeviceX(rect.x),
                     dc.LogicalToDeviceY(rect.y),
                     rect.width,
                     rect.height );
#elif (defined(__WXMAC__))

#if wxMAC_USE_CORE_GRAPHICS
    {
        CGRect cgrect = CGRectMake( rect.x , rect.y , rect.width, rect.height ) ;

#if 0
        Rect bounds ;
        win->GetPeer()->GetRect( &bounds ) ;

        wxLogDebug(wxT("Focus rect %d, %d, %d, %d"), rect.x, rect.y, rect.width, rect.height);
        wxLogDebug(wxT("Peer rect %d, %d, %d, %d"), bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);
#endif

        HIThemeFrameDrawInfo info ;
        memset( &info, 0 , sizeof(info) ) ;

        info.version = 0 ;
        info.kind = 0 ;
        info.state = kThemeStateActive;
        info.isFocused = true ;

        CGContextRef cgContext = (CGContextRef) win->MacGetCGContextRef() ;
        wxASSERT( cgContext ) ;

        HIThemeDrawFocusRect( &cgrect , true , cgContext , kHIThemeOrientationNormal ) ;
    }
 #else
    {
        Rect r;
        r.left = rect.x; r.top = rect.y; r.right = rect.GetRight(); r.bottom = rect.GetBottom();
        wxTopLevelWindowMac* top = win->MacGetTopLevelWindow();
        if ( top )
        {
            wxPoint pt(0, 0) ;
            wxMacControl::Convert( &pt , win->GetPeer() , top->GetPeer() ) ;
            OffsetRect( &r , pt.x , pt.y ) ;
        }

        DrawThemeFocusRect( &r , true ) ;
    }
#endif
#else
    wxUnusedVar(win);
    wxUnusedVar(flags);

    // draw the pixels manually because the "dots" in wxPen with wxDOT style
    // may be short traits and not really dots
    //
    // note that to behave in the same manner as DrawRect(), we must exclude
    // the bottom and right borders from the rectangle
    wxCoord x1 = rect.GetLeft(),
            y1 = rect.GetTop(),
            x2 = rect.GetRight(),
            y2 = rect.GetBottom();

    dc.SetPen(*wxBLACK_PEN);

#ifdef __WXMAC__
    dc.SetLogicalFunction(wxCOPY);
#else
    // this seems to be closer than what Windows does than wxINVERT although
    // I'm still not sure if it's correct
    dc.SetLogicalFunction(wxAND_REVERSE);
#endif

    wxCoord z;
    for ( z = x1 + 1; z < x2; z += 2 )
        dc.DrawPoint(z, rect.GetTop());

    wxCoord shift = z == x2 ? 0 : 1;
    for ( z = y1 + shift; z < y2; z += 2 )
        dc.DrawPoint(x2, z);

    shift = z == y2 ? 0 : 1;
    for ( z = x2 - shift; z > x1; z -= 2 )
        dc.DrawPoint(z, y2);

    shift = z == x1 ? 0 : 1;
    for ( z = y2 - shift; z > y1; z -= 2 )
        dc.DrawPoint(x1, z);

    dc.SetLogicalFunction(wxCOPY);
#endif
}


// -- GUI helper classes and functions --

class ewxAuiCommandCapture : public wxEvtHandler
{
public:

    ewxAuiCommandCapture() { m_last_id = 0; }
    int GetCommandId() const { return m_last_id; }

    bool ProcessEvent(wxEvent& evt)
    {
        if (evt.GetEventType() == wxEVT_COMMAND_MENU_SELECTED)
        {
            m_last_id = evt.GetId();
            return true;
        }

        if (GetNextHandler())
            return GetNextHandler()->ProcessEvent(evt);

        return false;
    }

private:
    int m_last_id;
};


// -- bitmaps --

#if defined( __WXMAC__ )
 static unsigned char close_bits[]={
     0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFE, 0x03, 0xF8, 0x01, 0xF0, 0x19, 0xF3,
     0xB8, 0xE3, 0xF0, 0xE1, 0xE0, 0xE0, 0xF0, 0xE1, 0xB8, 0xE3, 0x19, 0xF3,
     0x01, 0xF0, 0x03, 0xF8, 0x0F, 0xFE, 0xFF, 0xFF };
#elif defined( __WXGTK__)
 static unsigned char close_bits[]={
     0xff, 0xff, 0xff, 0xff, 0x07, 0xf0, 0xfb, 0xef, 0xdb, 0xed, 0x8b, 0xe8,
     0x1b, 0xec, 0x3b, 0xee, 0x1b, 0xec, 0x8b, 0xe8, 0xdb, 0xed, 0xfb, 0xef,
     0x07, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
#else
 static unsigned char close_bits[]={
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xf3, 0xcf, 0xf9,
     0x9f, 0xfc, 0x3f, 0xfe, 0x3f, 0xfe, 0x9f, 0xfc, 0xcf, 0xf9, 0xe7, 0xf3,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
#endif

static unsigned char left_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xfe, 0x3f, 0xfe,
   0x1f, 0xfe, 0x0f, 0xfe, 0x1f, 0xfe, 0x3f, 0xfe, 0x7f, 0xfe, 0xff, 0xfe,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static unsigned char right_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x9f, 0xff, 0x1f, 0xff,
   0x1f, 0xfe, 0x1f, 0xfc, 0x1f, 0xfe, 0x1f, 0xff, 0x9f, 0xff, 0xdf, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static unsigned char list_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xf8, 0xff, 0xff, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f, 0xfe, 0x7f, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};






// -- ewxAuiDefaultTabArt class implementation --

ewxAuiDefaultTabArt::ewxAuiDefaultTabArt()
{
    m_normal_font = *wxNORMAL_FONT;
    m_selected_font = *wxNORMAL_FONT;
    m_selected_font.SetWeight(wxBOLD);
    m_measuring_font = m_selected_font;

    m_fixed_tab_width = 100;
    m_tab_ctrl_height = 0;

#ifdef __WXMAC__
    wxBrush toolbarbrush;
    toolbarbrush.MacSetTheme( kThemeBrushToolbarBackground );
    wxColor base_colour = toolbarbrush.GetColour();
#else
    wxColor base_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#endif

    // the base_colour is too pale to use as our base colour,
    // so darken it a bit --
    if ((255-base_colour.Red()) +
        (255-base_colour.Green()) +
        (255-base_colour.Blue()) < 60)
    {
        base_colour = ewxAuiStepColour(base_colour, 92);
    }

    m_base_colour = base_colour;
    wxColor border_colour = ewxAuiStepColour(base_colour, 75);

    m_border_pen = wxPen(border_colour);
    m_base_colour_pen = wxPen(m_base_colour);
    m_base_colour_brush = wxBrush(m_base_colour);

    m_active_close_bmp = ewxAuiBitmapFromBits(close_bits, 16, 16, *wxBLACK);
    m_disabled_close_bmp = ewxAuiBitmapFromBits(close_bits, 16, 16, wxColour(128,128,128));

    m_active_left_bmp = ewxAuiBitmapFromBits(left_bits, 16, 16, *wxBLACK);
    m_disabled_left_bmp = ewxAuiBitmapFromBits(left_bits, 16, 16, wxColour(128,128,128));

    m_active_right_bmp = ewxAuiBitmapFromBits(right_bits, 16, 16, *wxBLACK);
    m_disabled_right_bmp = ewxAuiBitmapFromBits(right_bits, 16, 16, wxColour(128,128,128));

    m_active_windowlist_bmp = ewxAuiBitmapFromBits(list_bits, 16, 16, *wxBLACK);
    m_disabled_windowlist_bmp = ewxAuiBitmapFromBits(list_bits, 16, 16, wxColour(128,128,128));

    m_flags = 0;
}

ewxAuiDefaultTabArt::~ewxAuiDefaultTabArt()
{
}

ewxAuiTabArt* ewxAuiDefaultTabArt::Clone()
{
    ewxAuiDefaultTabArt* art = new ewxAuiDefaultTabArt;
    art->SetNormalFont(m_normal_font);
    art->SetSelectedFont(m_selected_font);
    art->SetMeasuringFont(m_measuring_font);

    return art;
}

void ewxAuiDefaultTabArt::SetFlags(unsigned int flags)
{
    m_flags = flags;
}

void ewxAuiDefaultTabArt::SetSizingInfo(const wxSize& tab_ctrl_size,
                                       size_t tab_count)
{
    m_fixed_tab_width = 100;

    int tot_width = (int)tab_ctrl_size.x - GetIndentSize() - 4;

    if (m_flags & ewxAUI_NB_CLOSE_BUTTON)
        tot_width -= m_active_close_bmp.GetWidth();
    if (m_flags & ewxAUI_NB_WINDOWLIST_BUTTON)
        tot_width -= m_active_windowlist_bmp.GetWidth();

    if (tab_count > 0)
    {
        m_fixed_tab_width = tot_width/(int)tab_count;
    }


    if (m_fixed_tab_width < 100)
        m_fixed_tab_width = 100;

    if (m_fixed_tab_width > tot_width/2)
        m_fixed_tab_width = tot_width/2;

    if (m_fixed_tab_width > 220)
        m_fixed_tab_width = 220;

    m_tab_ctrl_height = tab_ctrl_size.y;
}


void ewxAuiDefaultTabArt::DrawBackground(wxDC& dc,
                                        wxWindow* WXUNUSED(wnd),
                                        const wxRect& rect)
{
    // draw background
   wxColor top_color       = ewxAuiStepColour(m_base_colour, 90);
   wxColor bottom_color   = ewxAuiStepColour(m_base_colour, 170);
   wxRect r;

   if (m_flags &ewxAUI_NB_BOTTOM)
       r = wxRect(rect.x, rect.y, rect.width+2, rect.height);
   // TODO: else if (m_flags &ewxAUI_NB_LEFT) {}
   // TODO: else if (m_flags &ewxAUI_NB_RIGHT) {}
   else //for ewxAUI_NB_TOP
       r = wxRect(rect.x, rect.y, rect.width+2, rect.height-3);
    dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);

   // draw base lines
   dc.SetPen(m_border_pen);
   int y = rect.GetHeight();
   int w = rect.GetWidth();

   if (m_flags &ewxAUI_NB_BOTTOM)
   {
       dc.SetBrush(wxBrush(bottom_color));
       dc.DrawRectangle(-1, 0, w+2, 4);
   }
   // TODO: else if (m_flags &ewxAUI_NB_LEFT) {}
   // TODO: else if (m_flags &ewxAUI_NB_RIGHT) {}
   else //for ewxAUI_NB_TOP
   {
       dc.SetBrush(m_base_colour_brush);
       dc.DrawRectangle(-1, y-4, w+2, 4);
   }
}


// DrawTab() draws an individual tab.
//
// dc       - output dc
// in_rect  - rectangle the tab should be confined to
// caption  - tab's caption
// active   - whether or not the tab is active
// out_rect - actual output rectangle
// x_extent - the advance x; where the next tab should start

void ewxAuiDefaultTabArt::DrawTab(wxDC& dc,
                                 wxWindow* wnd,
                                 const ewxAuiNotebookPage& page,
                                 const wxRect& in_rect,
                                 int close_button_state,
                                 wxRect* out_tab_rect,
                                 wxRect* out_button_rect,
                                 int* x_extent)
{
    wxCoord normal_textx, normal_texty;
    wxCoord selected_textx, selected_texty;
    wxCoord texty;

    // if the caption is empty, measure some temporary text
    wxString caption = page.caption;
    if (caption.empty())
        caption = wxT("Xj");

    dc.SetFont(m_selected_font);
    dc.GetTextExtent(caption, &selected_textx, &selected_texty);

    dc.SetFont(m_normal_font);
    dc.GetTextExtent(caption, &normal_textx, &normal_texty);

    // figure out the size of the tab
    wxSize tab_size = GetTabSize(dc,
                                 wnd,
                                 page.caption,
                                 page.bitmap,
                                 page.active,
                                 close_button_state,
                                 x_extent);

    wxCoord tab_height = m_tab_ctrl_height - 3;
    wxCoord tab_width = tab_size.x;
    wxCoord tab_x = in_rect.x;
    wxCoord tab_y = in_rect.y + in_rect.height - tab_height;


    caption = page.caption;


    // select pen, brush and font for the tab to be drawn

    if (page.active)
    {
        dc.SetFont(m_selected_font);
        texty = selected_texty;
    }
     else
    {
        dc.SetFont(m_normal_font);
        texty = normal_texty;
    }


    // create points that will make the tab outline

    int clip_width = tab_width;
    if (tab_x + clip_width > in_rect.x + in_rect.width)
        clip_width = (in_rect.x + in_rect.width) - tab_x;

/*
    wxPoint clip_points[6];
    clip_points[0] = wxPoint(tab_x,              tab_y+tab_height-3);
    clip_points[1] = wxPoint(tab_x,              tab_y+2);
    clip_points[2] = wxPoint(tab_x+2,            tab_y);
    clip_points[3] = wxPoint(tab_x+clip_width-1, tab_y);
    clip_points[4] = wxPoint(tab_x+clip_width+1, tab_y+2);
    clip_points[5] = wxPoint(tab_x+clip_width+1, tab_y+tab_height-3);

    // FIXME: these ports don't provide wxRegion ctor from array of points
#if !defined(__WXDFB__) && !defined(__WXCOCOA__)
    // set the clipping region for the tab --
    wxRegion clipping_region(WXSIZEOF(clip_points), clip_points);
    dc.SetClippingRegion(clipping_region);
#endif // !wxDFB && !wxCocoa
*/
    // since the above code above doesn't play well with WXDFB or WXCOCOA,
    // we'll just use a rectangle for the clipping region for now --
    dc.SetClippingRegion(tab_x, tab_y, clip_width+1, tab_height-3);


    wxPoint border_points[6];
    if (m_flags &ewxAUI_NB_BOTTOM)
    {
       border_points[0] = wxPoint(tab_x,             tab_y);
       border_points[1] = wxPoint(tab_x,             tab_y+tab_height-6);
       border_points[2] = wxPoint(tab_x+2,           tab_y+tab_height-4);
       border_points[3] = wxPoint(tab_x+tab_width-2, tab_y+tab_height-4);
       border_points[4] = wxPoint(tab_x+tab_width,   tab_y+tab_height-6);
       border_points[5] = wxPoint(tab_x+tab_width,   tab_y);
    }
    else //if (m_flags & ewxAUI_NB_TOP) {}
    {
       border_points[0] = wxPoint(tab_x,             tab_y+tab_height-4);
       border_points[1] = wxPoint(tab_x,             tab_y+2);
       border_points[2] = wxPoint(tab_x+2,           tab_y);
       border_points[3] = wxPoint(tab_x+tab_width-2, tab_y);
       border_points[4] = wxPoint(tab_x+tab_width,   tab_y+2);
       border_points[5] = wxPoint(tab_x+tab_width,   tab_y+tab_height-4);
    }
    // TODO: else if (m_flags &ewxAUI_NB_LEFT) {}
    // TODO: else if (m_flags &ewxAUI_NB_RIGHT) {}

    int drawn_tab_yoff = border_points[1].y;
    int drawn_tab_height = border_points[0].y - border_points[1].y;


    if (page.active)
    {
        // draw active tab

        // draw base background color
        wxRect r(tab_x, tab_y, tab_width, tab_height);
        dc.SetPen(m_base_colour_pen);
        dc.SetBrush(m_base_colour_brush);
        dc.DrawRectangle(r.x+1, r.y+1, r.width-1, r.height-4);

        // this white helps fill out the gradient at the top of the tab
        dc.SetPen(*wxWHITE_PEN);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(r.x+2, r.y+1, r.width-3, r.height-4);

        // these two points help the rounded corners appear more antialiased
        dc.SetPen(m_base_colour_pen);
        dc.DrawPoint(r.x+2, r.y+1);
        dc.DrawPoint(r.x+r.width-2, r.y+1);

        // set rectangle down a bit for gradient drawing
        r.SetHeight(r.GetHeight()/2);
        r.x += 2;
        r.width -= 2;
        r.y += r.height;
        r.y -= 2;

        // draw gradient background
        wxColor top_color = *wxWHITE;
        wxColor bottom_color = m_base_colour;
        dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);
    }
     else
    {
        // draw inactive tab

        wxRect r(tab_x, tab_y+1, tab_width, tab_height-3);

        // start the gradent up a bit and leave the inside border inset
        // by a pixel for a 3D look.  Only the top half of the inactive
        // tab will have a slight gradient
        r.x += 3;
        r.y++;
        r.width -= 4;
        r.height /= 2;
        r.height--;

        // -- draw top gradient fill for glossy look
        wxColor top_color = m_base_colour;
        wxColor bottom_color = ewxAuiStepColour(top_color, 160);
        dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);

        r.y += r.height;
        r.y--;

        // -- draw bottom fill for glossy look
        top_color = m_base_colour;
        bottom_color = m_base_colour;
        dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);
    }

    // draw tab outline
    dc.SetPen(m_border_pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawPolygon(WXSIZEOF(border_points), border_points);

    // there are two horizontal grey lines at the bottom of the tab control,
    // this gets rid of the top one of those lines in the tab control
    if (page.active)
    {
       if (m_flags &ewxAUI_NB_BOTTOM)
           dc.SetPen(wxPen(wxColour(ewxAuiStepColour(m_base_colour, 170))));
       // TODO: else if (m_flags &ewxAUI_NB_LEFT) {}
       // TODO: else if (m_flags &ewxAUI_NB_RIGHT) {}
       else //for ewxAUI_NB_TOP
           dc.SetPen(m_base_colour_pen);
        dc.DrawLine(border_points[0].x+1,
                    border_points[0].y,
                    border_points[5].x,
                    border_points[5].y);
    }


    int text_offset = tab_x + 8;
    int close_button_width = 0;
    if (close_button_state != ewxAUI_BUTTON_STATE_HIDDEN)
    {
        close_button_width = m_active_close_bmp.GetWidth();
    }


    int bitmap_offset = 0;
    if (page.bitmap.IsOk())
    {
        bitmap_offset = tab_x + 8;

        // draw bitmap
        dc.DrawBitmap(page.bitmap,
                      bitmap_offset,
                      drawn_tab_yoff + (drawn_tab_height/2) - (page.bitmap.GetHeight()/2),
                      true);

        text_offset = bitmap_offset + page.bitmap.GetWidth();
        text_offset += 3; // bitmap padding
    }
     else
    {
        text_offset = tab_x + 8;
    }


    wxString draw_text = ewxAuiChopText(dc,
                          caption,
                          tab_width - (text_offset-tab_x) - close_button_width);

    // draw tab text
    dc.DrawText(draw_text,
                text_offset,
                drawn_tab_yoff + (drawn_tab_height)/2 - (texty/2) - 1);

    // draw close button if necessary
    if (close_button_state != ewxAUI_BUTTON_STATE_HIDDEN)
    {
        wxBitmap bmp = m_disabled_close_bmp;

        if (close_button_state == ewxAUI_BUTTON_STATE_HOVER ||
            close_button_state == ewxAUI_BUTTON_STATE_PRESSED)
        {
            bmp = m_active_close_bmp;
        }

        int offsetY = tab_y-1;
        if (m_flags & ewxAUI_NB_BOTTOM)
            offsetY = 1;

        wxRect rect(tab_x + tab_width - close_button_width - 1,
                    offsetY + (tab_height/2) - (bmp.GetHeight()/2),
                    close_button_width,
                    tab_height);
                    
        IndentPressedBitmap(&rect, close_button_state);
        dc.DrawBitmap(bmp, rect.x, rect.y, true);

        *out_button_rect = rect;
    }

    *out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);

#ifndef __WXMAC__
    // draw focus rectangle
    if (page.active && (wnd->FindFocus() == wnd))
    {
        wxRect focusRectText(text_offset, (drawn_tab_yoff + (drawn_tab_height)/2 - (texty/2) - 1),
            selected_textx, selected_texty);

        wxRect focusRect;
        wxRect focusRectBitmap;

        if (page.bitmap.IsOk())
            focusRectBitmap = wxRect(bitmap_offset, drawn_tab_yoff + (drawn_tab_height/2) - (page.bitmap.GetHeight()/2),
                                            page.bitmap.GetWidth(), page.bitmap.GetHeight());

        if (page.bitmap.IsOk() && draw_text.IsEmpty())
            focusRect = focusRectBitmap;
        else if (!page.bitmap.IsOk() && !draw_text.IsEmpty())
            focusRect = focusRectText;
        else if (page.bitmap.IsOk() && !draw_text.IsEmpty())
            focusRect = focusRectText.Union(focusRectBitmap);

        focusRect.Inflate(2, 2);

        DrawFocusRect(wnd, dc, focusRect, 0);
    }
#endif

    dc.DestroyClippingRegion();
}

int ewxAuiDefaultTabArt::GetIndentSize()
{
    return 5;
}

wxSize ewxAuiDefaultTabArt::GetTabSize(wxDC& dc,
                                      wxWindow* WXUNUSED(wnd),
                                      const wxString& caption,
                                      const wxBitmap& bitmap,
                                      bool WXUNUSED(active),
                                      int close_button_state,
                                      int* x_extent)
{
    wxCoord measured_textx, measured_texty, tmp;

    dc.SetFont(m_measuring_font);
    dc.GetTextExtent(caption, &measured_textx, &measured_texty);

    dc.GetTextExtent(wxT("ABCDEFXj"), &tmp, &measured_texty);

    // add padding around the text
    wxCoord tab_width = measured_textx;
    wxCoord tab_height = measured_texty;

    // if the close button is showing, add space for it
    if (close_button_state != ewxAUI_BUTTON_STATE_HIDDEN)
        tab_width += m_active_close_bmp.GetWidth() + 3;

    // if there's a bitmap, add space for it
    if (bitmap.IsOk())
    {
        tab_width += bitmap.GetWidth();
        tab_width += 3; // right side bitmap padding
        tab_height = wxMax(tab_height, bitmap.GetHeight());
    }

    // add padding
    tab_width += 16;
    tab_height += 10;

    if (m_flags & ewxAUI_NB_TAB_FIXED_WIDTH)
    {
        tab_width = m_fixed_tab_width;
    }

    *x_extent = tab_width;

    return wxSize(tab_width, tab_height);
}


void ewxAuiDefaultTabArt::DrawButton(wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxRect& in_rect,
                                    int bitmap_id,
                                    int button_state,
                                    int orientation,
                                    wxRect* out_rect)
{
    wxBitmap bmp;
    wxRect rect;

    switch (bitmap_id)
    {
        case ewxAUI_BUTTON_CLOSE:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_close_bmp;
                 else
                bmp = m_active_close_bmp;
            break;
        case ewxAUI_BUTTON_LEFT:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_left_bmp;
                 else
                bmp = m_active_left_bmp;
            break;
        case ewxAUI_BUTTON_RIGHT:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_right_bmp;
                 else
                bmp = m_active_right_bmp;
            break;
        case ewxAUI_BUTTON_WINDOWLIST:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_windowlist_bmp;
                 else
                bmp = m_active_windowlist_bmp;
            break;
    }


    if (!bmp.IsOk())
        return;

    rect = in_rect;

    if (orientation == wxLEFT)
    {
        rect.SetX(in_rect.x);
        rect.SetY(((in_rect.y + in_rect.height)/2) - (bmp.GetHeight()/2));
        rect.SetWidth(bmp.GetWidth());
        rect.SetHeight(bmp.GetHeight());
    }
     else
    {
        rect = wxRect(in_rect.x + in_rect.width - bmp.GetWidth(),
                      ((in_rect.y + in_rect.height)/2) - (bmp.GetHeight()/2),
                      bmp.GetWidth(), bmp.GetHeight());
    }

    IndentPressedBitmap(&rect, button_state);
    dc.DrawBitmap(bmp, rect.x, rect.y, true);

    *out_rect = rect;
}


int ewxAuiDefaultTabArt::ShowDropDown(wxWindow* wnd,
                                     const ewxAuiNotebookPageArray& pages,
                                     int active_idx)
{
    wxMenu menuPopup;

    size_t i, count = pages.GetCount();
    for (i = 0; i < count; ++i)
    {
        const ewxAuiNotebookPage& page = pages.Item(i);
        wxString caption = page.caption;

        // if there is no caption, make it a space.  This will prevent
        // an assert in the menu code.
        if (caption.IsEmpty())
            caption = wxT(" ");

        menuPopup.AppendCheckItem(1000+i, caption);
    }

    if (active_idx != -1)
    {
        menuPopup.Check(1000+active_idx, true);
    }

    // find out where to put the popup menu of window items
    wxPoint pt = ::wxGetMousePosition();
    pt = wnd->ScreenToClient(pt);

    // find out the screen coordinate at the bottom of the tab ctrl
    wxRect cli_rect = wnd->GetClientRect();
    pt.y = cli_rect.y + cli_rect.height;

    ewxAuiCommandCapture* cc = new ewxAuiCommandCapture;
    wnd->PushEventHandler(cc);
    wnd->PopupMenu(&menuPopup, pt);
    int command = cc->GetCommandId();
    wnd->PopEventHandler(true);

    if (command >= 1000)
        return command-1000;

    return -1;
}

int ewxAuiDefaultTabArt::GetBestTabCtrlSize(wxWindow* wnd,
                                           const ewxAuiNotebookPageArray& pages,
                                           const wxSize& required_bmp_size)
{
    wxClientDC dc(wnd);
    dc.SetFont(m_measuring_font);

    // sometimes a standard bitmap size needs to be enforced, especially
    // if some tabs have bitmaps and others don't.  This is important because
    // it prevents the tab control from resizing when tabs are added.
    wxBitmap measure_bmp;
    if (required_bmp_size.IsFullySpecified())
    {
        measure_bmp.Create(required_bmp_size.x,
                           required_bmp_size.y);
    }


    int max_y = 0;
    size_t i, page_count = pages.GetCount();
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = pages.Item(i);

        wxBitmap bmp;
        if (measure_bmp.IsOk())
            bmp = measure_bmp;
             else
            bmp = page.bitmap;

        // we don't use the caption text because we don't
        // want tab heights to be different in the case
        // of a very short piece of text on one tab and a very
        // tall piece of text on another tab
        int x_ext = 0;
        wxSize s = GetTabSize(dc,
                              wnd,
                              wxT("ABCDEFGHIj"),
                              bmp,
                              true,
                              ewxAUI_BUTTON_STATE_HIDDEN,
                              &x_ext);

        max_y = wxMax(max_y, s.y);
    }

    return max_y+2;
}

void ewxAuiDefaultTabArt::SetNormalFont(const wxFont& font)
{
    m_normal_font = font;
}

void ewxAuiDefaultTabArt::SetSelectedFont(const wxFont& font)
{
    m_selected_font = font;
}

void ewxAuiDefaultTabArt::SetMeasuringFont(const wxFont& font)
{
    m_measuring_font = font;
}


// -- ewxAuiSimpleTabArt class implementation --

ewxAuiSimpleTabArt::ewxAuiSimpleTabArt()
{
    m_normal_font = *wxNORMAL_FONT;
    m_selected_font = *wxNORMAL_FONT;
    m_selected_font.SetWeight(wxBOLD);
    m_measuring_font = m_selected_font;

    m_flags = 0;
    m_fixed_tab_width = 100;

    wxColour base_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);

    wxColour background_colour = base_colour;
    wxColour normaltab_colour = base_colour;
    wxColour selectedtab_colour = *wxWHITE;

    m_bkbrush = wxBrush(background_colour);
    m_normal_bkbrush = wxBrush(normaltab_colour);
    m_normal_bkpen = wxPen(normaltab_colour);
    m_selected_bkbrush = wxBrush(selectedtab_colour);
    m_selected_bkpen = wxPen(selectedtab_colour);

    m_active_close_bmp = ewxAuiBitmapFromBits(close_bits, 16, 16, *wxBLACK);
    m_disabled_close_bmp = ewxAuiBitmapFromBits(close_bits, 16, 16, wxColour(128,128,128));

    m_active_left_bmp = ewxAuiBitmapFromBits(left_bits, 16, 16, *wxBLACK);
    m_disabled_left_bmp = ewxAuiBitmapFromBits(left_bits, 16, 16, wxColour(128,128,128));

    m_active_right_bmp = ewxAuiBitmapFromBits(right_bits, 16, 16, *wxBLACK);
    m_disabled_right_bmp = ewxAuiBitmapFromBits(right_bits, 16, 16, wxColour(128,128,128));

    m_active_windowlist_bmp = ewxAuiBitmapFromBits(list_bits, 16, 16, *wxBLACK);
    m_disabled_windowlist_bmp = ewxAuiBitmapFromBits(list_bits, 16, 16, wxColour(128,128,128));

}

ewxAuiSimpleTabArt::~ewxAuiSimpleTabArt()
{
}

ewxAuiTabArt* ewxAuiSimpleTabArt::Clone()
{
    return wx_static_cast(ewxAuiTabArt*, new ewxAuiSimpleTabArt);
}


void ewxAuiSimpleTabArt::SetFlags(unsigned int flags)
{
    m_flags = flags;
}

void ewxAuiSimpleTabArt::SetSizingInfo(const wxSize& tab_ctrl_size,
                                      size_t tab_count)
{
    m_fixed_tab_width = 100;

    int tot_width = (int)tab_ctrl_size.x - GetIndentSize() - 4;

    if (m_flags & ewxAUI_NB_CLOSE_BUTTON)
        tot_width -= m_active_close_bmp.GetWidth();
    if (m_flags & ewxAUI_NB_WINDOWLIST_BUTTON)
        tot_width -= m_active_windowlist_bmp.GetWidth();

    if (tab_count > 0)
    {
        m_fixed_tab_width = tot_width/(int)tab_count;
    }


    if (m_fixed_tab_width < 100)
        m_fixed_tab_width = 100;

    if (m_fixed_tab_width > tot_width/2)
        m_fixed_tab_width = tot_width/2;

    if (m_fixed_tab_width > 220)
        m_fixed_tab_width = 220;
}

void ewxAuiSimpleTabArt::DrawBackground(wxDC& dc,
                                       wxWindow* WXUNUSED(wnd),
                                       const wxRect& rect)
{
    // draw background
    dc.SetBrush(m_bkbrush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(-1, -1, rect.GetWidth()+2, rect.GetHeight()+2);

    // draw base line
    dc.SetPen(*wxGREY_PEN);
    dc.DrawLine(0, rect.GetHeight()-1, rect.GetWidth(), rect.GetHeight()-1);
}


// DrawTab() draws an individual tab.
//
// dc       - output dc
// in_rect  - rectangle the tab should be confined to
// caption  - tab's caption
// active   - whether or not the tab is active
// out_rect - actual output rectangle
// x_extent - the advance x; where the next tab should start

void ewxAuiSimpleTabArt::DrawTab(wxDC& dc,
                                wxWindow* wnd,
                                const ewxAuiNotebookPage& page,
                                const wxRect& in_rect,
                                int close_button_state,
                                wxRect* out_tab_rect,
                                wxRect* out_button_rect,
                                int* x_extent)
{
    wxCoord normal_textx, normal_texty;
    wxCoord selected_textx, selected_texty;
    wxCoord textx, texty;

    // if the caption is empty, measure some temporary text
    wxString caption = page.caption;
    if (caption.empty())
        caption = wxT("Xj");

    dc.SetFont(m_selected_font);
    dc.GetTextExtent(caption, &selected_textx, &selected_texty);

    dc.SetFont(m_normal_font);
    dc.GetTextExtent(caption, &normal_textx, &normal_texty);

    // figure out the size of the tab
    wxSize tab_size = GetTabSize(dc,
                                 wnd,
                                 page.caption,
                                 page.bitmap,
                                 page.active,
                                 close_button_state,
                                 x_extent);

    wxCoord tab_height = tab_size.y;
    wxCoord tab_width = tab_size.x;
    wxCoord tab_x = in_rect.x;
    wxCoord tab_y = in_rect.y + in_rect.height - tab_height;

    caption = page.caption;

    // select pen, brush and font for the tab to be drawn

    if (page.active)
    {
        dc.SetPen(m_selected_bkpen);
        dc.SetBrush(m_selected_bkbrush);
        dc.SetFont(m_selected_font);
        textx = selected_textx;
        texty = selected_texty;
    }
     else
    {
        dc.SetPen(m_normal_bkpen);
        dc.SetBrush(m_normal_bkbrush);
        dc.SetFont(m_normal_font);
        textx = normal_textx;
        texty = normal_texty;
    }


    // -- draw line --

    wxPoint points[7];
    points[0].x = tab_x;
    points[0].y = tab_y + tab_height - 1;
    points[1].x = tab_x + tab_height - 3;
    points[1].y = tab_y + 2;
    points[2].x = tab_x + tab_height + 3;
    points[2].y = tab_y;
    points[3].x = tab_x + tab_width - 2;
    points[3].y = tab_y;
    points[4].x = tab_x + tab_width;
    points[4].y = tab_y + 2;
    points[5].x = tab_x + tab_width;
    points[5].y = tab_y + tab_height - 1;
    points[6] = points[0];

    dc.SetClippingRegion(in_rect);

    dc.DrawPolygon(WXSIZEOF(points) - 1, points);

    dc.SetPen(*wxGREY_PEN);

    //dc.DrawLines(active ? WXSIZEOF(points) - 1 : WXSIZEOF(points), points);
    dc.DrawLines(WXSIZEOF(points), points);


    int text_offset;

    int close_button_width = 0;
    if (close_button_state != ewxAUI_BUTTON_STATE_HIDDEN)
    {
        close_button_width = m_active_close_bmp.GetWidth();
        text_offset = tab_x + (tab_height/2) + ((tab_width-close_button_width)/2) - (textx/2);
    }
     else
    {
        text_offset = tab_x + (tab_height/3) + (tab_width/2) - (textx/2);
    }

    // set minimum text offset
    if (text_offset < tab_x + tab_height)
        text_offset = tab_x + tab_height;

    // chop text if necessary
    wxString draw_text = ewxAuiChopText(dc,
                          caption,
                          tab_width - (text_offset-tab_x) - close_button_width);

    // draw tab text
    dc.DrawText(draw_text,
                 text_offset,
                 (tab_y + tab_height)/2 - (texty/2) + 1);


#ifndef __WXMAC__
    // draw focus rectangle
    if (page.active && (wnd->FindFocus() == wnd))
    {
        wxRect focusRect(text_offset, ((tab_y + tab_height)/2 - (texty/2) + 1),
            selected_textx, selected_texty);

        focusRect.Inflate(2, 2);

        DrawFocusRect(wnd, dc, focusRect, 0);
    }
#endif

    // draw close button if necessary
    if (close_button_state != ewxAUI_BUTTON_STATE_HIDDEN)
    {
        wxBitmap bmp;
        if (page.active)
            bmp = m_active_close_bmp;
             else
            bmp = m_disabled_close_bmp;

        wxRect rect(tab_x + tab_width - close_button_width - 1,
                    tab_y + (tab_height/2) - (bmp.GetHeight()/2) + 1,
                    close_button_width,
                    tab_height - 1);
        DrawButtons(dc, rect, bmp, *wxWHITE, close_button_state);

        *out_button_rect = rect;
    }


    *out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);

    dc.DestroyClippingRegion();
}

int ewxAuiSimpleTabArt::GetIndentSize()
{
    return 0;
}

wxSize ewxAuiSimpleTabArt::GetTabSize(wxDC& dc,
                                     wxWindow* WXUNUSED(wnd),
                                     const wxString& caption,
                                     const wxBitmap& WXUNUSED(bitmap),
                                     bool WXUNUSED(active),
                                     int close_button_state,
                                     int* x_extent)
{
    wxCoord measured_textx, measured_texty;

    dc.SetFont(m_measuring_font);
    dc.GetTextExtent(caption, &measured_textx, &measured_texty);

    wxCoord tab_height = measured_texty + 4;
    wxCoord tab_width = measured_textx + tab_height + 5;

    if (close_button_state != ewxAUI_BUTTON_STATE_HIDDEN)
        tab_width += m_active_close_bmp.GetWidth();

    if (m_flags & ewxAUI_NB_TAB_FIXED_WIDTH)
    {
        tab_width = m_fixed_tab_width;
    }

    *x_extent = tab_width - (tab_height/2) - 1;

    return wxSize(tab_width, tab_height);
}


void ewxAuiSimpleTabArt::DrawButton(wxDC& dc,
                                   wxWindow* WXUNUSED(wnd),
                                   const wxRect& in_rect,
                                   int bitmap_id,
                                   int button_state,
                                   int orientation,
                                   wxRect* out_rect)
{
    wxBitmap bmp;
    wxRect rect;

    switch (bitmap_id)
    {
        case ewxAUI_BUTTON_CLOSE:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_close_bmp;
                 else
                bmp = m_active_close_bmp;
            break;
        case ewxAUI_BUTTON_LEFT:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_left_bmp;
                 else
                bmp = m_active_left_bmp;
            break;
        case ewxAUI_BUTTON_RIGHT:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_right_bmp;
                 else
                bmp = m_active_right_bmp;
            break;
        case ewxAUI_BUTTON_WINDOWLIST:
            if (button_state & ewxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_windowlist_bmp;
                 else
                bmp = m_active_windowlist_bmp;
            break;
    }

    if (!bmp.IsOk())
        return;

    rect = in_rect;

    if (orientation == wxLEFT)
    {
        rect.SetX(in_rect.x);
        rect.SetY(((in_rect.y + in_rect.height)/2) - (bmp.GetHeight()/2));
        rect.SetWidth(bmp.GetWidth());
        rect.SetHeight(bmp.GetHeight());
    }
     else
    {
        rect = wxRect(in_rect.x + in_rect.width - bmp.GetWidth(),
                      ((in_rect.y + in_rect.height)/2) - (bmp.GetHeight()/2),
                      bmp.GetWidth(), bmp.GetHeight());
    }


    DrawButtons(dc, rect, bmp, *wxWHITE, button_state);

    *out_rect = rect;
}


int ewxAuiSimpleTabArt::ShowDropDown(wxWindow* wnd,
                                    const ewxAuiNotebookPageArray& pages,
                                    int active_idx)
{
    wxMenu menuPopup;

    size_t i, count = pages.GetCount();
    for (i = 0; i < count; ++i)
    {
        const ewxAuiNotebookPage& page = pages.Item(i);
        menuPopup.AppendCheckItem(1000+i, page.caption);
    }

    if (active_idx != -1)
    {
        menuPopup.Check(1000+active_idx, true);
    }

    // find out where to put the popup menu of window
    // items.  Subtract 100 for now to center the menu
    // a bit, until a better mechanism can be implemented
    wxPoint pt = ::wxGetMousePosition();
    pt = wnd->ScreenToClient(pt);
    if (pt.x < 100)
        pt.x = 0;
         else
        pt.x -= 100;

    // find out the screen coordinate at the bottom of the tab ctrl
    wxRect cli_rect = wnd->GetClientRect();
    pt.y = cli_rect.y + cli_rect.height;

    ewxAuiCommandCapture* cc = new ewxAuiCommandCapture;
    wnd->PushEventHandler(cc);
    wnd->PopupMenu(&menuPopup, pt);
    int command = cc->GetCommandId();
    wnd->PopEventHandler(true);

    if (command >= 1000)
        return command-1000;

    return -1;
}

int ewxAuiSimpleTabArt::GetBestTabCtrlSize(wxWindow* wnd,
                                          const ewxAuiNotebookPageArray& WXUNUSED(pages),
                                          const wxSize& WXUNUSED(required_bmp_size))
{
    wxClientDC dc(wnd);
    dc.SetFont(m_measuring_font);
    int x_ext = 0;
    wxSize s = GetTabSize(dc,
                          wnd,
                          wxT("ABCDEFGHIj"),
                          wxNullBitmap,
                          true,
                          ewxAUI_BUTTON_STATE_HIDDEN,
                          &x_ext);
    return s.y+3;
}

void ewxAuiSimpleTabArt::SetNormalFont(const wxFont& font)
{
    m_normal_font = font;
}

void ewxAuiSimpleTabArt::SetSelectedFont(const wxFont& font)
{
    m_selected_font = font;
}

void ewxAuiSimpleTabArt::SetMeasuringFont(const wxFont& font)
{
    m_measuring_font = font;
}




// -- ewxAuiTabContainer class implementation --


// ewxAuiTabContainer is a class which contains information about each
// tab.  It also can render an entire tab control to a specified DC.
// It's not a window class itself, because this code will be used by
// the wxFrameMananger, where it is disadvantageous to have separate
// windows for each tab control in the case of "docked tabs"

// A derived class, ewxAuiTabCtrl, is an actual wxWindow-derived window
// which can be used as a tab control in the normal sense.


ewxAuiTabContainer::ewxAuiTabContainer()
{
    m_tab_offset = 0;
    m_flags = 0;
    m_art = new ewxAuiDefaultTabArt;

    AddButton(ewxAUI_BUTTON_LEFT, wxLEFT);
    AddButton(ewxAUI_BUTTON_RIGHT, wxRIGHT);
    AddButton(ewxAUI_BUTTON_WINDOWLIST, wxRIGHT);
    AddButton(ewxAUI_BUTTON_CLOSE, wxRIGHT);
}

ewxAuiTabContainer::~ewxAuiTabContainer()
{
    delete m_art;
}

void ewxAuiTabContainer::SetArtProvider(ewxAuiTabArt* art)
{
    delete m_art;
    m_art = art;

    if (m_art)
    {
        m_art->SetFlags(m_flags);
    }
}

ewxAuiTabArt* ewxAuiTabContainer::GetArtProvider() const
{
    return m_art;
}

void ewxAuiTabContainer::SetFlags(unsigned int flags)
{
    m_flags = flags;

    // check for new close button settings
    RemoveButton(ewxAUI_BUTTON_LEFT);
    RemoveButton(ewxAUI_BUTTON_RIGHT);
    RemoveButton(ewxAUI_BUTTON_WINDOWLIST);
    RemoveButton(ewxAUI_BUTTON_CLOSE);


    if (flags & ewxAUI_NB_SCROLL_BUTTONS)
    {
        AddButton(ewxAUI_BUTTON_LEFT, wxLEFT);
        AddButton(ewxAUI_BUTTON_RIGHT, wxRIGHT);
    }

    if (flags & ewxAUI_NB_WINDOWLIST_BUTTON)
    {
        AddButton(ewxAUI_BUTTON_WINDOWLIST, wxRIGHT);
    }

    if (flags & ewxAUI_NB_CLOSE_BUTTON)
    {
        AddButton(ewxAUI_BUTTON_CLOSE, wxRIGHT);
    }

    if (m_art)
    {
        m_art->SetFlags(m_flags);
    }
}

unsigned int ewxAuiTabContainer::GetFlags() const
{
    return m_flags;
}


void ewxAuiTabContainer::SetNormalFont(const wxFont& font)
{
    m_art->SetNormalFont(font);
}

void ewxAuiTabContainer::SetSelectedFont(const wxFont& font)
{
    m_art->SetSelectedFont(font);
}

void ewxAuiTabContainer::SetMeasuringFont(const wxFont& font)
{
    m_art->SetMeasuringFont(font);
}

void ewxAuiTabContainer::SetRect(const wxRect& rect)
{
    m_rect = rect;

    if (m_art)
    {
        m_art->SetSizingInfo(rect.GetSize(), m_pages.GetCount());
    }
}

bool ewxAuiTabContainer::AddPage(wxWindow* page,
                                const ewxAuiNotebookPage& info)
{
    ewxAuiNotebookPage page_info;
    page_info = info;
    page_info.window = page;

    m_pages.Add(page_info);

    // let the art provider know how many pages we have
    if (m_art)
    {
        m_art->SetSizingInfo(m_rect.GetSize(), m_pages.GetCount());
    }

    return true;
}

bool ewxAuiTabContainer::InsertPage(wxWindow* page,
                                   const ewxAuiNotebookPage& info,
                                   size_t idx)
{
    ewxAuiNotebookPage page_info;
    page_info = info;
    page_info.window = page;

    if (idx >= m_pages.GetCount())
        m_pages.Add(page_info);
         else
        m_pages.Insert(page_info, idx);

    // let the art provider know how many pages we have
    if (m_art)
    {
        m_art->SetSizingInfo(m_rect.GetSize(), m_pages.GetCount());
    }

    return true;
}

bool ewxAuiTabContainer::MovePage(wxWindow* page,
                                 size_t new_idx)
{
    int idx = GetIdxFromWindow(page);
    if (idx == -1)
        return false;

    // get page entry, make a copy of it
    ewxAuiNotebookPage p = GetPage(idx);

    // remove old page entry
    RemovePage(page);

    // insert page where it should be
    InsertPage(page, p, new_idx);

    return true;
}

bool ewxAuiTabContainer::RemovePage(wxWindow* wnd)
{
    size_t i, page_count = m_pages.GetCount();
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        if (page.window == wnd)
        {
            m_pages.RemoveAt(i);

            // let the art provider know how many pages we have
            if (m_art)
            {
                m_art->SetSizingInfo(m_rect.GetSize(), m_pages.GetCount());
            }

            return true;
        }
    }

    return false;
}

bool ewxAuiTabContainer::SetActivePage(wxWindow* wnd)
{
    bool found = false;

    size_t i, page_count = m_pages.GetCount();
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        if (page.window == wnd)
        {
            page.active = true;
            found = true;
        }
         else
        {
            page.active = false;
        }
    }

    return found;
}

void ewxAuiTabContainer::SetNoneActive()
{
    size_t i, page_count = m_pages.GetCount();
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        page.active = false;
    }
}

bool ewxAuiTabContainer::SetActivePage(size_t page)
{
    if (page >= m_pages.GetCount())
        return false;

    return SetActivePage(m_pages.Item(page).window);
}

int ewxAuiTabContainer::GetActivePage() const
{
    size_t i, page_count = m_pages.GetCount();
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        if (page.active)
            return i;
    }

    return -1;
}

wxWindow* ewxAuiTabContainer::GetWindowFromIdx(size_t idx) const
{
    if (idx >= m_pages.GetCount())
        return NULL;

    return m_pages[idx].window;
}

int ewxAuiTabContainer::GetIdxFromWindow(wxWindow* wnd) const
{
    size_t i, page_count = m_pages.GetCount();
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        if (page.window == wnd)
            return i;
    }
    return -1;
}

ewxAuiNotebookPage& ewxAuiTabContainer::GetPage(size_t idx)
{
    wxASSERT_MSG(idx < m_pages.GetCount(), wxT("Invalid Page index"));

    return m_pages[idx];
}

const ewxAuiNotebookPage& ewxAuiTabContainer::GetPage(size_t idx) const
{
    wxASSERT_MSG(idx < m_pages.GetCount(), wxT("Invalid Page index"));

    return m_pages[idx];
}

ewxAuiNotebookPageArray& ewxAuiTabContainer::GetPages()
{
    return m_pages;
}

size_t ewxAuiTabContainer::GetPageCount() const
{
    return m_pages.GetCount();
}

void ewxAuiTabContainer::AddButton(int id,
                                  int location,
                                  const wxBitmap& normal_bitmap,
                                  const wxBitmap& disabled_bitmap)
{
    ewxAuiTabContainerButton button;
    button.id = id;
    button.bitmap = normal_bitmap;
    button.dis_bitmap = disabled_bitmap;
    button.location = location;
    button.cur_state = ewxAUI_BUTTON_STATE_NORMAL;

    m_buttons.Add(button);
}

void ewxAuiTabContainer::RemoveButton(int id)
{
    size_t i, button_count = m_buttons.GetCount();

    for (i = 0; i < button_count; ++i)
    {
        if (m_buttons.Item(i).id == id)
        {
            m_buttons.RemoveAt(i);
            return;
        }
    }
}



size_t ewxAuiTabContainer::GetTabOffset() const
{
    return m_tab_offset;
}

void ewxAuiTabContainer::SetTabOffset(size_t offset)
{
    m_tab_offset = offset;
}




// Render() renders the tab catalog to the specified DC
// It is a virtual function and can be overridden to
// provide custom drawing capabilities
void ewxAuiTabContainer::Render(wxDC* raw_dc, wxWindow* wnd)
{
    if (!raw_dc || !raw_dc->IsOk())
        return;

    wxMemoryDC dc;

    // use the same layout direction as the window DC uses to ensure that the
    // text is rendered correctly
    dc.SetLayoutDirection(raw_dc->GetLayoutDirection());

    wxBitmap bmp;
    size_t i;
    size_t page_count = m_pages.GetCount();
    size_t button_count = m_buttons.GetCount();

    // create off-screen bitmap
    bmp.Create(m_rect.GetWidth(), m_rect.GetHeight());
    dc.SelectObject(bmp);

    if (!dc.IsOk())
        return;

    // find out if size of tabs is larger than can be
    // afforded on screen
    int total_width = 0;
    int visible_width = 0;

    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);

        // determine if a close button is on this tab
        bool close_button = false;
        if ((m_flags & ewxAUI_NB_CLOSE_ON_ALL_TABS) != 0 ||
            ((m_flags & ewxAUI_NB_CLOSE_ON_ACTIVE_TAB) != 0 && page.active))
        {
            close_button = true;
        }


        int x_extent = 0;
        wxSize size = m_art->GetTabSize(dc,
                            wnd,
                            page.caption,
                            page.bitmap,
                            page.active,
                            close_button ?
                              ewxAUI_BUTTON_STATE_NORMAL :
                              ewxAUI_BUTTON_STATE_HIDDEN,
                            &x_extent);

        if (i+1 < page_count)
            total_width += x_extent;
             else
            total_width += size.x;

        if (i >= m_tab_offset)
        {
            if (i+1 < page_count)
                visible_width += x_extent;
                 else
                visible_width += size.x;
        }
    }

    if (total_width > m_rect.GetWidth() || m_tab_offset != 0)
    {
        // show left/right buttons
        for (i = 0; i < button_count; ++i)
        {
            ewxAuiTabContainerButton& button = m_buttons.Item(i);
            if (button.id == ewxAUI_BUTTON_LEFT ||
                button.id == ewxAUI_BUTTON_RIGHT)
            {
                button.cur_state &= ~ewxAUI_BUTTON_STATE_HIDDEN;
            }
        }
    }
     else
    {
        // hide left/right buttons
        for (i = 0; i < button_count; ++i)
        {
            ewxAuiTabContainerButton& button = m_buttons.Item(i);
            if (button.id == ewxAUI_BUTTON_LEFT ||
                button.id == ewxAUI_BUTTON_RIGHT)
            {
                button.cur_state |= ewxAUI_BUTTON_STATE_HIDDEN;
            }
        }
    }

    // determine whether left button should be enabled
    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_buttons.Item(i);
        if (button.id == ewxAUI_BUTTON_LEFT)
        {
            if (m_tab_offset == 0)
                button.cur_state |= ewxAUI_BUTTON_STATE_DISABLED;
                 else
                button.cur_state &= ~ewxAUI_BUTTON_STATE_DISABLED;
        }
        if (button.id == ewxAUI_BUTTON_RIGHT)
        {
            if (visible_width < m_rect.GetWidth() - ((int)button_count*16))
                button.cur_state |= ewxAUI_BUTTON_STATE_DISABLED;
                 else
                button.cur_state &= ~ewxAUI_BUTTON_STATE_DISABLED;
        }
    }



    // draw background
    m_art->DrawBackground(dc, wnd, m_rect);

    // draw buttons
    int left_buttons_width = 0;
    int right_buttons_width = 0;

    int offset = 0;

    // draw the buttons on the right side
    offset = m_rect.x + m_rect.width;
    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_buttons.Item(button_count - i - 1);

        if (button.location != wxRIGHT)
            continue;
        if (button.cur_state & ewxAUI_BUTTON_STATE_HIDDEN)
            continue;

        wxRect button_rect = m_rect;
        button_rect.SetY(1);
        button_rect.SetWidth(offset);

        m_art->DrawButton(dc,
                          wnd,
                          button_rect,
                          button.id,
                          button.cur_state,
                          wxRIGHT,
                          &button.rect);

        offset -= button.rect.GetWidth();
        right_buttons_width += button.rect.GetWidth();
    }



    offset = 0;

    // draw the buttons on the left side

    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_buttons.Item(button_count - i - 1);

        if (button.location != wxLEFT)
            continue;
        if (button.cur_state & ewxAUI_BUTTON_STATE_HIDDEN)
            continue;

        wxRect button_rect(offset, 1, 1000, m_rect.height);

        m_art->DrawButton(dc,
                          wnd,
                          button_rect,
                          button.id,
                          button.cur_state,
                          wxLEFT,
                          &button.rect);

        offset += button.rect.GetWidth();
        left_buttons_width += button.rect.GetWidth();
    }

    offset = left_buttons_width;

    if (offset == 0)
        offset += m_art->GetIndentSize();


    // prepare the tab-close-button array
    // make sure tab button entries which aren't used are marked as hidden
    for (i = page_count; i < m_tab_close_buttons.GetCount(); ++i)
        m_tab_close_buttons.Item(i).cur_state = ewxAUI_BUTTON_STATE_HIDDEN;

    // make sure there are enough tab button entries to accommodate all tabs
    while (m_tab_close_buttons.GetCount() < page_count)
    {
        ewxAuiTabContainerButton tempbtn;
        tempbtn.id = ewxAUI_BUTTON_CLOSE;
        tempbtn.location = wxCENTER;
        tempbtn.cur_state = ewxAUI_BUTTON_STATE_HIDDEN;
        m_tab_close_buttons.Add(tempbtn);
    }


    // buttons before the tab offset must be set to hidden
    for (i = 0; i < m_tab_offset; ++i)
    {
        m_tab_close_buttons.Item(i).cur_state = ewxAUI_BUTTON_STATE_HIDDEN;
    }


    // draw the tabs

    size_t active = 999;
    int active_offset = 0;
    wxRect active_rect;
    wxRect active_focus_rect;

    int x_extent = 0;
    wxRect rect = m_rect;
    rect.y = 0;
    rect.height = m_rect.height;

    for (i = m_tab_offset; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        ewxAuiTabContainerButton& tab_button = m_tab_close_buttons.Item(i);

        // determine if a close button is on this tab
        if ((m_flags & ewxAUI_NB_CLOSE_ON_ALL_TABS) != 0 ||
            ((m_flags & ewxAUI_NB_CLOSE_ON_ACTIVE_TAB) != 0 && page.active))
        {
            if (tab_button.cur_state == ewxAUI_BUTTON_STATE_HIDDEN)
            {
                tab_button.id = ewxAUI_BUTTON_CLOSE;
                tab_button.cur_state = ewxAUI_BUTTON_STATE_NORMAL;
                tab_button.location = wxCENTER;
            }
        }
         else
        {
            tab_button.cur_state = ewxAUI_BUTTON_STATE_HIDDEN;
        }

        rect.x = offset;
        rect.width = m_rect.width - right_buttons_width - offset - 2;

        if (rect.width <= 0)
            break;

        m_art->DrawTab(dc,
                       wnd,
                       page,
                       rect,
                       tab_button.cur_state,
                       &page.rect,
                       &tab_button.rect,
                       &x_extent);

        if (page.active)
        {
            active = i;
            active_offset = offset;
            active_rect = rect;
            active_focus_rect = rect;
            active_focus_rect.width = x_extent;
        }

        offset += x_extent;
    }


    // make sure to deactivate buttons which are off the screen to the right
    for (++i; i < m_tab_close_buttons.GetCount(); ++i)
    {
        m_tab_close_buttons.Item(i).cur_state = ewxAUI_BUTTON_STATE_HIDDEN;
    }


    // draw the active tab again so it stands in the foreground
    if (active >= m_tab_offset && active < m_pages.GetCount())
    {
        ewxAuiNotebookPage& page = m_pages.Item(active);

        ewxAuiTabContainerButton& tab_button = m_tab_close_buttons.Item(active);

        rect.x = active_offset;
        m_art->DrawTab(dc,
                       wnd,
                       page,
                       active_rect,
                       tab_button.cur_state,
                       &page.rect,
                       &tab_button.rect,
                       &x_extent);
    }


    raw_dc->Blit(m_rect.x, m_rect.y,
                 m_rect.GetWidth(), m_rect.GetHeight(),
                 &dc, 0, 0);

#ifdef __WXMAC__
    // On Mac, need to draw the focus rect directly to the window
    if (wnd && (wnd->FindFocus() == wnd) && (active >= m_tab_offset && active < m_pages.GetCount()))
    {
        wxRect focusRect(active_focus_rect);
        focusRect.Inflate(-6, -6);
        DrawFocusRect(wnd, * raw_dc, focusRect, 0);
    }
#endif
}

// Is the tab visible?
bool ewxAuiTabContainer::IsTabVisible(int tabPage, int tabOffset, wxDC* dc, wxWindow* wnd)
{
    if (!dc || !dc->IsOk())
        return false;

    size_t i;
    size_t page_count = m_pages.GetCount();
    size_t button_count = m_buttons.GetCount();

    // Hasn't been rendered yet; assume it's visible
    if (m_tab_close_buttons.GetCount() < page_count)
        return true;

    // First check if both buttons are disabled - if so, there's no need to
    // check further for visibility.
    int arrowButtonVisibleCount = 0;
    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_buttons.Item(i);
        if (button.id == ewxAUI_BUTTON_LEFT ||
            button.id == ewxAUI_BUTTON_RIGHT)
        {
            if ((button.cur_state & ewxAUI_BUTTON_STATE_HIDDEN) == 0)
                arrowButtonVisibleCount ++;
        }
    }

    // Tab must be visible
    if (arrowButtonVisibleCount == 0)
        return true;

    // If tab is less than the given offset, it must be invisible by definition
    if (tabPage < tabOffset)
        return false;

    // draw buttons
    int left_buttons_width = 0;
    int right_buttons_width = 0;

    int offset = 0;

    // calculate size of the buttons on the right side
    offset = m_rect.x + m_rect.width;
    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_buttons.Item(button_count - i - 1);

        if (button.location != wxRIGHT)
            continue;
        if (button.cur_state & ewxAUI_BUTTON_STATE_HIDDEN)
            continue;

        offset -= button.rect.GetWidth();
        right_buttons_width += button.rect.GetWidth();
    }

    offset = 0;

    // calculate size of the buttons on the left side
    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_buttons.Item(button_count - i - 1);

        if (button.location != wxLEFT)
            continue;
        if (button.cur_state & ewxAUI_BUTTON_STATE_HIDDEN)
            continue;

        offset += button.rect.GetWidth();
        left_buttons_width += button.rect.GetWidth();
    }

    offset = left_buttons_width;

    if (offset == 0)
        offset += m_art->GetIndentSize();

    wxRect active_rect;

    wxRect rect = m_rect;
    rect.y = 0;
    rect.height = m_rect.height;

    // See if the given page is visible at the given tab offset (effectively scroll position)
    for (i = tabOffset; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        ewxAuiTabContainerButton& tab_button = m_tab_close_buttons.Item(i);

        rect.x = offset;
        rect.width = m_rect.width - right_buttons_width - offset - 2;

        if (rect.width <= 0)
            return false; // haven't found the tab, and we've run out of space, so return false

        int x_extent = 0;
        wxSize size = m_art->GetTabSize(*dc,
                            wnd,
                            page.caption,
                            page.bitmap,
                            page.active,
                            tab_button.cur_state,
                            &x_extent);

        offset += x_extent;

        if (i == (size_t) tabPage)
        {
            // If not all of the tab is visible, and supposing there's space to display it all,
            // we could do better so we return false.
            if (((m_rect.width - right_buttons_width - offset - 2) <= 0) && ((m_rect.width - right_buttons_width - left_buttons_width) > x_extent))
                return false;
            else
                return true;
        }
    }

    // Shouldn't really get here, but if it does, assume the tab is visible to prevent
    // further looping in calling code.
    return true;
}

// Make the tab visible if it wasn't already
void ewxAuiTabContainer::MakeTabVisible(int tabPage, wxWindow* win)
{
    wxClientDC dc(win);
    if (!IsTabVisible(tabPage, GetTabOffset(), & dc, win))
    {
        int i;
        for (i = 0; i < (int) m_pages.GetCount(); i++)
        {
            if (IsTabVisible(tabPage, i, & dc, win))
            {
                SetTabOffset(i);
                win->Refresh();
                return;
            }
        }
    }
}

// TabHitTest() tests if a tab was hit, passing the window pointer
// back if that condition was fulfilled.  The function returns
// true if a tab was hit, otherwise false
bool ewxAuiTabContainer::TabHitTest(int x, int y, wxWindow** hit) const
{
    if (!m_rect.Contains(x,y))
        return false;

    ewxAuiTabContainerButton* btn = NULL;
    if (ButtonHitTest(x, y, &btn) && !(btn->cur_state & ewxAUI_BUTTON_STATE_DISABLED))
    {
        if (m_buttons.Index(*btn) != wxNOT_FOUND)
            return false;
    }

    size_t i, page_count = m_pages.GetCount();

    for (i = m_tab_offset; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = m_pages.Item(i);
        if (page.rect.Contains(x,y))
        {
            if (hit)
                *hit = page.window;
            return true;
        }
    }

    return false;
}

// ButtonHitTest() tests if a button was hit. The function returns
// true if a button was hit, otherwise false
bool ewxAuiTabContainer::ButtonHitTest(int x, int y,
                                      ewxAuiTabContainerButton** hit) const
{
    if (!m_rect.Contains(x,y))
        return false;

    size_t i, button_count;


    button_count = m_buttons.GetCount();
    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_buttons.Item(i);
        if (button.rect.Contains(x,y) &&
            !(button.cur_state & ewxAUI_BUTTON_STATE_HIDDEN ))
        {
            if (hit)
                *hit = &button;
            return true;
        }
    }

    button_count = m_tab_close_buttons.GetCount();
    for (i = 0; i < button_count; ++i)
    {
        ewxAuiTabContainerButton& button = m_tab_close_buttons.Item(i);
        if (button.rect.Contains(x,y) &&
            !(button.cur_state & (ewxAUI_BUTTON_STATE_HIDDEN |
                                   ewxAUI_BUTTON_STATE_DISABLED)))
        {
            if (hit)
                *hit = &button;
            return true;
        }
    }

    return false;
}



// the utility function ShowWnd() is the same as show,
// except it handles ewxAuiMDIChildFrame windows as well,
// as the Show() method on this class is "unplugged"
static void ShowWnd(wxWindow* wnd, bool show)
{
#if wxUSE_MDI
    if (wnd->IsKindOf(CLASSINFO(ewxAuiMDIChildFrame)))
    {
        ewxAuiMDIChildFrame* cf = (ewxAuiMDIChildFrame*)wnd;
        cf->DoShow(show);
    }
    else
#endif
    {
        wnd->Show(show);
    }
}


// DoShowHide() this function shows the active window, then
// hides all of the other windows (in that order)
void ewxAuiTabContainer::DoShowHide()
{
    ewxAuiNotebookPageArray& pages = GetPages();
    size_t i, page_count = pages.GetCount();

    // show new active page first
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = pages.Item(i);
        if (page.active)
        {
            ShowWnd(page.window, true);
            break;
        }
    }

    // hide all other pages
    for (i = 0; i < page_count; ++i)
    {
        ewxAuiNotebookPage& page = pages.Item(i);
        if (!page.active)
            ShowWnd(page.window, false);
    }
}






// -- ewxAuiTabCtrl class implementation --



BEGIN_EVENT_TABLE(ewxAuiTabCtrl, wxControl)
    EVT_PAINT(ewxAuiTabCtrl::OnPaint)
    EVT_ERASE_BACKGROUND(ewxAuiTabCtrl::OnEraseBackground)
    EVT_SIZE(ewxAuiTabCtrl::OnSize)
    EVT_LEFT_DOWN(ewxAuiTabCtrl::OnLeftDown)
    EVT_LEFT_DCLICK(ewxAuiTabCtrl::OnLeftDClick)
    EVT_LEFT_UP(ewxAuiTabCtrl::OnLeftUp)
    EVT_MIDDLE_DOWN(ewxAuiTabCtrl::OnMiddleDown)
    EVT_MIDDLE_UP(ewxAuiTabCtrl::OnMiddleUp)
    EVT_RIGHT_DOWN(ewxAuiTabCtrl::OnRightDown)
    EVT_RIGHT_UP(ewxAuiTabCtrl::OnRightUp)
    EVT_MOTION(ewxAuiTabCtrl::OnMotion)
    EVT_LEAVE_WINDOW(ewxAuiTabCtrl::OnLeaveWindow)
    EVT_EWXAUINOTEBOOK_BUTTON(wxID_ANY, ewxAuiTabCtrl::OnButton)
    EVT_SET_FOCUS(ewxAuiTabCtrl::OnSetFocus)
    EVT_KILL_FOCUS(ewxAuiTabCtrl::OnKillFocus)
    EVT_CHAR(ewxAuiTabCtrl::OnChar)
    EVT_MOUSE_CAPTURE_LOST(ewxAuiTabCtrl::OnCaptureLost)
END_EVENT_TABLE()


ewxAuiTabCtrl::ewxAuiTabCtrl(wxWindow* parent,
                           wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style) : wxControl(parent, id, pos, size, style)
{
    m_click_pt = wxDefaultPosition;
    m_is_dragging = false;
    m_hover_button = NULL;
    m_pressed_button = NULL;
}

ewxAuiTabCtrl::~ewxAuiTabCtrl()
{
}

void ewxAuiTabCtrl::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);

    dc.SetFont(GetFont());

    if (GetPageCount() > 0)
        Render(&dc, this);
}

void ewxAuiTabCtrl::OnEraseBackground(wxEraseEvent& WXUNUSED(evt))
{
}

void ewxAuiTabCtrl::OnSize(wxSizeEvent& evt)
{
    wxSize s = evt.GetSize();
    wxRect r(0, 0, s.GetWidth(), s.GetHeight());
    SetRect(r);
}

void ewxAuiTabCtrl::OnLeftDown(wxMouseEvent& evt)
{
    CaptureMouse();
    m_click_pt = wxDefaultPosition;
    m_is_dragging = false;
    m_click_tab = NULL;
    m_pressed_button = NULL;


    wxWindow* wnd;
    if (TabHitTest(evt.m_x, evt.m_y, &wnd))
    {
        int new_selection = GetIdxFromWindow(wnd);

        // ewxAuiNotebooks always want to receive this event
        // even if the tab is already active, because they may
        // have multiple tab controls
        if (new_selection != GetActivePage() ||
            GetParent()->IsKindOf(CLASSINFO(ewxAuiNotebook)))
        {
            ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING, m_windowId);
            e.SetSelection(new_selection);
            e.SetOldSelection(GetActivePage());
            e.SetEventObject(this);
            GetEventHandler()->ProcessEvent(e);
        }

        m_click_pt.x = evt.m_x;
        m_click_pt.y = evt.m_y;
        m_click_tab = wnd;
    }

    if (m_hover_button)
    {
        m_pressed_button = m_hover_button;
        m_pressed_button->cur_state = ewxAUI_BUTTON_STATE_PRESSED;
        Refresh();
        Update();
    }
}

void ewxAuiTabCtrl::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
}

void ewxAuiTabCtrl::OnLeftUp(wxMouseEvent& evt)
{
    if (GetCapture() == this)
        ReleaseMouse();

    if (m_is_dragging)
    {
        m_is_dragging = false;

        ewxAuiNotebookEvent evt(wxEVT_COMMAND_AUINOTEBOOK_END_DRAG, m_windowId);
        evt.SetSelection(GetIdxFromWindow(m_click_tab));
        evt.SetOldSelection(evt.GetSelection());
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);

        return;
    }

    if (m_pressed_button)
    {
        // make sure we're still clicking the button
        ewxAuiTabContainerButton* button = NULL;
        if (!ButtonHitTest(evt.m_x, evt.m_y, &button) ||
            button->cur_state & ewxAUI_BUTTON_STATE_DISABLED)
            return;

        if (button != m_pressed_button)
        {
            m_pressed_button = NULL;
            return;
        }

        Refresh();
        Update();

        if (!(m_pressed_button->cur_state & ewxAUI_BUTTON_STATE_DISABLED))
        {
            ewxAuiNotebookEvent evt(wxEVT_COMMAND_AUINOTEBOOK_BUTTON, m_windowId);
            evt.SetSelection(GetIdxFromWindow(m_click_tab));
            evt.SetInt(m_pressed_button->id);
            evt.SetEventObject(this);
            GetEventHandler()->ProcessEvent(evt);
        }

        m_pressed_button = NULL;
    }

    m_click_pt = wxDefaultPosition;
    m_is_dragging = false;
    m_click_tab = NULL;
}

void ewxAuiTabCtrl::OnMiddleUp(wxMouseEvent& evt)
{
    wxWindow* wnd = NULL;
    if (!TabHitTest(evt.m_x, evt.m_y, &wnd))
        return;

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP, m_windowId);
    e.SetEventObject(this);
    e.SetSelection(GetIdxFromWindow(wnd));
    GetEventHandler()->ProcessEvent(e);
}

void ewxAuiTabCtrl::OnMiddleDown(wxMouseEvent& evt)
{
    wxWindow* wnd = NULL;
    if (!TabHitTest(evt.m_x, evt.m_y, &wnd))
        return;

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_DOWN, m_windowId);
    e.SetEventObject(this);
    e.SetSelection(GetIdxFromWindow(wnd));
    GetEventHandler()->ProcessEvent(e);
}

void ewxAuiTabCtrl::OnRightUp(wxMouseEvent& evt)
{
    wxWindow* wnd = NULL;
    if (!TabHitTest(evt.m_x, evt.m_y, &wnd))
        return;

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP, m_windowId);
    e.SetEventObject(this);
    e.SetSelection(GetIdxFromWindow(wnd));
    GetEventHandler()->ProcessEvent(e);
}

void ewxAuiTabCtrl::OnRightDown(wxMouseEvent& evt)
{
    wxWindow* wnd = NULL;
    if (!TabHitTest(evt.m_x, evt.m_y, &wnd))
        return;

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_DOWN, m_windowId);
    e.SetEventObject(this);
    e.SetSelection(GetIdxFromWindow(wnd));
    GetEventHandler()->ProcessEvent(e);
}

void ewxAuiTabCtrl::OnLeftDClick(wxMouseEvent& evt)
{
    wxWindow* wnd;
    ewxAuiTabContainerButton* button;
    if (!TabHitTest(evt.m_x, evt.m_y, &wnd) && !ButtonHitTest(evt.m_x, evt.m_y, &button))
    {
        ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_BG_DCLICK, m_windowId);
        e.SetEventObject(this);
        GetEventHandler()->ProcessEvent(e);
    }
}

void ewxAuiTabCtrl::OnMotion(wxMouseEvent& evt)
{
    wxPoint pos = evt.GetPosition();

    // check if the mouse is hovering above a button
    ewxAuiTabContainerButton* button;
    if (ButtonHitTest(pos.x, pos.y, &button) && !(button->cur_state & ewxAUI_BUTTON_STATE_DISABLED))
    {
        if (m_hover_button && button != m_hover_button)
        {
            m_hover_button->cur_state = ewxAUI_BUTTON_STATE_NORMAL;
            m_hover_button = NULL;
            Refresh();
            Update();
        }

        if (button->cur_state != ewxAUI_BUTTON_STATE_HOVER)
        {
            button->cur_state = ewxAUI_BUTTON_STATE_HOVER;
            Refresh();
            Update();
            m_hover_button = button;
            return;
        }
    }
     else
    {
        if (m_hover_button)
        {
            m_hover_button->cur_state = ewxAUI_BUTTON_STATE_NORMAL;
            m_hover_button = NULL;
            Refresh();
            Update();
        }
    }


    if (!evt.LeftIsDown() || m_click_pt == wxDefaultPosition)
        return;

    if (m_is_dragging)
    {
        ewxAuiNotebookEvent evt(wxEVT_COMMAND_AUINOTEBOOK_DRAG_MOTION, m_windowId);
        evt.SetSelection(GetIdxFromWindow(m_click_tab));
        evt.SetOldSelection(evt.GetSelection());
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
        return;
    }


    int drag_x_threshold = wxSystemSettings::GetMetric(wxSYS_DRAG_X);
    int drag_y_threshold = wxSystemSettings::GetMetric(wxSYS_DRAG_Y);

    if (abs(pos.x - m_click_pt.x) > drag_x_threshold ||
        abs(pos.y - m_click_pt.y) > drag_y_threshold)
    {
        ewxAuiNotebookEvent evt(wxEVT_COMMAND_AUINOTEBOOK_BEGIN_DRAG, m_windowId);
        evt.SetSelection(GetIdxFromWindow(m_click_tab));
        evt.SetOldSelection(evt.GetSelection());
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);

        m_is_dragging = true;
    }
}

void ewxAuiTabCtrl::OnLeaveWindow(wxMouseEvent& WXUNUSED(event))
{
    if (m_hover_button)
    {
        m_hover_button->cur_state = ewxAUI_BUTTON_STATE_NORMAL;
        m_hover_button = NULL;
        Refresh();
        Update();
    }
}

void ewxAuiTabCtrl::OnButton(ewxAuiNotebookEvent& event)
{
    int button = event.GetInt();

    if (button == ewxAUI_BUTTON_LEFT || button == ewxAUI_BUTTON_RIGHT)
    {
        if (button == ewxAUI_BUTTON_LEFT)
        {
            if (GetTabOffset() > 0)
            {
                SetTabOffset(GetTabOffset()-1);
                Refresh();
                Update();
            }
        }
         else
        {
            SetTabOffset(GetTabOffset()+1);
            Refresh();
            Update();
        }
    }
     else if (button == ewxAUI_BUTTON_WINDOWLIST)
    {
        int idx = GetArtProvider()->ShowDropDown(this, m_pages, GetActivePage());

        if (idx != -1)
        {
            ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING, m_windowId);
            e.SetSelection(idx);
            e.SetOldSelection(GetActivePage());
            e.SetEventObject(this);
            GetEventHandler()->ProcessEvent(e);
        }
    }
     else
    {
        event.Skip();
    }
}

void ewxAuiTabCtrl::OnSetFocus(wxFocusEvent& WXUNUSED(event))
{
    Refresh();
}

void ewxAuiTabCtrl::OnKillFocus(wxFocusEvent& WXUNUSED(event))
{
    Refresh();
}

void ewxAuiTabCtrl::OnChar(wxKeyEvent& event)
{
    if (GetActivePage() == -1)
    {
        event.Skip();
        return;
    }

    // We can't leave tab processing to the system; on Windows, tabs and keys
    // get eaten by the system and not processed properly if we specify both
    // wxTAB_TRAVERSAL and wxWANTS_CHARS. And if we specify just wxTAB_TRAVERSAL,
    // we don't key arrow key events.

    int key = event.GetKeyCode();

    if (key == WXK_NUMPAD_PAGEUP)
        key = WXK_PAGEUP;
    if (key == WXK_NUMPAD_PAGEDOWN)
        key = WXK_PAGEDOWN;
    if (key == WXK_NUMPAD_HOME)
        key = WXK_HOME;
    if (key == WXK_NUMPAD_END)
        key = WXK_END;
    if (key == WXK_NUMPAD_LEFT)
        key = WXK_LEFT;
    if (key == WXK_NUMPAD_RIGHT)
        key = WXK_RIGHT;

    if (key == WXK_TAB || key == WXK_PAGEUP || key == WXK_PAGEDOWN)
    {
        bool bCtrlDown = event.ControlDown();
        bool bShiftDown = event.ShiftDown();

        bool bForward = (key == WXK_TAB && !bShiftDown) || (key == WXK_PAGEDOWN);
        bool bWindowChange = (key == WXK_PAGEUP) || (key == WXK_PAGEDOWN) || bCtrlDown;
        bool bFromTab = (key == WXK_TAB);

        ewxAuiNotebook* nb = wxDynamicCast(GetParent(), ewxAuiNotebook);
        if (!nb)
        {
            event.Skip();
            return;
        }

        wxNavigationKeyEvent keyEvent;
        keyEvent.SetDirection(bForward);
        keyEvent.SetWindowChange(bWindowChange);
        keyEvent.SetFromTab(bFromTab);
        keyEvent.SetEventObject(nb);

        if (!nb->GetEventHandler()->ProcessEvent(keyEvent))
        {
            // Not processed? Do an explicit tab into the page.
            wxWindow* win = GetWindowFromIdx(GetActivePage());
            if (win)
                win->SetFocus();
        }
        return;
    }

    if (m_pages.GetCount() < 2)
    {
        event.Skip();
        return;
    }

    int newPage = -1;

    int forwardKey, backwardKey;
    if (GetLayoutDirection() == wxLayout_RightToLeft)
    {
        forwardKey = WXK_LEFT;
        backwardKey = WXK_RIGHT;
    }
    else
     {
        forwardKey = WXK_RIGHT;
        backwardKey = WXK_LEFT;
    }

    if (key == forwardKey)
    {
        if (m_pages.GetCount() > 1)
        {
            if (GetActivePage() == -1)
                newPage = 0;
            else if (GetActivePage() < (int) (m_pages.GetCount() - 1))
                newPage = GetActivePage() + 1;
        }
    }
    else if (key == backwardKey)
    {
        if (m_pages.GetCount() > 1)
        {
            if (GetActivePage() == -1)
                newPage = (int) (m_pages.GetCount() - 1);
            else if (GetActivePage() > 0)
                newPage = GetActivePage() - 1;
        }
    }
    else if (key == WXK_HOME)
    {
        newPage = 0;
    }
    else if (key == WXK_END)
    {
        newPage = (int) (m_pages.GetCount() - 1);
    }
    else
        event.Skip();

    if (newPage != -1)
    {
        ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING, m_windowId);
        e.SetSelection(newPage);
        e.SetOldSelection(newPage);
        e.SetEventObject(this);
        this->GetEventHandler()->ProcessEvent(e);
    }
    else
        event.Skip();
}

// wxTabFrame is an interesting case.  It's important that all child pages
// of the multi-notebook control are all actually children of that control
// (and not grandchildren).  wxTabFrame facilitates this.  There is one
// instance of wxTabFrame for each tab control inside the multi-notebook.
// It's important to know that wxTabFrame is not a real window, but it merely
// used to capture the dimensions/positioning of the internal tab control and
// it's managed page windows

class wxTabFrame : public wxWindow
{
public:

    wxTabFrame()
    {
        m_tabs = NULL;
        m_rect = wxRect(0,0,200,200);
        m_tab_ctrl_height = 20;
    }

    ~wxTabFrame()
    {
        wxDELETE(m_tabs);
    }

    void SetTabCtrlHeight(int h)
    {
        m_tab_ctrl_height = h;
    }

    void DoSetSize(int x, int y,
                   int width, int height,
                   int WXUNUSED(sizeFlags = wxSIZE_AUTO))
    {
        m_rect = wxRect(x, y, width, height);
        DoSizing();
    }

    void DoGetClientSize(int* x, int* y) const
    {
        *x = m_rect.width;
        *y = m_rect.height;
    }

    bool Show( bool WXUNUSED(show = true) ) { return false; }

    void DoSizing()
    {
        if (!m_tabs)
            return;

        if (m_tabs->IsFrozen() || m_tabs->GetParent()->IsFrozen())
            return;

        if (m_tabs->GetFlags() & ewxAUI_NB_BOTTOM)
        {
            m_tab_rect = wxRect (m_rect.x, m_rect.y + m_rect.height - m_tab_ctrl_height, m_rect.width, m_tab_ctrl_height);
            m_tabs->SetSize     (m_rect.x, m_rect.y + m_rect.height - m_tab_ctrl_height, m_rect.width, m_tab_ctrl_height);
            m_tabs->SetRect     (wxRect(0, 0, m_rect.width, m_tab_ctrl_height));
        }
        else //TODO: if (GetFlags() & ewxAUI_NB_TOP)
        {
            m_tab_rect = wxRect (m_rect.x, m_rect.y, m_rect.width, m_tab_ctrl_height);
            m_tabs->SetSize     (m_rect.x, m_rect.y, m_rect.width, m_tab_ctrl_height);
            m_tabs->SetRect     (wxRect(0, 0,        m_rect.width, m_tab_ctrl_height));
        }
        // TODO: else if (GetFlags() & ewxAUI_NB_LEFT){}
        // TODO: else if (GetFlags() & ewxAUI_NB_RIGHT){}

        m_tabs->Refresh();
        m_tabs->Update();

        ewxAuiNotebookPageArray& pages = m_tabs->GetPages();
        size_t i, page_count = pages.GetCount();

        for (i = 0; i < page_count; ++i)
        {
            ewxAuiNotebookPage& page = pages.Item(i);
            if (m_tabs->GetFlags() & ewxAUI_NB_BOTTOM)
            {
               page.window->SetSize(m_rect.x, m_rect.y,
                                    m_rect.width, m_rect.height - m_tab_ctrl_height);
            }
            else //TODO: if (GetFlags() & ewxAUI_NB_TOP)
            {
                page.window->SetSize(m_rect.x, m_rect.y + m_tab_ctrl_height,
                                    m_rect.width, m_rect.height - m_tab_ctrl_height);
            }
            // TODO: else if (GetFlags() & ewxAUI_NB_LEFT){}
            // TODO: else if (GetFlags() & ewxAUI_NB_RIGHT){}

#if wxUSE_MDI
            if (page.window->IsKindOf(CLASSINFO(ewxAuiMDIChildFrame)))
            {
                ewxAuiMDIChildFrame* wnd = (ewxAuiMDIChildFrame*)page.window;
                wnd->ApplyMDIChildFrameRect();
            }
#endif
        }
    }

    void DoGetSize(int* x, int* y) const
    {
        if (x)
            *x = m_rect.GetWidth();
        if (y)
            *y = m_rect.GetHeight();
    }

    void Update()
    {
        // does nothing
    }

public:

    wxRect m_rect;
    wxRect m_tab_rect;
    ewxAuiTabCtrl* m_tabs;
    int m_tab_ctrl_height;
};


const int ewxAuiBaseTabCtrlId = 5380;


// -- ewxAuiNotebook class implementation --

BEGIN_EVENT_TABLE(ewxAuiNotebook, wxControl)
    EVT_SIZE(ewxAuiNotebook::OnSize)
    EVT_CHILD_FOCUS(ewxAuiNotebook::OnChildFocus)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING,
                      ewxAuiNotebook::OnTabClicked)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_BEGIN_DRAG,
                      ewxAuiNotebook::OnTabBeginDrag)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_END_DRAG,
                      ewxAuiNotebook::OnTabEndDrag)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_DRAG_MOTION,
                      ewxAuiNotebook::OnTabDragMotion)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_BUTTON,
                      ewxAuiNotebook::OnTabButton)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_DOWN,
                      ewxAuiNotebook::OnTabMiddleDown)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP,
                      ewxAuiNotebook::OnTabMiddleUp)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_DOWN,
                      ewxAuiNotebook::OnTabRightDown)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP,
                      ewxAuiNotebook::OnTabRightUp)
    EVT_COMMAND_RANGE(ewxAuiBaseTabCtrlId, ewxAuiBaseTabCtrlId+500,
                      wxEVT_COMMAND_AUINOTEBOOK_BG_DCLICK,
                      ewxAuiNotebook::OnTabBgDClick)
    EVT_NAVIGATION_KEY(ewxAuiNotebook::OnNavigationKey)
END_EVENT_TABLE()

ewxAuiNotebook::ewxAuiNotebook()
{
    m_curpage = -1;
    m_tab_id_counter = ewxAuiBaseTabCtrlId;
    m_dummy_wnd = NULL;
    m_tab_ctrl_height = 20;
    m_requested_bmp_size = wxDefaultSize;
    m_requested_tabctrl_height = -1;
}

ewxAuiNotebook::ewxAuiNotebook(wxWindow *parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style) : wxControl(parent, id, pos, size, style)
{
    m_dummy_wnd = NULL;
    m_requested_bmp_size = wxDefaultSize;
    m_requested_tabctrl_height = -1;
    InitNotebook(style);
}

bool ewxAuiNotebook::Create(wxWindow* parent,
                           wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style)
{
    if (!wxControl::Create(parent, id, pos, size, style))
        return false;

    InitNotebook(style);

    return true;
}

// InitNotebook() contains common initialization
// code called by all constructors
void ewxAuiNotebook::InitNotebook(long style)
{
    m_curpage = -1;
    m_tab_id_counter = ewxAuiBaseTabCtrlId;
    m_dummy_wnd = NULL;
    m_flags = (unsigned int)style;
    m_tab_ctrl_height = 20;

    m_normal_font = *wxNORMAL_FONT;
    m_selected_font = *wxNORMAL_FONT;
    m_selected_font.SetWeight(wxBOLD);

    SetArtProvider(new ewxAuiDefaultTabArt);

    m_dummy_wnd = new wxWindow(this, wxID_ANY, wxPoint(0,0), wxSize(0,0));
    m_dummy_wnd->SetSize(200, 200);
    m_dummy_wnd->Show(false);

    m_mgr.SetManagedWindow(this);
    m_mgr.SetFlags(ewxAUI_MGR_DEFAULT);
    m_mgr.SetDockSizeConstraint(1.0, 1.0); // no dock size constraint

    m_mgr.AddPane(m_dummy_wnd,
              ewxAuiPaneInfo().Name(wxT("dummy")).Bottom().CaptionVisible(false).Show(false));

    m_mgr.Update();
}

ewxAuiNotebook::~ewxAuiNotebook()
{
    // Indicate we're deleting pages
    m_isBeingDeleted = true;

    while ( GetPageCount() > 0 )
        DeletePage(0);

    m_mgr.UnInit();
}

void ewxAuiNotebook::SetArtProvider(ewxAuiTabArt* art)
{
    m_tabs.SetArtProvider(art);

    UpdateTabCtrlHeight();
}

// SetTabCtrlHeight() is the highest-level override of the
// tab height.  A call to this function effectively enforces a
// specified tab ctrl height, overriding all other considerations,
// such as text or bitmap height.  It overrides any call to
// SetUniformBitmapSize().  Specifying a height of -1 reverts
// any previous call and returns to the default behavior

void ewxAuiNotebook::SetTabCtrlHeight(int height)
{
    m_requested_tabctrl_height = height;

    // if window is already initialized, recalculate the tab height
    if (m_dummy_wnd)
    {
        UpdateTabCtrlHeight();
    }
}


// SetUniformBitmapSize() ensures that all tabs will have
// the same height, even if some tabs don't have bitmaps
// Passing wxDefaultSize to this function will instruct
// the control to use dynamic tab height-- so when a tab
// with a large bitmap is added, the tab ctrl's height will
// automatically increase to accommodate the bitmap

void ewxAuiNotebook::SetUniformBitmapSize(const wxSize& size)
{
    m_requested_bmp_size = size;

    // if window is already initialized, recalculate the tab height
    if (m_dummy_wnd)
    {
        UpdateTabCtrlHeight();
    }
}

// UpdateTabCtrlHeight() does the actual tab resizing. It's meant
// to be used interally
void ewxAuiNotebook::UpdateTabCtrlHeight()
{
    // get the tab ctrl height we will use
    int height = CalculateTabCtrlHeight();

    // if the tab control height needs to change, update
    // all of our tab controls with the new height
    if (m_tab_ctrl_height != height)
    {
        ewxAuiTabArt* art = m_tabs.GetArtProvider();

        m_tab_ctrl_height = height;

        ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
        size_t i, pane_count = all_panes.GetCount();
        for (i = 0; i < pane_count; ++i)
        {
            ewxAuiPaneInfo& pane = all_panes.Item(i);
            if (pane.name == wxT("dummy"))
                continue;
            wxTabFrame* tab_frame = (wxTabFrame*)pane.window;
            ewxAuiTabCtrl* tabctrl = tab_frame->m_tabs;
            tab_frame->SetTabCtrlHeight(m_tab_ctrl_height);
            tabctrl->SetArtProvider(art->Clone());
            tab_frame->DoSizing();
        }
    }
}

void ewxAuiNotebook::UpdateHintWindowSize()
{
    wxSize size = CalculateNewSplitSize();

    // the placeholder hint window should be set to this size
    ewxAuiPaneInfo& info = m_mgr.GetPane(wxT("dummy"));
    if (info.IsOk())
    {
        info.MinSize(size);
        info.BestSize(size);
        m_dummy_wnd->SetSize(size);
    }
}


// calculates the size of the new split
wxSize ewxAuiNotebook::CalculateNewSplitSize()
{
    // count number of tab controls
    int tab_ctrl_count = 0;
    ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        ewxAuiPaneInfo& pane = all_panes.Item(i);
        if (pane.name == wxT("dummy"))
            continue;
        tab_ctrl_count++;
    }

    wxSize new_split_size;

    // if there is only one tab control, the first split
    // should happen around the middle
    if (tab_ctrl_count < 2)
    {
        new_split_size = GetClientSize();
        new_split_size.x /= 2;
        new_split_size.y /= 2;
    }
     else
    {
        // this is in place of a more complicated calculation
        // that needs to be implemented
        new_split_size = wxSize(180,180);
    }

    return new_split_size;
}

int ewxAuiNotebook::CalculateTabCtrlHeight()
{
    // if a fixed tab ctrl height is specified,
    // just return that instead of calculating a
    // tab height
    if (m_requested_tabctrl_height != -1)
        return m_requested_tabctrl_height;

    // find out new best tab height
    ewxAuiTabArt* art = m_tabs.GetArtProvider();

    return art->GetBestTabCtrlSize(this,
                                   m_tabs.GetPages(),
                                   m_requested_bmp_size);
}


ewxAuiTabArt* ewxAuiNotebook::GetArtProvider() const
{
    return m_tabs.GetArtProvider();
}

void ewxAuiNotebook::SetWindowStyleFlag(long style)
{
    wxControl::SetWindowStyleFlag(style);

    m_flags = (unsigned int)style;

    // if the control is already initialized
    if (m_mgr.GetManagedWindow() == (wxWindow*)this)
    {
        // let all of the tab children know about the new style

        ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
        size_t i, pane_count = all_panes.GetCount();
        for (i = 0; i < pane_count; ++i)
        {
            ewxAuiPaneInfo& pane = all_panes.Item(i);
            if (pane.name == wxT("dummy"))
                continue;
            wxTabFrame* tabframe = (wxTabFrame*)pane.window;
            ewxAuiTabCtrl* tabctrl = tabframe->m_tabs;
            tabctrl->SetFlags(m_flags);
            tabframe->DoSizing();
            tabctrl->Refresh();
            tabctrl->Update();
        }
    }
}


bool ewxAuiNotebook::AddPage(wxWindow* page,
                            const wxString& caption,
                            bool select,
                            const wxBitmap& bitmap)
{
    return InsertPage(GetPageCount(), page, caption, select, bitmap);
}

bool ewxAuiNotebook::InsertPage(size_t page_idx,
                               wxWindow* page,
                               const wxString& caption,
                               bool select,
                               const wxBitmap& bitmap)
{
    wxASSERT_MSG(page, wxT("page pointer must be non-NULL"));
    if (!page)
        return false;

    page->Reparent(this);

    ewxAuiNotebookPage info;
    info.window = page;
    info.caption = caption;
    info.bitmap = bitmap;
    info.active = false;

    // if there are currently no tabs, the first added
    // tab must be active
    if (m_tabs.GetPageCount() == 0)
        info.active = true;

    m_tabs.InsertPage(page, info, page_idx);

    // if that was the first page added, even if
    // select is false, it must become the "current page"
    // (though no select events will be fired)
    if (!select && m_tabs.GetPageCount() == 1)
        select = true;
        //m_curpage = GetPageIndex(page);

    ewxAuiTabCtrl* active_tabctrl = GetActiveTabCtrl();
    if (page_idx >= active_tabctrl->GetPageCount())
        active_tabctrl->AddPage(page, info);
         else
        active_tabctrl->InsertPage(page, info, page_idx);

    UpdateTabCtrlHeight();
    DoSizing();
    active_tabctrl->DoShowHide();

    // adjust selected index
    if(m_curpage >= (int) page_idx)
        m_curpage++;

    if (select)
    {
        int idx = m_tabs.GetIdxFromWindow(page);
        wxASSERT_MSG(idx != -1, wxT("Invalid Page index returned on ewxAuiNotebook::InsertPage()"));

        SetSelection(idx);
    }

    return true;
}


// DeletePage() removes a tab from the multi-notebook,
// and destroys the window as well
bool ewxAuiNotebook::DeletePage(size_t page_idx)
{
    if (page_idx >= m_tabs.GetPageCount())
        return false;

    wxWindow* wnd = m_tabs.GetWindowFromIdx(page_idx);

    // hide the window in advance, as this will
    // prevent flicker
    if ( !IsBeingDeleted() )
        ShowWnd(wnd, false);

    if (!RemovePage(page_idx))
        return false;

    // actually destroy the window now
#if wxUSE_MDI
    if (wnd->IsKindOf(CLASSINFO(ewxAuiMDIChildFrame)))
    {
        // delete the child frame with pending delete, as is
        // customary with frame windows
        if (!wxPendingDelete.Member(wnd))
            wxPendingDelete.Append(wnd);
    }
    else
#endif
    {
        wnd->Destroy();
    }

    return true;
}



// RemovePage() removes a tab from the multi-notebook,
// but does not destroy the window
bool ewxAuiNotebook::RemovePage(size_t page_idx)
{
    // save active window pointer
    wxWindow* active_wnd = NULL;
    if (m_curpage >= 0)
        active_wnd = m_tabs.GetWindowFromIdx(m_curpage);

    // save pointer of window being deleted
    wxWindow* wnd = m_tabs.GetWindowFromIdx(page_idx);
    wxWindow* new_active = NULL;

    // make sure we found the page
    if (!wnd)
        return false;

    // find out which onscreen tab ctrl owns this tab
    ewxAuiTabCtrl* ctrl;
    int ctrl_idx;
    if (!FindTab(wnd, &ctrl, &ctrl_idx))
        return false;

    bool is_curpage = (m_curpage == (int)page_idx);
    bool is_active_in_split = ctrl->GetPage(ctrl_idx).active;


    // remove the tab from main catalog
    if (!m_tabs.RemovePage(wnd))
        return false;

    // remove the tab from the onscreen tab ctrl
    ctrl->RemovePage(wnd);

    if (is_active_in_split)
    {
        int ctrl_new_page_count = (int)ctrl->GetPageCount();

        if (ctrl_idx >= ctrl_new_page_count)
            ctrl_idx = ctrl_new_page_count-1;

        if (ctrl_idx >= 0 && ctrl_idx < (int)ctrl->GetPageCount())
        {
            // set new page as active in the tab split
            ctrl->SetActivePage(ctrl_idx);

            // if the page deleted was the current page for the
            // entire tab control, then record the window
            // pointer of the new active page for activation
            if (is_curpage)
            {
                new_active = ctrl->GetWindowFromIdx(ctrl_idx);
            }
        }
    }
     else
    {
        // we are not deleting the active page, so keep it the same
        new_active = active_wnd;
    }


    if (!new_active)
    {
        // we haven't yet found a new page to active,
        // so select the next page from the main tab
        // catalogue

        if (page_idx < m_tabs.GetPageCount())
        {
            new_active = m_tabs.GetPage(page_idx).window;
        }

        if (!new_active && m_tabs.GetPageCount() > 0)
        {
            new_active = m_tabs.GetPage(0).window;
        }
    }


    RemoveEmptyTabFrames();

    // set new active pane
    m_curpage = -1;
    if (new_active && !m_isBeingDeleted)
    {
        SetSelection(m_tabs.GetIdxFromWindow(new_active));
    }

    return true;
}

// GetPageIndex() returns the index of the page, or -1 if the
// page could not be located in the notebook
int ewxAuiNotebook::GetPageIndex(wxWindow* page_wnd) const
{
    return m_tabs.GetIdxFromWindow(page_wnd);
}



// SetPageText() changes the tab caption of the specified page
bool ewxAuiNotebook::SetPageText(size_t page_idx, const wxString& text)
{
    if (page_idx >= m_tabs.GetPageCount())
        return false;

    // update our own tab catalog
    ewxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    page_info.caption = text;

    // update what's on screen
    ewxAuiTabCtrl* ctrl;
    int ctrl_idx;
    if (FindTab(page_info.window, &ctrl, &ctrl_idx))
    {
        ewxAuiNotebookPage& info = ctrl->GetPage(ctrl_idx);
        info.caption = text;
        ctrl->Refresh();
        ctrl->Update();
    }

    return true;
}

// returns the page caption
wxString ewxAuiNotebook::GetPageText(size_t page_idx) const
{
    if (page_idx >= m_tabs.GetPageCount())
        return wxEmptyString;

    // update our own tab catalog
    const ewxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    return page_info.caption;
}

bool ewxAuiNotebook::SetPageBitmap(size_t page_idx, const wxBitmap& bitmap)
{
    if (page_idx >= m_tabs.GetPageCount())
        return false;

    // update our own tab catalog
    ewxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    page_info.bitmap = bitmap;

    // tab height might have changed
    UpdateTabCtrlHeight();

    // update what's on screen
    ewxAuiTabCtrl* ctrl;
    int ctrl_idx;
    if (FindTab(page_info.window, &ctrl, &ctrl_idx))
    {
        ewxAuiNotebookPage& info = ctrl->GetPage(ctrl_idx);
        info.bitmap = bitmap;
        ctrl->Refresh();
        ctrl->Update();
    }

    return true;
}

// returns the page bitmap
wxBitmap ewxAuiNotebook::GetPageBitmap(size_t page_idx) const
{
    if (page_idx >= m_tabs.GetPageCount())
        return wxBitmap();

    // update our own tab catalog
    const ewxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    return page_info.bitmap;
}

// GetSelection() returns the index of the currently active page
int ewxAuiNotebook::GetSelection() const
{
    return m_curpage;
}

// SetSelection() sets the currently active page
size_t ewxAuiNotebook::SetSelection(size_t new_page)
{
    wxWindow* wnd = m_tabs.GetWindowFromIdx(new_page);
    if (!wnd)
        return m_curpage;

    // don't change the page unless necessary;
    // however, clicking again on a tab should give it the focus.
    if ((int)new_page == m_curpage)
    {
        ewxAuiTabCtrl* ctrl;
        int ctrl_idx;
        if (FindTab(wnd, &ctrl, &ctrl_idx))
        {
            if (FindFocus() != ctrl)
                ctrl->SetFocus();
        }
        return m_curpage;
    }

    ewxAuiNotebookEvent evt(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING, m_windowId);
    evt.SetSelection(new_page);
    evt.SetOldSelection(m_curpage);
    evt.SetEventObject(this);
    if (!GetEventHandler()->ProcessEvent(evt) || evt.IsAllowed())
    {
        int old_curpage = m_curpage;
        m_curpage = new_page;

        // program allows the page change
        evt.SetEventType(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED);
        (void)GetEventHandler()->ProcessEvent(evt);


        ewxAuiTabCtrl* ctrl;
        int ctrl_idx;
        if (FindTab(wnd, &ctrl, &ctrl_idx))
        {
            m_tabs.SetActivePage(wnd);

            ctrl->SetActivePage(ctrl_idx);
            DoSizing();
            ctrl->DoShowHide();

            ctrl->MakeTabVisible(ctrl_idx, ctrl);

            // set fonts
            ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
            size_t i, pane_count = all_panes.GetCount();
            for (i = 0; i < pane_count; ++i)
            {
                ewxAuiPaneInfo& pane = all_panes.Item(i);
                if (pane.name == wxT("dummy"))
                    continue;
                ewxAuiTabCtrl* tabctrl = ((wxTabFrame*)pane.window)->m_tabs;
                if (tabctrl != ctrl)
                    tabctrl->SetSelectedFont(m_normal_font);
                     else
                    tabctrl->SetSelectedFont(m_selected_font);
                tabctrl->Refresh();
            }

            // Set the focus to the page if we're not currently focused on the tab.
            // This is Firefox-like behaviour.
            if (wnd->IsShownOnScreen() && FindFocus() != ctrl)
                wnd->SetFocus();

            return old_curpage;
        }
    }

    return m_curpage;
}

// GetPageCount() returns the total number of
// pages managed by the multi-notebook
size_t ewxAuiNotebook::GetPageCount() const
{
    return m_tabs.GetPageCount();
}

// GetPage() returns the wxWindow pointer of the
// specified page
wxWindow* ewxAuiNotebook::GetPage(size_t page_idx) const
{
    wxASSERT(page_idx < m_tabs.GetPageCount());

    return m_tabs.GetWindowFromIdx(page_idx);
}

// DoSizing() performs all sizing operations in each tab control
void ewxAuiNotebook::DoSizing()
{
    ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        tabframe->DoSizing();
    }
}

// GetActiveTabCtrl() returns the active tab control.  It is
// called to determine which control gets new windows being added
ewxAuiTabCtrl* ewxAuiNotebook::GetActiveTabCtrl()
{
    if (m_curpage >= 0 && m_curpage < (int)m_tabs.GetPageCount())
    {
        ewxAuiTabCtrl* ctrl;
        int idx;

        // find the tab ctrl with the current page
        if (FindTab(m_tabs.GetPage(m_curpage).window,
                    &ctrl, &idx))
        {
            return ctrl;
        }
    }

    // no current page, just find the first tab ctrl
    ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        return tabframe->m_tabs;
    }

    // If there is no tabframe at all, create one
    wxTabFrame* tabframe = new wxTabFrame;
    tabframe->SetTabCtrlHeight(m_tab_ctrl_height);
    tabframe->m_tabs = new ewxAuiTabCtrl(this,
                                        m_tab_id_counter++,
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxNO_BORDER|wxWANTS_CHARS);
    tabframe->m_tabs->SetFlags(m_flags);
    tabframe->m_tabs->SetArtProvider(m_tabs.GetArtProvider()->Clone());
    m_mgr.AddPane(tabframe,
                  ewxAuiPaneInfo().Center().CaptionVisible(false));

    m_mgr.Update();

    return tabframe->m_tabs;
}

// FindTab() finds the tab control that currently contains the window as well
// as the index of the window in the tab control.  It returns true if the
// window was found, otherwise false.
bool ewxAuiNotebook::FindTab(wxWindow* page, ewxAuiTabCtrl** ctrl, int* idx)
{
    ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;

        int page_idx = tabframe->m_tabs->GetIdxFromWindow(page);
        if (page_idx != -1)
        {
            *ctrl = tabframe->m_tabs;
            *idx = page_idx;
            return true;
        }
    }

    return false;
}

void ewxAuiNotebook::Split(size_t page, int direction)
{
    wxSize cli_size = GetClientSize();

    // get the page's window pointer
    wxWindow* wnd = GetPage(page);
    if (!wnd)
        return;

    // notebooks with 1 or less pages can't be split
    if (GetPageCount() < 2)
        return;

    // find out which tab control the page currently belongs to
    ewxAuiTabCtrl *src_tabs, *dest_tabs;
    int src_idx = -1;
    src_tabs = NULL;
    if (!FindTab(wnd, &src_tabs, &src_idx))
        return;
    if (!src_tabs || src_idx == -1)
        return;

    // choose a split size
    wxSize split_size;
    if (GetPageCount() > 2)
    {
        split_size = CalculateNewSplitSize();
    }
     else
    {
        // because there are two panes, always split them
        // equally
        split_size = GetClientSize();
        split_size.x /= 2;
        split_size.y /= 2;
    }


    // create a new tab frame
    wxTabFrame* new_tabs = new wxTabFrame;
    new_tabs->m_rect = wxRect(wxPoint(0,0), split_size);
    new_tabs->SetTabCtrlHeight(m_tab_ctrl_height);
    new_tabs->m_tabs = new ewxAuiTabCtrl(this,
                                        m_tab_id_counter++,
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxNO_BORDER|wxWANTS_CHARS);
    new_tabs->m_tabs->SetArtProvider(m_tabs.GetArtProvider()->Clone());
    new_tabs->m_tabs->SetFlags(m_flags);
    dest_tabs = new_tabs->m_tabs;

    // create a pane info structure with the information
    // about where the pane should be added
    ewxAuiPaneInfo pane_info = ewxAuiPaneInfo().Bottom().CaptionVisible(false);
    wxPoint mouse_pt;

    if (direction == wxLEFT)
    {
        pane_info.Left();
        mouse_pt = wxPoint(0, cli_size.y/2);
    }
     else if (direction == wxRIGHT)
    {
        pane_info.Right();
        mouse_pt = wxPoint(cli_size.x, cli_size.y/2);
    }
     else if (direction == wxTOP)
    {
        pane_info.Top();
        mouse_pt = wxPoint(cli_size.x/2, 0);
    }
     else if (direction == wxBOTTOM)
    {
        pane_info.Bottom();
        mouse_pt = wxPoint(cli_size.x/2, cli_size.y);
    }

    m_mgr.AddPane(new_tabs, pane_info, mouse_pt);
    m_mgr.Update();

    // remove the page from the source tabs
    ewxAuiNotebookPage page_info = src_tabs->GetPage(src_idx);
    page_info.active = false;
    src_tabs->RemovePage(page_info.window);
    if (src_tabs->GetPageCount() > 0)
    {
        src_tabs->SetActivePage((size_t)0);
        src_tabs->DoShowHide();
        src_tabs->Refresh();
    }


    // add the page to the destination tabs
    dest_tabs->InsertPage(page_info.window, page_info, 0);

    if (src_tabs->GetPageCount() == 0)
    {
        RemoveEmptyTabFrames();
    }

    DoSizing();
    dest_tabs->DoShowHide();
    dest_tabs->Refresh();

    // force the set selection function reset the selection
    m_curpage = -1;

    // set the active page to the one we just split off
    SetSelection(m_tabs.GetIdxFromWindow(page_info.window));

    UpdateHintWindowSize();
}


void ewxAuiNotebook::OnSize(wxSizeEvent& evt)
{
    UpdateHintWindowSize();

    evt.Skip();
}

void ewxAuiNotebook::OnTabClicked(wxCommandEvent& command_evt)
{
    ewxAuiNotebookEvent& evt = (ewxAuiNotebookEvent&)command_evt;

    ewxAuiTabCtrl* ctrl = (ewxAuiTabCtrl*)evt.GetEventObject();
    wxASSERT(ctrl != NULL);

    wxWindow* wnd = ctrl->GetWindowFromIdx(evt.GetSelection());
    wxASSERT(wnd != NULL);

    int idx = m_tabs.GetIdxFromWindow(wnd);
    wxASSERT(idx != -1);


    // since a tab was clicked, let the parent know that we received
    // the focus, even if we will assign that focus immediately
    // to the child tab in the SetSelection call below
    // (the child focus event will also let ewxAuiManager, if any,
    // know that the notebook control has been activated)

    wxWindow* parent = GetParent();
    if (parent)
    {
        wxChildFocusEvent eventFocus(this);
        parent->GetEventHandler()->ProcessEvent(eventFocus);
    }


    SetSelection(idx);
}

void ewxAuiNotebook::OnTabBgDClick(wxCommandEvent& WXUNUSED(evt))
{
    // notify owner that the tabbar background has been double-clicked
    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_BG_DCLICK, m_windowId);
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}

void ewxAuiNotebook::OnTabBeginDrag(wxCommandEvent&)
{
    m_last_drag_x = 0;
}

void ewxAuiNotebook::OnTabDragMotion(wxCommandEvent& evt)
{
    wxPoint screen_pt = ::wxGetMousePosition();
    wxPoint client_pt = ScreenToClient(screen_pt);
    wxPoint zero(0,0);

    ewxAuiTabCtrl* src_tabs = (ewxAuiTabCtrl*)evt.GetEventObject();
    ewxAuiTabCtrl* dest_tabs = GetTabCtrlFromPoint(client_pt);

    if (dest_tabs == src_tabs)
    {
        if (src_tabs)
        {
            src_tabs->SetCursor(wxCursor(wxCURSOR_ARROW));
        }

        // always hide the hint for inner-tabctrl drag
        m_mgr.HideHint();

        // if tab moving is not allowed, leave
        if (!(m_flags & ewxAUI_NB_TAB_MOVE))
        {
            return;
        }

        wxPoint pt = dest_tabs->ScreenToClient(screen_pt);
        wxWindow* dest_location_tab;

        // this is an inner-tab drag/reposition
        if (dest_tabs->TabHitTest(pt.x, pt.y, &dest_location_tab))
        {
            int src_idx = evt.GetSelection();
            int dest_idx = dest_tabs->GetIdxFromWindow(dest_location_tab);

            // prevent jumpy drag
            if ((src_idx == dest_idx) || dest_idx == -1 ||
                (src_idx > dest_idx && m_last_drag_x <= pt.x) ||
                (src_idx < dest_idx && m_last_drag_x >= pt.x))
            {
                m_last_drag_x = pt.x;
                return;
            }


            wxWindow* src_tab = dest_tabs->GetWindowFromIdx(src_idx);
            dest_tabs->MovePage(src_tab, dest_idx);
            dest_tabs->SetActivePage((size_t)dest_idx);
            dest_tabs->DoShowHide();
            dest_tabs->Refresh();
            m_last_drag_x = pt.x;

        }

        return;
    }


    // if external drag is allowed, check if the tab is being dragged
    // over a different ewxAuiNotebook control
    if (m_flags & ewxAUI_NB_TAB_EXTERNAL_MOVE)
    {
        wxWindow* tab_ctrl = ::wxFindWindowAtPoint(screen_pt);

        // if we aren't over any window, stop here
        if (!tab_ctrl)
            return;

        // make sure we are not over the hint window
        if (!tab_ctrl->IsKindOf(CLASSINFO(wxFrame)))
        {
            while (tab_ctrl)
            {
                if (tab_ctrl->IsKindOf(CLASSINFO(ewxAuiTabCtrl)))
                    break;
                tab_ctrl = tab_ctrl->GetParent();
            }

            if (tab_ctrl)
            {
                ewxAuiNotebook* nb = (ewxAuiNotebook*)tab_ctrl->GetParent();

                if (nb != this)
                {
                    wxRect hint_rect = tab_ctrl->GetClientRect();
                    tab_ctrl->ClientToScreen(&hint_rect.x, &hint_rect.y);
                    m_mgr.ShowHint(hint_rect);
                    return;
                }
            }
        }
         else
        {
            if (!dest_tabs)
            {
                // we are either over a hint window, or not over a tab
                // window, and there is no where to drag to, so exit
                return;
            }
        }
    }


    // if there are less than two panes, split can't happen, so leave
    if (m_tabs.GetPageCount() < 2)
        return;

    // if tab moving is not allowed, leave
    if (!(m_flags & ewxAUI_NB_TAB_SPLIT))
        return;


    if (src_tabs)
    {
        src_tabs->SetCursor(wxCursor(wxCURSOR_SIZING));
    }


    if (dest_tabs)
    {
        wxRect hint_rect = dest_tabs->GetRect();
        ClientToScreen(&hint_rect.x, &hint_rect.y);
        m_mgr.ShowHint(hint_rect);
    }
     else
    {
        m_mgr.DrawHintRect(m_dummy_wnd, client_pt, zero);
    }
}



void ewxAuiNotebook::OnTabEndDrag(wxCommandEvent& command_evt)
{
    ewxAuiNotebookEvent& evt = (ewxAuiNotebookEvent&)command_evt;

    m_mgr.HideHint();


    ewxAuiTabCtrl* src_tabs = (ewxAuiTabCtrl*)evt.GetEventObject();
    ewxAuiTabCtrl* dest_tabs = NULL;
    if (src_tabs)
    {
        // set cursor back to an arrow
        src_tabs->SetCursor(wxCursor(wxCURSOR_ARROW));
    }

    // get the mouse position, which will be used to determine the drop point
    wxPoint mouse_screen_pt = ::wxGetMousePosition();
    wxPoint mouse_client_pt = ScreenToClient(mouse_screen_pt);



    // check for an external move
    if (m_flags & ewxAUI_NB_TAB_EXTERNAL_MOVE)
    {
        wxWindow* tab_ctrl = ::wxFindWindowAtPoint(mouse_screen_pt);

        while (tab_ctrl)
        {
            if (tab_ctrl->IsKindOf(CLASSINFO(ewxAuiTabCtrl)))
                break;
            tab_ctrl = tab_ctrl->GetParent();
        }

        if (tab_ctrl)
        {
            ewxAuiNotebook* nb = (ewxAuiNotebook*)tab_ctrl->GetParent();

            if (nb != this)
            {
                // find out from the destination control
                // if it's ok to drop this tab here
                ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND, m_windowId);
                e.SetSelection(evt.GetSelection());
                e.SetOldSelection(evt.GetSelection());
                e.SetEventObject(this);
                e.SetDragSource(this);
                e.Veto(); // dropping must be explicitly approved by control owner

                nb->GetEventHandler()->ProcessEvent(e);

                if (!e.IsAllowed())
                {
                    // no answer or negative answer
                    m_mgr.HideHint();
                    return;
                }

                // drop was allowed
                int src_idx = evt.GetSelection();
                wxWindow* src_page = src_tabs->GetWindowFromIdx(src_idx);

                // Check that it's not an impossible parent relationship
                wxWindow* p = nb;
                while (p && !p->IsTopLevel())
                {
                    if (p == src_page)
                    {
                        return;
                    }
                    p = p->GetParent();
                }

                // get main index of the page
                int main_idx = m_tabs.GetIdxFromWindow(src_page);

                // make a copy of the page info
                ewxAuiNotebookPage page_info = m_tabs.GetPage((size_t)main_idx);

                // remove the page from the source notebook
                RemovePage(main_idx);

                // reparent the page
                src_page->Reparent(nb);


                // found out the insert idx
                ewxAuiTabCtrl* dest_tabs = (ewxAuiTabCtrl*)tab_ctrl;
                wxPoint pt = dest_tabs->ScreenToClient(mouse_screen_pt);

                wxWindow* target = NULL;
                int insert_idx = -1;
                dest_tabs->TabHitTest(pt.x, pt.y, &target);
                if (target)
                {
                    insert_idx = dest_tabs->GetIdxFromWindow(target);
                }


                // add the page to the new notebook
                if (insert_idx == -1)
                    insert_idx = dest_tabs->GetPageCount();
                dest_tabs->InsertPage(page_info.window, page_info, insert_idx);
                nb->m_tabs.AddPage(page_info.window, page_info);

                nb->DoSizing();
                dest_tabs->DoShowHide();
                dest_tabs->Refresh();

                // set the selection in the destination tab control
                nb->SetSelection(nb->m_tabs.GetIdxFromWindow(page_info.window));

                // notify owner that the tab has been dragged
                ewxAuiNotebookEvent e2(wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE, m_windowId);
                e2.SetSelection(evt.GetSelection());
                e2.SetOldSelection(evt.GetSelection());
                e2.SetEventObject(this);
                GetEventHandler()->ProcessEvent(e2);

                return;
            }
        }
    }




    // only perform a tab split if it's allowed
    if ((m_flags & ewxAUI_NB_TAB_SPLIT) && m_tabs.GetPageCount() >= 2)
    {
        // If the pointer is in an existing tab frame, do a tab insert
        wxWindow* hit_wnd = ::wxFindWindowAtPoint(mouse_screen_pt);
        wxTabFrame* tab_frame = (wxTabFrame*)GetTabFrameFromTabCtrl(hit_wnd);
        int insert_idx = -1;
        if (tab_frame)
        {
            dest_tabs = tab_frame->m_tabs;

            if (dest_tabs == src_tabs)
                return;


            wxPoint pt = dest_tabs->ScreenToClient(mouse_screen_pt);
            wxWindow* target = NULL;
            dest_tabs->TabHitTest(pt.x, pt.y, &target);
            if (target)
            {
                insert_idx = dest_tabs->GetIdxFromWindow(target);
            }
        }
        else
        {
            wxPoint zero(0,0);
            wxRect rect = m_mgr.CalculateHintRect(m_dummy_wnd,
                                                  mouse_client_pt,
                                                  zero);
            if (rect.IsEmpty())
            {
                // there is no suitable drop location here, exit out
                return;
            }

            // If there is no tabframe at all, create one
            wxTabFrame* new_tabs = new wxTabFrame;
            new_tabs->m_rect = wxRect(wxPoint(0,0), CalculateNewSplitSize());
            new_tabs->SetTabCtrlHeight(m_tab_ctrl_height);
            new_tabs->m_tabs = new ewxAuiTabCtrl(this,
                                                m_tab_id_counter++,
                                                wxDefaultPosition,
                                                wxDefaultSize,
                                                wxNO_BORDER|wxWANTS_CHARS);
            new_tabs->m_tabs->SetArtProvider(m_tabs.GetArtProvider()->Clone());
            new_tabs->m_tabs->SetFlags(m_flags);

            m_mgr.AddPane(new_tabs,
                          ewxAuiPaneInfo().Bottom().CaptionVisible(false),
                          mouse_client_pt);
            m_mgr.Update();
            dest_tabs = new_tabs->m_tabs;
        }



        // remove the page from the source tabs
        ewxAuiNotebookPage page_info = src_tabs->GetPage(evt.GetSelection());
        page_info.active = false;
        src_tabs->RemovePage(page_info.window);
        if (src_tabs->GetPageCount() > 0)
        {
            src_tabs->SetActivePage((size_t)0);
            src_tabs->DoShowHide();
            src_tabs->Refresh();
        }



        // add the page to the destination tabs
        if (insert_idx == -1)
            insert_idx = dest_tabs->GetPageCount();
        dest_tabs->InsertPage(page_info.window, page_info, insert_idx);

        if (src_tabs->GetPageCount() == 0)
        {
            RemoveEmptyTabFrames();
        }

        DoSizing();
        dest_tabs->DoShowHide();
        dest_tabs->Refresh();

        // force the set selection function reset the selection
        m_curpage = -1;

        // set the active page to the one we just split off
        SetSelection(m_tabs.GetIdxFromWindow(page_info.window));

        UpdateHintWindowSize();
    }

    // notify owner that the tab has been dragged
    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE, m_windowId);
    e.SetSelection(evt.GetSelection());
    e.SetOldSelection(evt.GetSelection());
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}



ewxAuiTabCtrl* ewxAuiNotebook::GetTabCtrlFromPoint(const wxPoint& pt)
{
    // if we've just removed the last tab from the source
    // tab set, the remove the tab control completely
    ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        if (tabframe->m_tab_rect.Contains(pt))
            return tabframe->m_tabs;
    }

    return NULL;
}

wxWindow* ewxAuiNotebook::GetTabFrameFromTabCtrl(wxWindow* tab_ctrl)
{
    // if we've just removed the last tab from the source
    // tab set, the remove the tab control completely
    ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        if (tabframe->m_tabs == tab_ctrl)
        {
            return tabframe;
        }
    }

    return NULL;
}

void ewxAuiNotebook::RemoveEmptyTabFrames()
{
    // if we've just removed the last tab from the source
    // tab set, the remove the tab control completely
    ewxAuiPaneInfoArray all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tab_frame = (wxTabFrame*)all_panes.Item(i).window;
        if (tab_frame->m_tabs->GetPageCount() == 0)
        {
            m_mgr.DetachPane(tab_frame);

            // use pending delete because sometimes during
            // window closing, refreshs are pending
            if (!wxPendingDelete.Member(tab_frame->m_tabs))
                wxPendingDelete.Append(tab_frame->m_tabs);

            tab_frame->m_tabs = NULL;

            delete tab_frame;
        }
    }


    // check to see if there is still a center pane;
    // if there isn't, make a frame the center pane
    ewxAuiPaneInfoArray panes = m_mgr.GetAllPanes();
    pane_count = panes.GetCount();
    wxWindow* first_good = NULL;
    bool center_found = false;
    for (i = 0; i < pane_count; ++i)
    {
        if (panes.Item(i).name == wxT("dummy"))
            continue;
        if (panes.Item(i).dock_direction == ewxAUI_DOCK_CENTRE)
            center_found = true;
        if (!first_good)
            first_good = panes.Item(i).window;
    }

    if (!center_found && first_good)
    {
        m_mgr.GetPane(first_good).Centre();
    }

    if (!m_isBeingDeleted)
        m_mgr.Update();
}

void ewxAuiNotebook::OnChildFocus(wxChildFocusEvent& evt)
{
    // if we're dragging a tab, don't change the current selection.
    // This code prevents a bug that used to happen when the hint window
    // was hidden.  In the bug, the focus would return to the notebook
    // child, which would then enter this handler and call
    // SetSelection, which is not desired turn tab dragging.

    ewxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        ewxAuiPaneInfo& pane = all_panes.Item(i);
        if (pane.name == wxT("dummy"))
            continue;
        wxTabFrame* tabframe = (wxTabFrame*)pane.window;
        if (tabframe->m_tabs->IsDragging())
            return;
    }


    // change the tab selection to the child
    // which was focused
    int idx = m_tabs.GetIdxFromWindow(evt.GetWindow());
    if (idx != -1 && idx != m_curpage)
    {
        SetSelection(idx);
    }
}

void ewxAuiNotebook::OnNavigationKey(wxNavigationKeyEvent& event)
{
    if ( event.IsWindowChange() ) {
        // change pages
        // FIXME: the problem with this is that if we have a split notebook,
        // we selection may go all over the place.
        AdvanceSelection(event.GetDirection());
    }
    else {
        // we get this event in 3 cases
        //
        // a) one of our pages might have generated it because the user TABbed
        // out from it in which case we should propagate the event upwards and
        // our parent will take care of setting the focus to prev/next sibling
        //
        // or
        //
        // b) the parent panel wants to give the focus to us so that we
        // forward it to our selected page. We can't deal with this in
        // OnSetFocus() because we don't know which direction the focus came
        // from in this case and so can't choose between setting the focus to
        // first or last panel child
        //
        // or
        //
        // c) we ourselves (see MSWTranslateMessage) generated the event
        //
        wxWindow * const parent = GetParent();

        // the wxObject* casts are required to avoid MinGW GCC 2.95.3 ICE
        const bool isFromParent = event.GetEventObject() == (wxObject*) parent;
        const bool isFromSelf = event.GetEventObject() == (wxObject*) this;

        if ( isFromParent || isFromSelf )
        {
            // no, it doesn't come from child, case (b) or (c): forward to a
            // page but only if direction is backwards (TAB) or from ourselves,
            if ( GetSelection() != wxNOT_FOUND &&
                    (!event.GetDirection() || isFromSelf) )
            {
                // so that the page knows that the event comes from it's parent
                // and is being propagated downwards
                event.SetEventObject(this);

                wxWindow *page = GetPage(GetSelection());
                if ( !page->GetEventHandler()->ProcessEvent(event) )
                {
                    page->SetFocus();
                }
                //else: page manages focus inside it itself
            }
            else // otherwise set the focus to the notebook itself
            {
                SetFocus();
            }
        }
        else
        {
            // it comes from our child, case (a), pass to the parent, but only
            // if the direction is forwards. Otherwise set the focus to the
            // notebook itself. The notebook is always the 'first' control of a
            // page.
            if ( !event.GetDirection() )
            {
                SetFocus();
            }
            else if ( parent )
            {
                event.SetCurrentFocus(this);
                parent->GetEventHandler()->ProcessEvent(event);
            }
        }
    }
}

void ewxAuiNotebook::OnTabButton(wxCommandEvent& command_evt)
{
    ewxAuiNotebookEvent& evt = (ewxAuiNotebookEvent&)command_evt;
    ewxAuiTabCtrl* tabs = (ewxAuiTabCtrl*)evt.GetEventObject();

    int button_id = evt.GetInt();

    if (button_id == ewxAUI_BUTTON_CLOSE)
    {
        int selection = evt.GetSelection();

        if (selection == -1)
        {
            // if the close button is to the right, use the active
            // page selection to determine which page to close
            selection = tabs->GetActivePage();
        }

        if (selection != -1)
        {
            wxWindow* close_wnd = tabs->GetWindowFromIdx(selection);

            // ask owner if it's ok to close the tab
            ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, m_windowId);
            const int idx = m_tabs.GetIdxFromWindow(close_wnd);
            e.SetSelection(idx);
            e.SetOldSelection(evt.GetSelection());
            e.SetEventObject(this);
            GetEventHandler()->ProcessEvent(e);
            if (!e.IsAllowed())
                return;


#if wxUSE_MDI
            if (close_wnd->IsKindOf(CLASSINFO(ewxAuiMDIChildFrame)))
            {
                close_wnd->Close();
            }
            else
#endif
            {
                int main_idx = m_tabs.GetIdxFromWindow(close_wnd);
                DeletePage(main_idx);
            }

            // notify owner that the tab has been closed
            ewxAuiNotebookEvent e2(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, m_windowId);
            e2.SetSelection(idx);
            e2.SetEventObject(this);
            GetEventHandler()->ProcessEvent(e2);
        }
    }
}

void ewxAuiNotebook::OnTabMiddleDown(wxCommandEvent& evt)
{
    // patch event through to owner
    ewxAuiTabCtrl* tabs = (ewxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_DOWN, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}

void ewxAuiNotebook::OnTabMiddleUp(wxCommandEvent& evt)
{
    // if the ewxAUI_NB_MIDDLE_CLICK_CLOSE is specified, middle
    // click should act like a tab close action.  However, first
    // give the owner an opportunity to handle the middle up event
    // for custom action

    ewxAuiTabCtrl* tabs = (ewxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    if (GetEventHandler()->ProcessEvent(e))
        return;
    if (!e.IsAllowed())
        return;

    // check if we are supposed to close on middle-up
    if ((m_flags & ewxAUI_NB_MIDDLE_CLICK_CLOSE) == 0)
        return;

    // simulate the user pressing the close button on the tab
    evt.SetInt(ewxAUI_BUTTON_CLOSE);
    OnTabButton(evt);
}

void ewxAuiNotebook::OnTabRightDown(wxCommandEvent& evt)
{
    // patch event through to owner
    ewxAuiTabCtrl* tabs = (ewxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_DOWN, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}

void ewxAuiNotebook::OnTabRightUp(wxCommandEvent& evt)
{
    // patch event through to owner
    ewxAuiTabCtrl* tabs = (ewxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}



// Sets the normal font
void ewxAuiNotebook::SetNormalFont(const wxFont& font)
{
    m_normal_font = font;
    GetArtProvider()->SetNormalFont(font);
}

// Sets the selected tab font
void ewxAuiNotebook::SetSelectedFont(const wxFont& font)
{
    m_selected_font = font;
    GetArtProvider()->SetSelectedFont(font);
}

// Sets the measuring font
void ewxAuiNotebook::SetMeasuringFont(const wxFont& font)
{
    GetArtProvider()->SetMeasuringFont(font);
}

// Sets the tab font
bool ewxAuiNotebook::SetFont(const wxFont& font)
{
    wxControl::SetFont(font);

    wxFont normalFont(font);
    wxFont selectedFont(normalFont);
    selectedFont.SetWeight(wxBOLD);

    SetNormalFont(normalFont);
    SetSelectedFont(selectedFont);
    SetMeasuringFont(selectedFont);

    return true;
}

// Gets the tab control height
int ewxAuiNotebook::GetTabCtrlHeight() const
{
    return m_tab_ctrl_height;
}

// Gets the height of the notebook for a given page height
int ewxAuiNotebook::GetHeightForPageHeight(int pageHeight)
{
    UpdateTabCtrlHeight();

    int tabCtrlHeight = GetTabCtrlHeight();
    int decorHeight = 2;
    return tabCtrlHeight + pageHeight + decorHeight;
}

// Advances the selection, generation page selection events
void ewxAuiNotebook::AdvanceSelection(bool forward)
{
    if (GetPageCount() <= 1)
        return;

    int currentSelection = GetSelection();

    if (forward)
    {
        if (currentSelection == (int) (GetPageCount() - 1))
            return;
        else if (currentSelection == -1)
            currentSelection = 0;
        else
            currentSelection ++;
    }
    else
    {
        if (currentSelection <= 0)
            return;
        else
            currentSelection --;
    }

    SetSelection(currentSelection);
}

// Shows the window menu
bool ewxAuiNotebook::ShowWindowMenu()
{
    ewxAuiTabCtrl* tabCtrl = GetActiveTabCtrl();

    int idx = tabCtrl->GetArtProvider()->ShowDropDown(tabCtrl, tabCtrl->GetPages(), tabCtrl->GetActivePage());

    if (idx != -1)
    {
        ewxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING, tabCtrl->GetId());
        e.SetSelection(idx);
        e.SetOldSelection(tabCtrl->GetActivePage());
        e.SetEventObject(tabCtrl);
        GetEventHandler()->ProcessEvent(e);

        return true;
    }
    else
        return false;
}

#endif // wxUSE_AUI
