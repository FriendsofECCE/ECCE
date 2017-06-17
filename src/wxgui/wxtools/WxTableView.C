/**
 * @file
 * @author Ken Swanson
 *
 *  Visual element, based on the wxListCtrl class, that can be used
 *  to display tabular data.
 *
 *  Columns can be resized, and the table sorted by clicking on the
 *  column headers.  If the header of the current sort column is clicked,
 *  the sort order is reversed.  If that of a different column is clicked,
 *  the elements are sorted on the new column, with no changes to the sort
 *  order.  Row colors can also be set.
 *
 *  An application that uses WxTableView must define a class that
 *  derives from TableElement, and implement its virtual methods.
 *  Instances of the table elements are provided to the TableModel.
 *  The fields of the TableElement class map directly to columns in the
 *  table.
 *
 *  Changes to the model may be made at any time.  A call to the refresh
 *  method on the TableView will make any changes visible within the view
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <stdlib.h>
#include <unistd.h>

#include "wxgui/ewxBitmap.H"
#include "wxgui/WxTableView.H"
#include "wxgui/TableElement.H"

#include "util/StringConverter.H"

#include "wx/image.h"
#include "wx/colour.h"

IMPLEMENT_CLASS(WxTableView, wxWindow)

BEGIN_EVENT_TABLE(WxTableView, wxEvtHandler)
    EVT_SIZE(WxTableView::windowResizeCB)
    EVT_LIST_COL_BEGIN_DRAG(ID_LISTVIEW_ELEMENTS, WxTableView::elementsListCtrlColumnBeginDragCB)
    EVT_LIST_COL_END_DRAG(ID_LISTVIEW_ELEMENTS, WxTableView::elementsListCtrlColumnEndDragCB)
    EVT_LIST_COL_CLICK(ID_LISTVIEW_ELEMENTS, WxTableView::elementsListCtrlColumnLeftClickCB)
    EVT_LIST_ITEM_SELECTED(ID_LISTVIEW_ELEMENTS, WxTableView::elementsListCtrlItemSelectedCB)
    EVT_LIST_ITEM_ACTIVATED(ID_LISTVIEW_ELEMENTS, WxTableView::elementsListCtrlItemActivatedCB)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(ewxEVT_TABLE_SELECTION_CHANGED)
DEFINE_EVENT_TYPE(ewxEVT_TABLE_ELEMENT_ACTIVATED)

/**
 *  Callback function for the underlying wxListCtrl.
 */
int wxCALLBACK compareElements(long k1, long k2, long sortData)
{
    TableElement *elmnt1, *elmnt2;
    DefaultTableModel* mdl;

    mdl = (DefaultTableModel*)(sortData);

    elmnt1 = (TableElement*)(mdl->elementAt((int)(k1)));
    elmnt2 = (TableElement*)(mdl->elementAt((int)(k2)));

    return (mdl->compare(elmnt1, elmnt2));
}

/**
 *  Constructor for the TableView.
 *
 *  Normally only the parent window (such as a panel) and a numeric identifier
 *  need to be provided.
 *  The TableView creates an instance of DefaultTableModel.  In practice,
 *  an application would replace this with an instance that is more appropriate
 *  for the application's purpose.
 */
WxTableView::WxTableView(      wxWindow* parent,
                               wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                               long style,
                         const wxString& name) : wxWindow(parent, id, pos, size, style, name)
{
    p_viewFilter = NULL;
    p_dataModel = NULL;
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);


    p_listView = new wxListCtrl(this, ID_LISTVIEW_ELEMENTS,
                                wxDefaultPosition,  wxDefaultSize,
                                wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES|wxSUNKEN_BORDER|wxTAB_TRAVERSAL );


    sizer->Add(p_listView, 1, wxGROW, 0);
    wxColour c((char)(100), (char)(0), (char)(100));
    this->SetOwnBackgroundColour(c);
    this->SetSizer(sizer);

    wxColour mask((char)(255), (char)(0), (char)(0));
    p_itemImageCount = 2;
    p_ctrlImages = new wxImageList(16, 16, true);

    p_ctrlImages->Add(ewxBitmap(_T("empty16rmask.xpm"), wxBITMAP_TYPE_XPM), mask);
    p_ctrlImages->Add(ewxBitmap(_T("check16rmask.xpm"), wxBITMAP_TYPE_XPM), mask);
    p_ctrlImages->Add(ewxBitmap(_T("down16rmask.xpm"), wxBITMAP_TYPE_XPM), mask);
    p_ctrlImages->Add(ewxBitmap(_T("up16rmask.xpm"), wxBITMAP_TYPE_XPM), mask);


    p_listView->SetImageList(p_ctrlImages, wxIMAGE_LIST_SMALL);
    this->Layout();
    this->Fit();

    p_initWidth = p_listView->GetClientSize().GetWidth();
}

/**
 *
 *  Destructor for the TableView.  Also deletes the data model.
 */
WxTableView::~WxTableView()
{
    delete p_listView;
    delete p_ctrlImages;
}


void WxTableView::elementsListCtrlItemSelectedCB(wxListEvent& event)
{
    int itemIdx = event.GetIndex();
    int k;


    k = p_listView->GetItemData(itemIdx);

    if (k != p_slctElmntIdx)
    {
        p_slctElmntIdx = k;
        this->fireSelectionChanged();
    }
}

wxSize WxTableView::DoGetBestSize() const
{
    wxSize size(150, 75);

    return size;
}


void WxTableView::fireSelectionChanged()
{
    wxCommandEvent event(ewxEVT_TABLE_SELECTION_CHANGED, GetId());

    event.SetEventObject( this );
    GetEventHandler()->ProcessEvent( event );
}


void WxTableView::fireElementActivated()
{
    wxCommandEvent event(ewxEVT_TABLE_ELEMENT_ACTIVATED, GetId());

    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}


/**
 *  Returns the data model
 */
DefaultTableModel* WxTableView::getModel()
{
    return p_dataModel;
}


/**
 *  Sets the reference of the data model to the provided model
 */
void WxTableView::setModel(DefaultTableModel *model)
{
    TableColumn *clmn = NULL;
    string text;
    int align;
    wxListItem item;

    p_dataModel = model;

    int numCols = p_dataModel->getColumnCount();

    for (int j = 0; j < numCols; j++)
    {
        if (p_dataModel->isColumnVisible(j))
        {
            clmn = p_dataModel->getColumn(j);
            text = clmn->getLabel();
            align = clmn->getTextAlignment();
            p_listView->InsertColumn(j, (wxString)(text), align);
            p_listView->SetColumnWidth(j, clmn->getPreferredWidth());
        }
    }
}


void WxTableView::setFilter(TableViewFilter *filter)
{
    p_viewFilter = filter;
}


TableViewFilter *WxTableView::getFilter()
{
    return p_viewFilter;
}


TableElement* WxTableView::getSelectedElement()
{
    TableElement *elmnt = 0;

    if (p_slctElmntIdx >= 0)
    {
        elmnt = p_dataModel->elementAt(p_slctElmntIdx);
    }

    return elmnt;
}


/**
 *  Event handler for mouse clicks on the column headers
 */
void WxTableView::elementsListCtrlColumnLeftClickCB(wxListEvent& event)
{

    int col;
    wxListItem *clmnItem;
    int sortClmn;
    bool sortDesc;

    sortClmn = p_dataModel->getSortColumn();
    sortDesc = p_dataModel->getSortDescending();
    col = event.GetColumn();

    if (col == sortClmn)
    {
        p_dataModel->toggleSortDescending();
        sortDesc = !sortDesc;

    }
    else
    {
        clmnItem = new wxListItem();
        p_listView->GetColumn(sortClmn, *clmnItem);
        clmnItem->SetImage(-1);
        clmnItem->SetMask(wxLIST_MASK_IMAGE);
        p_listView->SetColumn(sortClmn, *clmnItem);

        p_dataModel->setSortColumn(col);
        sortClmn = col;
    }

    this->refresh();
}


void WxTableView::elementsListCtrlItemActivatedCB(wxListEvent& event)
{

    this->fireElementActivated();
}


void WxTableView::elementsListCtrlColumnBeginDragCB(wxListEvent& event)
{

    int clmnIdx = event.GetColumn();

    if (!p_dataModel->getColumn(clmnIdx)->getUserResizeAllowed())
        event.Veto();
}


void WxTableView::elementsListCtrlColumnEndDragCB(wxListEvent& event)
{


    int clmnIdx = event.GetColumn();
    TableColumn *clmn = p_dataModel->getColumn(clmnIdx);
    int width = p_listView->GetColumnWidth(clmnIdx);


    clmn->setPreferredWidth(width);
    width = clmn->getPreferredWidth();

    p_listView->SetColumnWidth(clmnIdx, width);
}


void WxTableView::setBackground(const wxColor& color)
{
    p_listView->SetBackgroundColour(color);
}


void WxTableView::windowResizeCB(wxSizeEvent& event)
{

    int tableWidth;

    this->Layout();

    tableWidth = p_listView->GetClientSize().GetWidth();

    if (p_dataModel != NULL)
        this->arrangeColumns(tableWidth);

    p_initWidth = tableWidth;
}


void WxTableView::arrangeColumns(int tableWidth, int idxStart)
{
    int sumClmnWeights = 0;
    int i;
    int n = p_dataModel->getColumnCount();
    int tblWidthChng;
    TableColumn *clmn = NULL;
    int w0, w1;
    int clmnIdx;
    int widthRmng;

    for (i = 0; i < n; i++)
    {
        clmn = p_dataModel->getColumn(i);

        if (clmn->isVisible())
            sumClmnWeights = sumClmnWeights + clmn->getSizeAdjustWeight();
    }

    if (sumClmnWeights > 0)
    {
        tblWidthChng = tableWidth - p_initWidth;

        clmnIdx = 0;
        widthRmng = tableWidth;

        for (i = idxStart; i < n; i++)
        {
            clmn = p_dataModel->getColumn(i);

            if (clmn->isVisible())
            {
                w0 = clmn->getPreferredWidth();
                w1 = w0 + clmn->getSizeAdjustWeight() * tblWidthChng / sumClmnWeights;

                if (w1 > widthRmng)
                {
                    w1 = widthRmng;

                    if (w1 < clmn->getMinimumWidth())
                        w1 = clmn->getMinimumWidth();
                }

                clmn->setPreferredWidth(w1);
                p_listView->SetColumnWidth(clmnIdx, w1);

                widthRmng = widthRmng - w1;
                clmnIdx++;
            }
        }
    }
}


void WxTableView::arrangeColumns(int tableWidth)
{
    this->arrangeColumns(tableWidth, 0);
}



/**
 *  Finds and returns the wxListItem that corresponds to the provided element index.
 *  The value of elmntIdx is the index of the element in the table model.
 *  If the element cannot be found, then a NULL result is returned.
 */
wxListItem* WxTableView::findItem(long elmntIdx)
{
    wxListItem *item = new wxListItem();
    int i, n;
    bool found;

    i = 0;
    n = p_listView->GetItemCount();
    found = false;

    while ((i < n) && !found)
    {
        item->Clear();
        item->SetId(i);

        p_listView->GetItem(*item);
        found = (item->GetData() == elmntIdx);
        i++;
    }

    if (!found)
    {
        item = 0;
    }

    return item;
}


/**
 *  Locates the wxListItem that corresponds to the provided element index.
 *  The value of elmntIdx is the index of the element in the table model.
 *  If the item is found, then the row number of the item is returned;
 *  if it cannot be found, then -1 is returned.
 */
int WxTableView::locateItem(long elmntIdx)
{
    wxListItem *item = new wxListItem();
    int i, n, k;
    bool found;

    i = 0;
    n = p_listView->GetItemCount();
    k = -1;
    found = false;

    while ((i < n) && !found)
    {
        item->Clear();
        item->SetId(i);

        p_listView->GetItem(*item);

        if (item->GetData() == elmntIdx)
        {
            found = true;
            k = i;
        }

        i++;
    }

    delete item;

    return k;
}


void WxTableView::addRowColor(const wxColour & color)
{
    p_rowColors.push_back(new wxColour(color));
}

/**
 *  Redisplay the list using current data and attributes of the model
 */
void WxTableView::update()
{
    int numCols;
    string text;
    int numItems, numVsbl;
    int iconIdx;
    wxImage icon;

    TableElement* elmnt;

    p_listView->DeleteAllItems();

    numCols = p_dataModel->getColumnCount();
    numItems = p_dataModel->size();
    numVsbl = 0;

    for (int i = 0; i < numItems; i++)
    {
        elmnt = p_dataModel->elementAt(i);

        if ((p_viewFilter == NULL) || p_viewFilter->accept(elmnt))
        {
            text = (wxString)(elmnt->getValueAsString(0));
            iconIdx = elmnt->getImageIndex();
            p_listView->InsertItem(numVsbl, text, iconIdx);

            for (int col = 1; col < numCols; col++)
            {
                text = (wxString)(elmnt->getValueAsString(col));
                p_listView->SetItem(numVsbl, col, (wxString)(elmnt->getValueAsString(col)));
            }

            p_listView->SetItemData(numVsbl, i);
            numVsbl++;
        }
    }

    this->refresh();
}


void WxTableView::setGridVisible(bool vsbl)
{
    long style = wxLC_REPORT|wxLC_SINGLE_SEL|wxSUNKEN_BORDER;

    if (vsbl)
        style = style|wxLC_HRULES|wxLC_VRULES;

    p_listView->SetWindowStyleFlag(style);
}


void WxTableView::refresh()
{
    wxListItem *clmnItem;
    int sortClmn;
    bool sortDesc;
    int slctRow;

    p_listView->SortItems(compareElements, reinterpret_cast<long>(p_dataModel));

    sortClmn = p_dataModel->getSortColumn();
    sortDesc = p_dataModel->getSortDescending();

    clmnItem = new wxListItem();
    p_listView->GetColumn(sortClmn, *clmnItem);
    clmnItem->SetImage(p_itemImageCount + (sortDesc ? 1 : 0));
    clmnItem->SetMask(wxLIST_MASK_IMAGE);
    p_listView->SetColumn(sortClmn, *clmnItem);

    wxColour *bkgd;
    int numRowColors = p_rowColors.size();

    if (numRowColors > 0)
    {
        int numItems = p_listView->GetItemCount();

        for (int i = 0; i < numItems; i++)
        {
            bkgd = p_rowColors[i % numRowColors];
            p_listView->SetItemBackgroundColour(i, *bkgd);
        }
    }

    if (p_slctElmntIdx >= 0)
    {
        //  One of the table elements is selected, so look for it.
        slctRow = this->locateItem(p_slctElmntIdx);

        if (slctRow >= 0)
        {
            p_listView->SetItemState(slctRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        }
        else
        {
            //  Selected element is not part of visible items, so change selection to nothing
            p_slctElmntIdx = -1;
            this->fireSelectionChanged();
        }
    }
}
