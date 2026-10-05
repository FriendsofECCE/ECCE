#include <iostream>
    using std::cout;
    using std::endl;
#include <string>
    using std::string;
#include <set>
    using std::set;
    using std::less;
#include <vector>

#include "wx/dc.h"
#include "wx/file.h"

#include "util/ErrMsg.H"
#include "util/Color.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "dsm/TGaussianBasisSet.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"

#include "wxgui/ewxTextCtrl.H"

#include "WxBasisSetDetail.H"


/*
int WxBasisSetDetail::p_indentLevel = 0;

void WxBasisSetDetail::DebugEnterMethod(string dscpn)
{
#ifdef WXBASISSETDETAIL_DEBUG
    if (p_indentLevel > 0)
    {
        for (int j = 1; j <= (p_indentLevel - 1); j++)
            cout << "  ";

        cout << "+-";
    }

    cout << dscpn << endl;
    p_indentLevel++;
#endif
}

void WxBasisSetDetail::DebugWriteMessage(string mesg)
{
#ifdef WXBASISSETDETAIL_DEBUG
    if (p_indentLevel > 0)
        cout << "  ";

    cout <<  "\t==> " << mesg << endl;
#endif
}


void WxBasisSetDetail::DebugLeaveMethod()
{
#ifdef WXBASISSETDETAIL_DEBUG
    p_indentLevel--;
#endif
}
*/

IMPLEMENT_CLASS( WxBasisSetDetail, WxBasisSetDetailGUI )


WxBasisSetDetail::WxBasisSetDetail(      wxWindow* parent,
                                         wxWindowID id,
                                   const wxString& caption,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                         long style)

                    : WxBasisSetDetailGUI(parent, id, caption, pos, size, style)
{
//    DebugEnterMethod("WxBasisSetDetail(wxWindow*, wxWindowID, const wxString&, const wxPoint&, const wxSize&, long)");

    p_graphImage = NULL;
    p_cptnBase = "ECCE Basis Set Details";
    this->createControls();

    this->Layout();
    this->Fit();

    this->SetMinSize(wxSize(WXBASISSETDETAIL_WINDOW_MINWIDTH, WXBASISSETDETAIL_WINDOW_MINHEIGHT));
//    DebugLeaveMethod();
}


WxBasisSetDetail::~WxBasisSetDetail()
{
//    DebugEnterMethod("string gbsname, gbs_details *)");

//    DebugLeaveMethod();
}


void WxBasisSetDetail::createControls()
{
//    DebugEnterMethod("string gbsname, gbs_details *)");
    long itemID;

    itemID = ID_PANEL_WXBASISSETDETAIL_REFERENCES;

    itemID = ID_TEXTCTRL_WXBASISSETDETAIL_REFERENCES;
    p_rfrncsTextCtrl = (ewxTextCtrl *)(FindWindowById(itemID));

    itemID = ID_TEXTCTRL_WXBASISSETDETAIL_DESCRIPTION;
    p_dscpnTextCtrl = (ewxTextCtrl *)(FindWindowById(itemID));

    itemID = ID_BITMAP_WXBASISSETDETAIL_GRAPH;
    p_graphBitmap = (wxStaticBitmap *)(FindWindowById(itemID));

    itemID = ID_SCROLLEDWINDOW_WXBASISSETDETAIL_GRAPH;
    p_graphPane = (wxScrolledWindow *)(FindWindowById(itemID));

    itemID = ID_BUTTON_WXBASISSETDETAIL_CLOSE;
    p_closeButton = (ewxButton *)(FindWindowById(itemID));

    itemID = ID_BUTTON_WXBASISSETDETAIL_HELP;
    p_helpButton = (ewxButton *)(FindWindowById(itemID));

//    DebugLeaveMethod();
}

void WxBasisSetDetail::saveSettings(Preferences *prefs)
{
    const string topic = "BASISSETDETAIL";

    wxRect r = this->GetRect();
    prefs->setInt(topic + ".LEFT", r.GetLeft());
    prefs->setInt(topic + ".TOP", r.GetTop());

    wxSize size = this->GetSize();
    prefs->setInt(topic + ".WIDTH", size.GetWidth());
    prefs->setInt(topic + ".HEIGHT", size.GetHeight());

    prefs->saveFile();
}


void WxBasisSetDetail::loadSettings(Preferences *prefs)
{
    int x = WXBASISSETDETAIL_WINDOW_DFLTLEFT;
    int y = WXBASISSETDETAIL_WINDOW_DFLTTOP;
    int w = WXBASISSETDETAIL_WINDOW_DFLTWIDTH;
    int h = WXBASISSETDETAIL_WINDOW_DFLTHEIGHT;

    const string topic = "BASISSETDETAIL";
    string key;

    key = topic + ".LEFT";

    if (prefs->isDefined(key))
        prefs->getInt(key, x);

    key = topic + ".TOP";

    if (prefs->isDefined(key))
        prefs->getInt(key, y);

     key = topic + ".WIDTH";

     if (prefs->isDefined(key))
        prefs->getInt(key, w);

     key = topic + ".HEIGHT";

     if (prefs->isDefined(key))
        prefs->getInt(key, h);

     this->SetSize(x, y, w, h);
}


void WxBasisSetDetail::showDetails(string gbsname,
                                   gbs_details *details)
{
//    DebugEnterMethod("string gbsname, gbs_details *)");
    string caption = p_cptnBase;
    string fpath = "";

    if (gbsname.length() > 0)
        caption += " -- " + gbsname;

    /*
    title += (name == "") ? string("") : " (" + string(name) + ")";

        p_lastDetails->setContext(gbs_name,
                              strdup(details->info.c_str()),
                                 strdup(details->reference.c_str()),
                                 strdup(details->image_path.c_str()));
    */


    p_rfrncsTextCtrl->SetValue(details->reference);
    p_dscpnTextCtrl->SetValue(details->info);
    fpath = details->image_path;

    p_graphImage = NULL;

    bool hasImage = ((fpath.size() > 0) && wxFile::Exists(fpath.c_str()));

    if (hasImage)
    {
        p_graphImage = new wxImage(fpath);

        int w, h;

        w = p_graphImage->GetWidth();
        h = p_graphImage->GetHeight();
        const wxBitmap bmp(*p_graphImage);

        p_graphBitmap->SetBitmap(bmp);
        p_graphPane->SetVirtualSize(w, h);
    }

    p_graphPane->Show(hasImage);
    this->SetTitle(caption);
    this->Refresh();
    this->Show();

//    DebugLeaveMethod();
}



//void WxBasisSetDetail::windowSizeCB( wxSizeEvent& event )
//{
//    DebugEnterMethod("WxBasisSetDetail(wxSizeEvent&)");

//    event.Skip();
//    DebugLeaveMethod();//
//}




void WxBasisSetDetail::display(const string& fpath)
{
//    DebugEnterMethod("display(const string&)");
//    DebugWriteMessage("fpath=" + fpath);

    if (fpath.size() > 0)
    {
        p_graphImage = new wxImage(fpath);

        int w, h;

        w = p_graphImage->GetWidth();
        h = p_graphImage->GetHeight();
        const wxBitmap bmp(*p_graphImage);

        p_graphBitmap->SetBitmap(bmp);
        p_graphPane->SetVirtualSize(w, h);

        Refresh();



    }
    else
    {
    }
}





void WxBasisSetDetail::closeButtonClickCB( wxCommandEvent& event )
{
//    DebugEnterMethod("closeButtonClickCB(wxCommandEventv&)");
    this->Close();
//    DebugLeaveMethod();
}



