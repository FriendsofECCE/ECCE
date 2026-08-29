#include <iostream>
  using namespace std;
#include <cstring>

#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/image.h"

#include "util/Ecce.H"
#include "util/Preferences.H"
#include "util/Color.H"

#include "dsm/DavDebug.H"

#include "tdat/TaskApp.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxApp.H"

#include <glib.h>

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

}  // namespace


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
   g_log_set_writer_func(filterKnownBenignGtkPizzaWarnings, nullptr, nullptr);

   wxInitAllImageHandlers();

   SetVendorName("EMSL");
   SetAppName("ECCE");
   SetUseBestVisual(true);

   Ecce::initialize();

   Color::initialize();

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
