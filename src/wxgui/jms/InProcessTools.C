/**
 * @file
 *
 * Tool windows inside the Organizer's process (#983).
 */
#include <map>
#include <vector>

#include "wx/app.h"
#include "wx/toplevel.h"

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/MqttLink.H"

#include "wxgui/InProcessTools.H"
#include "wxgui/WxJMSMessageDispatch.H"

namespace {

struct Window {
  string app;
  WxJMSMessageDispatch *d;
  wxTopLevelWindow *frame;
};

// Never destroyed: a dispatcher deleted during static destruction still
// untracks itself.
std::map<string, InProcessTools::Opener>& openers()
{
  static auto *m = new std::map<string, InProcessTools::Opener>;
  return *m;
}

std::vector<Window>& windows()
{
  static auto *w = new std::vector<Window>;
  return *w;
}

// As getAppMCB writes the poll's contextid: a URL ends in '/'.
string pollContext(string id)
{
  if (!id.empty() && (id.find("http") == 0 || id.find("file://") == 0) &&
      id[id.length() - 1] != '/')
    id.append("/");
  return id;
}

}


void InProcessTools::add(const string& app, Opener opener)
{
  openers()[app] = opener;
}


bool InProcessTools::handles(const string& app)
{
  return MqttLink::localDelivery() && openers().count(app) > 0;
}


bool InProcessTools::open(JMSMessage& startMsg)
{
  string app = startMsg.getProperty("appname");
  if (!handles(app)) return false;

  WxJMSMessageDispatch *d = 0;
  if (!startMsg.getIntProperty("forcenew")) {
    string ctx = pollContext(startMsg.getProperty("calcurl"));
    for (const Window& w : windows())
      if (w.app == app && w.d->getPollContext() == ctx) { d = w.d; break; }
  }
  if (!d) d = openers()[app]();
  if (!d) return false;

  // The window gets the request as the gateway would send it, by the
  // local path, so its invoke handler raises and fills it as in its own
  // process.
  JMSPublisher pub("Organizer");
  JMSMessage invoke(startMsg);
  invoke.setSender(Sender("Organizer", MqttLink::processId()));
  invoke.setTarget(Target("", d->getMyID()));
  return pub.publish("ecce_invoke", invoke);
}


void InProcessTools::track(const string& app, WxJMSMessageDispatch *d,
                           wxTopLevelWindow *frame)
{
  windows().push_back(Window{app, d, frame});
  // Data notices from user actions in this window (a save) carry its
  // tool name and id; the Organizer's window sets its own back.
  string name = d->getMyName(), id = d->getMyID();
  frame->Bind(wxEVT_ACTIVATE, [name, id](wxActivateEvent& ev) {
    if (ev.GetActive()) JMSPublisher::setActingTool(name, id);
    ev.Skip();
  });
}


void InProcessTools::untrack(WxJMSMessageDispatch *d)
{
  std::vector<Window>& w = windows();
  for (size_t i = 0; i < w.size(); i++)
    if (w[i].d == d) { w.erase(w.begin() + i); return; }
}


int InProcessTools::count(const string& app)
{
  int n = 0;
  for (const Window& w : windows())
    if (w.app == app) n++;
  return n;
}


bool InProcessTools::closeAll(bool force)
{
  // Close() may destroy the window and untrack it: work on a copy.
  std::vector<Window> w = windows();
  for (const Window& t : w) {
    // Closed already, by its own ecce_quit handler: asking again would
    // repeat its save question.
    if (wxTheApp->IsScheduledForDestruction(t.frame)) continue;
    if (!t.frame->Close(force)) return false;
  }
  return true;
}
