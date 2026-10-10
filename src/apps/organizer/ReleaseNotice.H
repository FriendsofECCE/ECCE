/**
 * @file
 *
 * The Organizer's "a newer ECCE is available" bar (#273).  The choice of
 * release is in util/ReleaseCheck.H; this fetches the list in the
 * background and shows the result in a wxInfoBar.
 */
#ifndef RELEASENOTICE_HH
#define RELEASENOTICE_HH

#include <string>
using std::string;

#include "wx/wx.h"
#include "wx/infobar.h"
#include "wx/webrequest.h"

#include "util/ReleaseCheck.H"

class ReleaseNotice : public wxEvtHandler
{
public:
  /// `bar` belongs to the caller's window; this only drives it.
  ReleaseNotice(wxInfoBar* bar);
  virtual ~ReleaseNotice();

  /// Does nothing when the check is off (preference or environment).
  /// Otherwise shows a remembered result and, when a day has passed,
  /// asks GitHub in the background.
  void start();

private:
  void begin();
  void onState(wxWebRequestEvent& event);
  void handleJson(const string& json);
  void show(const ReleaseCheck::Release& release);
  void onReleasePage(wxCommandEvent& event);
  void onDismissVersion(wxCommandEvent& event);

  wxInfoBar* p_bar;
  wxWebRequest p_request;
  ReleaseCheck::Release p_shown;
};

#endif
