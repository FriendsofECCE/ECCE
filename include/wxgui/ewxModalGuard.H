#ifndef _EWXMODALGUARD_H
#define _EWXMODALGUARD_H

class wxWindow;

// Applied to every modal dialog of the process (wxModalDialogHook), so no
// call site has to remember it:
//  - a busy cursor that is running is shown as the normal cursor while the
//    dialog waits for the user, and comes back afterwards;
//  - on GTK the dialog is presented once it is up, with a fresh X server
//    timestamp (ewxRaiseWindow): Mutter maps a window it was not asked to
//    activate with a valid timestamp without keyboard focus (#120).
// On XWayland, install() also turns off GTK's frame sync for every top-level
// window of the process (modal or not): GTK otherwise stops painting a
// window until Mutter reports a frame drawn, which it sometimes never does.
// ECCE_DEBUG_MODAL=<file> appends what each dialog met and received (busy
// cursor, parent state, focus, keys, scrolls, repaints, frame sync and the
// compositor's frame-drawn replies) to that file.
namespace ewxModalGuard {
  // Called once from ewxApp::OnInit.
  void install();
  // The top-level window's GDK frame counter (frames GTK has painted), -1
  // when there is none; for tests.
  long long frameCounter(wxWindow *win);
}

#endif
