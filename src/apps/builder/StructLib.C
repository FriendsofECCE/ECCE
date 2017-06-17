/**
 * @file StructLib.C
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "StructLib.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/imaglist.h>

#include <iostream>
#include <istream>
//    using std::cout;
//    using std::endl;
#include <string>
//    using std::traits;

#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "tdat/DisplayStyle.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/Resource.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxStaticText.H"

#include "inv/ChemKit/ChemRadii.H"
#include "inv/ChemKit/ChemColor.H"
#include "inv/ChemKit/ChemDisplayPath.H"

#include "viz/AddFragmentCmd.H"

#include "wxviz/SGSelection.H"

#include "StructLib.H"


#define VIEWER_MINSIZE      150



IMPLEMENT_DYNAMIC_CLASS( StructLib, StructLibGUI )


/*!
 * StructLib constructors
 */
StructLib::StructLib( )
          : StructLibGUI()
{
}


StructLib::StructLib( wxWindow* parent, wxWindowID id,
                      const wxPoint& pos, const wxSize& size,
                      long style )
          : StructLibGUI(parent, id, pos, size, style)
{

    p_inSizeAdjust = false;

    p_listItemImages = new wxImageList(16, 16, true, 2);

    p_listItemImages->Add(ewxBitmap("folder_close.xpm", wxBITMAP_TYPE_XPM));
    p_listItemImages->Add(ewxBitmap("molecule.xpm", wxBITMAP_TYPE_XPM));

    createControls();
    this->Layout();

    //refreshBookmarks();

    p_container = new SGContainer();
    p_viewer->setSceneGraph(p_container);

    p_urlSelectionBase = "";
    p_urlSelectionFolder = "";
    p_librarySelection = NULL;
}


void StructLib::createControls()
{
    long itemID;

    itemID = ID_CHOICE_STRUCTLIB_BOOKMARKS;
    p_bookmarksChoice = (ewxChoice *)(FindWindowById(itemID));

    itemID = ID_LISTCTRL_STRUCTLIB_STRUCTURES;
    p_structuresListCtrl = (wxListCtrl *)(FindWindowById(itemID));
    p_structuresListCtrl->SetImageList(p_listItemImages, wxIMAGE_LIST_SMALL);
//    connectToolKitFW(this);

    itemID = ID_PANEL_STRUCTLIB_FRAGVIEW;
    ewxPanel *panView = (ewxPanel *)(FindWindowById(itemID));
    p_viewer = new SGViewer(panView, wxID_ANY);
    p_viewer->Show(false);

    //  p_viewer->setSceneGraph(p_sg);
    //  p_viewer->setViewing(false);
    p_viewer->setStickyMode(SGViewer::PICK_MODE);
    p_viewer->getSel()->lassoType.setValue(SGSelection::NOLASSO);

    

    wxSizer *sizer = panView->GetSizer();
    sizer->Add(p_viewer, 1, wxEXPAND|wxALL, 0);

    itemID = ID_SPLITTERWINDOW_STRUCTLIB_VIEWER;
    p_viewerSplit = (wxSplitterWindow *)(FindWindowById(itemID));
    p_viewerSplit->SetMinimumPaneSize(VIEWER_MINSIZE);

    itemID = ID_BUTTON_UP1LEVEL;
    p_upOneLevelButton = (ewxBitmapButton *)(FindWindowById(itemID));
    p_upOneLevelButton->Enable(false);

    itemID = ID_STATIC_STRUCTLIB_NULLSELECTION;
    p_nullSelectionStaticText = (ewxStaticText *)(FindWindowById(itemID));
}


/*
void Builder::setContextNoConfirm(const string& url)

   Resource * res = EDSIFactory::getResource(url);
   ChemistryTask *tmptask = dynamic_cast<ChemistryTask*>(res);

      p_task = tmptask;


      //  take a look at this NewFragmentCmd, might need to implement something like ti.
         Command *cmd = new NewFragmentCmd("Clear", p_sg);
         execute(cmd);

                  SGFragment *frag = p_sg->getFragment();
         if (p_task->getFragment(*frag)) {




OR take a look at the ADddFragment command in Builder:: openMenuitemClickCB

mvm or pdb
*/

void StructLib::windowClosedCB(wxCloseEvent& event)
{
}



//  Has been suggested that the viewer window be square when first shown.  Always square?  If so, this
//  disallows adjustment of split bar position.  Appears as though the size of the molecule within the viewer is
//  dependent on the smaller of the two dimensions of the viewer.

void StructLib::windowSizedCB(wxSizeEvent& event)
{
    int w, h;

    if (!p_inSizeAdjust)
    {
        wxSize size = this->GetSize();
        w = size.GetWidth();
        h = size.GetHeight();
        int postn;

//        cout << "\tw=" << StringConverter::toString(w) << "; h=" << StringConverter::toString(h) << endl;

        if (p_viewerSplit->GetSplitMode() == wxSPLIT_HORIZONTAL)
        {
            if (w > 1.6 * h)
            {
                postn = w - h;
                //  Need to change to vertical split.
                p_viewerSplit->SetSplitMode(wxSPLIT_VERTICAL);
                SetMinSize(wxSize(400, 250));
            }
            else
            {
                //  Not changing orientation but
                postn = h - w;
            }
        }
        else if (p_viewerSplit->GetSplitMode() == wxSPLIT_VERTICAL)
        {
            if (h > 1.6 * w)
            {
                 //  Need to change to horizontal split.
                postn = h - w;

                p_viewerSplit->SetSplitMode(wxSPLIT_HORIZONTAL);
                SetMinSize(wxSize(250, 400));
            }
            else
            {
                postn = w - h;
            }

        }
//        cout << "\tWill set position to " << StringConverter::toString(postn) << endl;
        p_viewerSplit->SetSashPosition(postn);
        p_viewerSplit->UpdateWindowUI();
    }

    this->Layout();
}

void StructLib::configure()
{
//    cout << "StructLib::configure()" << endl;

    EDSIServerCentral srvrCntrl;

    connectResource(srvrCntrl.getDefaultStructureLibrary());
    connectResource(srvrCntrl.getDefaultUserHome(), StructLib::USER);
    connectResource(srvrCntrl.getDefaultUserHome().getEcceRoot(), StructLib::SYSTEM);

    if (p_bookmarksChoice->GetCount() > 0)
    {
        p_bookmarksChoice->SetSelection(0);
        Resource *rsrc = (Resource *)(p_bookmarksChoice->GetClientData(0));
        EcceURL url = rsrc->getURL();

        p_urlSelectionBase = url;
        p_urlSelectionFolder = url;
        p_upOneLevelButton->Enable(false);
        refreshStructures();
    }
}


void StructLib::connectResource(const EcceURL& url, const ResourceTypeConstants rsrcType)
{
//    cout << "StructLib::connectResource(" << url.toString() << ", " << StringConverter::toString(rsrcType) << ")" << endl;
    Resource *rsrc = EDSIFactory::getResource(url);

//    cout << "\trsrc is NULL?  " << StringConverter::toString(rsrc == NULL) << endl;

    if (rsrc != NULL)
    {
//        cout << "\tResource type = " + StringConverter::toString(rsrcType) << endl;

        if (rsrcType == STANDARD)
        {

    // p_bookmarksChoice->Clear();

            p_urlLibraryDefault = url;

            vector<Resource *> *vctRsrcs = rsrc->getChildren();
            int i, n;
            n = vctRsrcs->size();
//            cout << "\tn=" << StringConverter::toString(n) << endl;

            for (i = 0; i < n; i++)
            {
                Resource *rsrcChld = (*vctRsrcs)[i];
                p_bookmarksChoice->Append(rsrcChld->getName(), rsrcChld);
            }
        }
        else if (rsrcType == USER)
        {
            p_bookmarksChoice->Append("(Home Folder)", rsrc);
        }
        else if (rsrcType == SYSTEM)
        {
            p_bookmarksChoice->Append("(ECCE Root Folder)", rsrc);
        }
    }
}



/*
bool StructLib::Show(bool show)
{
    cout << "StructLib::Show(" << StringConverter::toString(show) << ")" << endl;

    //EDSIServerCentral srvrCntrl;
    //EcceURL& url = srvrCntrl.getDefaultStructureLibrary();

    if ((p_urlSelectionBase == "") && (p_bookmarksChoice->GetCount() > 0))
    {
        p_bookmarksChoice->SetSelection(0);
        Resource *rsrc = (Resource *)(p_bookmarksChoice->GetClientData(0));
        EcceURL url = rsrc->getURL();

        p_urlSelectionBase = url;
        p_urlSelectionFolder = url;
        p_upOneLevelButton->Enable(false);
        refreshStructures();
    }

    wxWindow::Show(show);
}
*/


void StructLib::mainSplitterSashChangingCB(wxSplitterEvent& event)
{
/*
    wxSize size = p_viewerSplit->GetSize();
    int postn = event.GetSashPosition();
    int minPostn, maxPostn;

 //   if (w < 200)
   //     event.SetSashPosition(-1);

    minPostn = 100;

    if (p_viewerSplit->GetSplitMode() == wxSPLIT_HORIZONTAL)
        maxPostn = size.GetHeight() - minPostn;
    else
        maxPostn = size.GetWidth() - minPostn;

    if (postn < minPostn)
        event.SetSashPosition(minPostn);
    else if (postn > maxPostn)
        event.SetSashPosition(maxPostn);
*/
    event.Veto();

}



void StructLib::structuresListItemSelectedCB( wxListEvent& event)
{
//    cout << "StructLib::structuresListItemSelectedCB(wxListEvent&)" << endl;

    wxListItem item = event.GetItem();
    Resource *rsrc = (Resource *)item.GetData();
//    cout << "\ttarget=" << rsrc->getName() << endl;

    ResourceDescriptor::RESOURCETYPE rt = rsrc->getResourceType();

    if (rt == ResourceDescriptor::RT_DOCUMENT)
    {
//        this->displayResource(rsrcChld);

        string fpath = (rsrc->getURL()).getFile();
        string ftype = StringConverter::getFileNameExt(fpath);

//        cout << "\tObtaining istream" << endl;
        istream *is = rsrc->getDocument();

       // cout << "\tInitializing type" << endl;
      //  const string type = "MVM";


//        cout << "\tInstantiating SGFragment object" << endl;
        p_librarySelection = new SGFragment();

        STLUtil::toUpper(ftype);

        if (STLUtil::compareString(ftype, "MVM") == 0)
            p_librarySelection->restoreMVM(*is);
        else if (STLUtil::compareString(ftype, "PDB") == 0)
            p_librarySelection->restorePDB(*is);
        else if (STLUtil::compareString(ftype, "XYZ") == 0)
            p_librarySelection->restoreXYZ(*is);

//        cout << "\tFormula:  " << p_librarySelection->formula() << endl;


//        cout << "\tInserting fragment" << endl;
        p_container->insertFragment(p_librarySelection);

//        cout << "\tChanging the style" << endl;
        // Restore from styles preferences
        string scheme = "Element";
        string style = "Ball And Stick";
        //string style = "Stick";
        //string style = "Wireframe";
        ewxConfig *config = ewxConfig::getConfig("vizstyles.ini");
        string bsStyle = config->Read(style).c_str();

        DisplayDescriptor *dd = 0;

        if (bsStyle != "")
            dd = new DisplayDescriptor(bsStyle);
        else
            dd = new DisplayDescriptor("default", style, scheme);

        if (!dd->isValid(dd->getStyle(), dd->getColorScheme(),
                        p_librarySelection->numResidues() > 0))
        {
            dd->setColorScheme(dd->getDefaultColorScheme(dd->getStyle()));
        }

        p_container->setStyle(*dd, true, p_librarySelection);
        delete dd;

        TPerTab tpt;
        int numAtoms = tpt.numAtoms();

        for (int ele=0; ele<numAtoms; ele++)
        {
            wxColour color(tpt.colorName(ele));
            ChemColor *colors = p_container->getColorNode();

            if (colors)
            {
                colors->atomColor.set1Value(ele, color.Red() / 255.0,
                                            color.Green()/255.0,
                                            color.Blue()/255.0);
            }

            ChemRadii *radii = p_container->getRadiiNode();
            radii->atomRadii.set1Value(ele, tpt.covalentRadius(ele));

            ChemRadii *cpkradii = p_container->getCPKRadiiNode();
            cpkradii->atomRadii.set1Value(ele, tpt.vwr(ele));
        }

        p_container->fixAllNubs();
        p_librarySelection->touchNumbers();
        p_container->touchChemDisplay();

        p_viewer->Show(true);
        p_viewer->viewAll();
        p_nullSelectionStaticText->Show(false);
    }
    else
    {
        p_viewer->Show(false);
        p_nullSelectionStaticText->Show(true);

    }

    this->Layout();
}


void StructLib::structuresListItemActivatedCB(wxListEvent& event)
{
    wxListItem item = event.GetItem();
    Resource *rsrcTrgt = (Resource *)item.GetData();

    ResourceDescriptor::RESOURCETYPE rt = rsrcTrgt->getResourceType();

    if (rt == ResourceDescriptor::RT_COLLECTION)
    {
        p_urlSelectionFolder = rsrcTrgt->getURL();

        refreshStructures();
        p_upOneLevelButton->Enable(true);
    }
}


void StructLib::upOneLevelButtonClickCB( wxCommandEvent& event )
{
    p_urlSelectionFolder = p_urlSelectionFolder.getParent();
    refreshStructures();

    p_upOneLevelButton->Enable(p_urlSelectionFolder != p_urlSelectionBase);
}


void StructLib::bookmarksChoiceSelectedCB(wxCommandEvent& event)
{
    int slctnIdx = p_bookmarksChoice->GetSelection();
    Resource *rsrc = (Resource *)(p_bookmarksChoice->GetClientData(slctnIdx));
    EcceURL url = rsrc->getURL();

    if (url != p_urlSelectionBase)
    {
        p_urlSelectionBase = url;
        p_urlSelectionFolder = url;
        p_upOneLevelButton->Enable(false);
        refreshStructures();
    }
}

/**
 *  Refresh the list of structures, or folders containing structures, from which a selection can be made.
 *  This method queries the ESDI factory for the children of the current folder, and assumes that the
 *  contents of that folder are sorted in increasing alphabetical order (from a to z).
 */
void StructLib::refreshStructures()
{
    int i, n;

    ResourceDescriptor::RESOURCETYPE rt;
    vector<Resource *> *vctRsrcs = NULL;

    Resource *rsrc = EDSIFactory::getResource(p_urlSelectionFolder);

    if (rsrc != NULL)
        vctRsrcs = rsrc->getChildren();

    if (vctRsrcs != NULL)
    {
        //  Assumes that the resources returned are already in increasing
        //  alphabetical order.  Better way is to do explicit sort, either
        //  with one of std lib functions, or with the wxListCtrl's own
        //  sort method.
        p_structuresListCtrl->DeleteAllItems();

        vector<Resource *> *vctRsrcs = rsrc->getChildren();
        n = vctRsrcs->size();

        //  Appears that the wxListCtrl class provides only an insertion,
        //  method (which puts the new item at the beginning of the list),
        //  and not an append method (which would put a new item at the
        //  end of the list).  Thus, all new items put into the list
        //  appear at index = 0.  The result is that the order of the
        //  items is reversed from that of the source collection.  One
        //  way around this is to insert them in reverse order.

        for (i = n - 1; i >= 0; i--)
        {
            Resource *rsrcChld = (*vctRsrcs)[i];
            rt = rsrcChld->getResourceType();
            string fname = rsrcChld->getName();

            if (rt == ResourceDescriptor::RT_COLLECTION)
            {
                wxListItem item;

                item.SetImage(0);
                item.SetText(fname);
                item.SetData(rsrcChld);

                (void)p_structuresListCtrl->InsertItem(item);
//              int k = p_structuresListCtrl->InsertItem(item);
//              cout << "Insertion at " << StringConverter::toString(k) << endl;
            }
            else
            {
                string ftype = StringConverter::getFileNameExt(fname);
                STLUtil::toUpper(ftype);

                if ((STLUtil::compareString(ftype, "MVM") == 0)
                        || (STLUtil::compareString(ftype, "PDB") == 0)
                        || (STLUtil::compareString(ftype, "XYZ") == 0))
                {
                    wxListItem item;

                    item.SetImage(1);
                    item.SetText(StringConverter::getFileNameBase(fname));
                    item.SetData(rsrcChld);

                    (void)p_structuresListCtrl->InsertItem(item);
//                  int k = p_structuresListCtrl->InsertItem(item);
//                  cout << "Insertion at " << StringConverter::toString(k) << endl;
                }
            }
        }

        p_librarySelection = NULL;
        p_nullSelectionStaticText->Show(true);
        p_viewer->Show(false);
        Layout();
    }
}


// Description
//   Returns the currrently selected chemical system.
Fragment *StructLib::getSelectedFragment() const
{
    return p_librarySelection;
}


// Returns atom number of currently selected atom in currently selected
// FragImage object.
int StructLib::getSelectedAtom() const
{
  int ret = -1;
  
  // Get selection object
  SGSelection *sel = p_viewer->getSel();

  if (p_librarySelection != NULL)
  {
    int  length = sel->getNumDisplaysSelected();

    for (int nDisplay=0; nDisplay<length; nDisplay++)
    {
      const MFVec2i &aI = sel->getDisplayPath(nDisplay)->getAtomIndex();
      int numAtomLoops = aI.getNum();
      int atomStart, atomEnd;
      for (int atomLoop = 0; atomLoop < numAtomLoops; atomLoop++)
      {
        aI[atomLoop].getValue(atomStart, atomEnd);
        ret = atomStart;
      }
    }
    // If nothing selected and there is only one nub in the frag, return nub idx
    if (ret == -1)
    {
      vector<int> * nubs = p_librarySelection->getNubsIndex();
      if (nubs->size() == 1)
        ret = (*nubs)[0];
      delete nubs;
    }
  }
  return ret;
}
