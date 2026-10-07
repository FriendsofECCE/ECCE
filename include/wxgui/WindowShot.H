#ifndef WINDOWSHOT_HH
#define WINDOWSHOT_HH

#include "wx/wx.h"

// Saves what a top-level window looks like as a PNG, for the test hooks
// that take screenshots (machregister, launcher).
bool ecceWindowShot(wxWindow* win, const wxString& file);

#endif
