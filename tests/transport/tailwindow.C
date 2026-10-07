// The Tail window (WxTailWindow) driven headless: opens it on a registered
// machine's file, waits for the file's lines, appends through APPEND-COMMAND
// (which writes a line $TAIL_MARKER), checks Pause holds a new line back and
// Resume shows it, then saves the window as PNG.  Needs a DISPLAY.
//
//   tailwindow MACHINE USER FILE PNG APPEND-COMMAND
//
// USER "-" means none.  FILE must already hold a line TAILLINE, or the
// text in $TAIL_WAIT.
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#include <unistd.h>

#include <wx/wx.h>

#include "wxgui/WxTailWindow.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    // An assert dialog would block a headless run forever.
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

static int failures = 0;

static void check(const char* name, bool ok, const std::string& detail = "")
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", name);
  if (!ok && !detail.empty()) printf("     [%s]\n", detail.c_str());
  if (!ok) failures++;
  fflush(stdout);
}

static void pump(int ms)
{
  for (int i = 0; i < ms / 50; i++) {
    wxTheApp->Yield(true);
    wxMilliSleep(50);
  }
}

static bool waitFor(WxTailWindow* w, const std::string& want, int secs)
{
  time_t end = time(0) + secs;
  while (time(0) < end) {
    if (w->text().find(want) != std::string::npos) return true;
    pump(100);
  }
  return w->text().find(want) != std::string::npos;
}

static bool append(const std::string& cmd, const std::string& marker)
{
  setenv("TAIL_MARKER", marker.c_str(), 1);
  return system(cmd.c_str()) == 0;
}

static void press(wxWindow* top, const char* label)
{
  wxWindow* b = wxWindow::FindWindowByLabel(label, top);
  if (!b) return;
  wxCommandEvent ev(wxEVT_BUTTON, b->GetId());
  ev.SetEventObject(b);
  b->GetEventHandler()->ProcessEvent(ev);
}

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc != 6) return 77;
  wxInitAllImageHandlers();
  const std::string machine = argv[1], file = argv[3], png = argv[4],
                    cmd = argv[5];
  const std::string user = std::string(argv[2]) == "-" ? "" : argv[2];

  std::string error;
  WxTailWindow* w = WxTailWindow::open(0, "h2o-scf", machine, "", user, file,
                                       error);
  check("the window opened", w != 0, error);
  if (!w) return 1;
  w->SetSize(wxSize(760, 420));
  printf("backend %s\n", w->backend().c_str());

  const char* wait = getenv("TAIL_WAIT");
  check("the file's lines are shown",
        waitFor(w, wait && *wait ? wait : "TAILLINE\n", 30), w->text());
  const std::string pid = std::to_string(getpid());
  check("an appended line is shown",
        append(cmd, "NEWLINE-" + pid) && waitFor(w, "NEWLINE-" + pid, 30),
        w->text());

  press(w, "Pause");
  append(cmd, "PAUSED-" + pid);
  pump(2000);
  check("Pause holds new lines back",
        w->text().find("PAUSED-" + pid) == std::string::npos, w->text());
  press(w, "Resume");
  check("Resume shows them", waitFor(w, "PAUSED-" + pid, 10), w->text());

  check("the PNG was saved", w->snapshot(png));
  printf("lines %d\n", w->lineCount());
  w->Close();
  pump(500);
  printf("%s\n", failures ? "FAILED" : "PASSED");
  fflush(stdout);
  // wx's own static teardown at exit is not what is under test.
  _exit(failures ? 1 : 0);
}
