/**
 * @file
 *
 * Tool windows inside the Organizer's process (#983).
 */
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

#include "wx/app.h"
#include "wx/toplevel.h"

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/MqttLink.H"

#include "wxgui/InProcessTools.H"
#include "wxgui/ToolTabs.H"
#include "wxgui/WxJMSMessageDispatch.H"

namespace {

struct Window {
  string app;
  WxJMSMessageDispatch *d;
  wxTopLevelWindow *frame;
  string ctx;   // the calculation it was opened for
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


static std::map<string, InProcessTools::Alias>& aliases()
{
  static auto *m = new std::map<string, InProcessTools::Alias>;
  return *m;
}


void InProcessTools::alias(const string& app, Alias to)
{
  aliases()[app] = to;
}


bool InProcessTools::handles(const string& app)
{
  return MqttLink::localDelivery() && openers().count(app) > 0;
}


bool InProcessTools::open(JMSMessage& startMsg)
{
  string app = startMsg.getProperty("appname");
  if (!handles(app)) return false;
  auto al = aliases().find(app);
  if (al != aliases().end()) {
    string other = al->second(startMsg.getProperty("calcurl"));
    if (!other.empty() && handles(other)) app = other;
  }

  WxJMSMessageDispatch *d = 0;
  if (!startMsg.getIntProperty("forcenew")) {
    string ctx = pollContext(startMsg.getProperty("calcurl"));
    // The window's own context is set only once its invoke has run, so a
    // second click before that (or a tool that keeps none) would open
    // another tab: compare the context it was opened for too.
    for (const Window& w : windows())
        if (w.app == app && (w.d->getPollContext() == ctx ||
                             (!ctx.empty() && w.ctx == ctx))) {
          d = w.d;
          break;
        }
  }
  string ctxNow = pollContext(startMsg.getProperty("calcurl"));
  bool made = false;
  if (!d) { d = openers()[app](); made = true; }
  if (!d) return false;
  for (Window& w : windows())
    if (w.d == d) {
      if (made) w.ctx = ctxNow;
      // One tab per calculation and tool: the existing tab comes to the
      // front even if the tool's invoke handler does not raise it.
      else ToolTabs::select(w.frame);
    }

  // The window gets the request as the gateway would send it, by the
  // local path, so its invoke handler raises and fills it as in its own
  // process.
  JMSPublisher pub("Organizer");
  JMSMessage invoke(startMsg);
  invoke.addProperty("appname", app);
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


static InProcessTools::CalcAction& calcActionFn()
{
  static auto *f = new InProcessTools::CalcAction;
  return *f;
}


void InProcessTools::setCalcAction(CalcAction fn)
{
  calcActionFn() = fn;
}


string InProcessTools::calcAction(const string& action, const string& url,
                                  const string& answer)
{
  if (!calcActionFn()) return "no Organizer in this process";
  return calcActionFn()(action, url, answer);
}


string InProcessTools::context(const wxTopLevelWindow *frame)
{
  for (const Window& w : windows())
    if (w.frame == frame) {
      string c = w.d->getPollContext();
      return c.empty() ? w.ctx : c;
    }
  return "";
}


namespace {
std::set<wxTopLevelWindow*>& closingFrames()
{
  static auto *s = new std::set<wxTopLevelWindow*>;
  return *s;
}
}


void InProcessTools::setClosing(wxTopLevelWindow *frame, bool on)
{
  if (on) closingFrames().insert(frame);
  else closingFrames().erase(frame);
}


bool InProcessTools::closing()
{
  return !closingFrames().empty();
}


bool InProcessTools::closeAll(bool force)
{
  // Close() may destroy the window and untrack it: work on a copy.
  std::vector<Window> w = windows();
  for (const Window& t : w) {
    // Closed already, by its own ecce_quit handler: asking again would
    // repeat its save question.
    if (wxTheApp->IsScheduledForDestruction(t.frame)) continue;
    // A tab's save question: show which tool asks.
    ToolTabs::select(t.frame);
    if (getenv("ECCE_TEST_ORGANIZER"))
      fprintf(stderr, "closeAll: closing %s '%s'\n", t.app.c_str(),
              (const char*)t.frame->GetTitle().utf8_str());
    if (!t.frame->Close(force)) return false;
  }
  return true;
}
