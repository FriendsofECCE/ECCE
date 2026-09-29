#ifndef BUILDERDOCKART_H_
#define BUILDERDOCKART_H_

// NOTE: the original ECCE build used a locally-patched fork of wxWidgets'
// AUI library ("ewxAUI", built from build/ewxaui) that added extra pane
// caption buttons (take focus / pin / options) with their own bitmaps and
// events on top of stock wxAuiDefaultDockArt. That patched library isn't
// available for wx3.2, and its extra caption buttons have no stock
// equivalent, so this now derives directly from the stock wx3.2
// wxAuiDefaultDockArt and only keeps the (unaffected) color customization.
#include <wx/aui/aui.h>

#include <functional>

class BuilderDockArt : public wxAuiDefaultDockArt
{
  public:
    BuilderDockArt();
    virtual ~BuilderDockArt();

    // Asks Builder whether a pane is folded; the pin slot is the fold toggle.
    void setFoldedQuery(std::function<bool(wxWindow*)> q) { p_isFolded = q; }

    virtual void DrawPaneButton(wxDC& dc, wxWindow* window, int button,
                                int buttonState, const wxRect& rect,
                                wxAuiPaneInfo& pane);

  private:
    std::function<bool(wxWindow*)> p_isFolded;
};

#endif // BUILDERDOCKART_H_
