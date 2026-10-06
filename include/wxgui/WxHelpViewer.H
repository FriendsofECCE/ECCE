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

    // The open window, or NULL.
    static WxHelpViewer* instance() { return p_instance; }

    // ECCE_HELP_DIR, else <ECCE_HOME>/doc/help.
    static std::string helpDir();

    // The page shown, as file[#anchor], relative to helpDir().
    std::string currentPage() const;

private:
    WxHelpViewer(const std::string& dir);
    void load(const std::string& page);
    void onBack(wxCommandEvent&);
    void onForward(wxCommandEvent&);
    void onBrowser(wxCommandEvent&);
    void onContents(wxCommandEvent&);
    void onLink(wxHtmlLinkEvent&);
    void onUpdateBack(wxUpdateUIEvent& e);
    void onUpdateForward(wxUpdateUIEvent& e);

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
