#ifndef WXHELPVIEWER_HH
#define WXHELPVIEWER_HH

#include <string>

#include "wx/frame.h"
#include "wx/html/htmlwin.h"
#include "wx/listbox.h"
#include "wx/weakref.h"

class wxButton;

/**
 * The help window (#219): the pages in <ECCE_HOME>/doc/help (rendered from
 * help/src at build time) in a wxHtmlWindow, with a contents list, back and
 * forward, and "Open in browser".  There is one window; show() reuses it.
 */
class WxHelpViewer : public wxFrame
{
public:
    // page: "register-machines.html" or "register-machines.html#anchor";
    // empty opens index.html.  Falls back to the browser preference if the
    // help is not installed.  Returns the window, or NULL on that fallback.
    static WxHelpViewer* show(const std::string& page = "");

    // The help for a window, by its help.urls key ("Organizer", "Launcher.menuBar",
    // ...): show(pageForKey(key)).
    static WxHelpViewer* showKey(const std::string& key);

    // The chapter for a key; the part after the first '.' is ignored.  A
    // window with no chapter of its own gets index.html.
    static std::string pageForKey(const std::string& key);

    // Test hook, called right after an app has run its own Help menu
    // handler: when ECCE_TEST_HELP=<png> is set, saves the open help window
    // to <png>, reports its page on stderr and exits.  Does nothing
    // otherwise.
    static void testSnapshot(const std::string& app);

    // Save the window contents as a PNG.
    bool snapshot(const std::string& png);

    // The open window, or NULL.
    static WxHelpViewer* instance() { return p_instance; }

    // ECCE_HELP_DIR, else <ECCE_HOME>/doc/help.
    static std::string helpDir();

    // The git tag whose help source matches a version, or main for a version
    // that is never tagged (a release candidate, "-dev", ...).
    static std::string helpRef(const std::string& version);

    // The page shown, as file[#anchor], relative to helpDir().
    std::string currentPage() const;

    // "anchor_y=.. scroll_y=.. view_h=.. view_w=..": where the anchor's
    // heading is in the document, how far the view is scrolled, the view size.
    std::string viewState() const;

private:
    WxHelpViewer(const std::string& dir);
    void load(const std::string& page);
    void scrollToAnchor();
    void onBack(wxCommandEvent&);
    void onForward(wxCommandEvent&);
    void onBrowser(wxCommandEvent&);
    void onContents(wxCommandEvent&);
    void onLink(wxHtmlLinkEvent&);
    void onUpdateBack(wxUpdateUIEvent& e);
    void onUpdateForward(wxUpdateUIEvent& e);

    bool p_scrollPending = false;
    std::string p_dir;
    wxHtmlWindow* p_html;
    wxListBox* p_toc;
    wxButton* p_back;
    wxButton* p_forward;
    wxArrayString p_tocFiles;
    wxDECLARE_EVENT_TABLE();
    static wxWeakRef<WxHelpViewer> p_instance;
};

#endif
