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

class BuilderDockArt : public wxAuiDefaultDockArt
{
  public:
    BuilderDockArt();
    virtual ~BuilderDockArt();
};

#endif // BUILDERDOCKART_H_
