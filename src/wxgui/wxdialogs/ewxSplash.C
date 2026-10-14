#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/splash.h"
#ifdef __WXMSW__
#include "wx/msw/wrapwin.h"
#endif
#include "wx/weakref.h"
#include "wx/timer.h"
#include "wx/dcmemory.h"
#include "wx/filefn.h"
#include <cstdlib>
#include <string>
using std::string;

#include "util/Ecce.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxSplash.H"


// Lets the splash drop behind other windows once its minimum time is up.
class SplashBehind : public wxTimer
{
public:
  explicit SplashBehind(wxSplashScreen *s) : p_splash(s) {}
  void Notify() override
  {
    if (p_splash)
      p_splash->SetWindowStyleFlag(p_splash->GetWindowStyleFlag() &
                                   ~wxSTAY_ON_TOP);
    delete this;
  }
private:
  wxWeakRef<wxSplashScreen> p_splash;
};

// The Organizer takes a few seconds to appear after the login; the title
// card says ECCE is starting.  It stays in front for 1.5 s, then the
// Organizer's window may cover it.
// Left out of test runs and with ECCE_NO_SPLASH.
void ewxShowSplash(int timeoutMs)
{
  const char *off = getenv("ECCE_NO_SPLASH");
  if ((off && *off && string(off) != "0") || getenv("ECCE_TEST_HOME") ||
      getenv("ECCE_NO_REAP") || !Ecce::guiAvailable())
    return;
  wxBitmap card;
  wxString file = ewxBitmap::pixmapFile("ecce-splash.png");
  if (!wxFileExists(file) || !card.LoadFile(file, wxBITMAP_TYPE_PNG) ||
      !card.IsOk())
    return;
  {
    wxMemoryDC dc(card);
    wxFont font(wxFontInfo(15).Family(wxFONTFAMILY_SWISS));
    dc.SetFont(font);
    dc.SetTextForeground(wxColour(205, 205, 212));
    dc.DrawText(wxString("Version ") + Ecce::ecceVersion() +
                wxString::FromUTF8(" \xC2\xB7 starting\xE2\x80\xA6"),
                277, 190);
    dc.SelectObject(wxNullBitmap);
  }
  wxSplashScreen *splash =
    new wxSplashScreen(card, wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
                       timeoutMs, 0, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxBORDER_NONE | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP);
#ifdef __WXMSW__
  // ecce.vbs starts ECCE hidden, and Windows applies that to a program's
  // first ShowWindow, which was the splash's; the second call is obeyed.
  ::ShowWindow((HWND) splash->GetHWND(), SW_SHOWNA);
#endif
  wxYield();
  // In front for at least 1.5 s so a quick start does not just flash it;
  // then the Organizer's window may cover it.  The splash may already be
  // gone (clicked), hence the weak reference.
  (new SplashBehind(splash))->StartOnce(1500);
}
