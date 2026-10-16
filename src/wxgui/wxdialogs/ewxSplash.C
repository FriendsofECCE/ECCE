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

#ifdef __WXMAC__
#include "wx/statbmp.h"

// wxSplashScreen is a floating tool window on macOS, which the system hides
// whenever the app is not the active one, so the splash never showed.  A
// plain frame at the same level stays.
class MacSplash : public wxFrame
{
public:
  MacSplash(const wxBitmap& card, int ms)
    : wxFrame(NULL, wxID_ANY, wxEmptyString, wxDefaultPosition, card.GetSize(),
              wxBORDER_NONE | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP)
  {
    wxStaticBitmap *pic = new wxStaticBitmap(this, wxID_ANY, card);
    SetClientSize(card.GetSize());
    CentreOnScreen();
    pic->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { Destroy(); });
    p_timer.SetOwner(this);
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) { Destroy(); });
    p_timer.StartOnce(ms);
    Show();
  }
private:
  wxTimer p_timer;
};
#endif


// The Organizer takes a few seconds to appear after the login; the title
// card says ECCE is starting.
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
#ifdef __WXMAC__
  new MacSplash(card, timeoutMs);
  wxYield();
  return;
#endif
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
  // A short splash stays in front for its whole time: dropping it behind
  // let the Organizer cover it at once on macOS.  A longer one may be
  // covered after 1.5 s.  The splash may already be gone (clicked), hence
  // the weak reference.
  if (timeoutMs > 3000) (new SplashBehind(splash))->StartOnce(1500);
}
