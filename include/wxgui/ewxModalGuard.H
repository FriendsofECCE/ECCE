#ifndef _EWXMODALGUARD_H
#define _EWXMODALGUARD_H

// Applied to every modal dialog of the process (wxModalDialogHook), so no
// call site has to remember it:
//  - a busy cursor that is running is shown as the normal cursor while the
//    dialog waits for the user, and comes back afterwards;
//  - on GTK the dialog is presented once it is up, with a fresh X server
//    timestamp (ewxRaiseWindow): Mutter maps a window it was not asked to
//    activate with a valid timestamp without keyboard focus (#120).
// ECCE_DEBUG_MODAL=<file> appends what each dialog met and received (busy
// cursor, parent state, focus, keys, scrolls, repaints) to that file.
namespace ewxModalGuard {
  // Called once from ewxApp::OnInit.
  void install();
}

#endif
