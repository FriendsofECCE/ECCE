// savedDefaultStyle(): the one-time move from the old default (#227).
#include <cstdio>
#include <wx/fileconf.h>
#include <wx/wfstream.h>
#include "wxviz/DefaultStyle.H"

static int bad = 0;
static void check(const char *what, bool ok)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) bad++;
}

// A config file holding `style` (or nothing), as a fresh process sees it.
static wxFileConfig *open(const wxString& path) { return new wxFileConfig("t", "t", path, "", wxCONFIG_USE_LOCAL_FILE); }

int main(int, char **argv)
{
  wxString dir = argv[1];
  wxString a = dir + "/a.ini", b = dir + "/b.ini", c = dir + "/c.ini";

  // Old default, never migrated: becomes Ball And Stick, flag written.
  { wxFileConfig *f = open(a); f->Write("DefaultStyle", "Ball And Wireframe"); f->Flush(); delete f; }
  { wxFileConfig *f = open(a);
    check("old default migrated", savedDefaultStyle(f) == "Ball And Stick"); delete f; }
  { wxFileConfig *f = open(a);
    check("flag written", f->HasEntry("DefaultStyleMigrated"));
    // The user now chooses Ball And Wireframe and quits.
    f->Write("DefaultStyle", "Ball And Wireframe"); f->Flush(); delete f; }
  { wxFileConfig *f = open(a);
    check("later choice of Ball And Wireframe kept", savedDefaultStyle(f) == "Ball And Wireframe"); delete f; }

  // Another saved style is left alone, and the flag is still written.
  { wxFileConfig *f = open(b); f->Write("DefaultStyle", "CPK"); f->Flush(); delete f; }
  { wxFileConfig *f = open(b);
    check("other style untouched", savedDefaultStyle(f) == "CPK");
    check("flag written for other style", f->HasEntry("DefaultStyleMigrated")); delete f; }

  // Nothing saved: the new default.
  { wxFileConfig *f = open(c);
    check("no saved style gives Ball And Stick", savedDefaultStyle(f) == "Ball And Stick"); delete f; }
  return bad ? 1 : 0;
}
