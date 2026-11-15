/**
 * @file
 *
 * A Launcher as a tab or window of the Organizer's process (#983): what
 * WxLauncherApp::OnInit does, without the once-per-process part (the
 * auth listener stays the Organizer's).
 */
#include <vector>
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "wxgui/ToolTabs.H"
#include "wxgui/LaunchEmbed.H"
#include "wxgui/ewxButton.H"
#include "LauncherMessages.H"
#include "WxLauncher.H"

static WxJMSMessageDispatch *openLauncher()
{
  WxLauncher *frame = new WxLauncher(NULL);
  ToolTabs::open(frame, 0, _("Launcher"), "launcher");
  frame->loadSettings("Launcher");
  LauncherMessages *msgs = new LauncherMessages(frame->panel(), frame);
  frame->setMessages(msgs);
  // The invoke that follows sets the calculation and shows the window.
  msgs->registerTopShell(frame);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(LAUNCHER, msgs, frame);
  return msgs;
}

// Organizer test hook "launcher machine|pick NAME", "launch", "get WHAT":
// the Launcher script's commands on the active Launcher tab.
static wxString launcherCommand(const wxString& arg)
{
  WxLauncher *w = dynamic_cast<WxLauncher*>(ToolTabs::activeTool());
  if (!w) return "not a Launcher tab";
  LaunchPanel *l = w->panel();
  wxString cmd = arg.BeforeFirst(' '), rest = arg.AfterFirst(' ');
  string name = std::string(rest.utf8_str());
  if (cmd == "machine") return l->selectMachine(name) ? "ok" : "not in the list";
  if (cmd == "pick") return l->pickMachine(name) ? "ok" : "not in the list";
  if (cmd == "launch") return l->clickLaunch() ? "ok" : "Launch is missing or disabled";
  if (cmd == "get") return wxString::FromUTF8(l->hookGet(name).c_str());
  return "bad command";
}

// The panel in the Set-up area: warnings are shown in it, not as pop-ups.
namespace {
class EmbeddedLaunch : public LaunchEmbed, public LaunchHost
{
public:
  EmbeddedLaunch(wxWindow *parent, const string& url,
                 std::function<void()> close, std::function<void()> resize,
                 std::function<void(const std::string&, int)> say)
    : p_close(close), p_resize(resize), p_say(say)
  {
    p_panel = new LaunchPanel(parent, this);
    p_panel->setContext(url);
  }
  ~EmbeddedLaunch() { if (p_panel) p_panel->Destroy(); }
  wxWindow *window() override { return p_panel; }
  bool go() override { return p_panel && p_panel->clickLaunch(); }
  string get(const string& what) override
  {
    if (!p_panel) return "";
    if (what != "info") return p_panel->hookGet(what);
    wxWindow *b = p_panel->FindWindow(LaunchPanel::ID_BUTTON_WXLAUNCHER_LAUNCH);
    string out = "machine=" + p_panel->hookGet("machine") + " launch=";
    out += !b ? "absent" : b->IsEnabled() ? "enabled" : "disabled";
    if (b) out += " label=" + string(b->GetLabel().utf8_str());
    // Controls that reach past the right edge of the area (a horizontal scroll).
    wxRect view = p_panel->GetParent()->GetScreenRect();
    int clipped = 0;
    string names;
    std::vector<wxWindow*> todo(1, p_panel);
    while (!todo.empty()) {
      wxWindow *c = todo.back();
      todo.pop_back();
      if (!c->IsShown()) continue;
      if (c->GetChildren().empty() && c->GetScreenRect().GetRight() > view.GetRight())
        { clipped++; names += " " + string(wxString(c->GetClassInfo()->GetClassName()).utf8_str()) + ":" + string(c->GetLabel().utf8_str()).substr(0,14) + "@" + std::to_string(c->GetScreenRect().GetRight() - view.GetLeft()); }
      for (wxWindow *k : c->GetChildren()) todo.push_back(k);
    }
    out += wxString::Format(" clipped=%d panel=%d view=%d", clipped,
                            p_panel->GetSize().x, view.width).ToStdString() + names;
    return out;
  }
  void closeRequest() override { if (p_close) p_close(); }
  void deferMessages(bool) override {}
  void relayout() override { if (p_resize) p_resize(); }
  void launchAllowed(bool, bool) override {}
  bool inlineWarnings() const override { return true; }
  void message(const string& t, int sev) override { if (p_say) p_say(t, sev); }
private:
  wxWeakRef<LaunchPanel> p_panel;
  std::function<void()> p_close, p_resize;
  std::function<void(const std::string&, int)> p_say;
};
}

void registerLauncherInProcess()
{
  LaunchEmbed::maker() = [](wxWindow *parent, const string& url,
                            std::function<void()> close,
                            std::function<void()> resize,
                            std::function<void(const std::string&, int)> say) -> LaunchEmbed* {
    return new EmbeddedLaunch(parent, url, close, resize, say);
  };
  ToolTabs::addCommand("launcher", launcherCommand);
  InProcessTools::add(LAUNCHER, openLauncher);
}
