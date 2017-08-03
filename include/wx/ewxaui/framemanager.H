///////////////////////////////////////////////////////////////////////////////
// Name:        wx/ewxaui/framemanager.H
// Purpose:     wxaui: wx advanced user interface - docking window manager
// Author:      Benjamin I. Williams
// Modified by:
// Created:     2005-05-17
// RCS-ID:      $Id: framemanager.H 57885 2009-01-07 14:51:49Z JS $
// Copyright:   (C) Copyright 2005, Kirix Corporation, All Rights Reserved.
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

#ifndef _EWX_FRAMEMANAGER_H_
#define _EWX_FRAMEMANAGER_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_AUI

#include "wx/dynarray.h"
#include "wx/gdicmn.h"
#include "wx/window.h"
#include "wx/timer.h"
#include "wx/sizer.h"
#include "wx/aui/framemanager.h"

enum ewxAuiManagerDock
{
    ewxAUI_DOCK_NONE = 0,
    ewxAUI_DOCK_TOP = 1,
    ewxAUI_DOCK_RIGHT = 2,
    ewxAUI_DOCK_BOTTOM = 3,
    ewxAUI_DOCK_LEFT = 4,
    ewxAUI_DOCK_CENTER = 5,
    ewxAUI_DOCK_CENTRE = ewxAUI_DOCK_CENTER
};

enum ewxAuiManagerOption
{
    ewxAUI_MGR_ALLOW_FLOATING           = 1 << 0,
    ewxAUI_MGR_ALLOW_ACTIVE_PANE        = 1 << 1,
    ewxAUI_MGR_TRANSPARENT_DRAG         = 1 << 2,
    ewxAUI_MGR_TRANSPARENT_HINT         = 1 << 3,
    ewxAUI_MGR_VENETIAN_BLINDS_HINT     = 1 << 4,
    ewxAUI_MGR_RECTANGLE_HINT           = 1 << 5,
    ewxAUI_MGR_HINT_FADE                = 1 << 6,
    ewxAUI_MGR_NO_VENETIAN_BLINDS_FADE  = 1 << 7,
    ewxAUI_MGR_LIVE_RESIZE              = 1 << 8,

    ewxAUI_MGR_DEFAULT = ewxAUI_MGR_ALLOW_FLOATING |
                        ewxAUI_MGR_TRANSPARENT_HINT |
                        ewxAUI_MGR_HINT_FADE |
                        ewxAUI_MGR_NO_VENETIAN_BLINDS_FADE
};


enum ewxAuiPaneDockArtSetting
{
    ewxAUI_DOCKART_SASH_SIZE = 0,
    ewxAUI_DOCKART_CAPTION_SIZE = 1,
    ewxAUI_DOCKART_GRIPPER_SIZE = 2,
    ewxAUI_DOCKART_PANE_BORDER_SIZE = 3,
    ewxAUI_DOCKART_PANE_BUTTON_SIZE = 4,
    ewxAUI_DOCKART_BACKGROUND_COLOUR = 5,
    ewxAUI_DOCKART_SASH_COLOUR = 6,
    ewxAUI_DOCKART_ACTIVE_CAPTION_COLOUR = 7,
    ewxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR = 8,
    ewxAUI_DOCKART_INACTIVE_CAPTION_COLOUR = 9,
    ewxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR = 10,
    ewxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR = 11,
    ewxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR = 12,
    ewxAUI_DOCKART_BORDER_COLOUR = 13,
    ewxAUI_DOCKART_GRIPPER_COLOUR = 14,
    ewxAUI_DOCKART_CAPTION_FONT = 15,
    ewxAUI_DOCKART_GRADIENT_TYPE = 16
};

enum ewxAuiPaneDockArtGradients
{
    ewxAUI_GRADIENT_NONE = 0,
    ewxAUI_GRADIENT_VERTICAL = 1,
    ewxAUI_GRADIENT_HORIZONTAL = 2
};

enum ewxAuiPaneButtonState
{
    ewxAUI_BUTTON_STATE_NORMAL   = 0,
    ewxAUI_BUTTON_STATE_HOVER    = 1 << 1,
    ewxAUI_BUTTON_STATE_PRESSED  = 1 << 2,
    ewxAUI_BUTTON_STATE_DISABLED = 1 << 3,
    ewxAUI_BUTTON_STATE_HIDDEN   = 1 << 4,
    ewxAUI_BUTTON_STATE_CHECKED  = 1 << 5
};

enum ewxAuiButtonId
{
    ewxAUI_BUTTON_CLOSE = 101,
    ewxAUI_BUTTON_MAXIMIZE_RESTORE = 102,
    ewxAUI_BUTTON_MINIMIZE = 103,
    ewxAUI_BUTTON_PIN = 104,
    ewxAUI_BUTTON_OPTIONS = 105,
    ewxAUI_BUTTON_WINDOWLIST = 106,
    ewxAUI_BUTTON_LEFT = 107,
    ewxAUI_BUTTON_RIGHT = 108,
    ewxAUI_BUTTON_UP = 109,
    ewxAUI_BUTTON_DOWN = 110,
    ewxAUI_BUTTON_OPEN = 201,
    ewxAUI_BUTTON_TAKE_FOCUS = 202,
    ewxAUI_BUTTON_ADD_FOCUS = 203
};

enum ewxAuiPaneInsertLevel
{
    ewxAUI_INSERT_PANE = 0,
    ewxAUI_INSERT_ROW = 1,
    ewxAUI_INSERT_DOCK = 2
};




// forwards and array declarations
class ewxAuiDockUIPart;
class ewxAuiPaneButton;
class ewxAuiPaneInfo;
class ewxAuiDockInfo;
class ewxAuiDockArt;
class ewxAuiManagerEvent;

#ifndef SWIG
WX_DECLARE_OBJARRAY(ewxAuiDockInfo, ewxAuiDockInfoArray);
WX_DECLARE_OBJARRAY(ewxAuiDockUIPart, ewxAuiDockUIPartArray);
WX_DECLARE_OBJARRAY(ewxAuiPaneButton, ewxAuiPaneButtonArray);
WX_DECLARE_OBJARRAY(ewxAuiPaneInfo, ewxAuiPaneInfoArray);
WX_DEFINE_ARRAY_PTR(ewxAuiPaneInfo*, ewxAuiPaneInfoPtrArray);
WX_DEFINE_ARRAY_PTR(ewxAuiDockInfo*, ewxAuiDockInfoPtrArray);
#endif // SWIG

extern ewxAuiDockInfo ewxAuiNullDockInfo;
extern ewxAuiPaneInfo ewxAuiNullPaneInfo;



class ewxAuiPaneInfo
{
public:

    ewxAuiPaneInfo()
    {
        window = NULL;
        frame = NULL;
        state = 0;
        dock_direction = ewxAUI_DOCK_LEFT;
        dock_layer = 0;
        dock_row = 0;
        dock_pos = 0;
        floating_pos = wxDefaultPosition;
        floating_size = wxDefaultSize;
        best_size = wxDefaultSize;
        min_size = wxDefaultSize;
        max_size = wxDefaultSize;
        dock_proportion = 0;

        DefaultPane();
    }

    ~ewxAuiPaneInfo() {}

#ifndef SWIG
    ewxAuiPaneInfo(const ewxAuiPaneInfo& c)
    {
        name = c.name;
        caption = c.caption;
        window = c.window;
        frame = c.frame;
        state = c.state;
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        dock_pos = c.dock_pos;
        best_size = c.best_size;
        min_size = c.min_size;
        max_size = c.max_size;
        floating_pos = c.floating_pos;
        floating_size = c.floating_size;
        dock_proportion = c.dock_proportion;
        buttons = c.buttons;
        rect = c.rect;
    }

    ewxAuiPaneInfo& operator=(const ewxAuiPaneInfo& c)
    {
        name = c.name;
        caption = c.caption;
        window = c.window;
        frame = c.frame;
        state = c.state;
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        dock_pos = c.dock_pos;
        best_size = c.best_size;
        min_size = c.min_size;
        max_size = c.max_size;
        floating_pos = c.floating_pos;
        floating_size = c.floating_size;
        dock_proportion = c.dock_proportion;
        buttons = c.buttons;
        rect = c.rect;
        return *this;
    }
#endif // SWIG

    // Write the safe parts of a newly loaded PaneInfo structure "source" into "this"
    // used on loading perspectives etc.
    void SafeSet(ewxAuiPaneInfo source)
    {
        // note source is not passed by reference so we can overwrite, to keep the
        // unsafe bits of "dest"
        source.window = window;
        source.frame = frame;
        source.buttons = buttons;
        // now assign
        *this = source;
    }

    bool IsOk() const { return (window != NULL) ? true : false; }
    bool IsFixed() const { return !HasFlag(optionResizable); }
    bool IsResizable() const { return HasFlag(optionResizable); }
    bool IsShown() const { return !HasFlag(optionHidden); }
    bool IsFloating() const { return HasFlag(optionFloating); }
    bool IsDocked() const { return !HasFlag(optionFloating); }
    bool IsToolbar() const { return HasFlag(optionToolbar); }
    bool IsTopDockable() const { return HasFlag(optionTopDockable); }
    bool IsBottomDockable() const { return HasFlag(optionBottomDockable); }
    bool IsLeftDockable() const { return HasFlag(optionLeftDockable); }
    bool IsRightDockable() const { return HasFlag(optionRightDockable); }
    bool IsFloatable() const { return HasFlag(optionFloatable); }
    bool IsMovable() const { return HasFlag(optionMovable); }
    bool IsDestroyOnClose() const { return HasFlag(optionDestroyOnClose); }
    bool IsMaximized() const { return HasFlag(optionMaximized); }
    bool HasCaption() const { return HasFlag(optionCaption); }
    bool HasGripper() const { return HasFlag(optionGripper); }
    bool HasBorder() const { return HasFlag(optionPaneBorder); }
    bool HasCloseButton() const { return HasFlag(buttonClose); }
    bool HasMaximizeButton() const { return HasFlag(buttonMaximize); }
    bool HasMinimizeButton() const { return HasFlag(buttonMinimize); }
    bool HasPinButton() const { return HasFlag(buttonPin); }
    bool HasGripperTop() const { return HasFlag(optionGripperTop); }

    bool IsAlwaysFixed() const { return HasFlag(optionAlwaysFixed); }
    bool IsOpen() const { return HasFlag(optionOpen); }
    bool HasFocus() const { return HasFlag(optionFocus); }
    bool IsPinned() const { return HasFlag(optionPin); }
    bool HasOptionsButton() const { return HasFlag(buttonOptions); }
    bool HasOpenButton() const { return HasFlag(buttonOpen); }
    bool HasTakeFocusButton() const { return HasFlag(buttonTakeFocus); }
    bool HasAddFocusButton() const { return HasFlag(buttonAddFocus); }

#ifdef SWIG
    %typemap(out) ewxAuiPaneInfo& { $result = $self; Py_INCREF($result); }
#endif
    ewxAuiPaneInfo& Window(wxWindow* w) { window = w; return *this; }
    ewxAuiPaneInfo& Name(const wxString& n) { name = n; return *this; }
    ewxAuiPaneInfo& Caption(const wxString& c) { caption = c; return *this; }
    ewxAuiPaneInfo& Left() { dock_direction = ewxAUI_DOCK_LEFT; return *this; }
    ewxAuiPaneInfo& Right() { dock_direction = ewxAUI_DOCK_RIGHT; return *this; }
    ewxAuiPaneInfo& Top() { dock_direction = ewxAUI_DOCK_TOP; return *this; }
    ewxAuiPaneInfo& Bottom() { dock_direction = ewxAUI_DOCK_BOTTOM; return *this; }
    ewxAuiPaneInfo& Center() { dock_direction = ewxAUI_DOCK_CENTER; return *this; }
    ewxAuiPaneInfo& Centre() { dock_direction = ewxAUI_DOCK_CENTRE; return *this; }
    ewxAuiPaneInfo& Direction(int direction) { dock_direction = direction; return *this; }
    ewxAuiPaneInfo& Layer(int layer) { dock_layer = layer; return *this; }
    ewxAuiPaneInfo& Row(int row) { dock_row = row; return *this; }
    ewxAuiPaneInfo& Position(int pos) { dock_pos = pos; return *this; }
    ewxAuiPaneInfo& BestSize(const wxSize& size) { best_size = size; return *this; }
    ewxAuiPaneInfo& MinSize(const wxSize& size) { min_size = size; return *this; }
    ewxAuiPaneInfo& MaxSize(const wxSize& size) { max_size = size; return *this; }
    ewxAuiPaneInfo& BestSize(int x, int y) { best_size.Set(x,y); return *this; }
    ewxAuiPaneInfo& MinSize(int x, int y) { min_size.Set(x,y); return *this; }
    ewxAuiPaneInfo& MaxSize(int x, int y) { max_size.Set(x,y); return *this; }
    ewxAuiPaneInfo& FloatingPosition(const wxPoint& pos) { floating_pos = pos; return *this; }
    ewxAuiPaneInfo& FloatingPosition(int x, int y) { floating_pos.x = x; floating_pos.y = y; return *this; }
    ewxAuiPaneInfo& FloatingSize(const wxSize& size) { floating_size = size; return *this; }
    ewxAuiPaneInfo& FloatingSize(int x, int y) { floating_size.Set(x,y); return *this; }
    ewxAuiPaneInfo& Fixed() { return SetFlag(optionResizable, false); }
    ewxAuiPaneInfo& Resizable(bool resizable = true) { return SetFlag(optionResizable, resizable); }
    ewxAuiPaneInfo& Dock() { return SetFlag(optionFloating, false); }
    ewxAuiPaneInfo& Float() { return SetFlag(optionFloating, true); }
    ewxAuiPaneInfo& Hide() { return SetFlag(optionHidden, true); }
    ewxAuiPaneInfo& Show(bool show = true) { return SetFlag(optionHidden, !show); }
    ewxAuiPaneInfo& CaptionVisible(bool visible = true) { return SetFlag(optionCaption, visible); }
    ewxAuiPaneInfo& Maximize() { return SetFlag(optionMaximized, true); }
    ewxAuiPaneInfo& Restore() { return SetFlag(optionMaximized, false); }
    ewxAuiPaneInfo& PaneBorder(bool visible = true) { return SetFlag(optionPaneBorder, visible); }
    ewxAuiPaneInfo& Gripper(bool visible = true) { return SetFlag(optionGripper, visible); }
    ewxAuiPaneInfo& GripperTop(bool attop = true) { return SetFlag(optionGripperTop, attop); }
    ewxAuiPaneInfo& CloseButton(bool visible = true) { return SetFlag(buttonClose, visible); }
    ewxAuiPaneInfo& MaximizeButton(bool visible = true) { return SetFlag(buttonMaximize, visible); }
    ewxAuiPaneInfo& MinimizeButton(bool visible = true) { return SetFlag(buttonMinimize, visible); }
    ewxAuiPaneInfo& PinButton(bool visible = true) { return SetFlag(buttonPin, visible); }
    ewxAuiPaneInfo& DestroyOnClose(bool b = true) { return SetFlag(optionDestroyOnClose, b); }
    ewxAuiPaneInfo& TopDockable(bool b = true) { return SetFlag(optionTopDockable, b); }
    ewxAuiPaneInfo& BottomDockable(bool b = true) { return SetFlag(optionBottomDockable, b); }
    ewxAuiPaneInfo& LeftDockable(bool b = true) { return SetFlag(optionLeftDockable, b); }
    ewxAuiPaneInfo& RightDockable(bool b = true) { return SetFlag(optionRightDockable, b); }
    ewxAuiPaneInfo& Floatable(bool b = true) { return SetFlag(optionFloatable, b); }
    ewxAuiPaneInfo& Movable(bool b = true) { return SetFlag(optionMovable, b); }

    ewxAuiPaneInfo& AlwaysFixed(bool b = true) { if (b) { Fixed(); } else { Resizable(); } return SetFlag(optionAlwaysFixed, b); }
    ewxAuiPaneInfo& Open(bool b = true) { return SetFlag(optionOpen, b); }
    ewxAuiPaneInfo& Focus(bool b = true) { return SetFlag(optionFocus, b); }
    ewxAuiPaneInfo& Pin(bool b = true) { return SetFlag(optionPin, b); }
    ewxAuiPaneInfo& OptionsButton(bool b = true) { return SetFlag(buttonOptions, b); }
    ewxAuiPaneInfo& OpenButton(bool b = true) { return SetFlag(buttonOpen, b); }
    ewxAuiPaneInfo& TakeFocusButton(bool b = true) { return SetFlag(buttonTakeFocus, b); }
    ewxAuiPaneInfo& AddFocusButton(bool b = true) { return SetFlag(buttonAddFocus, b); }
#if wxABI_VERSION >= 20807
    ewxAuiPaneInfo& DockFixed(bool b = true) { return SetFlag(optionDockFixed, b); }
#endif

    ewxAuiPaneInfo& Dockable(bool b = true)
    {
        return TopDockable(b).BottomDockable(b).LeftDockable(b).RightDockable(b);
    }

    ewxAuiPaneInfo& DefaultPane()
    {
        state |= optionTopDockable | optionBottomDockable |
                 optionLeftDockable | optionRightDockable |
                 optionFloatable | optionMovable | optionResizable |
            buttonOpen |
                 optionCaption | optionPaneBorder | buttonClose;
        return *this;
    }

    ewxAuiPaneInfo& CentrePane() { return CenterPane(); }
    ewxAuiPaneInfo& CenterPane()
    {
        state = 0;
        return Center().PaneBorder().Resizable();
    }

    ewxAuiPaneInfo& ToolbarPane()
    {
        DefaultPane();
        state |= (optionToolbar | optionGripper);
        state &= ~(optionResizable | optionCaption);
        if (dock_layer == 0)
            dock_layer = 10;
        return *this;
    }

    ewxAuiPaneInfo& SetFlag(unsigned int flag, bool option_state)
    {
        if (option_state)
            state |= flag;
             else
            state &= ~flag;
        return *this;
    }

    bool HasFlag(unsigned int flag) const
    {
        return (state & flag) ? true:false;
    }

#ifdef SWIG
    %typemap(out) ewxAuiPaneInfo& ;
#endif

public:

    // NOTE: You can add and subtract flags from this list,
    // but do not change the values of the flags, because
    // they are stored in a binary integer format in the
    // perspective string.  If you really need to change the
    // values around, you'll have to ensure backwards-compatibility
    // in the perspective loading code.
    enum ewxAuiPaneState
    {
        optionFloating        = 1 << 0,
        optionHidden          = 1 << 1,
        optionLeftDockable    = 1 << 2,
        optionRightDockable   = 1 << 3,
        optionTopDockable     = 1 << 4,
        optionBottomDockable  = 1 << 5,
        optionFloatable       = 1 << 6,
        optionMovable         = 1 << 7,
        optionResizable       = 1 << 8,
        optionPaneBorder      = 1 << 9,
        optionCaption         = 1 << 10,
        optionGripper         = 1 << 11,
        optionDestroyOnClose  = 1 << 12,
        optionToolbar         = 1 << 13,
        optionActive          = 1 << 14,
        optionGripperTop      = 1 << 15,
        optionMaximized       = 1 << 16,
        optionDockFixed       = 1 << 17,

        optionAlwaysFixed = 1 << 29,
        optionOpen = 1 << 18,
        optionFocus = 1 << 19,
        optionPin = 1 << 20,
        buttonOptions = 1 << 25,

        buttonClose           = 1 << 21,
        buttonMaximize        = 1 << 22,
        buttonMinimize        = 1 << 23,
        buttonPin             = 1 << 24,

        buttonOpen         = 1 << 26,
        buttonTakeFocus         = 1 << 27,
        buttonAddFocus         = 1 << 28,

        savedHiddenState      = 1 << 30, // used internally
        actionPane            = 1 << 31  // used internally
    };

public:
    wxString name;        // name of the pane
    wxString caption;     // caption displayed on the window

    wxWindow* window;     // window that is in this pane
    wxFrame* frame;       // floating frame window that holds the pane
    unsigned int state;   // a combination of wxPaneState values

    int dock_direction;   // dock direction (top, bottom, left, right, center)
    int dock_layer;       // layer number (0 = innermost layer)
    int dock_row;         // row number on the docking bar (0 = first row)
    int dock_pos;         // position inside the row (0 = first position)

    wxSize best_size;     // size that the layout engine will prefer
    wxSize min_size;      // minimum size the pane window can tolerate
    wxSize max_size;      // maximum size the pane window can tolerate

    wxPoint floating_pos; // position while floating
    wxSize floating_size; // size while floating
    int dock_proportion;  // proportion while docked

    ewxAuiPaneButtonArray buttons; // buttons on the pane

    wxRect rect;              // current rectangle (populated by ewxAUI)
};



class WXDLLIMPEXP_FWD_AUI ewxAuiFloatingFrame;

class ewxAuiManager : public wxEvtHandler
{
friend class ewxAuiFloatingFrame;

public:

    ewxAuiManager(wxWindow* managed_wnd = NULL,
                   unsigned int flags = ewxAUI_MGR_DEFAULT);
    virtual ~ewxAuiManager();
    void UnInit();

    void SetFlags(unsigned int flags);
    unsigned int GetFlags() const;

    void SetManagedWindow(wxWindow* managed_wnd);
    wxWindow* GetManagedWindow() const;

    static ewxAuiManager* GetManager(wxWindow* window);

    void SetArtProvider(ewxAuiDockArt* art_provider, bool take_ownership=true);
    ewxAuiDockArt* GetArtProvider() const;

    ewxAuiPaneInfo& GetPane(wxWindow* window);
    ewxAuiPaneInfo& GetPane(const wxString& name);
    ewxAuiPaneInfoArray& GetAllPanes();

    bool AddPane(wxWindow* window,
                 const ewxAuiPaneInfo& pane_info);

    bool AddPane(wxWindow* window,
                 const ewxAuiPaneInfo& pane_info,
                 const wxPoint& drop_pos);

    bool AddPane(wxWindow* window,
                 int direction = wxLEFT,
                 const wxString& caption = wxEmptyString);

    bool InsertPane(wxWindow* window,
                 const ewxAuiPaneInfo& insert_location,
                 int insert_level = ewxAUI_INSERT_PANE);

    bool DetachPane(wxWindow* window);

    void Update();

    wxString SavePaneInfo(ewxAuiPaneInfo& pane);
    void LoadPaneInfo(wxString pane_part, ewxAuiPaneInfo &pane);
    wxString SavePerspective();
    bool LoadPerspective(const wxString& perspective, bool update = true);

    void SetDockSizeConstraint(double width_pct, double height_pct);
    void GetDockSizeConstraint(double* width_pct, double* height_pct) const;

    void ClosePane(ewxAuiPaneInfo& pane_info);
    void MaximizePane(ewxAuiPaneInfo& pane_info);
    void RestorePane(ewxAuiPaneInfo& pane_info);
    void RestoreMaximizedPane();

public:

    virtual ewxAuiFloatingFrame* CreateFloatingFrame(wxWindow* parent, const ewxAuiPaneInfo& p);

    void StartPaneDrag(
                 wxWindow* pane_window,
                 const wxPoint& offset);

    wxRect CalculateHintRect(
                 wxWindow* pane_window,
                 const wxPoint& pt,
                 const wxPoint& offset);

    void DrawHintRect(
                 wxWindow* pane_window,
                 const wxPoint& pt,
                 const wxPoint& offset);

    virtual void ShowHint(const wxRect& rect);
    virtual void HideHint();

public:

    // deprecated -- please use SetManagedWindow() and
    // and GetManagedWindow() instead

    wxDEPRECATED( void SetFrame(wxFrame* frame) );
    wxDEPRECATED( wxFrame* GetFrame() const );

protected:

    void UpdateHintWindowConfig();

    void DoFrameLayout();

    void LayoutAddPane(wxSizer* container,
                       ewxAuiDockInfo& dock,
                       ewxAuiPaneInfo& pane,
                       ewxAuiDockUIPartArray& uiparts,
                       bool spacer_only);

    void LayoutAddDock(wxSizer* container,
                       ewxAuiDockInfo& dock,
                       ewxAuiDockUIPartArray& uiparts,
                       bool spacer_only);

    wxSizer* LayoutAll(ewxAuiPaneInfoArray& panes,
                       ewxAuiDockInfoArray& docks,
                       ewxAuiDockUIPartArray& uiparts,
                       bool spacer_only = false);

    virtual bool ProcessDockResult(ewxAuiPaneInfo& target,
                                   const ewxAuiPaneInfo& new_pos);

    bool DoDrop(ewxAuiDockInfoArray& docks,
                ewxAuiPaneInfoArray& panes,
                ewxAuiPaneInfo& drop,
                const wxPoint& pt,
                const wxPoint& action_offset = wxPoint(0,0));

    ewxAuiDockUIPart* HitTest(int x, int y);
    ewxAuiDockUIPart* GetPanePart(wxWindow* pane);
    int GetDockPixelOffset(ewxAuiPaneInfo& test);
    void OnFloatingPaneMoveStart(wxWindow* window);
    void OnFloatingPaneMoving(wxWindow* window, wxDirection dir );
    void OnFloatingPaneMoved(wxWindow* window, wxDirection dir);
    void OnFloatingPaneActivated(wxWindow* window);
    void OnFloatingPaneClosed(wxWindow* window, wxCloseEvent& evt);
    void OnFloatingPaneResized(wxWindow* window, const wxSize& size);
    void Render(wxDC* dc);
    void Repaint(wxDC* dc = NULL);
    void ProcessMgrEvent(ewxAuiManagerEvent& event);
    void UpdateButtonOnScreen(ewxAuiDockUIPart* button_ui_part,
                              const wxMouseEvent& event);
    void GetPanePositionsAndSizes(ewxAuiDockInfo& dock,
                              wxArrayInt& positions,
                              wxArrayInt& sizes);

#if wxABI_VERSION >= 20810
    /// Ends a resize action, or for live update, resizes the sash
    bool DoEndResizeAction(wxMouseEvent& event);
#endif

public:

    // public events (which can be invoked externally)
    void OnRender(ewxAuiManagerEvent& evt);
    void OnPaneButton(ewxAuiManagerEvent& evt);

protected:

    // protected events
    void OnPaint(wxPaintEvent& evt);
    void OnEraseBackground(wxEraseEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnSetCursor(wxSetCursorEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMotion(wxMouseEvent& evt);
    void OnLeaveWindow(wxMouseEvent& evt);
    void OnChildFocus(wxChildFocusEvent& evt);
    void OnHintFadeTimer(wxTimerEvent& evt);
    void OnFindManager(ewxAuiManagerEvent& evt);

protected:

    enum
    {
        actionNone = 0,
        actionResize,
        actionClickButton,
        actionClickCaption,
        actionDragToolbarPane,
        actionDragFloatingPane
    };

protected:

    wxWindow* m_frame;           // the window being managed
    ewxAuiDockArt* m_art;            // dock art object which does all drawing

    bool m_we_own_m_art;
    unsigned int m_flags;        // manager flags ewxAUI_MGR_*

    ewxAuiPaneInfoArray m_panes;     // array of panes structures
    ewxAuiDockInfoArray m_docks;     // array of docks structures
    ewxAuiDockUIPartArray m_uiparts; // array of UI parts (captions, buttons, etc)

    int m_action;                // current mouse action
    wxPoint m_action_start;      // position where the action click started
    wxPoint m_action_offset;     // offset from upper left of the item clicked
    ewxAuiDockUIPart* m_action_part; // ptr to the part the action happened to
    wxWindow* m_action_window;   // action frame or window (NULL if none)
    wxRect m_action_hintrect;    // hint rectangle for the action
    wxRect m_last_rect;
    ewxAuiDockUIPart* m_hover_button;// button uipart being hovered over
    wxRect m_last_hint;          // last hint rectangle
    wxPoint m_last_mouse_move;   // last mouse move position (see OnMotion)
    bool m_skipping;
    bool m_has_maximized;

    double m_dock_constraint_x;  // 0.0 .. 1.0; max pct of window width a dock can consume
    double m_dock_constraint_y;  // 0.0 .. 1.0; max pct of window height a dock can consume

    wxFrame* m_hint_wnd;         // transparent hint window, if supported by platform
    wxTimer m_hint_fadetimer;    // transparent fade timer
    wxByte m_hint_fadeamt;       // transparent fade amount
    wxByte m_hint_fademax;       // maximum value of hint fade

    void* m_reserved;

#ifndef SWIG
    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(ewxAuiManager)
#endif // SWIG
};



// event declarations/classes

class ewxAuiManagerEvent : public wxEvent
{
public:
    ewxAuiManagerEvent(wxEventType type=wxEVT_NULL) : wxEvent(0, type)
    {
        manager = NULL;
        pane = NULL;
        button = 0;
        veto_flag = false;
        canveto_flag = true;
        dc = NULL;
    }
#ifndef SWIG
    ewxAuiManagerEvent(const ewxAuiManagerEvent& c) : wxEvent(c)
    {
        manager = c.manager;
        pane = c.pane;
        button = c.button;
        veto_flag = c.veto_flag;
        canveto_flag = c.canveto_flag;
        dc = c.dc;
    }
#endif
    wxEvent *Clone() const { return new ewxAuiManagerEvent(*this); }

    void SetManager(ewxAuiManager* mgr) { manager = mgr; }
    void SetPane(ewxAuiPaneInfo* p) { pane = p; }
    void SetButton(int b) { button = b; }
    void SetDC(wxDC* pdc) { dc = pdc; }

    ewxAuiManager* GetManager() const { return manager; }
    ewxAuiPaneInfo* GetPane() const { return pane; }
    int GetButton() const { return button; }
    wxDC* GetDC() const { return dc; }

    void Veto(bool veto = true) { veto_flag = veto; }
    bool GetVeto() const { return veto_flag; }
    void SetCanVeto(bool can_veto) { canveto_flag = can_veto; }
    bool CanVeto() const { return  canveto_flag && veto_flag; }

public:
    ewxAuiManager* manager;
    ewxAuiPaneInfo* pane;
    int button;
    bool veto_flag;
    bool canveto_flag;
    wxDC* dc;

#ifndef SWIG
private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(ewxAuiManagerEvent)
#endif
};


class ewxAuiDockInfo
{
public:
    ewxAuiDockInfo()
    {
        dock_direction = 0;
        dock_layer = 0;
        dock_row = 0;
        size = 0;
        min_size = 0;
        resizable = true;
        fixed = false;
        toolbar = false;
        reserved1 = false;
    }

#ifndef SWIG
    ewxAuiDockInfo(const ewxAuiDockInfo& c)
    {
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        size = c.size;
        min_size = c.min_size;
        resizable = c.resizable;
        fixed = c.fixed;
        toolbar = c.toolbar;
        panes = c.panes;
        rect = c.rect;
        reserved1 = c.reserved1;
    }

    ewxAuiDockInfo& operator=(const ewxAuiDockInfo& c)
    {
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        size = c.size;
        min_size = c.min_size;
        resizable = c.resizable;
        fixed = c.fixed;
        toolbar = c.toolbar;
        panes = c.panes;
        rect = c.rect;
        reserved1 = c.reserved1;
        return *this;
    }
#endif // SWIG

    bool IsOk() const { return (dock_direction != 0) ? true : false; }
    bool IsHorizontal() const { return (dock_direction == ewxAUI_DOCK_TOP ||
                             dock_direction == ewxAUI_DOCK_BOTTOM) ? true:false; }
    bool IsVertical() const { return (dock_direction == ewxAUI_DOCK_LEFT ||
                             dock_direction == ewxAUI_DOCK_RIGHT ||
                             dock_direction == ewxAUI_DOCK_CENTER) ? true:false; }
public:
    ewxAuiPaneInfoPtrArray panes; // array of panes
    wxRect rect;              // current rectangle
    int dock_direction;       // dock direction (top, bottom, left, right, center)
    int dock_layer;           // layer number (0 = innermost layer)
    int dock_row;             // row number on the docking bar (0 = first row)
    int size;                 // size of the dock
    int min_size;             // minimum size of a dock (0 if there is no min)
    bool resizable;           // flag indicating whether the dock is resizable
    bool toolbar;             // flag indicating dock contains only toolbars
    bool fixed;               // flag indicating that the dock operates on
                              // absolute coordinates as opposed to proportional
    bool reserved1;
};


class ewxAuiDockUIPart
{
public:
    enum
    {
        typeCaption,
        typeGripper,
        typeDock,
        typeDockSizer,
        typePane,
        typePaneSizer,
        typeBackground,
        typePaneBorder,
        typePaneButton
    };

    int type;                // ui part type (see enum above)
    int orientation;         // orientation (either wxHORIZONTAL or wxVERTICAL)
    ewxAuiDockInfo* dock;        // which dock the item is associated with
    ewxAuiPaneInfo* pane;        // which pane the item is associated with
    ewxAuiPaneButton* button;    // which pane button the item is associated with
    wxSizer* cont_sizer;     // the part's containing sizer
    wxSizerItem* sizer_item; // the sizer item of the part
    wxRect rect;             // client coord rectangle of the part itself
};


class ewxAuiPaneButton
{
public:
    int button_id;        // id of the button (e.g. buttonClose)
};



#ifndef SWIG
// wx event machinery

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_BUTTON, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_CLOSE, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_MAXIMIZE, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_RESTORE, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_RENDER, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_FIND_MANAGER, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_OPEN, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_OPTIONS, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_TAKE_FOCUS, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_PANE_ADD_FOCUS, 0)
    DECLARE_EVENT_TYPE(wxEVT_EWXAUI_UPDATE, 0)
END_DECLARE_EVENT_TYPES()


typedef void (wxEvtHandler::*ewxAuiManagerEventFunction)(ewxAuiManagerEvent&);

#define ewxAuiManagerEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(ewxAuiManagerEventFunction, &func)

#define EVT_EWXAUI_PANE_BUTTON(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_BUTTON, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_CLOSE(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_CLOSE, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_MAXIMIZE(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_MAXIMIZE, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_RESTORE(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_RESTORE, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_RENDER(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_RENDER, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_FIND_MANAGER(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_FIND_MANAGER, ewxAuiManagerEventHandler(func))

#define EVT_EWXAUI_PANE_OPEN(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_OPEN, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_OPTIONS(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_OPTIONS, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_TAKE_FOCUS(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_TAKE_FOCUS, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_ADD_FOCUS(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_ADD_FOCUS, ewxAuiManagerEventHandler(func))
#define EVT_EWXAUI_UPDATE(func) \
   wx__DECLARE_EVT0(wxEVT_EWXAUI_UPDATE, ewxAuiManagerEventHandler(func))

#else

%constant wxEventType wxEVT_EWXAUI_PANE_BUTTON;
%constant wxEventType wxEVT_EWXAUI_PANE_CLOSE;
%constant wxEventType wxEVT_EWXAUI_PANE_MAXIMIZE;
%constant wxEventType wxEVT_EWXAUI_PANE_RESTORE;
%constant wxEventType wxEVT_EWXAUI_RENDER;
%constant wxEventType wxEVT_EWXAUI_FIND_MANAGER;

%pythoncode {
    EVT_EWXAUI_PANE_BUTTON = wx.PyEventBinder( wxEVT_EWXAUI_PANE_BUTTON )
    EVT_EWXAUI_PANE_CLOSE = wx.PyEventBinder( wxEVT_EWXAUI_PANE_CLOSE )
    EVT_EWXAUI_PANE_MAXIMIZE = wx.PyEventBinder( wxEVT_EWXAUI_PANE_MAXIMIZE )
    EVT_EWXAUI_PANE_RESTORE = wx.PyEventBinder( wxEVT_EWXAUI_PANE_RESTORE )
    EVT_EWXAUI_RENDER = wx.PyEventBinder( wxEVT_EWXAUI_RENDER )
    EVT_EWXAUI_FIND_MANAGER = wx.PyEventBinder( wxEVT_EWXAUI_FIND_MANAGER )
}
#endif // SWIG

#endif // wxUSE_AUI
#endif //_EWX_FRAMEMANAGER_H_

