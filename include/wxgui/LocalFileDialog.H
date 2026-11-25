/**
 * @file
 *
 * The platform's own file chooser (GTK, Windows, macOS) for files on this
 * computer (#342): bookmarks, recent files and search, which ECCE's
 * composite dialog lacks. That dialog stays for browsing a remote machine.
 */
#ifndef LOCALFILEDIALOG_HH
#define LOCALFILEDIALOG_HH

#include <wx/string.h>

class wxWindow;

/**
 * Shows wxFileDialog with <wildcard> (wx's "label|*.a;*.b|..." form),
 * starting in the folder last used under <key>, else the home folder.
 * False on Cancel. <filterIndex>, if given, receives the chosen filter.
 * With ECCE_TEST_LOCAL_FILE set, returns that path without a dialog (a
 * native dialog cannot be driven by a test).
 */
bool ecceLocalFile(wxWindow *parent, const wxString& title,
                   const wxString& wildcard, long style, const wxString& key,
                   wxString& path, int *filterIndex = 0);

#endif
