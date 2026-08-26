#ifndef EWXAUICOMPAT_H_
#define EWXAUICOMPAT_H_

// The original ECCE Builder was written against a locally-patched fork of
// wxWidgets' AUI library ("ewxAUI", built from build/ewxaui) that added
// extra pane-caption buttons (take focus / add focus (pin) / options /
// open-close) with their own custom wxAuiManagerEvent-derived events on
// top of stock wxAUI. That patched library is not available for wx3.2.
//
// wxEVT_EWXAUI_PANE_CLOSE / EVT_EWXAUI_PANE_CLOSE map directly onto the
// stock wxEVT_AUI_PANE_CLOSE / EVT_AUI_PANE_CLOSE (same semantics: the
// pane's own close button was clicked).
//
// The other four events (take focus, add focus, options, open) were fired
// by mouse hit-testing on the extra custom caption buttons that stock
// wxAUI's dock art doesn't draw (see BuilderDockArt); nothing in stock
// wx3.2 AUI generates them anymore. They are kept here as ordinary custom
// event types (still postable/handleable, e.g. Builder posts
// wxEVT_EWXAUI_UPDATE to itself after processing a close) so the handler
// code in Builder.C compiles and keeps its original structure, but they
// are no longer triggered by AUI pane-button clicks.
#include <wx/aui/aui.h>

wxDECLARE_EVENT(wxEVT_EWXAUI_PANE_TAKE_FOCUS, wxAuiManagerEvent);
wxDECLARE_EVENT(wxEVT_EWXAUI_PANE_ADD_FOCUS, wxAuiManagerEvent);
wxDECLARE_EVENT(wxEVT_EWXAUI_PANE_OPTIONS, wxAuiManagerEvent);
wxDECLARE_EVENT(wxEVT_EWXAUI_PANE_OPEN, wxAuiManagerEvent);
wxDECLARE_EVENT(wxEVT_EWXAUI_UPDATE, wxAuiManagerEvent);

#define EVT_EWXAUI_PANE_CLOSE(func) EVT_AUI_PANE_CLOSE(func)

#define EVT_EWXAUI_PANE_TAKE_FOCUS(func) \
    wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_TAKE_FOCUS, wxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_ADD_FOCUS(func) \
    wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_ADD_FOCUS, wxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_OPTIONS(func) \
    wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_OPTIONS, wxAuiManagerEventHandler(func))
#define EVT_EWXAUI_PANE_OPEN(func) \
    wx__DECLARE_EVT0(wxEVT_EWXAUI_PANE_OPEN, wxAuiManagerEventHandler(func))
#define EVT_EWXAUI_UPDATE(func) \
    wx__DECLARE_EVT0(wxEVT_EWXAUI_UPDATE, wxAuiManagerEventHandler(func))

#endif // EWXAUICOMPAT_H_
