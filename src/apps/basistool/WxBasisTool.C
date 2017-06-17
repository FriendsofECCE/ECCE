/**
 *  @file
 *  @author Ken Swanson
 *
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <istream>
 //   using std::cout;
 //   using std::endl;
#include <fstream>
    using std::ofstream;
#include <vector>
    using std::vector;
#include <string>
    using std::string;


#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/ErrMsg.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/BrowserHelp.H"

#include "tdat/TFormula.H"
#include "tdat/TPerTab.H"

#include "dsm/GBSRules.H"
#include "dsm/TCalculation.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/ResourceTool.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxMenuItem.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/PerTabPanel.H"

#include "GridSelectionBlock.H"
#include "WxBasisTool.H"
#include "WxBasisSetCoefficients.H"
#include "WxBasisSetDetail.H"
#include "WxContractionEditor.H"



/*int WxBasisTool::p_indentLevel = 0;


void WxBasisTool::DebugEnterMethod(string dscpn)
{
#ifdef WXBASISTOOL_DEBUG
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

void WxBasisTool::DebugWriteMessage(string mesg)
{
#ifdef WXBASISTOOL_DEBUG
    if (p_indentLevel > 0)
        cout << "  ";

    cout <<  "\t==> " << mesg << endl;
#endif
}


void WxBasisTool::DebugLeaveMethod()
{
#ifdef WXBASISTOOL_DEBUG
    p_indentLevel--;
#endif
}
*/


const char* WxBasisTool::p_columnLabels[] = { "Orbital", "Polarization",
                                              "Diffuse", "Rydberg", "ECP",
                                              "DFT Charge", "DFT Exchange"};

IMPLEMENT_CLASS( WxBasisTool, WxBasisToolGUI )

BEGIN_EVENT_TABLE( WxBasisTool, WxBasisToolGUI )
    EVT_BUTTON(ID_PERTAB_WXBASISTOOL_ELEMENTS,
               WxBasisTool::elementSelectionChangedCB)
END_EVENT_TABLE()


WxBasisTool::WxBasisTool(      wxWindow* parent,
                               wxWindowID id,
                         const wxString& caption,
                         const wxPoint& pos,
                         const wxSize& size,
                               long style)
        : WxBasisToolGUI(parent, id, caption, pos, size, style),
          JMSPublisher("WxBasisTool")
{
    WXBASISTOOL_BASISSET_CATEGORY_PANEL_ITEMS[0] =
                ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_0;
    WXBASISTOOL_BASISSET_CATEGORY_PANEL_ITEMS[1] =
                ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_1;
    WXBASISTOOL_BASISSET_CATEGORY_PANEL_ITEMS[2] =
                ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_2;
    WXBASISTOOL_BASISSET_CATEGORY_PANEL_ITEMS[3] =
                ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_3;

    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[0] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_00;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[1] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_01;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[2] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_02;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[3] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_03;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[4] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_04;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[5] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_05;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[6] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_06;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[7] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_07;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[8] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_08;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[9] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_09;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[10] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_10;
    WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[11] =
                ID_PANEL_WXBASISTOOL_BASISSET_TYPE_11;

    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[0] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[1] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_01;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[2] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_02;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[3] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_03;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[4] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_04;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[5] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_05;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[6] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_06;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[7] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_07;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[8] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_08;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[9] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_09;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[10] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_10;
    WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[11] =
                ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_11;

    WXBASISTOOL_POLARIZATION_RADIOBUTTON_ITEMS[0] =
                ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_0;
    WXBASISTOOL_POLARIZATION_RADIOBUTTON_ITEMS[1] =
                ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1;

    WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[0] =
                ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0;
    WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[1] =
                ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_1;
    WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[2] =
                ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_2;

    WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[0] = 
                ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0;
    WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[1] = 
                ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_1;
    WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[2] = 
                ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_2;

    WXBASISTOOL_CONTEXT_BASISSETS_PANEL_ITEMS[0] =
                ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_0;
    WXBASISTOOL_CONTEXT_BASISSETS_PANEL_ITEMS[1] =
                ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_1;
    WXBASISTOOL_CONTEXT_BASISSETS_PANEL_ITEMS[2] =
                ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_2;

    p_tcalcUndo = NULL;
    p_prefs = NULL;
    p_tcalc = NULL;
//    p_tcalc = new TCalculation();
 //   p_tcalc->gbsConfig(new TGBSConfig);
    p_gbsFactory = NULL;
    p_multSpptd = true;
    p_multAllwd = false;
    p_editAllwd = true;
    p_urlCalc = NULL;
    p_calcChgd = false;

    for (int i = 0; i <= 2; i++)
        p_gridSlctnBlock[i] = new GridSelectionBlock();

    p_typeSlctn = TGaussianBasisSet::pople;

    wxAcceleratorEntry itmAccels[1];
    itmAccels[0].Set(wxACCEL_CTRL, (int) 'H', ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT);
    wxAcceleratorTable tblAccels(1, itmAccels);
    this->SetAcceleratorTable(tblAccels);

    initControls();

    p_basisSetTypes[0] = TGaussianBasisSet::pople;
    p_basisSetTypes[1] = TGaussianBasisSet::other_segmented;
    p_basisSetTypes[2] = TGaussianBasisSet::correlation_consistent;
    p_basisSetTypes[3] = TGaussianBasisSet::other_generally_contracted;
    p_basisSetTypes[4] = TGaussianBasisSet::ECPOrbital;
    p_basisSetTypes[5] = TGaussianBasisSet::DFTOrbital;

    p_basisSetTypes[6] = TGaussianBasisSet::polarization;
    p_basisSetTypes[7] = TGaussianBasisSet::diffuse;
    p_basisSetTypes[8] = TGaussianBasisSet::rydberg;

    p_basisSetTypes[9] = TGaussianBasisSet::ecp;

    p_basisSetTypes[10] = TGaussianBasisSet::charge;
    p_basisSetTypes[11] = TGaussianBasisSet::exchange;

    for (int i = 0; i <= 5; i++)
        p_basisSetCtgys[i] = 0;

    for (int i = 6; i <= 8; i++)
        p_basisSetCtgys[i] = 1;

    p_basisSetCtgys[9] = 2;

    for (int i = 10; i <= 11; i++)
        p_basisSetCtgys[i] = 3;

    p_inCtrlUpdate = true;
    p_basisSetsNotebook->SetSelection(p_basisSetCtgys[p_typeSlctn]);

    p_cntxtOprtnsPanel->Layout();
    p_cntxtOprtnsPanel->Fit();

    this->Layout();
    this->Fit();

    p_splitPostnMin = (p_cntxtMoleculePanel->GetBestSize()).GetHeight();
    p_splitPostnMax = (p_mainSplitWindow->GetSize()).GetHeight()
                         - ((p_dividerPanel->GetSize()).GetHeight()
                             + (p_cntxtOprtnsPanel->GetBestSize()).GetHeight());

    p_mainSplitWindow->SetSashPosition(p_splitPostnMin);
    p_mainSplitWindow->SetSashGravity(0.5);

    p_inCtrlUpdate = false;

    p_cntrcEdit = NULL;     //  These instantiated only when needed; i.e., when "Show Editor" Button clicked
    p_bsetDetail = NULL;
    p_bsetCoeffs = NULL;
    p_theoryDscpn = "";

    // set desktop icon
    SetIcon(wxIcon(ewxBitmap::pixmapFile(
            ResourceDescriptor::getResourceDescriptor().getTool(BASISTOOL)
            ->getIcon()), wxBITMAP_TYPE_XPM));
}


WxBasisTool::~WxBasisTool()
{
    if (p_prefs != NULL)
    {
        this->saveSettings();
        delete p_prefs;
    }

    if (p_gbsFactory != NULL)
    {
        delete p_gbsFactory;
        p_gbsFactory = NULL;
    }

    if (p_cntrcEdit != NULL)
    {
        delete p_cntrcEdit;
        p_cntrcEdit = NULL;
    }

    if (p_bsetDetail != NULL)
    {
        delete p_bsetDetail;
        p_bsetDetail = NULL;
    }

    if (p_bsetCoeffs != NULL)
    {
        delete p_bsetCoeffs;
        p_bsetCoeffs = NULL;
    }
}



void WxBasisTool::initControls()
{
    long itemID;

    itemID = ID_SPLITTER_WXBASISTOOL_MAIN;
    p_mainSplitWindow = (wxSplitterWindow *)(FindWindowById(itemID));

    itemID = ID_NOTEBOOK_WXBASISTOOL_BASISSETS;
    p_basisSetsNotebook = (ewxNotebook *)(FindWindowById(itemID));

    for (int i = 0; i < 4; i++)
    {
        itemID = WXBASISTOOL_BASISSET_CATEGORY_PANEL_ITEMS[i];
        p_basisSetCategoryPanel[i] = (ewxPanel *)(FindWindowById(itemID));
    }

    itemID = ID_PANEL_WXBASISTOOL_DIVIDER;
    p_dividerPanel = (ewxPanel *)(FindWindowById(itemID));

    itemID = ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0;
    p_basisSetCategoryNotebook[0] = (ewxNotebook *)(FindWindowById(itemID));

    itemID = ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_1;
    p_basisSetCategoryNotebook[1] = (ewxNotebook *)(FindWindowById(itemID));

    for (int i = 0; i < 12; i++)
    {
        itemID = WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[i];
        p_basisSetTypePanel[i] = (ewxPanel *)(FindWindowById(itemID));

        itemID = WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[i];
        p_basisSetTypeListBox[i] = (ewxListBox *)(FindWindowById(itemID));
    }

//    itemID = ID_STATIC_WXBASISTOOL_CONTEXT_CODE;
//    p_contextCodeStatic = (ewxStaticText *)(FindWindowById(itemID));

    itemID = ID_BUTTON_WXBASISTOOL_SHOWEDITOR;
    p_showEditorButton = (ewxButton *)(FindWindowById(itemID));

    itemID = ID_PANEL_WXBASISTOOL_CONTEXTMOLECULE;
    p_cntxtMoleculePanel = (ewxPanel *)(FindWindowById(itemID));

    itemID = ID_PANEL_WXBASISTOOL_PERTABLE;
    p_elementsPanel = (ewxPanel *)(FindWindowById(itemID));


//    p_elementsTable = new PerTabPanel(p_moleculePanel, ID_PERTAB_WXBASISTOOL_ELEMENTS, wxDefaultPosition, wxSize(-1, 320), wxNO_BORDER );
    p_elementsTable = new PerTabPanel(p_elementsPanel, true, ID_ITEM_DEFAULT, false, false, false, ID_PERTAB_WXBASISTOOL_ELEMENTS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
    //p_elementsTable->SetMinSize(wxSize(320, 200));
    wxSizer* szrElmts = p_elementsPanel->GetSizer();
    szrElmts->Add(p_elementsTable, 1, wxGROW|wxALL, 3);
   // szrElmts->Layout();

    p_elementsPanel->Layout();
    p_elementsPanel->Fit();

    p_cntxtMoleculePanel->Layout();
    p_cntxtMoleculePanel->Fit();

    itemID = ID_FEEDBACK_WXBASISTOOL_MESSAGES;
    p_messagesFeedback = (WxFeedback *)(FindWindowById(itemID));
    p_messagesFeedback->setEditStatus(WxFeedback::EDIT);
    p_messagesFeedback->setSaveHandler(this);
    p_messagesFeedback->setDropHandler(this);

    itemID = ID_STATIC_WXBASISTOOL_CONTEXT_FORMULA;
    p_contextFormulaStatic = (ewxStaticText *)(FindWindowById(itemID));

    for (int i = 0; i < 2; i++)
    {
        itemID = WXBASISTOOL_POLARIZATION_RADIOBUTTON_ITEMS[i];
        p_polarizationRadioButton[i]
                = (ewxRadioButton *)(FindWindowById(itemID));
    }

    itemID = ID_CHECKBOX_WXBASISTOOL_OPTIMIZE;
    p_optimizeCheckBox = (ewxCheckBox *)(FindWindowById(itemID));

    itemID = ID_STATIC_WXBASISTOOL_FUNCTIONS;
    p_functionsStatic = (ewxStaticText *)(FindWindowById(itemID));

    itemID = ID_STATIC_WXBASISTOOL_PRIMATIVES;
    p_primativesStatic = (ewxStaticText *)(FindWindowById(itemID));

    itemID = ID_STATIC_WXBASISTOOL_EFFPRIMATIVES;
    p_effPrimativesStatic = (ewxStaticText *)(FindWindowById(itemID));

    itemID = ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS;
    p_contextBasisSetsNotebook = (ewxNotebook *)(FindWindowById(itemID));

    for (int i = 0; i <= 2; i++)
    {
        itemID = WXBASISTOOL_CONTEXT_BASISSETS_PANEL_ITEMS[i];
        p_contextBasisSetsPanel[i] = (ewxPanel *)(FindWindowById(itemID));

        itemID = WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[i];

        p_contextBasisSetsGrid[i] = (ewxGrid *)(FindWindowById(itemID));
        p_contextBasisSetsGrid[i]->SetReadOnly(true);

        for (int j = 0; j < 7; j++)
        {
            p_contextBasisSetsGrid[i]->SetColLabelValue(j, p_columnLabels[j]);
            p_contextBasisSetsGrid[i]->SetColMinimalAcceptableWidth(0);
        }
    }

    itemID = ID_BUTTON_WXBASISTOOL_DETAILS;
    p_detailsButton = (ewxButton *)(FindWindowById(itemID));

    itemID = ID_BUTTON_WXBASISTOOL_CONTEXT_ADD;
    p_contextAddButton = (ewxButton *)(FindWindowById(itemID));

    itemID = ID_PANEL_WXBASISTOOL_CONTEXT_OPERATIONS;
    p_cntxtOprtnsPanel = (ewxPanel *)(FindWindowById(itemID));

    wxMenuBar *mbar = GetMenuBar();

    itemID = ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION;
    p_editClearSlctnMenuItem = (ewxMenuItem *)(mbar->FindItem(itemID));

    itemID = ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL;
    p_editClearAllMenuItem = (ewxMenuItem *)(mbar->FindItem(itemID));

    itemID = wxID_SAVE;
    p_programSaveMenuItem = (ewxMenuItem *)(mbar->FindItem(itemID));
    p_programSaveMenuItem->Enable(false);

    itemID = ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS;
    p_programCoeffsMenuItem = (ewxMenuItem *)(mbar->FindItem(itemID));

    itemID = ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE;
    p_contextAllowMultipleCheckbox = (ewxCheckBox *)(FindWindowById(itemID));
    p_contextAllowMultipleCheckbox->Show(p_multSpptd);
    p_contextAllowMultipleCheckbox->SetValue(p_multAllwd);

    this->Layout();
//    this->Validate();
    this->Fit();

    this->SetMinSize(wxSize(WXBASISTOOL_WINDOW_MINWIDTH,
                            WXBASISTOOL_WINDOW_MINHEIGHT));
}


void WxBasisTool::loadSettings(string fname)
{
    p_prefs = new Preferences(fname);

    int x = WXBASISTOOL_WINDOW_DFLTLEFT;
    int y = WXBASISTOOL_WINDOW_DFLTTOP;
    int w = WXBASISTOOL_WINDOW_DFLTWIDTH;
    int h = WXBASISTOOL_WINDOW_DFLTHEIGHT;

    const string topic = "BASISTOOL";
    string key;

    key = topic + ".X";

    if (p_prefs->isDefined(key))
        p_prefs->getInt(key, x);

    key = topic + ".Y";

    if (p_prefs->isDefined(key))
        p_prefs->getInt(key, y);

     key = topic + ".WIDTH";

     if (p_prefs->isDefined(key))
        p_prefs->getInt(key, w);

     key = topic + ".HEIGHT";

     if (p_prefs->isDefined(key))
        p_prefs->getInt(key, h);

     this->SetSize(x, y, w, h);
}


void WxBasisTool::saveSettings()
{
    const string topic = "BASISTOOL";

    wxRect r = this->GetRect();
    p_prefs->setInt(topic + ".X", r.GetLeft());
    p_prefs->setInt(topic + ".Y", r.GetTop());

    wxSize size = this->GetSize();
    p_prefs->setInt(topic + ".WIDTH", size.GetWidth());
    p_prefs->setInt(topic + ".HEIGHT", size.GetHeight());

    if (p_cntrcEdit != NULL)
        p_cntrcEdit->saveSettings(p_prefs);

    if (p_bsetDetail != NULL)
        p_bsetDetail->saveSettings(p_prefs);

    if (p_bsetCoeffs != NULL)
        p_bsetCoeffs->saveSettings(p_prefs);

    p_prefs->saveFile();
}


PerTabPanel * WxBasisTool::getPeriodicTable()
{
    return p_elementsTable;
}


WxFeedback * WxBasisTool::getMessagesFeedback()
{
    return p_messagesFeedback;
}


void WxBasisTool::windowSizeCB( wxSizeEvent& event )
{
//    cout << "windowSizeCB(wxSizeEvent&)" << endl;

    if (!p_inCtrlUpdate)
    {
        p_splitPostnMax = (p_mainSplitWindow->GetSize()).GetHeight()
                             - ((p_dividerPanel->GetSize()).GetHeight()
                                 + (p_cntxtOprtnsPanel->GetBestSize()).GetHeight() + 12);
    }

    event.Skip();
}


void WxBasisTool::windowCloseCB(wxCloseEvent& event)
{
//    cout << "windowCloseCB(wxCloseEvent&)" << endl;
    bool cncld = false;

    if (event.CanVeto())
    {
        cncld = promptContextSave();
    }

    if (cncld)
        event.Veto();
    else
        this->Destroy();

}


void WxBasisTool::openLibraryAccess(EcceURL url)
{
    p_gbsFactory = new EDSIGaussianBasisSetLibrary(url);

    if ((p_typeSlctn == 10) || (p_typeSlctn == 11))
    {
        updateBasisSets(10);
        updateBasisSets(11);
    }
    else
    {
        updateBasisSets(p_typeSlctn);
    }
}

void WxBasisTool::windowSplitterSashPosChangingCB(wxSplitterEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        int postn = event.GetSashPosition();

        if (postn < p_splitPostnMin)
        {
            p_inCtrlUpdate = true;
            event.SetSashPosition(p_splitPostnMin);
            p_inCtrlUpdate = false;
        }

        if (postn > p_splitPostnMax)
        {
            p_inCtrlUpdate = true;
            event.SetSashPosition(p_splitPostnMax);
            p_inCtrlUpdate = false;
        }
    }
}


void WxBasisTool::windowSplitterSashPosChangedCB(wxSplitterEvent& event)
{
}


void WxBasisTool::windowSplitterSizeCB(wxSizeEvent& event)
{
    event.Skip();
}


void WxBasisTool::contextPolarizationRadioButtonSelectedCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        bool cncld = false;

        TGBSConfig *cnfg = p_tcalc->gbsConfig();

        if (p_polarizationRadioButton[TGaussianBasisSet::Spherical]->GetValue())
        {
            cnfg->coordsys(TGaussianBasisSet::Spherical);
        }
/*
        else

            //  The following four lines will revert the selection back to spherical
            //  Comment out, or delete, if we want to allow this selection even in
            //  light of the discrepency
            p_inCtrlUpdate = true;
            p_polarizationRadioButton[TGaussianBasisSet::Spherical]->SetValue(true);
            cncld = true;
            p_inCtrlUpdate = false;
        }
*/
        else
        {

            if (cnfg->hasSpherical())
            {
                string mesg = "Some of the basis functions were designed for use "
                      "with spherical polarization components but the "
                      "cartesian polarization components toggle is selected.";
                p_messagesFeedback->setMessage(mesg);
            }

            cnfg->coordsys(TGaussianBasisSet::Cartesian);
        }

        if (!cncld)
        {
            updateNumbers(cnfg);

            if ((p_bsetCoeffs != NULL) && (p_bsetCoeffs->IsShown()))
                p_bsetCoeffs->showDetails(p_tcalc);

            showContextChanged(true);
        }
    }
}


void WxBasisTool::contextOptimizeCheckboxClickCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
        showContextChanged(true);
}


void WxBasisTool::basisSetsNotebookPageChangingCB(wxNotebookEvent& event)
{
}


void WxBasisTool::basisSetsNotebookPageChangedCB(wxNotebookEvent& event)
{
    int ctgy;

    if (!p_inCtrlUpdate)
    {
        ctgy = p_basisSetsNotebook->GetSelection();

        if (ctgy == 0)
        {
            p_typeSlctn = p_basisSetCategoryNotebook[0]->GetSelection();
        }
        else if (ctgy == 1)
        {
            p_typeSlctn = 6 + p_basisSetCategoryNotebook[1]->GetSelection();
        }
        else if (ctgy == 2)
        {
            p_typeSlctn = 9;
        }
        else if (ctgy == 3)
        {
            p_typeSlctn = 10;
        }

        if ((p_typeSlctn == 10) || (p_typeSlctn == 11))
        {
            updateBasisSets(p_basisSetTypes[10]);
            updateBasisSets(p_basisSetTypes[11]);
        }
        else
        {
            updateBasisSets(p_basisSetTypes[p_typeSlctn]);
        }
    }

    p_detailsButton->Enable(false);
    p_contextAddButton->Enable(false);
    p_lastSlctn = "";
}


void WxBasisTool::basisSetCategoryNotebookPageChangingCB(wxNotebookEvent& event)
{
}


void WxBasisTool::basisSetCategoryNotebookPageChangedCB(wxNotebookEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        int ctgy = p_basisSetsNotebook->GetSelection();

        if (ctgy == 0)
        {
            p_typeSlctn = event.GetSelection();
        }
        else if (ctgy == 1)
        {
            p_typeSlctn = 6 + event.GetSelection();
        }
    }

    updateBasisSets(p_typeSlctn);

//    int k = event.GetSelection();

//    if (k >= 0)
//        updateBasisSets(k);

    p_detailsButton->Enable(false);
    p_contextAddButton->Enable(false);
    p_lastSlctn = "";
}


void WxBasisTool::elementSelectionChangedCB(wxCommandEvent& event)
{
//    cout << "\tp_typeSlctn=" << StringConverter::toString(p_typeSlctn) << endl;

    if ((p_typeSlctn == 10) || (p_typeSlctn == 11))
    {
        updateBasisSets(p_basisSetTypes[10]);
        updateBasisSets(p_basisSetTypes[11]);
    }
    else
    {
        updateBasisSets(p_basisSetTypes[p_typeSlctn]);
    }
}


void WxBasisTool::detailsButtonClickCB(wxCommandEvent& event)
{
    if (p_lastSlctn.size() > 0)
    {
        TGaussianBasisSet::GBSType gbstype = p_basisSetTypes[p_typeSlctn];
        gbs_details *details = p_gbsFactory->details(p_lastSlctn.c_str(), gbstype);

        if (p_bsetDetail == NULL)
        {
            p_bsetDetail = new WxBasisSetDetail(this);

            if (p_prefs != NULL)
                p_bsetDetail->loadSettings(p_prefs);
        }

        p_bsetDetail->showDetails(p_lastSlctn, details);
    }
}



void WxBasisTool::basisSetTypeListboxSelectedCB(wxCommandEvent& event)
{
    if (event.IsSelection())
    {

        int id = event.GetId();
//        cout << "id=" << StringConverter::toString(id) << endl;

        if (id == ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_10)
            p_typeSlctn = 10;
        else if (id == ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_11)
            p_typeSlctn = 11;

        p_lastSlctn = (string)(p_basisSetTypeListBox[p_typeSlctn]->GetStringSelection());
        TGaussianBasisSet::GBSType bstype = p_basisSetTypes[p_typeSlctn];
        showCoverage(p_lastSlctn, bstype);
        p_detailsButton->Enable(true);
        p_contextAddButton->Enable(p_editAllwd);
    }
}


void WxBasisTool::basisSetTypeListboxDoubleClickedCB(wxCommandEvent& event)
{
    if ((p_lastSlctn.size() > 0) && p_editAllwd)
    {
        addBasisSet(p_lastSlctn);
    }
}


void WxBasisTool::showEditorButtonClickCB(wxCommandEvent& event)
{
    if (p_cntrcEdit == NULL)
    {
        p_cntrcEdit = new WxContractionEditor(this);

        if (p_prefs != NULL)
            p_cntrcEdit->loadSettings(p_prefs);
    }

    p_cntrcEdit->setConfiguration(p_tcalc->gbsConfig());
    p_cntrcEdit->selectElement(0);
    p_cntrcEdit->Show();
}


bool WxBasisTool::confirmCollapse(TGBSConfig::ConfigType destType)
{
    string s1, s2;

    switch (destType)
    {
        case TGBSConfig::simple:
            s1 = "Simple";
            s2 = "simple";
            break;

        case TGBSConfig::element:
            s1 = "Element";
            s2 = "element";
            break;

        default:
            break;
    }

    string title = "Convert to " + s1;
    string prompt = "To convert to a " + s2 + " molecular basis, the data "
                    "set will have to be collapsed.\nDo you want to continue?";

    int style = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
    wxMessageDialog *dlgMesg = new wxMessageDialog(this, prompt, title, style);

    int result = dlgMesg->ShowModal();

    return (result == wxID_YES);
}


void WxBasisTool::helpProgramMenuItemClickCB(wxCommandEvent& event)
{
//    cout << "WxBasisTool::helpProgramMenuItemClickCB(wxCommandEvent& event)" << endl;

    BrowserHelp help;
    help.showHelpPage(BASISTOOL);
//01227   EcceHelp().showHelpPage(XtName(curWid));
}


void WxBasisTool::helpSupportMenuItemClickCB(wxCommandEvent& event)
{
//    cout << "WxBasisTool::helpSupportMenuItemClickCB(wxCommandEvent& event)" << endl;

    BrowserHelp help;
    help.showFeedbackPage();
}



void WxBasisTool::contextBasisSetsNotebookPageChangingCB(wxNotebookEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        TGBSConfig *cnfg = p_tcalc->gbsConfig();
        EE_ASSERT(cnfg, EE_FATAL, "Config is NULL!" );

        TGBSConfig::ConfigType destType, currType;
        bool cnfrm = true;

        currType = cnfg->type();
        destType = (TGBSConfig::ConfigType)event.GetSelection();

        // This lets us know if we are condensing data or not
        int numGroups = cnfg->numUniqueGroups();

        if ((destType < currType) && (numGroups > 1))
        {
            cnfrm = confirmCollapse(destType);
        }

        if (!cnfrm)
            event.Veto();
    }
}


/**
 *
 *   This method is called when the user selects a new molecular basis
 *   tab - simple, element, atom.  It can also be triggered
 *   programatically.
 *
 *   Whenever moving from a more complex to simpler structure, data is
 *   lost so the user must be prompted.  To go from a simpler to more
 *   complex structure there is no need to prompt the user.
 */
void WxBasisTool::contextBasisSetsNotebookPageChangedCB(wxNotebookEvent& event)
{
  //  cout << "WxBasisTool::contextBasisSetsNotebookPageChangedCB(wxNotebookEvent&)" << endl;

    if (!p_inCtrlUpdate)
    {
        TGBSConfig *cnfg = p_tcalc->gbsConfig();
        TGBSConfig::ConfigType destType;

        destType = (TGBSConfig::ConfigType)event.GetSelection();

        int numGroups = cnfg->numUniqueGroups();

        string groupName = cnfg->groupTagOfHeaviestElement();

        string tag = cnfg->uniqueTagStr();

        vector<string>* orbitals = cnfg->getOrbitalGBSIdentifiers(groupName);
        vector<GBSKey>* auxiliaries = cnfg->getAuxGBSIdentifiers(groupName);

        cnfg->removeAllGroups();

        // Going to simple
        if (destType == TGBSConfig::simple)
        {
            TGBSGroup* group = new TGBSGroup();
            int i;

            for (i = 0; i < orbitals->size(); i++)
            {
                vector<TGaussianBasisSet*> gbsList = p_gbsFactory->lookup((*orbitals)[i].c_str(), TGaussianBasisSet::UnknownGBSType, tag.c_str());
                group->insertOrbitalGBS((*orbitals)[i], gbsList, true);
            }

            for (i = 0; i < auxiliaries->size(); i++)
            {
                //   For now, ignore any appended-tight or
                //   appended-diffuse aux functons (that may have been added via the
                //   contraction editor).  All Amica-specific contraction
                //   data is removed when switching config between simple or
                //   element.

                if ((*auxiliaries)[i].name.find("appendedFunction") == string::npos)
                {
                    vector<TGaussianBasisSet*> gbsList = p_gbsFactory->lookup((*auxiliaries)[i].name.c_str(), (*auxiliaries)[i].type, tag.c_str());
                    group->insertGBS(new TGaussianBasisSet (*(gbsList[0])), true);
                }
            }

            // need to use fragment for the group tag because it
            // is putting the elements in a different order than
            // config->getUniqueTagStr() ... need to fix this
            cnfg->insertGBSGroup(p_tcalc->fragment()->uniqueTagStr(), group);

            // don't verify group coverage (it's ok if some groups are empty
            // at this point - we only care about that when saving)
            if (!checkCodeRules(false))
            {
                cnfg->resetGroup(tag, 0);
            }
            else if (numGroups > 1)
            {
                ErrMsg errs;
                errs.flush();  // clear out any old messages
                GBSRules::checkECPCoverage(cnfg);

                if (errs.check("EE_ECP_COVERAGE"))
                {
                    p_messagesFeedback->setMessage(errs.messageText("EE_ECP_COVERAGE"), WxFeedback::WARNING);
                    errs.flush("EE_ECP_COVERAGE");
                }
            }
        }
        else if (destType == TGBSConfig::element)
        {
            TGBSConfigTags tagVec(tag.c_str());

            for (int i = 0; i < tagVec.size(); i++)
            {
                TGBSGroup* group = new TGBSGroup();
                int j;

                for (j = 0; j < orbitals->size(); j++)
                {
                    vector<TGaussianBasisSet*> gbsList = p_gbsFactory->lookup((*orbitals)[j].c_str(), TGaussianBasisSet::UnknownGBSType, tagVec[i].c_str());
                    group->insertOrbitalGBS((*orbitals)[j], gbsList, true);
                }

                for (j = 0; j < auxiliaries->size(); j++)
                {

                    //   For now, ignore any appended-tight or
                    //   appended-diffuse aux functons (that may have been added via the
                    //   contraction editor).  All Amica-specific contraction
                    //   data is removed when switching config between simple or
                    //   element.

                    if ((*auxiliaries)[j].name.find("appendedFunction") == string::npos)
                    {
                        vector<TGaussianBasisSet*> gbsList = p_gbsFactory->lookup((*auxiliaries)[j].name.c_str(), (*auxiliaries)[j].type, tagVec[i].c_str());
                        group->insertGBS(new TGaussianBasisSet (*(gbsList[0])), true);
                    }
                }

                cnfg->insertGBSGroup(tagVec[i], group);
            }
        }

        delete auxiliaries;
        delete orbitals;


        cnfg->type(destType);
        resetBSTable(cnfg, p_tcalc->fragment());
        resetCodeObjects(true);

        int k = p_contextBasisSetsNotebook->GetSelection();
        int r1 = p_gridSlctnBlock[k]->getTopRow();
        int rz = p_gridSlctnBlock[k]->getBottomRow();

        string s = "";

        for (int r = r1; r <= rz; r++)
        {
            s.append((string)(p_contextBasisSetsGrid[k]->GetRowLabelValue(r1)));
        }

        selectElements(s);
        showContextChanged(true);
    }
}


void WxBasisTool::contextAddButtonClickCB(wxCommandEvent& event)
{
    if ((p_lastSlctn.size() > 0) && p_editAllwd)
        addBasisSet(p_lastSlctn);
}


void WxBasisTool::programCoefficientsMenuItemClickCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        if (p_bsetCoeffs == NULL)
        {
            p_bsetCoeffs = new WxBasisSetCoefficients(this);

            if (p_prefs != NULL)
                p_bsetCoeffs->loadSettings(p_prefs);
        }

        p_bsetCoeffs->showDetails(p_tcalc);
    }
}


void WxBasisTool::programSaveMenuItemClickCB(wxCommandEvent& event)
{
//    cout << "WxBasisTool::programSaveMenuItemClickCB(wxCommandEvent& event)" << endl;
    processSave();
}


void WxBasisTool::contextMultipleAllowedCheckboxClickCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
        p_multAllwd = event.IsChecked();
}


void WxBasisTool::programQuitMenuItemClickCB(wxCommandEvent& event)
{
//    cout << "WxBasisTool::programQuitMenuItemClickCB(wxCommandEvent& event)" << endl;

    bool cncld = promptContextSave();

    if (!cncld)
        Close(true);
}


void WxBasisTool::addBasisSet(string name)
{
//    cout << "addBasisSet(" << name << ")" << endl;

    Fragment *frag = p_tcalc->fragment();

    // Can only add basis set if fragment exists
    if (frag == NULL)
    {
        string mesg = "You must have a chemical system defined "
                      "before you can set up a basis set configuration.";
        p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
    }
    else
    {
        // Set for undo (in case there are consistency or completeness errors):
        TGBSConfig *config = p_tcalc->gbsConfig();
        TGBSConfig *undoConfig = 0;
        bool changeMade = true;

        if (config != 0)
            undoConfig = new TGBSConfig(*config);

        // UPDATE CONFIG WITH THE NEW BASIS SET
        TGBSGroup* group = 0;     // the group that contains the new basis set
        string tag;               // the tag for that group

        int mode = p_contextBasisSetsNotebook->GetSelection();

        // UPDATE SIMPLE CONFIGURATION (only one group in the config):
        if (mode == 0)
        {
            tag = frag->uniqueTagStr();
            group = updateGroup(config, tag, name);

            if (group == 0)
                changeMade = false;

        }
        else
        {
            // UPDATE ELEMENT OR ATOM CONFIGURATION

            int r1 = p_gridSlctnBlock[mode]->getTopRow();
            int rz = p_gridSlctnBlock[mode]->getBottomRow();
            int n = rz - r1 + 1;

            if (n == 0)
            {
                string mesg = "Please identify the element(s) which "
                              "will be associated with this basis set "
                              "by selecting one or more rows.";
                p_messagesFeedback->setMessage(mesg);
            }

            for (int row = r1; row <= rz; row++)
            {
                tag = p_contextBasisSetsGrid[mode]->GetRowLabelValue(row);
                group = updateGroup(config, tag, name);
                changeMade = (changeMade && (group != 0));
            }
        }

        // Now that the config has been updated, check to see if config is in
        // a consistent state
        if (changeMade && checkCodeRules(false))
        {
            // checkCodeRules prints err msgs

            // If an ecp was inserted, check ecp coverage
            // (this should only be performed one time, when it is first inserted)
            // (this only produces warnings)

            int ctgy = p_basisSetCtgys[p_typeSlctn];

            if (((ctgy == 1) && (p_typeSlctn == 7)) || (ctgy == 2))
            {
                ErrMsg errs;
                errs.flush(); // clear out any old messages
                GBSRules::checkECPCoverage(config);

                if (errs.check("EE_ECP_COVERAGE"))
                {
                    string mesg = errs.messageText("EE_ECP_COVERAGE");
                    p_messagesFeedback->setMessage(mesg, WxFeedback::WARNING);
                    errs.flush("EE_ECP_COVERAGE");
                }
            }

            // changes were valid, so go ahead and update gui table
            int r1 = p_gridSlctnBlock[mode]->getTopRow();
            int rz = p_gridSlctnBlock[mode]->getBottomRow();
            int n = rz - r1 + 1;


            int k = p_contextBasisSetsNotebook->GetSelection();

            if (n > 0)
            {
                for (int row = r1; row <= rz; row++)
                {
                    setGbsToTable(p_contextBasisSetsGrid[k], row, group);
                }
            }
            else
            {
                // this only happens when config is simple and no rows are selected
                setGbsToTable(p_contextBasisSetsGrid[k], 0, group);
            }

            //resizeHack();

            // update other widgets
            resetCodeObjects(true);
            indicateGBSConfig();

            if ((p_bsetCoeffs != NULL) && (p_bsetCoeffs->IsShown()))
                p_bsetCoeffs->showDetails(p_tcalc);

            TGBSConfig *cnfg = p_tcalc->gbsConfig();
            Fragment *frag = p_tcalc->fragment();

            updateNumbers(cnfg, frag);
            showContextChanged(true);

            if ((p_cntrcEdit != NULL) && (p_cntrcEdit->IsShown()))
                p_cntrcEdit->updateTables();

        }
        else
        {
            // changes were not valid, so revert to the previous config
            delete config;
            p_tcalc->gbsConfig(undoConfig);
        }
    }
}


void WxBasisTool::contextBasisSetsGridCellSelectCB(wxGridEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        int r1, c1, rz, cz;

        ewxGrid *trgt = (ewxGrid *)(event.GetEventObject());

        bool found = false;
        int i = 0;
        int k = -1;
        int n = sizeof(WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS) / sizeof(WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[0]);

        while ((i < n) && !found)
        {
            if (trgt->GetId() == WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[i])
            {
                found = true;
                k = i;
            }

            i++;
        }

        if (found)
        {
            string mesg;

            r1 = event.GetRow();
            c1 = event.GetCol();
            rz = r1;
            cz = ((c1 == 0) ? 6 : c1);

            mesg = "Event at (" + StringConverter::toString(r1)
                        + ", " + StringConverter::toString(c1) + ")";

            p_inCtrlUpdate = true;
            trgt->SelectBlock(r1, c1, rz, cz);
            p_gridSlctnBlock[k]->setTopLeft(r1, c1);
            p_gridSlctnBlock[k]->setBottomRight(rz, cz);
            p_inCtrlUpdate = false;
        }
    }

    event.Skip(true);
}


void WxBasisTool::contextBasisSetsGridRangeSelectCB(wxGridRangeSelectEvent& event)
{
    string mesg;
    int r1, rz;
    int c1, cz;

    if (event.Selecting() && !p_inCtrlUpdate)
    {
        ewxGrid *trgt = (ewxGrid *)(event.GetEventObject());
        bool found = false;
        int i = 0;
        int k = -1;
        int n = sizeof(WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS) / sizeof(WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[0]);

        while ((i < n) && !found)
        {
            if (trgt->GetId() == WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[i])
            {
                found = true;
                k = i;
            }

            i++;
        }
//        int index = trgt->GetId() - BASEID_GRID_WXBASISTOOL_CONTEXT_BASISSETS;

        if (found)
        {
            r1 = event.GetTopRow();
            c1 = event.GetLeftCol();
            rz = event.GetBottomRow();
            cz = event.GetRightCol();
            p_inCtrlUpdate = true;

            if (c1 == clmnOrbital)
            {
                cz = 6;
                trgt->SelectBlock(r1, c1, rz, cz);
            }

            p_gridSlctnBlock[k]->setTopLeft(r1, c1);
            p_gridSlctnBlock[k]->setBottomRight(rz, cz);


            string s = "";

            for (int r = r1; r <= rz; r++)
            {
                s.append((string)(p_contextBasisSetsGrid[k]->GetRowLabelValue(r)));
            }

            selectElements(s);

          //  selectElements((string)(trgt->GetRowLabelValue(r1)));

            trgt->SetGridCursor(r1, c1);
            p_inCtrlUpdate = false;
        }
    }
}


void WxBasisTool::editClearAllMenuItemClickCB(wxCommandEvent& evt)
{
    TGBSConfig *config = p_tcalc->gbsConfig();
    bool changeMade = false;

    int cnfgMode = p_contextBasisSetsNotebook->GetSelection();
    ewxGrid *grid = p_contextBasisSetsGrid[cnfgMode];
    string tag;

    int r1, rz;
    int c1, cz;

    r1 = 0;
    rz = grid->GetNumberRows() - 1;
    c1 = 0;
    cz = grid->GetNumberCols() - 1;

    p_gridSlctnBlock[cnfgMode]->setTopLeft(r1, c1);
    p_gridSlctnBlock[cnfgMode]->setBottomRight(rz, cz);

    for (int row = r1; row <= rz; row++)
    {
        changeMade = true;

        if (cnfgMode == 0)      // Simple
            tag = p_tcalc->fragment()->uniqueTagStr().c_str();
        else                    // Element (or Atomic)
            tag = grid->GetRowLabelValue(row);

        config->resetGroup(tag, NULL);

        for (int col = c1; col <= cz; col++)
        {
            grid->SetCellValue(row, col, "");
        }
    }

    grid->Update();

    if (changeMade)
    {
        updateNumbers(config, p_tcalc->fragment());
        showContextChanged(true);
        indicateGBSConfig();

        if ((p_bsetCoeffs != NULL) && (p_bsetCoeffs->IsShown()))
            p_bsetCoeffs->showDetails(p_tcalc);

        if ((p_cntrcEdit != NULL) && (p_cntrcEdit->IsShown()))
            p_cntrcEdit->updateTables();
    }
}


/**
 *  Remove selected basis sets from the config.
 *
 *  (It is assumed that this menu will be insensitive if the calculation
 *  is locked)
 */
void WxBasisTool::editClearSelectionMenuItemClickCB(wxCommandEvent& evt)
{
    TGBSConfig *config = p_tcalc->gbsConfig();
    TGBSGroup* group = NULL;
    bool changeMade = false;

    int cnfgMode = p_contextBasisSetsNotebook->GetSelection();
    ewxGrid *grid = p_contextBasisSetsGrid[cnfgMode];
    string tag;

    int r1, rz;
    int c1, cz;

    r1 = p_gridSlctnBlock[cnfgMode]->getTopRow();
    rz = p_gridSlctnBlock[cnfgMode]->getBottomRow();
    c1 = p_gridSlctnBlock[cnfgMode]->getLeftColumn();
    cz = p_gridSlctnBlock[cnfgMode]->getRightColumn();

    for (int row = r1; row <= rz; row++)
    {
        changeMade = true;

        if (cnfgMode == 0)      // Simple
            tag = p_tcalc->fragment()->uniqueTagStr().c_str();
        else                    // Element (or Atomic)
            tag = grid->GetRowLabelValue(row);

        if (c1 == clmnOrbital)
        {
            cz = 6;
            config->resetGroup(tag, NULL);

            for (int col = c1; col <= cz; col++)
            {
                grid->SetCellValue(row, col, "");
            }
        }
        else
        {
            group = (*(config->find(tag))).second;

            if (group != NULL)
            {
                bool wrkd;

                for (int col = c1; col <= cz; col++)
                {
                    if (col == clmnPolarization)
                        wrkd = group->removeGBS(TGaussianBasisSet::polarization);
                    else if (col == clmnDiffuse)
                        wrkd = group->removeGBS(TGaussianBasisSet::rydberg);
                    else if (col == clmnRydberg)
                        wrkd = group->removeGBS(TGaussianBasisSet::diffuse);
                    else if (col == clmnDFTCharge)
                        wrkd = group->removeGBS(TGaussianBasisSet::charge);
                    else if (col == clmnDFTExchange)
                        wrkd = group->removeGBS(TGaussianBasisSet::exchange);
                    else if (col == clmnECP)
                        wrkd = group->removeGBS(TGaussianBasisSet::ecp);

                    if (wrkd)
                        grid->SetCellValue(row, col, "");
                }
            }
        }
    }

    if (changeMade)
    {
        updateNumbers(config, p_tcalc->fragment());
        showContextChanged(true);
        indicateGBSConfig();

        if ((p_bsetCoeffs != NULL) && (p_bsetCoeffs->IsShown()))
            p_bsetCoeffs->showDetails(p_tcalc);

        if ((p_cntrcEdit != NULL) && p_cntrcEdit->IsShown())
            p_cntrcEdit->updateTables();
    }
}




//void WxBasisTool::displayContractionEditor()
//{
//    DebugEnterMethod("displayContractionEditor()");
    //  2006.1030
    //  Not yet dealing with the contraction editor, so comment out existing
    //  code, but keep as place-holder.
//    if (p_ceditor != 0)
//    {
//        p_ceditor->display(p_tcalc->gbsConfig());
//    }
  //  DebugLeaveMethod();
////}


/**
 *  Description
 *   Generate Exponents and Coefficients for the basis set config
 *   and display them in the dialog.
 *
 * Implementation
 *   To make this work, we need to first generate the ChemSys file.
 */
/*
void WxBasisTool::displayEC()
{
    DebugEnterMethod("displayEC()");
    //  2006.1030
    //  Placeholder for now.  Implementing code is commented out.

//    if (p_lastEC)
//  {
//      TGBSConfig *config = p_tCalc->gbsConfig();
//      const JCode* code = p_tCalc->application();

        if (config && code)
        {
            string code_name = code->name();
            char* basis_data = (char*)config->dump(code_name.c_str(), false);
            string label = config->name() + " formatted for " + code_name;

            p_lastEC->getrosTexti()->settextValue(basis_data);
            XmString xmstr = tu_cvt_string_to_xmstring((char*)label.c_str());
            p_lastEC->getlabeli()->setlabelString(xmstr);
            XmStringFree(xmstr);
        }
        else
        {
            p_lastEC->getrosTexti()->settextValue("");
            XmString xmstr = tu_cvt_string_to_xmstring("");
            p_lastEC->getlabeli()->setlabelString(xmstr);
            XmStringFree(xmstr);
        }

        raise_window(p_lastEC->getTextDlg());
    }

    DebugLeaveMethod();
}
*/

/**
 *
 *  Returns a pointer to the group that was updated.  Returns NULL
 *  if the basis set could not be added.  Reasons why the basis set
 *  could not be added are:
 *
 *     1) No orbital basis set exists for the group yet
 *     2) Basis set does not support the atoms in the tag
 *
 *  Appropriate error messages are printed to feedback area.
 */
TGBSGroup* WxBasisTool::updateGroup(TGBSConfig* config, string& tag, string& bsName)
{
//    cout << "updateGroup(TGBSConfig*, string&, string&)" << endl;

    TGBSGroup* group = 0; // return value
    TGaussianBasisSet::GBSType bstype = p_basisSetTypes[p_typeSlctn];

    // Check if the basis set supports the tag:
    string errMsg =  "Basis set ";
    errMsg += bsName;
    errMsg += " does not cover ";

    vector<TGaussianBasisSet*> gbsList = p_gbsFactory->lookup(bsName.c_str(), bstype, tag.c_str());

    // If multiple orbital basis sets are not supported, we want to replace
    // the group.  Otherwise, we want to append to existing group.
    bool replaceGroup = false;

    // Find the group in which to insert the basis set:
    TGBSConfig::iterator group_it = config->find(tag);

    EE_ASSERT( (group_it != config->end()), EE_FATAL,
                ("Trying to insert basis set into nonexistent or improperly "
                "labeled group: "
                + tag).c_str());

    group = (*(group_it)).second;

    // this is ugly - need to make an accessor to get this info
    if (((group == NULL) || (group->getOrbitalGBSMappings()->size() == 0)) && gbsList[0]->p_category == TGaussianBasisSet::AUXILIARY)
    {
        // trying to insert an auxiliary gbs before orbital
        string mesg = "You first must supply an orbital basis set for group ";
        mesg += tag;
        p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
    }
    else
    {
        // ok to insert

        // If inserting an orbital gbs:
        if (gbsList[0]->p_category == TGaussianBasisSet::ORBITAL)
        {
            // first check if that orbital gbs supports all elements in the tag:
            if (!p_gbsFactory->isSupported(bsName, bstype, tag))
            {
                p_messagesFeedback->setMessage((errMsg + tag).c_str(), WxFeedback::ERROR);
            }
            else
            {
                if (!p_multAllwd || (group == NULL))
                {
                    //clear out old group and start fresh
                    config->removeGBSGroup(tag);
                    group = new TGBSGroup();
                    replaceGroup = true;
                }

                // Pass in a recompute=true flag so that any appended tight or
                // diffuse functions will be recalculated based on the new exponents
                // included in this basis set
                group->insertOrbitalGBS(bsName, gbsList, true);

                if (replaceGroup)
                {
                    config->insertGBSGroup(tag, group);
                }
            }
        }
        else
        {
            // Inserting an auxilliary basis set to an existing group
            // Still add basis set, but print warning if not supported:

            if (!p_gbsFactory->isSupported(bsName, bstype, tag))
            {
                p_messagesFeedback->setMessage((errMsg + tag).c_str(), WxFeedback::WARNING);
            }

            group = (*group_it).second;

            // only one auxilliary BS will be added at a time
            // Pass in a recompute=true flag so that any appended tight or
            // diffuse functions will be recalculated based on the new exponents
            // included in this basis set
            group->insertGBS(new TGaussianBasisSet (*(gbsList[0])), true);
        }
    }

    return group;
}


EcceURL * WxBasisTool::getContext()
{
    return p_urlCalc;
}


bool WxBasisTool::promptContextSave(const bool& allowCancel)
{
//    cout << "WxBasisTool::promptContextSave(const bool& allowCancel)" << endl;

    bool cncld = false;

    if ((p_tcalc != NULL) && p_calcChgd &&
        p_messagesFeedback->getEditStatus() != WxFeedback::READONLY) {
        string prompt = "The current calculation has unsaved changes!  Do you "
                        "want to save changes before quitting?";
        string title = "Save Basis Set Tool Changes?";
        long style = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
        style |= allowCancel ? wxCANCEL : 0;

        ewxMessageDialog *dlgMesg = new ewxMessageDialog(this, prompt,
                                                         title, style);
        wxWindowID result = dlgMesg->ShowModal();

        cncld = (result == wxID_CANCEL);

        if (result == wxID_YES)
            processSave();
    }

    return cncld;
}


bool WxBasisTool::isSameContext(EcceURL* url, const string& theoryType)
{
//    cout << "WxBasisTool::isSameContext(EcceURL *, const string&)" << endl;
//    cout << "\t       url = " + url->toString() << endl;
//    cout << "\ttheoryType = " + theoryType << endl;

  bool ret;

  if (p_urlCalc == (EcceURL*)0)
    ret = false;
  else if (theoryType=="general" || theoryType=="single")
    ret = url->toString()==p_urlCalc->toString() && theoryType==p_theoryDscpn;
  else
    ret = url->toString() == p_urlCalc->toString();

//    cout << "\t==> " + StringConverter::toString(ret) << endl;

  return ret;
}

/**
 *  This function is called when starting the basis tool from a
 *  calculation.  It uses the passed url to load up the
 *  appropriate info from the calc and display it to screen.
 *  The method expects the calc url to be either a DavCalculation
 *  or a DirDyVTST calculation.
 *
 *  This function is also called when the BST receives a message
 *  from another application that something has changed.
 *
 *  Preconditions: Code, theory, and fragment must be defined before the
 *                 Basis Tool is started.
 *
 *  The url passed to this function should represent either an ICalculation
 *  or a DirDyVTST calculation.  For the DirDyVTST calculation, only the
 *  NEW_CALC changeType applies when responding to messages from other
 *  applications.
 *
 *  @todo should probably modify to allow basis tool to respond to
 *        chaanges to a DirDyVTST calc.  For now its ok to just support
 *         the invokeMCB (ie NEW_CALC), but may want to somehow support
 *         the other MCB functions (urlBasisMCB, urlCodeTheoryMCB,
 *         urlSubjectMCB) as well.  Also, dragging a DirDyVTST calc into
 *         the BasisTool doesn't work either - see dropCalculation()
 *         method.
 *
 *   @param url URL of calculation
 *
 *   @param changeType the type of set context call, can be for a new
 *          calulation, basis set is changed by another app, code theory
 *          changed, or the fragment has changed.
 *
 *    @param changeMsg message to display in feedback area if changes
 *           were made to the basis set to make it consistent with the
 *          current fragment/code.
 *
 *     @param theoryType indicates the type of theory for a DirDyVTST
 *           calculation.  Possible values are: "single" for single point
 *           theory, "general" for general theory, or undefined.  The
 *          theoryType only applies to NEW_CALC calls.
 */

void WxBasisTool::setContext(EcceURL *url, contextChangeType changeType,
                             const string& changeMsg, const string& theoryType)
{
//    cout << "WxBasisTool::setContext(EcceURL *url, contextChangeType changeType, const string& changeMsg, const string& theoryType)" << endl;
//    cout << "\t       url = " + url->toString() << endl;
//    cout << "\tchangeType = " + StringConverter::toString((int)changeType) << endl;
//    cout << "\t changeMsg = " + changeMsg << endl;
//    cout << "\ttheoryType = " + theoryType << endl;

    // Bail if it is the same context as the current one and just
    // raise the window in the stacking order (done by caller)
    if (isSameContext(url, theoryType) && changeType==cntxtNewCalculation)
    {
//        cout << "Returning" << endl;
        return;
    }

    ewxMessageDialog *dlgMesg;
    EcceURL urlCalc;

    if (changeType == cntxtNewCalculation)
    {
        // Set the theory type for new calcs
        this->Iconize(false);
    }

    //  Prompt user if there is already a calculation being edited
    //  and changes haven't yet been saved
    bool cncld = promptContextSave();

    if (!cncld)
    {
        bool valid = false;
        ICalculation *tmpicalc = NULL;

        // First lets make sure is all is ok with the object we were handed.
        Resource *rsc = EDSIFactory::getResource(*url);

        if (rsc != NULL)
        {
            tmpicalc = dynamic_cast<ICalculation *>(rsc);

            try
            {
                valid = ((tmpicalc != NULL) && tmpicalc->isValid());
            }
            catch (CancelException& cex)
            {
                // Here we will just quietly do nothing.
            }
            catch (RetryException& cex)
            {
                // Here we will post a message telling the user they are messed up.
                string prompt = "The maximum number of authentication attempts "
                            "has been exceeded.  Contact your ECCE "
                            "administrator if you have forgotten your "
                            "password.";
                string title = "Authentication Failure";
                long style = wxOK | wxICON_EXCLAMATION;
                dlgMesg = new ewxMessageDialog(this, prompt, title, style);
                dlgMesg->ShowModal();
            }
        }

        if (valid)
        {
            // Need to allocate this up here because of SGI bug:
            TGBSConfig* tcnfg = new TGBSConfig; // used if dav gbs is NULL

            // Remove the old TCalc (default TCalc is used if starting in
            // browse mode):
            if (p_tcalc != NULL)
            {
                delete p_tcalc;
                p_tcalc = NULL;
            }

            p_theoryDscpn = theoryType;

            if (p_urlCalc != (EcceURL*)0)
              delete p_urlCalc;
            // allocate memory for url or else it will be deleted
            p_urlCalc = new EcceURL(*url);

            p_calcChgd = false;

            // Load new p_tcalc from the DAV server:
            // (Only load the parts that are used in the basis tool)
            ICalculation* davCalc = tmpicalc;
            p_tcalc = new TCalculation;
            p_tcalc->gbsConfig(davCalc->gbsConfig(p_theoryDscpn));
            p_tcalc->fragment(davCalc->fragment());
            p_tcalc->application(davCalc->application());
            p_tcalc->theory(davCalc->theory(p_theoryDscpn));
            p_tcalc->setState(davCalc->getState());
            davCalc = NULL;

            p_messagesFeedback->setContextLabel(
                    EDSIServerCentral().mapURLtoName(url->toString()));
            p_messagesFeedback->setContextURL(url->toString());
            p_messagesFeedback->setRunState(p_tcalc->getState());
            if (p_tcalc->getState() > ResourceDescriptor::STATE_READY) {
              p_messagesFeedback->setEditStatus(WxFeedback::READONLY);
            }

            // Verify that fragment, code, and theory are defined:
            Fragment* frag = p_tcalc->fragment();
            string assertMessage = "";

            if ((frag == NULL) || (frag->uniqueTagStr() == ""))
            {
                assertMessage = "chemical system";
                p_contextFormulaStatic->SetLabel("(none)");
            }
            else if (p_tcalc->application() == NULL)
            {
                assertMessage = "computational code";
            }
            else if (p_tcalc->theory() == NULL)
            {
                assertMessage = "level of theory";
            }

            if (assertMessage != "")
            {
                assertMessage = "A basis set cannot be assigned to this "
                                "calculation until a " + assertMessage +
                                " is specified.";
                p_messagesFeedback->setMessage(assertMessage, WxFeedback::ERROR);
                delete p_tcalc;
                delete tcnfg;

                if (p_tcalcUndo)
                    p_tcalc = new TCalculation(p_tcalcUndo);
                else
                    p_tcalc = NULL;

                this->setEditingAllowed(false);

            }
            else
            {
                // calculation is valid for import to the basis tool
                // If no basis set defined yet, set up an empty one:
                TGBSConfig *config = p_tcalc->gbsConfig();

                if (config == NULL)
                {
                    p_tcalc->gbsConfig(tcnfg); // defaults to simple type
                    config = p_tcalc->gbsConfig();
                    config->resetGroup(p_tcalc->fragment()->uniqueTagStr(), NULL);
                }
                else
                {
                    delete tcnfg; // don't use it
                }

                TGBSConfig* oldConfig = new TGBSConfig(*config);

                // Make sure the config is consistent with the fragment/code
                // (in case they were changed persistently since the last time)
                ICalcUtils::makeConsistent(frag, config, p_tcalc->application(), p_tcalc->theory());
                p_tcalc->gbsConfig(config);

                // if the config had to change for consistency, tell everybody
                if (!oldConfig->isEqual(config))
                {
                    if (changeMsg != "")
                    {
                        p_messagesFeedback->setMessage(changeMsg, WxFeedback::WARNING);
                    }

                    saveContext();
                }

                delete oldConfig;

                // Set p_tcalcUndo to the new calc
                if (p_tcalcUndo != 0)
                {
                    delete p_tcalcUndo;
                }

                p_tcalcUndo = new TCalculation(p_tcalc);

                // only auto-optimize the code-related widgets (i.e. polarization,
                // optimize) if the code has been changed, otherwise leave what
                // the user has in there
                bool autoOptimize = (changeType == cntxtTheoryChange);

//                p_contextCodeStatic->SetLabel(p_tcalc->application()->name());

                TGBSConfig *cnfg = p_tcalc->gbsConfig();
                Fragment *frag = p_tcalc->fragment();

                showFormula(cnfg);                  // show molecular formula'


                setPolarizationAndOptimize(cnfg);   // sets according to config values
                resetCodeObjects(autoOptimize);     // may change dft, ecp, spherical,
                                                    // optimize in order
                                                    // conform to code rules
/*
                bool prevState = p_inCtrlUpdate;
                p_inCtrlUpdate = true;
                int numGroups = cnfg->numUniqueGroups();
//                cout << "\tnumGroups=" << StringConverter::toString(numGroups) << endl;
                bool simple = (numGroups <= 1);
//                cout << "\tsimple=" << StringConverter::toString(simple) << endl;
                int k = (simple ? 0 : 1);

//                cout << "\tSelecting " << StringConverter::toString(k) << endl;
                p_contextBasisSetsNotebook->SetSelection(k);
                p_inCtrlUpdate = prevState;
                resetBSTable(cnfg, frag);           // show config data on screen
*/
            refreshBasisSetsDisplay();
            updateNumbers(cnfg, frag);

                // Deactivate some widgets if calculation is in a locked state
             //   indicateChemSys();
                indicateGBSConfig();


                // Check state of calculation:
                #if CALC_ON
                EDSIServerCentral servers;
                p_messagesFeedback->showCalcInfo(p_tcalc->getState(), true, servers.mapURLtoName(p_urlCalc));
                #endif

                this->setEditingAllowed(true);
            }

        //  setPollContext(url);
        }
        else
        {
            tmpicalc = 0;
        }
    }
}


void WxBasisTool::refreshBasisSetsDisplay()
{
    TGBSConfig *cnfg = p_tcalc->gbsConfig();
    Fragment *frag = p_tcalc->fragment();

    TGBSConfig::ConfigType cnfgType = cnfg->type();

    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    if (cnfgType == TGBSConfig::UnknownConfigType)
    {
        int numGroups = cnfg->numUniqueGroups();
        cout << "\tnumGroups=" << StringConverter::toString(numGroups) << endl;
        cnfgType = ((numGroups <= 1) ? TGBSConfig::simple : TGBSConfig::element);
    }

    int k = ((cnfgType == TGBSConfig::simple) ? 0 : 1);

    // cout << "\tSelecting " << StringConverter::toString(k) << endl;
    p_contextBasisSetsNotebook->SetSelection(k);

    // Clear existing information
    while (p_contextBasisSetsGrid[k]->GetNumberRows() > 0)
        p_contextBasisSetsGrid[k]->DeleteRows(0);

    int idx = 0;
    TGBSConfig::iterator config_it;


    if (cnfgType == TGBSConfig::simple)
    {
        p_contextBasisSetsGrid[k]->AppendRows();

        if (frag != NULL)
            p_contextBasisSetsGrid[k]->SetRowLabelValue(idx, frag->formula());

       //if ((*config_it).second)
        config_it = cnfg->begin();

        setGbsToTable(p_contextBasisSetsGrid[k], idx, (*config_it).second);
    }
    else if (cnfgType == TGBSConfig::element)
    {
        for (config_it = cnfg->begin(); config_it != cnfg->end(); config_it++)
        {
            p_contextBasisSetsGrid[k]->AppendRows();

            p_contextBasisSetsGrid[k]->SetRowLabelValue(idx, (*config_it).first);

            if ((*config_it).second)
                setGbsToTable(p_contextBasisSetsGrid[k], idx, (*config_it).second);

            idx++;
        }
    }

    p_inCtrlUpdate = prevState;
}


void WxBasisTool::setEditingAllowed(bool enbld)
{
    p_editAllwd = enbld;

    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    p_editClearSlctnMenuItem->Enable(p_editAllwd);
    p_editClearAllMenuItem->Enable(p_editAllwd);
    //p_programSaveMenuItem->Enable(p_editAllwd);
    p_programCoeffsMenuItem->Enable(p_editAllwd);

    p_contextAddButton->Enable(p_editAllwd);
    p_showEditorButton->Enable(p_editAllwd);

    p_contextBasisSetsNotebook->Enable(p_editAllwd);

    for (int i = 0; i < 2; i++)
    {
        p_polarizationRadioButton[i]->Enable(p_editAllwd);
    }

    p_optimizeCheckBox->Enable(p_editAllwd);
    p_contextBasisSetsNotebook->Enable(p_editAllwd);

    p_inCtrlUpdate = prevState;
}


void WxBasisTool::selectElements(Fragment *frag)
{
//    cout << "WxBasisTool::selectElements(Fragment *)" << endl;
//    cout << "\tformula=" << frag->formula() << endl;
    TPerTab *ptab = p_elementsTable->getTPerTab();

    EcceSortedVector<TTagCountPair, less<TTagCountPair> > * vctPairs = TFormula::parse_formula(frag->formula());

    p_elementsTable->clearSelection();

    int a;

    int n = vctPairs->size();
//    cout << "\tnumPairs=" << StringConverter::toString(n) << endl;

    for (int i = 0; i < n; i++)
    {
        TTagCountPair pair = (*vctPairs)[i];
        string elmtTag = pair.tag;
        a = ptab->atomicNumber(elmtTag);
//        cout << "\t\ti=" << StringConverter::toString(i) << "; elmtTag=" << elmtTag << "; a=" + StringConverter::toString(a) << endl;
        p_elementsTable->selectElement(a);
    }

}


void WxBasisTool::selectElements(string elmts)
{
//    cout << "WxBasisTool::selectElements(" << elmts << ")" << endl;
    
    TPerTab *ptab = p_elementsTable->getTPerTab();

    EcceSortedVector<TTagCountPair, less<TTagCountPair> > * vctPairs = TFormula::parse_formula(elmts);

    p_elementsTable->clearSelection();

    int a;

    int n = vctPairs->size();
//    cout << "\tnumPairs=" << StringConverter::toString(n) << endl;

    for (int i = 0; i < n; i++)
    {
        TTagCountPair pair = (*vctPairs)[i];
        string elmtTag = pair.tag;
        a = ptab->atomicNumber(elmtTag);
//        cout << "\t\ti=" << StringConverter::toString(i) << "; elmtTag=" << elmtTag << "; a=" + StringConverter::toString(a) << endl;
        p_elementsTable->selectElement(a);
    }

}



// Description
//   Reset the interface to show that the user is now back in
//   calculation mode.  This means the DFT/ECP stuff has to be
//   set according to the calc code.  Also the Formula is a label and
//   not a type-in.  The code name is a label and not an option menu.
//   See intoBrowseMode.
//
// Implementation
//   There should be no way of getting here if there is no calc.
//
//void WxBasisTool::outOfBrowseMode(bool autoOptimize)
//{
    //  KRS 2006.1020
    //  This stuff shouldn't be necessary any more as we are not
    //  going to allow Browse Mode.  The only reason to retain this,
    //  for the moment at least, is for the functionality of updating
    //  the dialogue components.
    //
    //  getplayModei()->setset(False); // browse mode toggle

    //  // This "if" is just to avoid redundant work.
    //  // Change widgets for calculation mode
    //  if (!XtIsManaged(getchemFormulaValuei()->getlabel())) {
    //  p_messagesFeedback->mapCalcInfo();
    //  XtUnmanageChild(getchemFormulaInputi()->gettext());
    //  XtManageChild(getchemFormulaValuei()->getlabel());
    //  XtUnmanageChild(getimportFragBtni()->getmenuBtn());
    //  XtManageChild(getsaveBtni()->getmenuBtn());
    //  XtUnmanageChild(getcodeListOpMenu());
    //  XtManageChild(getcodeNameLabel());
    //  }

/*
  // Set the top table according to config type
    Widget curTab = p_topTab;

    TGBSConfig::ConfigType curtype = p_tCalc->gbsConfig()->type();

    if (curtype == TGBSConfig::simple)
        p_topTab = getsimpleForm();
    else if(curtype == TGBSConfig::element)
        p_topTab = getelementForm();
    else if(curtype == TGBSConfig::atom)
        p_topTab = getatomForm();

    // Only invoke the callback if the tab is really changing
    if (p_topTab != curTab)
    {
        p_newCalc = true; // this only gets set when a new calc is dropped in
        XtVaSetValues(getconfigTab(),XmNxrtGearActivePageWidget,p_topTab,NULL);
    }

    p_contextCodeStatic->SetLabel(p_tcalc->application()->name());

    TGBSConfig *cnfg = p_tcalc->gbsConfig();
    Fragment *frag = p_tcalc->fragment();

    showFormula(cnfg);      // show molecular formula

    setPolarizationAndOptimize(cnfg);    // sets according to config values
    resetCodeObjects(autoOptimize);  // may change dft, ecp, spherical,
                                     // optimize in order
                                     // conform to code rules

    resetBSTable(cnfg, frag);     // show config data on screen

    // Deactivate some widgets if calculation is in a locked state
    indicateChemSys();
    indicateGBSConfig();

 */
//}

/*void WxBasisTool::indicateChemSys()
{
//    p_contextPanel->Enable(p_tcalc->fragment() != NULL);
}
*/
/**
 *  Description
 *   Set objects (in)senstive to indicate the exitence of a valid
 *   GBSConfig.
 *
 *   The revert button is tricky since it is enabled/disabled as
 *   changes are made (something to revert to). (We dont have a revert
 *   button anymore!)
 *
 *   The clear* buttons are also tricky because they must never be
 *   enabled if the calc is not modifiable.
 */
void WxBasisTool::indicateGBSConfig()
{
    TGBSConfig* config = p_tcalc->gbsConfig();
    bool configExists = (config != 0);

    // Can only export and print coefficients if config is complete
    // bool configComplete  = GBSRules::isComplete(p_tcalc->fragment(),
    //                                                 config,
    //                                                 p_tcalc->application(),
    //                                                 p_tcalc->theory());
    //  KRS (2006.1031)  Enable Export button if (configExists && configComplete)
    //  KRS (2006.1031)  Enable Exponents/Coefficients button if (configExists && configComplete)

    // Calculation is writable if it is not past the READY state
    // or if it is in browse mode:
    bool writable = (p_tcalc->getState() <= ResourceDescriptor::STATE_READY);

    // Desensitize these widgets if the calc is not writable:
    p_editClearSlctnMenuItem->Enable(configExists && writable);

    TGBSConfig::ConfigType type = p_tcalc->gbsConfig()->type();

    for (int i = 0; i <= 1; i++)
    {
        bool allwd = ((type == (TGBSConfig::ConfigType)(i)) || writable);
        p_contextBasisSetsGrid[i]->Enable(allwd);
        p_contextBasisSetsPanel[i]->Enable(allwd);
    }

//    XtSetSensitive(geteditConfigBtn(), (Boolean) writable);
}


/**
 *  Returns true if the calculation's basis configuration can be
 *  modified.  Otherwise, it displays the appropriate message and returns
 *  false.

bool WxBasisTool::addAllowed() const
{
//    cout << "addAllowed()" << endl;

    bool result = false;

    if (p_tcalc->getState() >= ResourceDescriptor::STATE_SUBMITTED)
    {
        string mesg = "This calculation is at or past the submitted "
                      "state and cannot be modified.";
        p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
    }
    else if (!p_tcalc->fragment())
    {
        string mesg = "You must have a chemical system defined "
                      "before you can set up a basis set configuration.";
        p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
    }
    else
    {
        result = true;
    }

    return result;
}
*/

void WxBasisTool::updateContextURL(EcceURL *urlPrev, EcceURL *urlCurr)
{
    string pathPrev = urlPrev->toString();
    string pathCurr = urlCurr->toString();

    string pathCtxt = p_urlCalc->toString();
    pathCtxt = pathCtxt.replace(0, pathPrev.length(), pathCurr);

    if (p_urlCalc != (EcceURL*)0)
      delete p_urlCalc;
    p_urlCalc = new EcceURL(pathCtxt);

    EDSIServerCentral servers;
    p_messagesFeedback->setContextLabel(servers.mapURLtoName(pathCtxt));
}


void WxBasisTool::refreshPeriodicTable()
{
 //   p_elementsTable->resetPerTabPrefs();
}


void WxBasisTool::refreshFeedbackPanel()
{
    p_messagesFeedback->readPreferences();
}


void WxBasisTool::refreshContextFormula()
{
    // Pick up the new fragment:
    ICalculation* davCalc = dynamic_cast<ICalculation*>(EDSIFactory::getResource(*p_urlCalc));
    Fragment* frag = davCalc->fragment();

    // Check to see if the fragment has been erased:
    if (frag == NULL || frag->uniqueTagStr() == "")
    {
        davCalc->gbsConfig((TGBSConfig*) 0, p_theoryDscpn); // erase the config

        if (p_tcalcUndo != NULL)  // erase the revert calculation
        {
            delete p_tcalcUndo;
            p_tcalcUndo = NULL;
        }

        string msg = "The chemical system for this calculation has been deleted"
                            " - switching to browse mode.";
        p_messagesFeedback->setMessage(msg.c_str());
    //  intoBrowseMode();
    //  indicateCalculation(False);
    //  unsavedState(False);
    }
    else
    {
        // Print message to notify user of change:
        string dscpn = "The Chemical System for this calculation was "
                       "changed by another application.  The basis set "
                       "configuration has changed to maintain consistency.";
        setContext(p_urlCalc, cntxtFragmentChange, dscpn, "");
    }
}


void WxBasisTool::refreshContextStatus(ResourceDescriptor::RUNSTATE stateTrgt)
{
    p_tcalc->setState(stateTrgt);
    p_messagesFeedback->setRunState(stateTrgt);
    if (stateTrgt > ResourceDescriptor::STATE_READY) {
      p_messagesFeedback->setEditStatus(WxFeedback::READONLY);
    } else {
      p_messagesFeedback->setEditStatus(WxFeedback::EDIT);
    }
    resetCodeObjects(false); //true);
    indicateGBSConfig();
}



/**
 *  Description
 *  Shows the formula.  This works on the appropriate widget as
 *  determined by whether the user is in browse mode.  For browse mode,
 *  the formula is placed in the editable text field.  For calculation
 *  mode it is place in the uneditable label.  The elements are not
 *  reflected in the pertab since pertab selection is used for the
 *  "narrow to selected" functionality.
 */
void WxBasisTool::showFormula(TGBSConfig* cnfg)
{
    Fragment* frag = p_tcalc->fragment();
    string formula;

    if (frag)
    {
        formula = frag->formula();
    }

    if (cnfg)
    {
        p_contextFormulaStatic->SetLabel(formula);
    }

    this->selectElements(frag);

        //  here do selection of formula elements within the periodic table.
        //  Can use the getTPerTab() method on the Periodic table class; this
        //  will allow me to get the atomic numbers of the elements

    p_elementsTable->Refresh();

}


void WxBasisTool::setPolarizationAndOptimize(TGBSConfig* cnfg)
{
    // Activate spherical or cartesian based on TGBSConfig setting:
    bool prev = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    int k = cnfg->coordsys();
    p_polarizationRadioButton[k]->SetValue(true);
    p_optimizeCheckBox->SetValue(cnfg->optimize());

    p_inCtrlUpdate = prev;
}


/**
 *  Description
 *   Based on the code and the current configuration, set the
 *   Polarization Component radios and the Optimize toggle.  It also
 *   enables/disables ECP/DFT options as necessary.
 *
 * Implementation
 *   This method assumes that the proper code label is already set and
 *   the configuration has already been error checked/corrected.
 *   It is intended to be used when the code is changed - thus the word
 *   reset.
 *
 */
void WxBasisTool::resetCodeObjects(bool autoOptimize)
{
//    cout << "resetCodeObjects(" <<  StringConverter::toString(autoOptimize) << ")" << endl;

    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    const JCode* cap = p_tcalc->application();
    TGBSConfig *config = p_tcalc->gbsConfig();
    EE_ASSERT(cap, EE_FATAL, "No code defined!");
    EE_ASSERT(config, EE_FATAL, "No config defined!");

    bool writable = p_tcalc->getState() <= ResourceDescriptor::STATE_READY;

    // 1) SET POLARIZATION RADIO BUTTONS:
    bool hasSpherical = config->hasSpherical();
    bool sphericalsSupportedCode = false;
    bool sphericalsSupportedTheory;
    cap->get_bool("SphericalCoordinates", sphericalsSupportedCode);

//    cout << "\tsphericalsSupportedCode=" << StringConverter::toString(sphericalsSupportedCode) << endl;

    // If spherical is selected, but the code doesn't support it we
    // have to switch to cartesian and give a warning.
    // (this could only happen in browse mode)
    if (!sphericalsSupportedCode && p_polarizationRadioButton[TGaussianBasisSet::Spherical]->GetValue())
    {
        if (p_programCoeffsMenuItem->IsEnabled())
        {
            p_messagesFeedback->setMessage("Spherical polarization components are "
            "not supported by this code - changing to cartesian.", WxFeedback::WARNING);
        }

//        cout << "\tSwitching to Cartesian" << endl;
        p_polarizationRadioButton[TGaussianBasisSet::Cartesian]->SetValue(true);
        config->coordsys(TGaussianBasisSet::Cartesian);
    }


    // Check to see that spherical is supported by the theory as well:
    if (sphericalsSupportedCode)
    {
        // get theories for which sphericals are not supported
        vector<string> nospherical;
        cap->get_string_list("TheoryNoSpherical", nospherical);

        // check all non-spherical theories to see if our theory is one of them
        sphericalsSupportedTheory = true;

        TTheory *thry = p_tcalc->theory();
        string cat = thry->category();

        if(cat != "UNKNOWN")
        {
            vector<string>::iterator it;

            for (it = nospherical.begin(); it != nospherical.end(); it++)
            {
                if ((*it) == cat)
                {
                    sphericalsSupportedTheory = false;
                }
            }
        }

//        cout << "\tsphericalsSupportedTheory=" << StringConverter::toString(sphericalsSupportedTheory) << endl;

        // If spherical is selected, but the theory doesn't support it we
        // have to switch to cartesian and give a warning.
        // (this could only happen in browse mode)
        if (!sphericalsSupportedTheory && p_polarizationRadioButton[TGaussianBasisSet::Spherical]->GetValue())
        {
            if (p_programCoeffsMenuItem->IsEnabled())
            {
                p_messagesFeedback->setMessage("Spherical polarization "
                                               "components are "
                                               "not supported for this theory "
                                               "- changing to cartesian.",
                                               WxFeedback::WARNING);
            }

//            cout << "\tHave to switch to Cartesian" << endl;
            p_polarizationRadioButton[TGaussianBasisSet::Cartesian]->SetValue(true);
            config->coordsys(TGaussianBasisSet::Cartesian);
            showContextChanged(true);
        }
    }

//    cout << "\tautoOptimize=" << StringConverter::toString(autoOptimize) << endl;
//    cout << "\twritable=" << StringConverter::toString(writable) << endl;

    // Switch to spherical if we want to autoOptimize
    // (we only do this at the time the basis set is added to the config)
    if (autoOptimize && writable)
    {
//        cout << "\tOptimizing" << endl;

        if (sphericalsSupportedCode && sphericalsSupportedTheory && hasSpherical)
        {
//            cout << "\tSpherical Okay" << endl;

            if (p_polarizationRadioButton[TGaussianBasisSet::Cartesian]->GetValue())
            {
//                cout << "\tSwitching to Spherical" << endl;

                config->coordsys(TGaussianBasisSet::Spherical);
                p_polarizationRadioButton[TGaussianBasisSet::Spherical]->SetValue(true);
                showContextChanged(true);
            }
        }
        else if (p_polarizationRadioButton[TGaussianBasisSet::Spherical]->GetValue())
        {
//            cout << "\tMust be Cartesian; switching" << endl;

            config->coordsys(TGaussianBasisSet::Cartesian);
            p_polarizationRadioButton[TGaussianBasisSet::Cartesian]->SetValue(true);
            showContextChanged(true);
        }
    }

    // Desensitize spherical button based on config and code data:
    //  a) If code only supports cartesian, desensitize spherical.
    p_polarizationRadioButton[TGaussianBasisSet::Spherical]->Enable(sphericalsSupportedTheory && sphericalsSupportedCode && writable);

    // Desensitize cartesian if calculation is locked:
    p_polarizationRadioButton[TGaussianBasisSet::Cartesian]->Enable(writable);

    // 2) SET OPTIMIZE TOGGLE
    bool preferOpt;
    bool hasGC = config->hasGeneralContractions();
    cap->get_bool("PreferOptimization", preferOpt);

    // Don't select optimize if basis sets don't have general contractions
    p_optimizeCheckBox->Enable(hasGC && writable);
    cap->get_bool("AllowMultipleOrbitals", p_multSpptd);
    p_multAllwd = (p_multAllwd && p_multSpptd);

    p_inCtrlUpdate = true;
    p_contextAllowMultipleCheckbox->Show(p_multSpptd);
    p_contextAllowMultipleCheckbox->SetValue(p_multAllwd);

    if (writable)
    {
        if (!hasGC)
        {
            if (p_optimizeCheckBox->GetValue())
            {
                config->optimize(false);
                p_optimizeCheckBox->SetValue(false);
                showContextChanged(true);
            }
        }
        else if (autoOptimize && preferOpt && config->optimize() == false)
        {
            config->optimize(true);
            p_optimizeCheckBox->SetValue(true);
            showContextChanged(true);
        }
        else if (autoOptimize && !preferOpt && config->optimize() == true)
        {
            config->optimize(false);
            p_optimizeCheckBox->SetValue(false);
            showContextChanged(true);
        }
    }

    const JCode *code = p_tcalc->application();
    TGBSConfig *cnfg = p_tcalc->gbsConfig();
    Fragment *frag = p_tcalc->fragment();
    TTheory *thry = p_tcalc->theory();

    resetECPObjects(code);
    resetDFTObjects(code, thry);

    updateNumbers(cnfg, frag);

    p_inCtrlUpdate = prevState;
}


/**
 *  Description
 *   Note that ECP Orbitals are just orbitals so never make them
 *   insensitive.  Only the ECPs themselves become inaccessible.  However,
 *   this means that the function that adds the orbital must now all
 *   the ECPs to be added if not supported by the code.
 *      ECPs
 *      ECP Column in all tables
 */
void WxBasisTool::resetECPObjects(const JCode* code)
{
    bool codeSupports = true;

    if (code)
        code->get_bool("ECPS", codeSupports);

    p_basisSetCategoryPanel[2]->Enable(codeSupports);
    setGridColumnVisible(clmnECP, codeSupports);
}

/**
 *  Note that DFT Orbitals are just orbitals so never make them
 *  insensitive.  Only the Fitting sets themselves become inaccessible.
 *  However, this means that the function that adds the orbital must now all
 *  the ECPs to be added if not supported by the code.
 *     DFTs
 *     DFT charge and exchange Column in all tables
 */
void WxBasisTool::resetDFTObjects(const JCode *code, TTheory *thry)
{
    bool codeSupports = true;

    if (code)
    {
        code->get_bool("DFTFitting", codeSupports);
    }

    bool theorySupports = ((thry != NULL) && ((thry->category()).find("DFT") != string::npos));
    p_basisSetCategoryPanel[3]->Enable(theorySupports && codeSupports);
    setGridColumnVisible(clmnDFTCharge, (theorySupports && codeSupports));
    setGridColumnVisible(clmnDFTExchange, (theorySupports && codeSupports));
}


void WxBasisTool::resetBSTable(TGBSConfig* cnfg, Fragment* frag)
{
//    cout << "WxBasisTool::resetBSTable(TGBSConfig *, Fragment *)" << endl;

    EE_ASSERT(cnfg, EE_FATAL, "TGBSConfig not defined!");
    bool simple;

    int k = p_contextBasisSetsNotebook->GetSelection();

    // Clear existing table

    while (p_contextBasisSetsGrid[k]->GetNumberRows() > 0)
    {
        p_contextBasisSetsGrid[k]->DeleteRows(0);
    }

    simple = (k == 0);


    int idx = 0;
    TGBSConfig::iterator config_it;

    for (config_it = cnfg->begin(); config_it != cnfg->end(); config_it++)
    {
        p_contextBasisSetsGrid[k]->AppendRows();

        if (simple)
        {
            if (frag != NULL)
                p_contextBasisSetsGrid[k]->SetRowLabelValue(idx, frag->formula());
        }
        else
            p_contextBasisSetsGrid[k]->SetRowLabelValue(idx, (*config_it).first);

        if ((*config_it).second)
            setGbsToTable(p_contextBasisSetsGrid[k], idx, (*config_it).second);

        idx++;
    }


  //  resizeHack();
    updateNumbers(cnfg, frag);

}


void WxBasisTool::setGbsToTable(ewxGrid* grid, int row, TGBSGroup* group)
{
    // Do something only if the group has  basis sets:
    if ((group != NULL) && !group->empty())
    {
        int numcols = grid->GetNumberCols();

        for (int col = 0; col < numcols; col++)
        {
            grid->SetCellValue(row, col, "");
        }

        TGBSGroup::iterator group_it;
        const OrbitalMap* orbitalMap = group->getOrbitalGBSMappings();
        OrbitalMap::const_iterator orbIt;
        string contents;

        // First fill the orbital basis set column, which may include
        // multiple basis sets, and refers to each basis set by its
        // aggregate name
        for (orbIt = orbitalMap->begin(); orbIt != orbitalMap->end(); orbIt++)
        {
            contents = grid->GetCellValue(row, clmnOrbital);

            if (contents != "")
            {
                contents += ",\n";
            }

            contents += (*orbIt).first;
            grid->SetCellValue(row, clmnOrbital, contents);
        }

        // Then fill the rest of the columns:
        for (group_it = group->begin(); group_it != group->end(); group_it++)
        {
            switch((*group_it).second->p_category)
            {
                case TGaussianBasisSet::ORBITAL:
                {
                    break; // don't do anything, as these are handled above
                }
                case TGaussianBasisSet::AUXILIARY:
                {
                    switch((*group_it).second->p_type)
                    {
                        case TGaussianBasisSet::polarization:
                        {
                            contents = grid->GetCellValue(row, clmnPolarization);

                            if (contents != "")
                            {
                                contents += ",\n";
                            }

                            contents += (*group_it).second->p_name;
                            grid->SetCellValue(row, clmnPolarization, contents);

                            break;
                        }
                        case TGaussianBasisSet::rydberg:
                        {
                            contents = grid->GetCellValue(row, clmnRydberg);

                            if (contents != "")
                            {
                                contents += ",\n";
                            }

                            contents += (*group_it).second->p_name;
                            grid->SetCellValue(row, clmnRydberg, contents);

                            break;
                        }
                        case TGaussianBasisSet::diffuse:
                        {
                            contents = grid->GetCellValue(row, clmnDiffuse);

                            if (contents != "")
                            {
                                contents += ",\n";
                            }

                            contents += (*group_it).second->p_name;
                            grid->SetCellValue(row, clmnDiffuse, contents);

                            break;
                        }
                        case TGaussianBasisSet::charge:
                        {
                            contents = (*group_it).second->p_name;
                            grid->SetCellValue(row, clmnDFTCharge, contents);
                            break;
                        }
                        case TGaussianBasisSet::exchange:
                        {
                            contents = (*group_it).second->p_name;
                            grid->SetCellValue(row, clmnDFTExchange, contents);
                            break;
                        }
                        case TGaussianBasisSet:: appended_tight:
                        case TGaussianBasisSet:: appended_diffuse:
                        {
                            // dont do anything because we don't want these to show on tbl
                            break;
                        }
                        default:
                        {
                            string msg = "Undefined Basis Type: ";
                            msg += TGaussianBasisSet::gbs_type_formatter[(*group_it).second->p_type];
                            EE_RT_ASSERT(FALSE, EE_WARNING, msg);
                        }
                    }

                    break;
                }
                default:
                {
                    string msg = "Undefined Category: ";
                    msg += TGaussianBasisSet::gbsCategory_formatter[(*group_it).second->p_category];
                    EE_RT_ASSERT(FALSE, EE_WARNING, msg);
                }
            }

            // check ecp too:
            if (group->ecp() != NULL)
            {
                contents = group->ecp()->p_name;
                grid->SetCellValue(row, clmnECP, contents);
            }
        }

//        XtVaSetValues(topTable.getTable(),
  //                          XmNxrtTblContext, XrtTblSetContext(row,XRTTBL_ALL),
    //                        XmNxrtTblPixelHeightContext, XRTTBL_VARIABLE,
      //                      NULL);
        grid->AutoSize();
    }
}

/*
void WxBasisTool::hideColumn(int c)
{
    DebugEnterMethod("hideColumn(int)");
    DebugWriteMessage("c=" + StringConverter::toString(c));
    DebugWriteMessage("vsbl=" + StringConverter::toString(p_gridClmns[c]->isVisible()));

    if (p_gridClmns[c]->isVisible())
    {
        int postn = p_gridClmns[c]->getPosition();
        DebugWriteMessage("postn=" + StringConverter::toString(postn));

        p_gridClmns[c]->setVisible(false);

        for (int i = 0; i < 3; i++)
        {
            p_contextBasisSetsGrid[i]->DeleteCols(postn);
        }

        c++;

        while (c < 7)
        {

            if (p_gridClmns[c]->isVisible())
                p_gridClmns[c]->decrementPosition();

            c++;
        }

        resizeHack();
    }



//    for (int i = 0; i < 3; i++)
//    {
//        p_moleculeBasesGrid[i]->InsertColumn(;
//    }

    DebugLeaveMethod();
}
*/

void WxBasisTool::setGridColumnVisible(int col, bool vsbl)
{
    for (int i = 0; i < 3; i++)
    {
        p_contextBasisSetsGrid[i]->SetColLabelValue(col, (vsbl ? p_columnLabels[col] : ""));
        p_contextBasisSetsGrid[i]->SetColSize(col, (vsbl ? 30 : 0));
        p_contextBasisSetsGrid[i]->AutoSize();
    }
}

// Description
/*void WxBasisTool::showColumn(int c)
{
    DebugEnterMethod("showColumn(int)");
    DebugWriteMessage("c=" + StringConverter::toString(c));
    DebugWriteMessage("vsbl=" + StringConverter::toString(p_gridClmns[c]->isVisible()));

    if (!p_gridClmns[c]->isVisible())
    {
        //  Beginning at index c, and working towards 0,
        //  find the Find the first column with index <
        int postn = c;
        bool done = false;

        while ((postn > 0) && !done)
        {
            if (p_gridClmns[postn - 1]->isVisible())
            {
                done = true;
            }
            else
            {
                postn--;
            }
        }

        DebugWriteMessage("postn=" + StringConverter::toString(postn));

        for (int i = 0; i < 3; i++)
        {
            p_contextBasisSetsGrid[i]->InsertCols(postn);
            p_contextBasisSetsGrid[i]->SetColLabelValue(postn, p_gridClmns[c]->getCaption());
        }

        p_gridClmns[c]->setVisible(true);
        p_gridClmns[c]->setPosition(postn);

        c++;

        while (c < 7)
        {
            if (p_gridClmns[c]->isVisible())
                p_gridClmns[c]->incrementPosition();

            c++;
        }


        resizeHack();

    }
*/

/*
    p_gridClmns[c]->setVisible(true);


    int numVsbl = 0;

    for (int c = 0; c < 7; c++)
    {
        if (p_gridClmns[c]->isVisible())
            numVsbl++;
    }

    for (int i = 0; i < 3; i++)
    {
        p_moleculeBasesGrid[i]->



    DebugLeaveMethod();
}
*/

/**
 *   The table scroll bar policy is as-needed.  However, they don't
 *   appear when they are needed.  Perhaps this is because they are
 *   inside the tab.
 *
 *   I tried sending a ConfigureNotify to various widgets but none
 *   of this worked.
 *
 *   Setting the size of the bst to its current width/height has
 *   no affect.
 *
 *   Trying to set XmNxrtTblDisplay*ScrollBar (ie the policy)
 *   doesn't work even if you turn on always then back to as needed.
 *
 *   Setting the XmNxrtTblDisplayHorizScrollBar policy to
 *   XRTTBL_DISPSB_ALWAYS and then back to XRTTBL_DISPSB_AS_NEEDED
 *   doesn't help.
 *
 *   Increasing and descrasing the num visible columns doesn't work.
 *   Arg, I give.  make it always visible.
 *
 *   Setting the bst size to width+1 and then back to width works.
 */
//void WxBasisTool::resizeHack()/
//{
/*
//  Dimension width, height, border;
//  Position x,y;

//  XtVaGetValues(wid,XmNx,&x,XmNy,&y,XmNheight,&height,XmNwidth,&width, XmNborderWidth,&border,NULL);
//  XtVaSetValues(wid,XmNwidth,width+1,NULL);


  // This works and is less disruptive than resizing the whole table
  Widget w = getconfigTab();
  int cnt=0;
  XtVaGetValues(w,XmNrightOffset,&cnt,NULL);
  XtVaSetValues(w,XmNrightOffset,cnt+1,NULL);
  XtVaSetValues(w,XmNrightOffset,cnt,NULL);
*/
//}


/**
 *    Send a message to other tools to let them know that the
 *    calculation's bs configuration has changed.
 */
void WxBasisTool::notifyChange()
{
//    cout << "WxBasisTool::notifyChange()" << endl;
    EE_ASSERT(p_tcalc,EE_FATAL,"Calculation missing");

    JMSMessage* msg = newMessage();
    msg->addProperty("url", p_urlCalc->toString());

    // Default is to publish to all subscribers
    publish("ecce_url_basis", *msg);
    delete msg;
}



void WxBasisTool::updateNumbers(TGBSConfig* cnfg, Fragment* frag)
{
    // This buffer is used for the numbers which should be < even
    // in the distant future.
    int n;

    if ((cnfg == NULL) || (frag == NULL))
    {
        p_functionsStatic->SetLabel("--");
        p_primativesStatic->SetLabel("--");
        p_effPrimativesStatic->SetLabel("--");
    }
    else
    {
        TagCountMap* ttcMap = frag->tagCountsSTL();
        //  Method above has created a new object, does not deallocate it.
        //  Deallocation left up to the calling program unit.

        // Display num functions:
        n = cnfg->num_functions(*ttcMap);
        p_functionsStatic->SetLabel(StringConverter::toString(n));

        n = cnfg->num_primitives(*ttcMap);
        p_primativesStatic->SetLabel(StringConverter::toString(n));

        // Display effective num primitives:
        bool genC = false;
        const JCode* code = p_tcalc->application();

        if (code)
            code->get_bool("GeneralContractions", genC);

        n = cnfg->eff_num_primitives(*ttcMap, genC);
        p_effPrimativesStatic->SetLabel(StringConverter::toString(n));

        delete ttcMap;      //  Deallocating object created above
    }
}


/**
 *  Convenience method
 */
void WxBasisTool::updateNumbers(TGBSConfig *cnfg)
{
    Fragment *frag = p_tcalc->fragment();
    this->updateNumbers(cnfg, frag);
}


/**
 *  Convenience method
 */
void WxBasisTool::updateNumbers()
{


    TGBSConfig *cnfg = p_tcalc->gbsConfig();
    Fragment *frag = p_tcalc->fragment();
    this->updateNumbers(cnfg, frag);
}


/**
 *   Save the basis configuration persistently.  This method assumes that
 *   all errors have been checked, and the gbs config is valid to save.
 */
void WxBasisTool::saveContext()
{
//    cout << "WxBasisTool::saveContext()" << endl;

    // change the state of calc to created, since input file
    // must be regenerated:
    p_tcalc->setState(ResourceDescriptor::STATE_CREATED);
//    cout << "\tp_urlCalc=" << p_urlCalc->toString() << endl;

    // persistent save ...
    // (only saving the parts that were potentially modified)
    ICalculation* davCalc =
        dynamic_cast<ICalculation*>(EDSIFactory::getResource(*p_urlCalc));

    if (davCalc->gbsConfig(p_tcalc->gbsConfig(), p_theoryDscpn) &&
        davCalc->setState(p_tcalc->getState()))
    {
        // update undo calc:
        if (p_tcalcUndo != 0)
        {
            delete p_tcalcUndo;
        }

        p_tcalcUndo = new TCalculation(p_tcalc);
         // update interface ...
        showContextChanged(false);
        p_messagesFeedback->setMessage("Configuration saved to calculation.", WxFeedback::INFO);

        ResourceDescriptor::RUNSTATE rs = (ResourceDescriptor::RUNSTATE)p_tcalc->getState();

        if (p_messagesFeedback->getRunState() != rs)
        {
            p_messagesFeedback->setRunState(rs);
        }

        notifyChange();
    }
    else
    {
        p_messagesFeedback->setMessage("Save failed -- Unable to access web server.", WxFeedback::ERROR);
    }
}


/**
 *   Save the current configuration to the calculation.  This method
 *   assumes that the user interface guarentees that it is called only
 *   when there is a valid calculation and the current configuration
 *   has changed.  If any inconsistencies are detected, the save is not
 *   done.
 *
 *   Also, since we decided to let the user have full control over
 *   the Polarization Compoenent radios and the Optimize toggle, we
 *   warn them here if either of these options seem inconsistent with
 *   what we would have done.
 *
 *   A new persistent GBSConfig is created.  Since our working copy
 *   doesn't keep the toggles during user editing, we grab them right
 *   from the interface and set them in GBSConfig.
 */
void WxBasisTool::processSave()
{
//    cout << "WxBasisTool::processSave()" << endl;

    ErrMsg errs;
    TGBSConfig* tcnfg = p_tcalc->gbsConfig();


    // print optimization suggestion message
    if (tcnfg->hasGeneralContractions())
    {
        const JCode* code = p_tcalc->application();
        bool optimizeIndctd;
        code->get_bool("PreferOptimization", optimizeIndctd);
        bool optimizeSlctd = p_optimizeCheckBox->GetValue();

        if (optimizeIndctd && !optimizeSlctd)
        {
            string mesg = "Recommend optimizing";
            p_messagesFeedback->setMessage(mesg);
        }
        else if (optimizeSlctd && !optimizeIndctd)
        {
            string mesg = "Recommend NOT optimizing";
            p_messagesFeedback->setMessage(mesg);
        }
    }

    // Then check that the config is complete
    // Don't allow a save if config has errors:
    if (checkCodeRules(true)) // perform full check
    {
        saveContext();
    }

    errs.flush();
}


void WxBasisTool::processDrop(const string& url)
{
  setContext(new EcceURL(url), cntxtNewCalculation, "", "");
}


void WxBasisTool::saveContext(EcceURL *urlTrgt)
{
//    cout << "WxBasisTool::saveContext(EcceURL *urlTrgt)" << endl;

    Resource *rsrc = EDSIFactory::getResource(urlTrgt->getParent());

    if (rsrc != NULL)
    {
        SFile *ftemp = TempStorage::getTempFile();
        saveToFile(ftemp->path().c_str());
        rsrc = rsrc->createChild(urlTrgt->getFilePathTail(), ftemp);
        delete ftemp;
    }

    if (rsrc == NULL)
    {
        string dscpn = "Save failed: It is likely that you do not have "
                       "permission to write to \n" + urlTrgt->toString();
        p_messagesFeedback->setMessage(dscpn, WxFeedback::ERROR);
    }
    else
    {
        JMSMessage *msg = newMessage();
        msg->addProperty("url", rsrc->getURL().toString());
        publish("ecce_url_created", *msg);
        delete msg;
    }
}


/**
 *   Callback from ExportWin class.  The string represented by file is
 *   assumed to be a valid, writable file at this point.  The
 *   exponenents and coefficients are dumped out.
 */
bool WxBasisTool::saveToFile(const char *fpath)
{
//    cout << "WxBasisTool::saveToFile(const char *fpath)" << endl;

    // Check preconditions:
    TGBSConfig *config = p_tcalc->gbsConfig();
    const JCode *code = p_tcalc->application();
    EE_ASSERT((config != NULL), EE_FATAL, "No config defined!");
    EE_ASSERT((code != NULL), EE_FATAL, "No code defined!");
    EE_ASSERT((fpath != NULL), EE_FATAL, "Can't write to null file!");

    // Dump exponents and coefficients to file:
    ofstream gbs(fpath);
    gbs << config->dump(code->name().c_str(), false);
    gbs.close();

    return true;
}



void WxBasisTool::showErrorMessage(string mesg)
{
    p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
}


void WxBasisTool::updateBasisSets(int index, bool slctFirst)
{
    // convert string vector to proper format

    p_basisSetTypeListBox[index]->Clear();
    vector<string> *symbols = p_elementsTable->getSelectionSymbols();
    vector<const char*> *atoms = new vector<const char*>();

    for (int i = 0; i < symbols->size(); i++)
    {
        atoms->push_back((*symbols)[i].c_str());
    }

    vector<const char*> *names;

    TGaussianBasisSet::GBSType type = p_basisSetTypes[index];
    names = p_gbsFactory->gbsNameList(type, atoms);

    if (names != NULL)
    {
        vector<const char*>::const_iterator it;

        for (it = names->begin(); it != names->end(); it++)
        {
            p_basisSetTypeListBox[index]->Append(_T((char *)(*it)));
        }

//        displayBasisList(p_orbitalBSetsListBox[index], *names);

 //       if (names->size() == 0)
 //       {
 //           p_elementsTable->clearContext();
 //       }

        /*
        if (selectFirst)
        {
            int card=0;
            XtVaGetValues(wList, XmNitemCount,&card,NULL);

            if (card > 0)
            {
                selectBasis(wList,(char*)(*names)[0]);
            }
        }
        */
    }
    else
    {
        string mesg = "Unable to access basis set library";
        p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
    }
}



/**
 *   Invoke GBSConfig::isConsistent() which puts errors on the queue
 *   if any rules are violated.  This method is intended to be called
 *   for checking of ongoing work within the BST.  It is not responsible
 *   for correcting errors.  The method makeConsistent() is similar
 *   but will take necessary action on a GBSConfig to bring it into
 *   conformity.
 *
 *   Currently, I just use the message text from the Error queue.
 *   However, this may not be nice enough and flexible enough for gui
 *   messages.  I left the original code in (before it was moved to
 *   GBSConfig so that I could easily modify this code to do better
 *   error messages.
 *
 *   Note: all these checks are ones that can be done AFTER a basis set
 *   has been added to the config.  Checks that must be done BEFORE insertion,
 *   to determine HOW the insertion must take place, are not reported
 *   in this function.
 */
bool WxBasisTool::checkCodeRules(bool verifyAll)
{
    bool result = true;
    TGBSConfig* config = p_tcalc->gbsConfig();
    const JCode *cap = p_tcalc->application();

    // flush out the message stack before error checking is started
    ErrMsg errs;
    errs.flush();

    bool complete;
    bool consistent;

    complete = GBSRules::isComplete(p_tcalc->fragment(),
                        config, cap, p_tcalc->theory(), verifyAll);

    consistent = GBSRules::isConsistent(p_tcalc->fragment(),
                        config, cap, p_tcalc->theory());

    result = complete && consistent;

    if (errs.count() > 0)
    {
        (void)reportGBSRuleViolations();
    }

    return result;
}


bool WxBasisTool::reportGBSRuleViolations()
{
    // Print out all error messages and/or warning messages:
    // If all errors can be corrected, then return true
    string mesg;
    bool result = true;
    TGBSConfig* config = p_tcalc->gbsConfig();
    const JCode *cap = p_tcalc->application();
    ErrMsg errs;
    string key;

    key = "EE_GBS_GROUPS_EMPTY";
    //  Check group coverage (only used if verifyGroups == true)

    if (errs.check(key))
    {
        p_messagesFeedback->setMessage(errs.messageText(key));
        errs.flush(key);
        result = false;
    }

    key = "EE_GBS_ONE_ORBITAL";     //  Check multiple orbitals

    if (errs.check(key) && p_multAllwd)
    {
        p_messagesFeedback->setMessage(errs.messageText(key));
        result = false;
    }

    errs.flush(key);


    key = "EE_ORBITAL_COVERAGE";    //  Check element coverage

    if (errs.check(key))
    {
        p_messagesFeedback->setMessage(errs.messageText(key));
        errs.flush(key);
        result = false;
    }

    key = "EE_GBS_EXCEED_MAX_LVAL";     //  Check max L value

    if (errs.check(key))
    {
        int lval;
        cap->get_int("MaxLValue", lval);

        mesg = "The maximum L value (";
        mesg += StringConverter::toString(lval);
        mesg += ") for this code would be exceeded.";

        p_messagesFeedback->setMessage(mesg);
        errs.flush(key);
        result = false;
    }

    key = "EE_GBS_EXCEED_MAX_TOTAL_PRIMS";     //  Check Max Primitives

    if (errs.check(key))
    {
        int prmtvs;
        cap->get_int("MaxTotalPrimitives", prmtvs);

        mesg = "The maximum number of Gaussian primitives (";
        mesg += StringConverter::toString(prmtvs);
        mesg += ") for this code would be exceeded.";

        p_messagesFeedback->setMessage(mesg);
        errs.flush(key);
        result = false;
    }

    key = "EE_GBS_EXCEED_MAX_TOTAL_FUNCS";     //  Check Max Functions

    if (errs.check(key))
    {
        int fnctns;
        cap->get_int("MaxTotalFunctions", fnctns);

        mesg = "The maximum number of basis functions (";
        mesg += StringConverter::toString(fnctns);
        mesg += ") for this code would be exceeded.";

        p_messagesFeedback->setMessage(mesg);
        errs.flush(key);
        result = false;
    }

    key = "EE_GBS_EXCEED_MAX_PRIMS";     //  Check Max Prims per contraction

    while (errs.check(key))
    {
        string dscpn;
        int shell, prmtvs;

        dscpn = errs.messageText(key);
        sscanf(dscpn.c_str(), "%d %d", &shell, &prmtvs);

        mesg = "The maximum number of ";
        mesg += StringConverter::toString(TGaussianBasisSet::shell_formatter[shell]);
        mesg += " primitives per contracted basis functions (";
        mesg += StringConverter::toString(prmtvs);
        mesg += ")  for this code would be exceeded.";

        p_messagesFeedback->setMessage(mesg);
        errs.flush(key);
        result = false;
    }

    key = "EE_GBS_ECP";     //  Check ECPS - design change.
                            //  We now clear the ecp portion but
                            //      leave the ECP orbital in place.
    if (errs.check(key))
    {
        mesg = "The code does not support ECPs.  They ";
        mesg += "have been removed from the configuration.";
        p_messagesFeedback->setMessage(mesg);

        errs.flush(key);
        config->clearType(TGaussianBasisSet::ecp);
        showContextChanged(true);
        result = result && true;
    }

    key = "EE_GBS_DFTFITTING";     //  Check DFT support by code and theory

    if (errs.check(key))
    {
        mesg = "The code or theory does not support the addition of fitting";
        mesg += " sets.  They have been removed from the configuration.";
        p_messagesFeedback->setMessage(mesg);

        errs.flush("EE_GBS_DFTFITTING");
        config->clearType(TGaussianBasisSet::exchange);
        config->clearType(TGaussianBasisSet::charge);
        result = result && true;
    }

    key = "EE_GBS_DFTFITTING_THEORY";

    if (errs.check(key))
    {
        mesg = "The code or theory does not support the addition of fitting";
        mesg += " sets.  They have been removed from the configuration.";
        p_messagesFeedback->setMessage(mesg);

        config->clearType(TGaussianBasisSet::exchange);
        config->clearType(TGaussianBasisSet::charge);

        errs.flush(key);
        showContextChanged(true);
        result = result && true;
    }

    key = "EE_GBS_DFTFIT_ALL";
    //  Check DFT coverage (used only if verifyGroups == true)

    if (errs.check(key))
    {
        p_messagesFeedback->setMessage(errs.messageText(key));
        errs.flush(key);
        result = false;
    }

    key = "EE_GBS_SPHERICAL";       //  Check Spherical by theory

    if (errs.check(key))
    {
        p_messagesFeedback->setMessage(errs.messageText(key));

        errs.flush(key);

        config->coordsys(TGaussianBasisSet::Cartesian);

        bool prevState = p_inCtrlUpdate;
        p_inCtrlUpdate = true;
        p_polarizationRadioButton[TGaussianBasisSet::Cartesian]->SetValue(true);

        showContextChanged(true);
        p_inCtrlUpdate = prevState;


        result = result && true;
    }

    return result;
}


/**
 *   It is assmumed that if it is possible to have unsaved state, then
 *   the only two feedback states we care about are modified and edit.
 *   In all cases, the current status is checked first to minimize
 *   unnecesary updates.  This is done so that this function may be
 *   called every time any change is made.  This method safely does
 *   nothing if the BST is in browse mode.
 *
 *   Both the menu bar save button and the save icon are handled.
 */
/*void WxBasisTool::indicateContexSaveControls(bool state)
{
     p_programSaveMenuItem->Enable(state);
     p_calcChgd = state;

     WxFeedback::EditStatus status;

     status = (state ? WxFeedback::MODIFIED : WxFeedback::EDIT);

//     FeedbackSaveHandler *handler = p_messagesFeedback->getSaveHandler();

     p_messagesFeedback->setEditStatus(WxFeedback::MODIFIED);
 }
*/


void WxBasisTool::showContextChanged(bool chgd)
{
  p_calcChgd = chgd;
  if (p_messagesFeedback->getEditStatus() != WxFeedback::READONLY) {
    p_messagesFeedback->setEditStatus(chgd ? WxFeedback::MODIFIED :
                                             WxFeedback::EDIT);
    p_programSaveMenuItem->Enable(chgd);
  } else {
    p_programSaveMenuItem->Enable(false);
  }
  /*
    if (p_calcChgd != chgd)
    {
        WxFeedback::EditStatus status
                = (p_calcChgd ? WxFeedback::MODIFIED : WxFeedback::EDIT);
        p_messagesFeedback->setEditStatus(status);
        p_programSaveMenuItem->Enable(p_calcChgd);
    }
  */
}


/**
 *  Shows the elements that are supported by the given basis set in
 *  the periodic table.
 */
void WxBasisTool::showCoverage(string bsName,
                               TGaussianBasisSet::GBSType bsType)
{
    p_elementsTable->resetMarks();

    if (bsName.size() > 0)
    {
        vector<const char*>* symbols =
                      p_gbsFactory->atoms(bsName.c_str(), bsType);
        p_elementsTable->showMarks(symbols);
        delete symbols;
    }

    p_elementsTable->Refresh();
 }
