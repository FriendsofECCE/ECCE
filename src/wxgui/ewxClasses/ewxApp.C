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
#include "wx/statbmp.h"
#include "wx/hyperlink.h"
#include "wx/collpane.h"
#include "wx/generic/aboutdlgg.h"
#include <fstream>
#include <sstream>

#include "util/Ecce.H"
#include "util/GLPath.H"
#include "util/Preferences.H"
#include "util/Color.H"

#include "dsm/DavDebug.H"

#include "tdat/TaskApp.H"

#include "wxgui/ewxWindowUtils.H"
#include <wx/filefn.h>
#include "wxgui/ewxApp.H"
#include "wxgui/ewxModalGuard.H"
#include "wxgui/ewxReportProblem.H"
#include "wxgui/ewxBitmap.H"

#ifdef __WXMAC__
#include <CoreFoundation/CoreFoundation.h>
#include <objc/message.h>
#include <objc/runtime.h>

namespace {

// A tool the gateway (a background UIElement app) starts is not made the
// active app by macOS, so its window opened behind others or minimised.
// NSApplication is reached through the runtime: no Objective-C source.
void activateThisApp()
{
  typedef id (*Msg0)(id, SEL);
  typedef void (*MsgB)(id, SEL, BOOL);
  id app = ((Msg0)objc_msgSend)((id)objc_getClass("NSApplication"),
                                sel_registerName("sharedApplication"));
  if (app)
    ((MsgB)objc_msgSend)(app, sel_registerName("activateIgnoringOtherApps:"), YES);
}

// On the first idle with a shown top-level window: un-minimise, raise, activate.
void bringFirstWindowForward(wxIdleEvent& ev)
{
  static bool done = false;
  ev.Skip();
  if (done) return;
  for (wxWindowList::iterator it = wxTopLevelWindows.begin();
       it != wxTopLevelWindows.end(); ++it) {
    wxTopLevelWindow *w = wxDynamicCast(*it, wxTopLevelWindow);
    if (!w || !w->IsShown()) continue;
    done = true;
    if (w->IsIconized()) w->Iconize(false);
    w->Raise();
    activateThisApp();
    return;
  }
}

// The bundle's name for the app menu ("Quit ECCE Builder").
wxString bundleDisplayName()
{
  CFBundleRef bundle = CFBundleGetMainBundle();
  CFTypeRef v = bundle ? CFBundleGetValueForInfoDictionaryKey(
                           bundle, CFSTR("CFBundleDisplayName")) : NULL;
  char buf[256];
  if (v && CFGetTypeID(v) == CFStringGetTypeID() &&
      CFStringGetCString((CFStringRef)v, buf, sizeof(buf), kCFStringEncodingUTF8))
    return wxString::FromUTF8(buf);
  return wxString();
}

}  // namespace
#endif

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
   ewxModalGuard::install();

   wxInitAllImageHandlers();

   SetVendorName("EMSL");
   SetAppName("ECCE");
#ifdef __WXMAC__
   {
      wxString shown = bundleDisplayName();
      if (!shown.empty()) SetAppDisplayName(shown);
   }
   Bind(wxEVT_IDLE, bringFirstWindowForward);
#endif
   SetUseBestVisual(true);

   Ecce::initialize();

   Bind(wxEVT_MENU, &ewxApp::OnAbout, this, wxID_ABOUT);
   Bind(wxEVT_MENU, &ewxApp::OnReportProblem, this, ID_REPORT_PROBLEM);
   //  Test hook: Help > About once the app is up (for screenshots).
   if (getenv("ECCE_SHOW_ABOUT")) {
      CallAfter([this]() {
         CallAfter([this]() { wxCommandEvent e; OnAbout(e); });
      });
   }

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


#ifdef __WXMAC__
void ewxApp::MacReopenApp()
{
   for (wxWindowList::iterator it = wxTopLevelWindows.begin();
        it != wxTopLevelWindows.end(); ++it) {
      wxTopLevelWindow *w = wxDynamicCast(*it, wxTopLevelWindow);
      if (!w || !w->IsShown()) continue;
      if (w->IsIconized()) w->Iconize(false);
      w->Raise();
   }
}
#endif


void ewxApp::OnReportProblem(wxCommandEvent& WXUNUSED(event))
{
   ewxReportProblem::run(GetTopWindow());
}


void ewxApp::OnAbout(wxCommandEvent& WXUNUSED(event))
{
   wxAboutDialogInfo info;
   info.SetName("ECCE");
   wxString version = Ecce::ecceVersion();
   if (*Ecce::buildCommit())
      version += wxString(" (development build ") + Ecce::buildCommit() + ")";
   info.SetVersion(version);
   wxString desc = wxString::FromUTF8("Extensible Computational Chemistry Environment: "
      "build molecules, set up and run calculations, and look at the "
      "results.\n\n"
      "ECCE was created at the Environmental Molecular Sciences Laboratory "
      "(EMSL) of Pacific Northwest National Laboratory, from 1994, as a "
      "problem-solving environment for computational chemistry: one program "
      "to build a molecule, set up a calculation, run it on a remote "
      "computer, and keep and analyse the results. A new code is added by "
      "registering it, describing how to write its input, run it and read "
      "its results, rather than by changing ECCE itself. PNNL made ECCE "
      "open source, and its last version, 7.0, appeared in 2013.\n\n"
      "Since 2017, ECCE has been maintained as the FriendsofECCE project on "
      "GitHub by C. Andr\xC3\xA9 Ohlin (Ume\xC3\xA5 University) and Matthew "
      "Asplund (Brigham Young University). In 2018 PNNL's ECCE website "
      "pointed its users to the project. Since 2026 ECCE has been rebuilt "
      "for current systems (version 8), and as a client and server with "
      "native clients for Windows and macOS and a built-in quantum "
      "chemistry program (version 9).\n\n"
      "Neither Pacific Northwest National Laboratory, Battelle Memorial "
      "Institute nor the U.S. Department of Energy is responsible for, or "
      "endorses, the modifications made by the FriendsofECCE project.\n\n"
      "Report problems: Help > Report a Problem... collects the diagnostics "
      "for an issue at github.com/FriendsofECCE/ECCE.");
   const std::string gl = GLPath::describeForDisplay();
   if (!gl.empty()) desc += "\n\n" + wxString::FromUTF8(gl.c_str());
   info.SetDescription(desc);
   info.SetCopyright(wxString::FromUTF8(
      "Copyright \xC2\xA9 1994-2012 Pacific Northwest National Laboratory, "
      "Battelle Memorial Institute.\n"
      "Copyright \xC2\xA9 2017-2026 C. Andr\xC3\xA9 Ohlin."));
   info.SetWebSite("https://github.com/FriendsofECCE/ECCE");
   info.AddDeveloper(wxString::FromUTF8("C. Andr\xC3\xA9 Ohlin (maintainer)"));
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

   // ECCE's logo; without a usable icon GTK draws a broken-image glyph,
   // and a window's own icon can be a bundle entry GTK cannot draw here.
   wxBitmap logo;
#ifdef __WXGTK__
   if (wxFileExists(ewxBitmap::pixmapFile("ecce-logo-128.png")) &&
       logo.LoadFile(ewxBitmap::pixmapFile("ecce-logo-128.png"),
                     wxBITMAP_TYPE_PNG) && logo.IsOk()) {
      wxIcon icon;
      icon.CopyFromBitmap(logo);
      info.SetIcon(icon);
   } else
#endif
   {
      wxTopLevelWindow *top = wxDynamicCast(GetTopWindow(), wxTopLevelWindow);
      if (top && top->GetIcon().IsOk()) info.SetIcon(top->GetIcon());
   }

   showAbout(info, GetTopWindow());
}


//  Our own About window: GTK's and wx's generic box centre every block on
//  its own, so the copyright, text and folds started at different places.
//  Here everything beside the logo shares one left edge.
void ewxApp::showAbout(const wxAboutDialogInfo& info, wxWindow *parent)
{
   wxDialog dlg(parent, wxID_ANY, "About " + info.GetName());
   wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
   if (info.GetIcon().IsOk()) {
      wxBitmap b;
      b.CopyFromIcon(info.GetIcon());
      row->Add(new wxStaticBitmap(&dlg, wxID_ANY, b), 0, wxALL | wxALIGN_TOP, 12);
   }
   wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
   const int width = 560;
   wxStaticText *title = new wxStaticText(&dlg, wxID_ANY,
                                          info.GetName() + " " + info.GetVersion());
   title->SetFont(title->GetFont().Bold().Larger());
   col->Add(title, 0, wxBOTTOM, 8);
   wxStaticText *desc = new wxStaticText(&dlg, wxID_ANY, info.GetDescription());
   desc->Wrap(width);
   col->Add(desc, 0, wxBOTTOM, 8);
   wxStaticText *copy = new wxStaticText(&dlg, wxID_ANY, info.GetCopyright());
   copy->SetFont(copy->GetFont().Smaller());
   copy->Wrap(width);
   col->Add(copy, 0, wxBOTTOM, 6);
   if (info.HasWebSite())
      col->Add(new wxHyperlinkCtrl(&dlg, wxID_ANY, info.GetWebSiteURL(),
                                   info.GetWebSiteURL()), 0, wxBOTTOM, 6);
   auto fold = [&](const wxString& label, const wxString& text, bool scroll) {
      if (text.empty()) return;
      wxCollapsiblePane *cp = new wxCollapsiblePane(&dlg, wxID_ANY, label);
      wxWindow *pane = cp->GetPane();
      wxBoxSizer *ps = new wxBoxSizer(wxVERTICAL);
      if (scroll) {
         wxTextCtrl *t = new wxTextCtrl(pane, wxID_ANY, text, wxDefaultPosition,
                                        wxSize(width, 220),
                                        wxTE_MULTILINE | wxTE_READONLY);
         ps->Add(t, 1, wxEXPAND);
      } else {
         wxStaticText *t = new wxStaticText(pane, wxID_ANY, text);
         t->Wrap(width);
         ps->Add(t, 0);
      }
      pane->SetSizer(ps);
      cp->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED,
               [&dlg](wxCollapsiblePaneEvent&) { dlg.Fit(); });
      col->Add(cp, 0, wxEXPAND);
   };
   fold(_("License"), info.GetLicence(), true);
   fold(_("Developers"), wxJoin(info.GetDevelopers(), '\n', '\0'), false);
   fold(_("Artists"), wxJoin(info.GetArtists(), '\n', '\0'), false);
   row->Add(col, 1, wxTOP | wxRIGHT | wxBOTTOM, 12);
   wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
   top->Add(row, 1, wxEXPAND);
   top->Add(dlg.CreateStdDialogButtonSizer(wxOK), 0, wxEXPAND | wxALL, 10);
   dlg.SetSizerAndFit(top);
   dlg.CentreOnParent();
   dlg.ShowModal();
}
