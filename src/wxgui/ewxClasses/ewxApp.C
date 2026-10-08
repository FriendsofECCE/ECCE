#include <locale.h>
#include <iostream>
  using namespace std;
#include <cstring>

#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/image.h"
#include "wx/aboutdlg.h"
#include <fstream>
#include <sstream>

#include "util/Ecce.H"
#include "util/Preferences.H"
#include "util/Color.H"

#include "dsm/DavDebug.H"

#include "tdat/TaskApp.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxApp.H"

#ifdef __WXGTK__
#include <glib.h>
#ifdef __WXGTK3__
extern "C" {
  #include <gtk/gtk.h>
}
#endif

namespace {

// Every widget added to a wxPizza container (wx's internal GTK3 fixed-
// position container, src/gtk/win_gtk.cpp) gets one real
// gtk_widget_size_allocate() call at a hardcoded 1x1 pixels the instant
// it's added -- pizza_add() there does put(widget, 0, 0, 1, 1)
// unconditionally, before wx's own sizer Layout() has ever run to give it
// a real size. Confirmed directly against wxWidgets 3.2.8's own shipped
// source (matches the libwxgtk3.2 3.2.8+dfsg-2 package installed here).
// Most widgets silently tolerate this transient nonsense allocation, but
// GtkEntry (wxSpinCtrl's native widget included) and GtkNotebook's tab
// header both run an internal CSS-gadget consistency check that emits a
// "Negative content width/height" warning when that 1x1 allocation goes
// negative after subtracting the current theme's own border/padding.
// This is a wxWidgets/GTK3 backend structural quirk, not an ECCE bug --
// not something any application-level size hint can avoid, since it
// happens during widget construction, structurally before any sizer
// Layout() pass -- and not patchable without shipping our own patched
// wxWidgets build, exactly the vendoring this project moved away from.
// Filtered here instead of left as console spam on every single launch;
// every other GLib/GTK log message still goes through the default writer
// completely unfiltered, so a real, new warning is never hidden by this.
GLogWriterOutput filterKnownBenignGtkPizzaWarnings(GLogLevelFlags logLevel,
                                                    const GLogField* fields,
                                                    gsize nFields,
                                                    gpointer userData)
{
  const char* domain = nullptr;
  const char* message = nullptr;
  for (gsize i = 0; i < nFields; ++i) {
    if (strcmp(fields[i].key, "GLIB_DOMAIN") == 0)
      domain = static_cast<const char*>(fields[i].value);
    else if (strcmp(fields[i].key, "MESSAGE") == 0)
      message = static_cast<const char*>(fields[i].value);
  }

  if (domain && strcmp(domain, "Gtk") == 0 && message &&
      (strstr(message, "Negative content width") ||
       strstr(message, "Negative content height"))) {
    return G_LOG_WRITER_HANDLED;
  }

  return g_log_writer_default(logLevel, fields, nFields, userData);
}

#ifdef __WXGTK3__
// ECCE_TEST_BACKDROP=1: a headless display has no window manager, so no
// window ever loses focus; this holds every top-level window of the process
// in the :backdrop state, for the screenshots in tests/look.
class BackdropHold : public wxTimer
{
  public:
    virtual void Notify()
    {
      for (wxWindowList::iterator it = wxTopLevelWindows.begin();
           it != wxTopLevelWindows.end(); ++it)
        if ((*it)->GetHandle())
          gtk_widget_set_state_flags((*it)->GetHandle(), GTK_STATE_FLAG_BACKDROP, FALSE);
    }
};
#endif

}  // namespace
#endif  // __WXGTK__


/**
 * Text keeps its normal colour when the window is unfocused; disabled
 * text keeps looking disabled.  Adwaita fades every label in :backdrop to
 * a mix of text and background, which makes any ECCE window that is not
 * the focused one look inactive, and ECCE runs several side by side.
 */
void ewxApp::applyBackdropStyle()
{
#ifdef __WXGTK3__
   if (getenv("ECCE_TEST_BACKDROP")) (new BackdropHold)->Start(200);
   const char *mode = getenv("ECCE_BACKDROP");
   if (mode && strcmp(mode, "theme") == 0) return;
   GdkScreen *screen = gdk_screen_get_default();
   if (!screen) return;

   // The theme's own named colours; a theme without them rejects the
   // sheet, and the colours wx reads from the same theme stand in.
   const char *named =
      "label:backdrop, entry:backdrop, .view:backdrop,\n"
      "treeview.view:backdrop { color: @theme_fg_color; }\n"
      "entry:backdrop, .view:backdrop { color: @theme_text_color; }\n"
      "label:backdrop:disabled, entry:backdrop:disabled,\n"
      "treeview.view:backdrop:disabled { color: @insensitive_fg_color; }\n";
   GtkCssProvider *provider = gtk_css_provider_new();
   GError *error = NULL;
   if (!gtk_css_provider_load_from_data(provider, named, -1, &error)) {
      g_clear_error(&error);
      wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
      wxColour dim = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
      wxString css;
      css.Printf("label:backdrop, entry:backdrop, .view:backdrop { color: %s; }\n"
                 "label:backdrop:disabled, entry:backdrop:disabled,\n"
                 ".view:backdrop:disabled { color: %s; }\n",
                 fg.GetAsString(wxC2S_HTML_SYNTAX), dim.GetAsString(wxC2S_HTML_SYNTAX));
      gtk_css_provider_load_from_data(provider, css.utf8_str(), -1, NULL);
   }
   gtk_style_context_add_provider_for_screen(screen,
      GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   g_object_unref(provider);
#endif
}


/**
 * Destructor.
 */
ewxApp::~ewxApp()
{
}


/**
 * Constructor.
 * Sets vendor and app names associated with how applications may
 * be grouped by window managers.
 * Performs other common initializations.
 */
bool ewxApp::OnInit()
{
#ifdef __WXGTK__
   g_log_set_writer_func(filterKnownBenignGtkPizzaWarnings, nullptr, nullptr);
#endif

   applyBackdropStyle();

   wxInitAllImageHandlers();

   SetVendorName("EMSL");
   SetAppName("ECCE");
   SetUseBestVisual(true);

   Ecce::initialize();

   Bind(wxEVT_MENU, &ewxApp::OnAbout, this, wxID_ABOUT);

   Color::initialize();

   // The toolkit's own init may have re-read the locale from the environment.
   setlocale(LC_NUMERIC, "C");

   DavDebug::setDebugContext(("/tmp/dav" + getName()).c_str());

   return true;
}

/**
 * Load global preferences and update accordingly.
 * Implementation delegated to ewxWindowUtils method of same name.
 * This method will
 *   <li>find ID_FEEDBACK  and set beep preferences</li>
 *   <li>call setUnitFamily for TaskApp implementations.  This works only
 *   if the top frame can be cast to a TaskApp</li>
 */
void ewxApp::processGlobalPreferenceChange()
{
   wxWindow *top = GetTopWindow();
   ewxWindowUtils::processGlobalPreferenceChange(top);

}


void ewxApp::OnAbout(wxCommandEvent& WXUNUSED(event))
{
   wxAboutDialogInfo info;
   info.SetName("ECCE");
   info.SetVersion(Ecce::ecceVersion());
   info.SetDescription(_("Extensible Computational Chemistry Environment\n\n"
      "Originally developed at the Environmental Molecular Sciences "
      "Laboratory (EMSL), Pacific Northwest National Laboratory, operated "
      "for the U.S. Department of Energy by Battelle. Neither Pacific "
      "Northwest National Laboratory, Battelle Memorial Institute nor the "
      "U.S. Department of Energy is responsible for, or endorses, the "
      "modifications made since 2012."));
   info.SetCopyright(wxString::FromUTF8(
      "Copyright \xC2\xA9 1994-2012 Pacific Northwest National Laboratory, "
      "Battelle Memorial Institute.\n"
      "Copyright \xC2\xA9 2017-2026 CA Ohlin."));
   info.SetWebSite("https://github.com/FriendsofECCE/ECCE");
   info.AddDeveloper("CA Ohlin");
   info.AddDeveloper("Matthew Asplund");
   info.AddArtist(_("Icons: Lucide contributors (ISC licence), "
                    "https://lucide.dev"));
   info.AddDeveloper(_("The original ECCE team at EMSL, Pacific Northwest "
                       "National Laboratory"));

   // Full text when the install carries it; the notice alone otherwise.
   std::string path = std::string(Ecce::ecceHome()) + "/LICENSE";
   std::ifstream in(path.c_str());
   std::ostringstream text;
   if (in) text << in.rdbuf();
   if (in && !text.str().empty()) {
      info.SetLicence(wxString::FromUTF8(text.str().c_str()));
   } else {
      info.SetLicence(wxString::FromUTF8(
         "Licensed under the Educational Community License, Version 2.0 "
         "\xE2\x80\x94 http://opensource.org/licenses/ecl2.php"));
   }

   // The app's own tool icon; without one GTK draws a broken-image glyph.
   wxTopLevelWindow *top = wxDynamicCast(GetTopWindow(), wxTopLevelWindow);
   if (top && top->GetIcon().IsOk()) info.SetIcon(top->GetIcon());

   wxAboutBox(info, GetTopWindow());
}
