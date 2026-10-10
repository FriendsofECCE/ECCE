#include "ReleaseNotice.H"

#include <ctime>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "util/BrowserHelp.H"
#include "util/Ecce.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

namespace {
  const int ID_RELEASE_PAGE = wxNewId();
  const int ID_DISMISS_VERSION = wxNewId();

  bool enabled()
  {
    if (ReleaseCheck::disabledByEnvironment()) return false;
    Preferences pref(PrefLabels::GLOBALPREFFILE);
    bool on = true;
    pref.getBool(PrefLabels::UPDATECHECK, on);
    return on;
  }

  // Failures are never shown to the user; one line in the log is enough.
  void note(const string& what)
  {
    fprintf(stderr, "ECCE update check: %s\n", what.c_str());
  }
}


ReleaseNotice::ReleaseNotice(wxInfoBar* bar)
  : p_bar(bar)
{
  p_bar->AddButton(ID_RELEASE_PAGE, _("Release notes"));
  p_bar->AddButton(ID_DISMISS_VERSION, _("Don't remind me about this version"));
  p_bar->Bind(wxEVT_BUTTON, &ReleaseNotice::onReleasePage, this, ID_RELEASE_PAGE);
  p_bar->Bind(wxEVT_BUTTON, &ReleaseNotice::onDismissVersion, this,
              ID_DISMISS_VERSION);
}


ReleaseNotice::~ReleaseNotice()
{
  if (p_request.IsOk()) p_request.Cancel();
}


void ReleaseNotice::start()
{
  if (!enabled()) return;

  // Seconds as a string: an int preference would overflow in 2038.
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  string last, ver, url;
  pref.getString(PrefLabels::UPDATELAST, last);
  pref.getString(PrefLabels::UPDATEFOUND, ver);
  pref.getString(PrefLabels::UPDATEFOUNDURL, url);

  long lastTime = atol(last.c_str());
  if (ReleaseCheck::checkDue(lastTime, (long)time(NULL))) {
    // After the Organizer is up and idle, never in the start-up path.
    CallAfter(&ReleaseNotice::begin);
    return;
  }
  ReleaseCheck::Release cached;
  cached.version = ver;
  cached.url = url;
  if (!ver.empty() && ReleaseCheck::compareVersions(ver, Ecce::ecceVersion()) > 0)
    show(cached);
}


void ReleaseNotice::begin()
{
  // Stamped before the request so a failing network is retried tomorrow,
  // not at every start.
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  char buf[32];
  snprintf(buf, sizeof buf, "%ld", (long)time(NULL));
  pref.setString(PrefLabels::UPDATELAST, buf);
  pref.saveFile();

  string url = ReleaseCheck::releasesUrl();
  if (url.compare(0, 7, "file://") == 0) {
    std::ifstream in(url.substr(7).c_str());
    std::stringstream ss;
    ss << in.rdbuf();
    handleJson(ss.str());
    return;
  }

  wxWebSession& session = wxWebSession::GetDefault();
  session.AddCommonHeader("User-Agent",
                          wxString("ECCE/") + Ecce::ecceVersion());
  session.AddCommonHeader("Accept", "application/vnd.github+json");
  p_request = session.CreateRequest(this, wxString::FromUTF8(url.c_str()));
  if (!p_request.IsOk()) { note("no request"); return; }
  Bind(wxEVT_WEBREQUEST_STATE, &ReleaseNotice::onState, this);
  p_request.Start();
}


void ReleaseNotice::onState(wxWebRequestEvent& event)
{
  switch (event.GetState()) {
    case wxWebRequest::State_Completed:
      if (event.GetResponse().GetStatus() == 200)
        handleJson(event.GetResponse().AsString().utf8_string());
      else
        note("HTTP status " +
             std::to_string(event.GetResponse().GetStatus()));
      break;
    case wxWebRequest::State_Failed:
      note(event.GetErrorDescription().utf8_string());
      break;
    default:
      break;
  }
}


void ReleaseNotice::handleJson(const string& json)
{
  ReleaseCheck::Release found;
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  bool have = ReleaseCheck::newerRelease(Ecce::ecceVersion(), json, found);
  pref.setString(PrefLabels::UPDATEFOUND, have ? found.version : "");
  pref.setString(PrefLabels::UPDATEFOUNDURL, have ? found.url : "");
  pref.saveFile();
  if (have) show(found);
}


void ReleaseNotice::show(const ReleaseCheck::Release& release)
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  string dismissed;
  pref.getString(PrefLabels::UPDATEDISMISSED, dismissed);
  if (dismissed == release.version) return;

  p_shown = release;
  p_bar->ShowMessage(wxString::Format(_("ECCE %s is available."),
                         wxString::FromUTF8(release.version.c_str())),
                     wxICON_INFORMATION);
}


void ReleaseNotice::onReleasePage(wxCommandEvent&)
{
  if (!p_shown.url.empty()) BrowserHelp().showPage(p_shown.url);
}


void ReleaseNotice::onDismissVersion(wxCommandEvent&)
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  pref.setString(PrefLabels::UPDATEDISMISSED, p_shown.version);
  pref.saveFile();
  p_bar->Dismiss();
}
