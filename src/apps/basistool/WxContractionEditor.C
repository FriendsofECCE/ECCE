#include <iostream>
//    using std::cout;
//    using std::endl;
#include <string>
    using std::string;
#include <set>
    using std::set;
    using std::less;
#include <vector>

#include "util/Color.H"
#include "util/ErrMsg.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "dsm/TGaussianBasisSet.H"

#include "wxgui/ewxChoice.H"
#include "wxgui/ewxGrid.H"

#include "WxContractionEditor.H"
#include "WxBasisTool.H"


          // Contraction Table
   enum ContractionColumns { clmnCntrcSet,
                             clmnCntrcDesc,
                             clmnCntrcUncontract,
                             clmnCntrcDelete };

   // Summary Table

enum SummaryColumns { clmnSmmryLValue,
                      clmnSmmryPrim,
                      clmnSmmryCont,
                      clmnSmmryTight,
                      clmnSmmryDiffuse };

static string SummaryColumnLabels[] = { "L", "#Prim", "#Cont", "Tight", "Diffuse" };


IMPLEMENT_CLASS( WxContractionEditor, WxContractionEditorGUI )


WxContractionEditor::WxContractionEditor(      wxWindow* parent,
                                         wxWindowID id,
                                   const wxString& caption,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                         long style)
             : WxContractionEditorGUI(parent, id, caption, pos, size, style)
{
//    DebugEnterMethod("WxContractionEditor(wxWindow*, wxWindowID, const wxString&, const wxPoint&, const wxSize&, long)");

    p_prntWindow = (wxWindow *)(parent);

//    p_cnfgElmts = new vector<string>;
    p_inCtrlUpdate = false;
    p_elmtSlctnIdx = -1;

    p_cntrcGridModels = new vector<GBSContractionGridModel *>;
    p_smmryGridModels = new vector<GBSSummaryGridModel *>;


    initControls();

//    p_elmtCount = 1;


//    DebugLeaveMethod();

}


WxContractionEditor::~WxContractionEditor()
{
//    DebugEnterMethod("~WxContractionEditor()");
//    DebugLeaveMethod();
}


void WxContractionEditor::initControls()
{
//    DebugEnterMethod("initControls()");
    long itemID;

    itemID = ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS;
    p_elmtsChoice = (ewxChoice *)(FindWindowById(itemID));

    itemID = ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS;
    p_cntrctsGrid = (ewxGrid *)(FindWindowById(itemID));

    itemID = ID_GRID_WXCONTRACTIONEDITOR_SUMMARY;
    p_summaryGrid = (ewxGrid *)(FindWindowById(itemID));

    itemID = ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS;
    p_diffFnctnsEdit = (WxParameterEdit *)(FindWindowById(itemID));

    int col;
    wxColour bkgd;

    for (col = 0; col < 4; col++)
    {
        p_cntrctsClmnAttrbs[col] = new wxGridCellAttr();
    }

    p_cntrctsGrid->SetColLabelValue(0, "Contraction Set");
    p_cntrctsGrid->SetColLabelValue(1, "Contraction");
    p_cntrctsGrid->SetColLabelValue(2, "Uncontract");
    p_cntrctsGrid->SetColLabelValue(3, "Delete");

    p_cntrctsClmnAttrbs[0]->SetReadOnly();
    p_cntrctsClmnAttrbs[0]->SetBackgroundColour(ewxStyledWindow::getButtonColor());

    p_cntrctsClmnAttrbs[1]->SetReadOnly();
    p_cntrctsClmnAttrbs[1]->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());

    p_cntrctsClmnAttrbs[2]->SetRenderer(new wxGridCellBoolRenderer());
    p_cntrctsClmnAttrbs[2]->SetEditor(new wxGridCellBoolEditor());

    p_cntrctsClmnAttrbs[3]->SetRenderer(new wxGridCellBoolRenderer());
    p_cntrctsClmnAttrbs[3]->SetEditor(new wxGridCellBoolEditor());

    for (col = 0; col <= 3; col++)
    {
        p_cntrctsGrid->SetColAttr(col, p_cntrctsClmnAttrbs[col]);
    }

    for (col = 0; col <= 4; col++)
    {
        p_summaryClmnAttrbs[col] = new wxGridCellAttr();
    }

    p_summaryClmnAttrbs[0]->SetReadOnly();
    p_summaryClmnAttrbs[0]->SetBackgroundColour(ewxStyledWindow::getWindowColor());

    p_summaryClmnAttrbs[1]->SetReadOnly();
    p_summaryClmnAttrbs[1]->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());

    p_summaryClmnAttrbs[2]->SetReadOnly();
    p_summaryClmnAttrbs[2]->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());

    p_summaryClmnAttrbs[3]->SetReadOnly(false);
    p_summaryClmnAttrbs[3]->SetBackgroundColour(ewxStyledWindow::getInputColor());
    p_summaryClmnAttrbs[3]->SetEditor(new wxGridCellNumberEditor(0, 10000));
    p_summaryClmnAttrbs[3]->SetRenderer(new wxGridCellNumberRenderer());

    p_summaryClmnAttrbs[4]->SetReadOnly(false);
    p_summaryClmnAttrbs[4]->SetBackgroundColour(ewxStyledWindow::getInputColor());
    p_summaryClmnAttrbs[4]->SetEditor(new wxGridCellNumberEditor(0, 10000));
    p_summaryClmnAttrbs[4]->SetRenderer(new wxGridCellNumberRenderer());

    GBSSummaryGridItem::ItemField field;
    string label;

    for (col = 0; col < 5; col++)
    {
        if (col == 0)
        {
            label = "L";
        }
        else
        {
            field = (GBSSummaryGridItem::ItemField)(col - 1);
            label = GBSSummaryGridModel::getFieldLabel(field);
        }

        p_summaryGrid->SetColLabelValue(col, label);
        p_summaryGrid->SetColAttr(col, p_summaryClmnAttrbs[col]);
    }

//    DebugLeaveMethod();
}

void WxContractionEditor::saveSettings(Preferences *prefs)
{
    const string topic = "CONTRACTIONEDITOR";

    wxRect r = this->GetRect();
    prefs->setInt(topic + ".LEFT", r.GetLeft());
    prefs->setInt(topic + ".TOP", r.GetTop());

    wxSize size = this->GetSize();
    prefs->setInt(topic + ".WIDTH", size.GetWidth());
    prefs->setInt(topic + ".HEIGHT", size.GetHeight());

    prefs->saveFile();
}


void WxContractionEditor::loadSettings(Preferences *prefs)
{
    int x = WXCONTRACTIONEDITOR_WINDOW_DFLTLEFT;
    int y = WXCONTRACTIONEDITOR_WINDOW_DFLTTOP;
    int w = WXCONTRACTIONEDITOR_WINDOW_DFLTWIDTH;
    int h = WXCONTRACTIONEDITOR_WINDOW_DFLTHEIGHT;

    const string topic = "CONTRACTIONEDITOR";
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


void WxContractionEditor::setConfiguration(TGBSConfig* cnfg)
{
//    DebugEnterMethod("setConfiguration(TGBSConfig)");
    p_gbscnfg = cnfg;
    set<string, less<string> > *elmts = p_gbscnfg->elements();
    set<string, less<string> >::iterator it;

    p_elmtsChoice->Clear();
    p_cntrcGridModels->clear();
    p_smmryGridModels->clear();

    GBSSummaryGridModel         *mdlSmmry;
    GBSContractionGridModel     *mdlCntrc;

    for (it = elmts->begin(); it != elmts->end(); it++)
    {
        string symbol = *it;

        if (symbol.length() > 0)
        {
            p_elmtsChoice->Append(*it);

            mdlCntrc = new GBSContractionGridModel(p_gbscnfg, symbol);
            mdlSmmry = new GBSSummaryGridModel(p_gbscnfg, symbol);

            vector<int>* tightF= p_gbscnfg->getTightFunctionCount(symbol);
            vector<int>* diffF = p_gbscnfg->getDiffuseFunctionCount(symbol);

            for (int r = 0; r < mdlSmmry->size(); r++)
            {
                int totalFrom = mdlCntrc->getTotalFrom(r);
                int totalTo = mdlCntrc->getTotalTo(r);
                mdlSmmry->setItemFieldValue(r, GBSSummaryGridItem::PRIMARY, totalFrom);
                mdlSmmry->setItemFieldValue(r, GBSSummaryGridItem::CONTRACTED, totalTo);

                if ((tightF != NULL) && (diffF != NULL))
                {
                    if (totalFrom > 0)
                    {
                        int tightValue = (*tightF)[r];
                        int diffValue = (*diffF)[r];

                        if (tightValue > 0)
                        {
                            mdlSmmry->setItemFieldValue(r, GBSSummaryGridItem::TIGHT, tightValue);
                        }

                        if (diffValue == 0)
                        {
                            // no diffuse is set yet, but a blanket diffuse is assigned for
                            // all shells
                            diffValue = p_diffFnctnsEdit->getValue();

                            // now add them to the config:
                            TGBSGroup* group = p_gbscnfg->findGroup(symbol);

                            if ((group == 0) || !(group->addDiffuse(symbol, r, diffValue)))
                            {
                                diffValue = 0;
                            }
                        }

                        if (diffValue > 0)
                        {
                            mdlSmmry->setItemFieldValue(r, GBSSummaryGridItem::DIFFUSE, diffValue);
                        }
                    }
                }
            }

            p_cntrcGridModels->push_back(mdlCntrc);
            p_smmryGridModels->push_back(mdlSmmry);
        }
    }

//    DebugLeaveMethod();
}


void WxContractionEditor::elementsChoiceSelectedCB(wxCommandEvent& event)
{
//    DebugEnterMethod("elementsChoiceSelectedCB(wxCommandEvent&)");

    if (!p_inCtrlUpdate)
    {
        int k = p_elmtsChoice->GetSelection();

        p_elmtSlctnIdx = k;
        updateTables();

    }

//    DebugLeaveMethod();
}


void WxContractionEditor::contractGridCellChangedCB(wxGridEvent& event)
{
//    DebugEnterMethod("contractGridCellChangedCB(wxGridEvent&)");

    int row, col;
    string value;
    bool slctd;
    int shellIdx;

    row = event.GetRow();
    col = event.GetCol();
//    value = p_cntrctsGrid->GetCellValue(row, col);
//    slctd = (value.length() > 0);
//    value = p_cntrctsGrid->GetCellValue(row, col);
    slctd = event.IsSelection();


    string mesg = "row=" + StringConverter::toString(row)
                         + "; col=" + StringConverter::toString(col)
//                         + "; value=" + value
                         + "; slctd=" + StringConverter::toString(slctd);
//    DebugWriteMessage(mesg);


    // find out what element
    string symbol = getSelectionSymbol();
    bool recurse = (col == 3);

    int k = p_elmtsChoice->GetSelection();
    GBSContractionGridModel *mdlCntrc = (*p_cntrcGridModels)[k];

    if (col == 2)
        mdlCntrc->toggleItemUncontracted(row);
    else if (col == 3)
        mdlCntrc->toggleItemDeleted(row);

    shellIdx = mdlCntrc->getItemShellIndex(row);

    // Now update the summary table
    updateSummaryTable(symbol, shellIdx, recurse);
    event.Skip();

//    DebugLeaveMethod();
}


void WxContractionEditor::summaryGridCellChangedCB(wxGridEvent& event)
{
//    DebugEnterMethod("summaryGridCellChangedCB(wxGridEvent&)");

    int row, col;
    long lval;
    int ival = 0;

    row = event.GetRow();
    col = event.GetCol();

    wxString s = p_summaryGrid->GetCellValue(row, col);

    if (s.ToLong(&lval))
        ival = (int)lval;

    updateSummaryValue(row, col, ival);

    event.Skip();
//    DebugLeaveMethod();
}


void WxContractionEditor::contractGridCellSelectedCB(wxGridEvent& event)
{
//    DebugEnterMethod("contractGridCellSelectedCB(wxGridEvent&)");

    if (!p_inCtrlUpdate)
    {
        int row, col;

        row = event.GetRow();
        col = event.GetCol();

        string dscpn = "row=" + StringConverter::toString(row) + ", col=" + StringConverter::toString(col);

        bool editable = !p_cntrctsGrid->IsReadOnly(row, col);    //->CanEnableCellControl();
        dscpn += "; editable=" + StringConverter::toString(editable);

        if (editable)
        {
            wxGridCellEditor *ctrl = p_cntrctsGrid->GetCellEditor(row, col);
            bool isNULL = (ctrl == NULL);

            dscpn += "; editor NULL?  " + StringConverter::toString(isNULL);
        }

//        DebugWriteMessage(dscpn);
    }

    event.Skip();
//    DebugLeaveMethod();
}


void WxContractionEditor::summaryGridCellSelectedCB( wxGridEvent& event )
{
//    DebugEnterMethod("summaryGridCellSelectedCB(wxGridEvent&)");

    if (!p_inCtrlUpdate)
    {
        int row, col;

        row = event.GetRow();
        col = event.GetCol();

        string dscpn = "row=" + StringConverter::toString(row) + ", col=" + StringConverter::toString(col);

        bool editable = !p_cntrctsGrid->IsReadOnly(row, col);    //->CanEnableCellControl();
        dscpn += "; editable=" + StringConverter::toString(editable);


        if (editable)
        {
            wxGridCellEditor *ctrl = p_cntrctsGrid->GetCellEditor(row, col);
            bool isNULL = (ctrl == NULL);

            dscpn += "; editor NULL?  " + StringConverter::toString(isNULL);
        }

//        DebugWriteMessage(dscpn);
    }

    event.Skip();
//    DebugLeaveMethod();
}



/**
 * Responds to user changing a delete/uncontract tgl setting by recomputing
 * values in the summary table.
 */
void WxContractionEditor::contractGridEditorHiddenCB(wxGridEvent& event)
{
//    DebugEnterMethod("contractGridEditorHiddenCB(wxGridEvent&)");

    event.Skip();

//    DebugLeaveMethod();
}


void WxContractionEditor::summaryGridEditorHiddenCB(wxGridEvent& event)
{
//    DebugEnterMethod("summaryGridEditorHiddenCB(wxGridEvent&)");

    event.Skip();

//    DebugLeaveMethod();
}


void WxContractionEditor::closeButtonClickCB(wxCommandEvent& event)
{
//    DebugEnterMethod("closeButtonClickCB(wxGridEvent&)");
    this->Close();
//    DebugLeaveMethod();
}



/**
 *  Programmatically select different element.  Redisplay both tables.
 */
void WxContractionEditor::selectElement(int idx)
{
//    DebugEnterMethod("selectElement(int)");

        p_inCtrlUpdate = true;
        p_elmtsChoice->SetSelection(idx);
        p_elmtSlctnIdx = idx;

        updateTables();
        p_inCtrlUpdate = false;

//    DebugLeaveMethod();
}


string WxContractionEditor::getSelectionSymbol()
{
//    DebugEnterMethod("getSelectionSymbol()");

    string symbol = (string)(p_elmtsChoice->GetStringSelection());

//    DebugLeaveMethod();

    return symbol;
}


void WxContractionEditor::updateTables()
{
//    DebugEnterMethod("WxContractionEditor::updateTables()");

    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    GBSSummaryGridModel *mdlSmmry = (*p_smmryGridModels)[p_elmtSlctnIdx];
    GBSContractionGridModel *mdlCntrc = (*p_cntrcGridModels)[p_elmtSlctnIdx];


    int numRows;

    p_cntrctsGrid->ClearGrid();         //  aka ctable
    p_summaryGrid->ClearGrid();         //  aka stable

    numRows = p_cntrctsGrid->GetNumberRows();
    p_cntrctsGrid->DeleteRows(0, numRows);

    numRows = p_summaryGrid->GetNumberRows();
    p_summaryGrid->DeleteRows(0, numRows);


    int count = mdlCntrc->size();

    for (int r = 0; r < count; r++)
    {
        p_cntrctsGrid->AppendRows();

        string gbsname = mdlCntrc->getItemBasisSetName(r);
        p_cntrctsGrid->SetCellValue(r, clmnCntrcSet, gbsname);

        string cntrctn = mdlCntrc->getItemContraction(r);
        p_cntrctsGrid->SetCellValue(r, clmnCntrcDesc, cntrctn);

        bool v;

        if (mdlCntrc->getItemUncontractAllowed(r))
        {
            v = mdlCntrc->isItemUncontracted(r);
            p_cntrctsGrid->SetCellValue(r, 2, (v ? "1" : ""));
        }
        else
        {
            p_cntrctsGrid->GetTable()->SetValue(r, 2, "");
            wxGridCellAttr *attr = p_cntrctsGrid->GetOrCreateCellAttr(r, 2);
            attr->SetRenderer(new wxGridCellStringRenderer());
            attr->SetReadOnly();
            attr->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());
        }

        v = mdlCntrc->isItemDeleted(r);
        p_cntrctsGrid->SetCellValue(r, 3, (v ? "1" : ""));
    }

    p_cntrctsGrid->AutoSizeColumns();
    p_cntrctsGrid->ForceRefresh();

    count = mdlSmmry->size();
    string s;

    // now display the tight and diffuse numbers
    for (int r = 0; r < count; r++)
    {
        p_summaryGrid->AppendRows();

        string shell(TGaussianBasisSet::shell_formatter[r]);
        STLUtil::toLower(shell);
        p_summaryGrid->SetCellValue(r, 0, shell);

        s = mdlSmmry->formatItemField(r, GBSSummaryGridItem::PRIMARY);
        p_summaryGrid->SetCellValue(r, 1, s);

        s = mdlSmmry->formatItemField(r, GBSSummaryGridItem::CONTRACTED);
        p_summaryGrid->SetCellValue(r, 2, s);

        s = mdlSmmry->formatItemField(r, GBSSummaryGridItem::TIGHT);
        p_summaryGrid->SetCellValue(r, 3, s);

        s = mdlSmmry->formatItemField(r, GBSSummaryGridItem::DIFFUSE);
        p_summaryGrid->SetCellValue(r, 4, s);
    }

    p_summaryGrid->AutoSizeColumns();
    p_summaryGrid->ForceRefresh();

    p_inCtrlUpdate = prevState;

//    DebugLeaveMethod();
}


void WxContractionEditor::updateSummaryTable(const string& symbol,
                                             const int& shell,
                                             const bool& recurse)
{
//    DebugEnterMethod("updateSummaryTable(" + symbol + ", " + StringConverter::toString(shell) + ", " + StringConverter::toString(recurse) + ")");

    string tmp;
    int value;
    int endShell;
    WxBasisTool *prnt = (WxBasisTool *)(p_prntWindow);

    int k = p_elmtsChoice->GetSelection();
    GBSSummaryGridModel *mdlSmmry = (*p_smmryGridModels)[k];

    // Decide whether to fix all the shells downward (this only happens
    // if deleting a shell's contraction
    if (recurse)
    {
        endShell = TGaussianBasisSet::maxShell;
    }
    else
    {
        endShell = shell;
    }

    // First update the tight and diffuse functions in the config:
    TGBSGroup* group = p_gbscnfg->findGroup(symbol);

    if (group != 0)
    {
        vector<int> from;
        vector<int> to;
        vector<bool> hasRawData;

        // have to update every subsequent shell because their diffuse
        // functions may be affected

        // Do this just one time
        p_gbscnfg->hasContractionInfo(hasRawData, symbol);

        bool wrkd;
        string s;

        if (endShell > mdlSmmry->size())
            endShell = mdlSmmry->size();

        for (int shellIdx = 0; shellIdx <= endShell; shellIdx++)
        {
            value = mdlSmmry->getItemFieldValue(shellIdx, GBSSummaryGridItem::TIGHT);
            wrkd = group->addTight(symbol, shellIdx, value);

            if (wrkd)
            {
                mdlSmmry->setItemFieldValue(shellIdx, GBSSummaryGridItem::TIGHT, value);
            }
            else
            {
                //      p_prntTool->msgContractionError();
            }

            s = "";

            if (wrkd && (value != 0))
                s = StringConverter::toString(value);

            p_summaryGrid->SetCellValue(shellIdx, 3, s);

            value = mdlSmmry->getItemFieldValue(shellIdx, GBSSummaryGridItem::DIFFUSE);
            wrkd = group->addDiffuse(symbol, shellIdx, value);

            if (wrkd)
            {
                mdlSmmry->setItemFieldValue(shellIdx, GBSSummaryGridItem::DIFFUSE, value);
            }

            s = "";

            if (wrkd && (value != 0))
                s = StringConverter::toString(value);

            p_summaryGrid->SetCellValue(shellIdx, 4, s);

            p_gbscnfg->contractionInfo(from, to, symbol);

            mdlSmmry->setItemFieldValue(shellIdx, GBSSummaryGridItem::PRIMARY, from[shellIdx]);
            mdlSmmry->setItemFieldValue(shellIdx, GBSSummaryGridItem::CONTRACTED, to[shellIdx]);

            value = mdlSmmry->getItemFieldValue(shellIdx, GBSSummaryGridItem::PRIMARY);
            p_summaryGrid->SetCellValue(shellIdx, 1, StringConverter::toString(value));

            value = mdlSmmry->getItemFieldValue(shellIdx, GBSSummaryGridItem::CONTRACTED);
            p_summaryGrid->SetCellValue(shellIdx, 2, StringConverter::toString(value));
        }

        prnt->updateNumbers();
        prnt->showContextChanged(true);
    }

//    DebugLeaveMethod();
}


/**
 * Fill in all diffuse columns with the specified value.
 * As a side affect, the summary table must be recomputed.
 */
/*CCB*/
void WxContractionEditor::diffuseParamEditValueChangedCB(wxCommandEvent& event) //(Widget, XtPointer, XtPointer)
{
//    DebugEnterMethod("diffuseParamEditValueChangedCB()");

    int value = p_diffFnctnsEdit->getValue();

    int k = p_elmtsChoice->GetSelection();
    GBSSummaryGridModel *mdlSmmry = (*p_smmryGridModels)[k];

    int numShells = mdlSmmry->size();
//    DebugWriteMessage("numShells=" + StringConverter::toString(numShells));

    for (int shellIdx = 0; shellIdx < numShells; shellIdx++)
    {
        if (mdlSmmry->getItemFieldValue(shellIdx, GBSSummaryGridItem::PRIMARY) > 0)
        {
            mdlSmmry->setItemFieldValue(shellIdx, GBSSummaryGridItem::DIFFUSE, value);
            updateSummaryValue(shellIdx, GBSSummaryGridItem::DIFFUSE, value);
        }
    }

 //   DebugLeaveMethod();
}


void WxContractionEditor::updateSummaryValue(const int& row,
                                             const int& col,
                                             const int& value)
{
//    DebugEnterMethod("updateSummaryValue(" + StringConverter::toString(row) + ", " + StringConverter::toString(col) + ", " + StringConverter::toString(value) + ")");


    // find out what element
    string symbol = getSelectionSymbol();

    // find out what shell
    int shellIdx = row;

    // find out what group
    TGBSGroup* group = p_gbscnfg->findGroup(symbol);

    if (group != NULL)
    {
        // now update the summary table:
        updateSummaryTable(symbol, shellIdx, false);
    }

//    DebugLeaveMethod();
}

