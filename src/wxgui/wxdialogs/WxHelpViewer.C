#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/filename.h"
#include "wx/sizer.h"
#include "wx/splitter.h"
#include "wx/textfile.h"

#include <stdlib.h>
#include <unistd.h>

#include "util/BrowserHelp.H"
#include "util/Ecce.H"
#include "wxgui/WxHelpViewer.H"

enum { ID_BACK = wxID_HIGHEST + 700, ID_FORWARD, ID_BROWSER, ID_TOC };

wxWeakRef<WxHelpViewer> WxHelpViewer::p_instance;

wxBEGIN_EVENT_TABLE(WxHelpViewer, wxFrame)
    EVT_BUTTON(ID_BACK, WxHelpViewer::onBack)
    EVT_BUTTON(ID_FORWARD, WxHelpViewer::onForward)
    EVT_BUTTON(ID_BROWSER, WxHelpViewer::onBrowser)
    EVT_LISTBOX(ID_TOC, WxHelpViewer::onContents)
    EVT_HTML_LINK_CLICKED(wxID_ANY, WxHelpViewer::onLink)
    EVT_UPDATE_UI(ID_BACK, WxHelpViewer::onUpdateBack)
    EVT_UPDATE_UI(ID_FORWARD, WxHelpViewer::onUpdateForward)
wxEND_EVENT_TABLE()


std::string WxHelpViewer::helpDir()
{
    const char* d = getenv("ECCE_HELP_DIR");
    if (d != NULL && *d != '\0')
        return d;
    return std::string(Ecce::ecceHome()) + "/doc/help";
}


WxHelpViewer* WxHelpViewer::show(const std::string& page)
{
    std::string dir = helpDir();
    if (access((dir + "/index.html").c_str(), R_OK) != 0)
    {
        //  Not installed (a build without help): the source on GitHub.
        std::string f = page.substr(0, page.find('#'));
        if (f.size() > 5 && f.compare(f.size() - 5, 5, ".html") == 0)
            f = f.substr(0, f.size() - 5) + ".md";
        else
            f = "index.md";
        BrowserHelp().showPage(
            "https://github.com/FriendsofECCE/ECCE/blob/main/help/src/" + f);
        return NULL;
    }
    if (p_instance == NULL)
        p_instance = new WxHelpViewer(dir);
    p_instance->load(page.empty() ? "index.html" : page);
    p_instance->Show();
    p_instance->Raise();
    return p_instance;
}


WxHelpViewer::WxHelpViewer(const std::string& dir)
    : wxFrame(NULL, wxID_ANY, "ECCE Help", wxDefaultPosition, wxSize(900, 700)),
      p_dir(dir)
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* bar = new wxBoxSizer(wxHORIZONTAL);
    p_back = new wxButton(panel, ID_BACK, "Back");
    p_forward = new wxButton(panel, ID_FORWARD, "Forward");
    bar->Add(p_back, 0, wxALL, 4);
    bar->Add(p_forward, 0, wxTOP | wxBOTTOM | wxRIGHT, 4);
    bar->AddStretchSpacer(1);
    bar->Add(new wxButton(panel, ID_BROWSER, "Open in browser"), 0, wxALL, 4);
    top->Add(bar, 0, wxEXPAND);

    wxSplitterWindow* split = new wxSplitterWindow(panel, wxID_ANY);
    p_toc = new wxListBox(split, ID_TOC);
    p_html = new wxHtmlWindow(split, wxID_ANY, wxDefaultPosition,
                              wxDefaultSize, wxHW_SCROLLBAR_AUTO);
    split->SplitVertically(p_toc, p_html, 220);
    split->SetMinimumPaneSize(120);
    top->Add(split, 1, wxEXPAND);
    panel->SetSizer(top);

    wxTextFile toc(wxString::FromUTF8((dir + "/toc.txt").c_str()));
    if (toc.Open(wxConvUTF8))
        for (wxString l = toc.GetFirstLine(); !toc.Eof(); l = toc.GetNextLine())
        {
            int tab = l.Find('\t');
            if (tab == wxNOT_FOUND)
                continue;
            p_tocFiles.Add(l.Left(tab));
            p_toc->Append(l.Mid(tab + 1));
        }
    CentreOnScreen();
}


std::string WxHelpViewer::currentPage() const
{
    wxString f = wxFileName(p_html->GetOpenedPage()).GetFullName();
    wxString a = p_html->GetOpenedAnchor();
    if (!a.empty())
        f += "#" + a;
    return std::string(f.utf8_str());
}


void WxHelpViewer::load(const std::string& page)
{
    p_html->LoadPage(wxString::FromUTF8((p_dir + "/" + page).c_str()));
    wxString f = wxString::FromUTF8(page.substr(0, page.find('#')).c_str());
    int i = p_tocFiles.Index(f);
    if (i != wxNOT_FOUND)
        p_toc->SetSelection(i);
    else
        p_toc->SetSelection(wxNOT_FOUND);
}


void WxHelpViewer::onBack(wxCommandEvent&)
{
    p_html->HistoryBack();
}


void WxHelpViewer::onForward(wxCommandEvent&)
{
    p_html->HistoryForward();
}


void WxHelpViewer::onBrowser(wxCommandEvent&)
{
    std::string f = p_dir + "/" + currentPage();
    BrowserHelp().showPage("file://" + f);
}


void WxHelpViewer::onContents(wxCommandEvent& e)
{
    int i = e.GetSelection();
    if (i >= 0 && i < (int)p_tocFiles.size())
        load(std::string(p_tocFiles[i].utf8_str()));
}


void WxHelpViewer::onLink(wxHtmlLinkEvent& e)
{
    wxString href = e.GetLinkInfo().GetHref();
    if (href.StartsWith("http://") || href.StartsWith("https://"))
    {
        BrowserHelp().showPage(std::string(href.utf8_str()));
        return;
    }
    e.Skip();
}


void WxHelpViewer::onUpdateBack(wxUpdateUIEvent& e)
{
    e.Enable(p_html->HistoryCanBack());
}


void WxHelpViewer::onUpdateForward(wxUpdateUIEvent& e)
{
    e.Enable(p_html->HistoryCanForward());
}
