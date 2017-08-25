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


/* void WxBasisSetDetail::setContext(const char *name,
                                   const char *info,
                                   const char *reference,
                                   const char *image_path)
{

    bool hasName = (name != "");

    XtSetSensitive(getvpane(),hasName);
    XtSetSensitive(getbsDescription(),hasName);
    XtSetSensitive(getexportBtn(),hasName);

    getbsDescriptioni()->settextValue((char*)info);
    getbsReferencei()->settextValue((char*)reference);

    display(image_path);


 }

void WxBasisSetDetail::graphPanelPaintCB(wxPaintEvent& event)
{
    wxPaintDC dc(p_graphPanel);

    p_graphPanel->PrepareDC(dc);
    PrepareDC(dc);

    dc.BeginDrawing();

    dc.SetBackground(ewxStyledWindow::getWindowColor());
    dc.Clear();

    const wxBitmap bmp(p_graphImage);

    if (p_graphImage != NULL)
    {
        dc.DrawBitmap(bmp, 0, 0, false);
    }

    dc.EndDrawing();

    event.Skip();


}
 */


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



   /*
   Pixmap pixmap = (Pixmap) 0;

   if (image != "")
   {
        SFile imgFile(image);
        string fullFileName = imgFile.path(True);
        int pos = fullFileName.find("gif");

        if (pos != string::npos)
            fullFileName.replace(pos,3,"xpm");

        imgFile = fullFileName;
        Dimension width,height;
        pixmapSize(fullFileName,width,height);
        adjustSize(width,height);

        if (imgFile.exists())
        {
            int ret;
            pixmap = (Pixmap)tu_convert(getWxBasisSetDetail(), (char*)fullFileName.c_str(),
                                        XtRPixmap, &ret);
     }
   }

   if (pixmap != (Pixmap)0)
   {
        XtSetSensitive(getgraphForm(),True);
        XtSetSensitive(getgraphTabBtn(),True);
        setgraphLabelPixmap(pixmap);
        XtMapWidget(getxpmScroll());
        XtUnmapWidget(getxpmNABtnBox());
    }
    else
    {
        XtSetSensitive(getgraphForm(),False);
        XtSetSensitive(getgraphTabBtn(),False);
        XtUnmapWidget(getxpmScroll());
        XtMapWidget(getxpmNABtnBox());
   }
*/
//    DebugLeaveMethod();
 //}


void WxBasisSetDetail::closeButtonClickCB( wxCommandEvent& event )
{
//    DebugEnterMethod("closeButtonClickCB(wxCommandEventv&)");
    this->Close();
//    DebugLeaveMethod();
}



/*
 WxBasisSetDetail::WxBasisSetDetail(char* name, TuTemplate* parent)
       : Tu_WxBasisSetDetail_Template(name, parent)
 {
   p_topTab = (Widget)0;
   p_exportWin = (ExportWin*)0;
 }
 
 WxBasisSetDetail::~WxBasisSetDetail()
 {
   tu_busy_remove_widget(getWxBasisSetDetail());
   if (p_exportWin != (ExportWin*)0) delete p_exportWin;
 }
 
 void WxBasisSetDetail::createCB(Widget w, XtPointer, XtPointer)
 {
   tuUtil::registerWMCloseWindowCallback(w,WxBasisSetDetail::wmCloseCB,(XtPointer)this);
   tu_busy_add_widget(getWxBasisSetDetail());
   p_topTab = gettextForm();
   // 550 seems like a good height for the window ...
   XtVaSetValues(getoutForm(),XmNheight,550,NULL);
 }
 
 // Description
 //   Ties the window manager close callback to the close button.
 void WxBasisSetDetail::wmCloseCB(Widget w, XtPointer thisPtr, XtPointer callData)
 {
   // invoke the desired callback function for a WM close
   ((WxBasisSetDetail *) thisPtr)->closeCB(w, (XtPointer) 0, callData);
 }
 
 // Description
 //   Callback from ExportWin class.  The string represented by file is
 //   assumed to be a valid, writable file at this point.  The
 //   description is dumped out.
 Boolean WxBasisSetDetail::saveToFile(const char* file)
 {
   string label;  // we don't use this
   ofstream os(file);
   os <<  getbsReferencei()->gettextValue();
   os << "\n\n";
   os << getbsDescriptioni()->gettextValue();
   os << endl;
   os.close();
   return True;
 }
 

 void WxBasisSetDetail::tabSelectCB(Widget, XtPointer, XtPointer pcbs)
 {
    XrtGearPageCallbackStruct *cbs = (XrtGearPageCallbackStruct*)pcbs;
    p_topTab = cbs->page_widget;
 }


 void WxBasisSetDetail::exportCB(Widget, XtPointer, XtPointer)
 {
    if (p_exportWin == (ExportWin*)0)
    {
        p_exportWin = new ExportWin("ExportWin",this);
        p_exportWin->registerSaveCallback(this,
                               (mptrExportWin)&WxBasisSetDetail::saveToFile);
    }

    p_exportWin->show();
 }
 

 void WxBasisSetDetail::closeCB(Widget, XtPointer, XtPointer)
 {
    popdown();
 }

 

 // Description
 //   I found that within the tab, if I didn't unmanage first, the resize
 //   didn't work.
 void WxBasisSetDetail::adjustSize(Dimension width, Dimension height)
 {
   static Boolean firstTime = True;

   if (firstTime == True)
    {
        XtUnmanageChild(getgraphForm());
    //    XtVaSetValues(getgraphForm(),XmNwidth,width,height+40,NULL);
        XtVaSetValues(getgraphForm(),XmNheight,height+40,NULL);
        XtManageChild(getgraphForm());
        firstTime = False;
    }
 }
*/
