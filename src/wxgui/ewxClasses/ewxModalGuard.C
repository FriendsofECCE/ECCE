#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/modalhook.h"
#include "wx/weakref.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

#ifdef __WXGTK3__
extern "C" {
  #include <gtk/gtk.h>
}
#endif

#include "wxgui/ewxModalGuard.H"
#include "wxgui/ewxWindowUtils.H"

namespace {

// ECCE_DEBUG_MODAL=<file>: one line per event, appended, so the detached
// apps of a session can share one log.
void note(const char *fmt, ...) WX_ATTRIBUTE_PRINTF(1, 2);
void note(const char *fmt, ...)
{
  static const char *where = getenv("ECCE_DEBUG_MODAL");
  if (!where || !*where) return;
  FILE *log = fopen(where, "a");
  if (!log) return;
  fprintf(log, "pid=%lu ", (unsigned long)wxGetProcessId());
  va_list args;
  va_start(args, fmt);
  vfprintf(log, fmt, args);
  va_end(args);
  fputc('\n', log);
  fclose(log);
}

bool logging()
{
  const char *where = getenv("ECCE_DEBUG_MODAL");
  return where && *where;
}

std::string titleOf(wxDialog *dlg)
{
  std::string t = (const char *)dlg->GetTitle().utf8_str();
  return t.empty() ? std::string("(untitled)") : t;
}


struct Shown {
  int depth = 0;                       // wxFileDialog enters twice
  int busy = 0;                        // busy cursors ended for it
  long draws = 0, keys = 0, scrolls = 0, buttons = 0;
#ifdef __WXGTK3__
  gulong drawHandler = 0;
#endif
};

std::map<wxDialog*, Shown> g_shown;


#ifdef __WXGTK3__
std::string gtkState(wxDialog *dlg)
{
  GtkWidget *w = (GtkWidget*)dlg->GetHandle();
  if (!w || !GTK_IS_WINDOW(w)) return "no-gtk-window";
  GtkWindow *win = GTK_WINDOW(w);
  GtkWidget *focus = gtk_window_get_focus(win);
  GdkFrameClock *clock = gtk_widget_get_frame_clock(w);
  char buf[256];
  snprintf(buf, sizeof buf,
           "mapped=%d active=%d toplevelFocus=%d focusWidget=%s "
           "focusHas=%d frame=%lld",
           (int)gtk_widget_get_mapped(w),
           (int)gtk_window_is_active(win),
           (int)gtk_window_has_toplevel_focus(win),
           focus ? G_OBJECT_TYPE_NAME(focus) : "none",
           focus ? (int)gtk_widget_has_focus(focus) : -1,
           clock ? (long long)gdk_frame_clock_get_frame_counter(clock) : -1LL);
  return buf;
}

gboolean countDraw(GtkWidget*, cairo_t*, gpointer data)
{
  std::map<wxDialog*, Shown>::iterator it =
      g_shown.find(static_cast<wxDialog*>(data));
  if (it != g_shown.end()) it->second.draws++;
  return FALSE;
}

// Input reaching any widget of a shown modal dialog: keys are counted once,
// at the top-level window GTK hands them to first.
gboolean inputHook(GSignalInvocationHint *hint, guint n, const GValue *values,
                   gpointer)
{
  if (n < 1) return TRUE;
  GObject *obj = (GObject*)g_value_get_object(&values[0]);
  if (!obj || !GTK_IS_WIDGET(obj)) return TRUE;
  GtkWidget *widget = GTK_WIDGET(obj);
  GtkWidget *top = gtk_widget_get_toplevel(widget);
  const char *signal = g_signal_name(hint->signal_id);
  for (std::map<wxDialog*, Shown>::iterator it = g_shown.begin();
       it != g_shown.end(); ++it) {
    if ((GtkWidget*)it->first->GetHandle() != top) continue;
    Shown &s = it->second;
    if (signal[0] == 'k') {
      if (widget != top) return TRUE;
      s.keys++;
    } else if (signal[0] == 's') {
      s.scrolls++;
    } else {
      s.buttons++;
    }
    note("input '%s' %s on %s draws=%ld %s", titleOf(it->first).c_str(),
         signal, G_OBJECT_TYPE_NAME(widget), s.draws,
         gtkState(it->first).c_str());
    return TRUE;
  }
  return TRUE;
}

void installInputHooks()
{
  static bool done = false;
  if (done || !logging()) return;
  done = true;
  const char *names[] = { "key-press-event", "scroll-event",
                          "button-press-event" };
  for (const char *name : names) {
    guint id = g_signal_lookup(name, GTK_TYPE_WIDGET);
    if (id) g_signal_add_emission_hook(id, 0, inputHook, NULL, NULL);
  }
}
#endif


class ModalGuard : public wxModalDialogHook
{
  protected:
    virtual int Enter(wxDialog *dlg)
    {
      Shown &s = g_shown[dlg];
      if (s.depth++ > 0) return wxID_NONE;

      // Counted off and back on rather than wxBusyCursorSuspender, which
      // does nothing on GTK when no global cursor was set before.
      const bool busy = wxIsBusy();
      while (wxIsBusy()) {
        wxEndBusyCursor();
        s.busy++;
      }

      if (logging()) {
        wxWindow *parent = dlg->GetParent();
        int disabledTops = 0;
        for (wxWindowList::compatibility_iterator n =
               wxTopLevelWindows.GetFirst(); n; n = n->GetNext())
          if (n->GetData()->IsShown() && !n->GetData()->IsEnabled())
            disabledTops++;
        const char *im = getenv("GTK_IM_MODULE");
        std::string setting = "?";
#ifdef __WXGTK3__
        gchar *module = 0;
        g_object_get(gtk_settings_get_default(), "gtk-im-module", &module,
                     NULL);
        setting = module ? module : "";
        g_free(module);
#endif
        note("enter '%s' class=%s busy=%d parent=%s parentShown=%d "
             "parentEnabled=%d parentFrozen=%d disabledTopLevels=%d "
             "GTK_IM_MODULE=%s gtk-im-module=%s",
             titleOf(dlg).c_str(), dlg->GetClassInfo()->GetClassName(),
             (int)busy,
             parent ? (const char*)parent->GetClassInfo()->GetClassName()
                    : "none",
             parent ? (int)parent->IsShown() : -1,
             parent ? (int)parent->IsEnabled() : -1,
             parent ? (int)parent->IsFrozen() : -1,
             disabledTops, im ? im : "(unset)", setting.c_str());
      }

#ifdef __WXGTK3__
      installInputHooks();
      if (logging() && dlg->GetHandle())
        s.drawHandler = g_signal_connect(dlg->GetHandle(), "draw",
                                         G_CALLBACK(countDraw), dlg);
#endif

#ifdef __WXGTK__
      // Runs inside the dialog's own event loop, once it has been shown.
      wxWeakRef<wxDialog> ref(dlg);
      dlg->CallAfter([ref]() {
        wxDialog *d = ref.get();
        if (!d || !d->IsShown()) return;
#ifdef __WXGTK3__
        std::string before = logging() ? gtkState(d) : "";
#endif
        ewxRaiseWindow(d);
#ifdef __WXGTK3__
        if (!logging()) return;
        note("present '%s' busyNow=%d before: %s", titleOf(d).c_str(),
             (int)wxIsBusy(), before.c_str());
        // What Mutter made of it, half a second later.
        wxTimer *later = new wxTimer();
        later->Bind(wxEVT_TIMER, [ref, later](wxTimerEvent&) {
          if (wxDialog *d2 = ref.get())
            note("settled '%s' draws=%ld %s", titleOf(d2).c_str(),
                 g_shown.count(d2) ? g_shown[d2].draws : -1L,
                 gtkState(d2).c_str());
          later->Stop();
          wxTheApp->CallAfter([later]() { delete later; });
        });
        later->StartOnce(500);
#endif
      });
#endif
      return wxID_NONE;
    }

    virtual void Exit(wxDialog *dlg)
    {
      std::map<wxDialog*, Shown>::iterator it = g_shown.find(dlg);
      if (it == g_shown.end()) return;
      Shown &s = it->second;
      if (--s.depth > 0) return;
      for (int i = 0; i < s.busy; i++) wxBeginBusyCursor();
#ifdef __WXGTK3__
      if (s.drawHandler && dlg->GetHandle())
        g_signal_handler_disconnect(dlg->GetHandle(), s.drawHandler);
#endif
      note("exit '%s' code=%d keys=%ld scrolls=%ld buttons=%ld draws=%ld "
           "busyAfter=%d", titleOf(dlg).c_str(), dlg->GetReturnCode(),
           s.keys, s.scrolls, s.buttons, s.draws, (int)wxIsBusy());
      g_shown.erase(it);
    }
};

}  // namespace


void ewxModalGuard::install()
{
  static ModalGuard *guard = 0;
  if (guard) return;
  guard = new ModalGuard();
  guard->Register();
}
