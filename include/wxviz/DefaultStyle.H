#ifndef DEFAULTSTYLE_HH
#define DEFAULTSTYLE_HH

#include <wx/config.h>
#include <wx/string.h>

// The viewer's saved DefaultStyle.  Earlier versions wrote "Ball And
// Wireframe" on every exit whether or not the user had chosen it, so the
// first time this runs against a config that has not been migrated, that
// value becomes "Ball And Stick" (the new default, #227) and the migration
// is recorded.  A style chosen afterwards, or any other saved style, is kept.
inline wxString savedDefaultStyle(wxConfigBase *config)
{
  const wxString key("DefaultStyle"), done("DefaultStyleMigrated");
  wxString style = config->Read(key, "Ball And Stick");
  if (!config->HasEntry(done)) {
    if (style == "Ball And Wireframe") {
      style = "Ball And Stick";
      config->Write(key, style);
    }
    config->Write(done, 1L);
    config->Flush();
  }
  return style;
}

#endif
