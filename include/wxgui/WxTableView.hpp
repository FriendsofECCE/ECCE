/**
 *  @file
 *  @author Ken Swanson
 *
 *  Visual element, based on the wxListCtrl class, that can be used
 *  to display tabular data.
 *
 */


#ifndef WXTABLEVIEW_HH
#define WXTABLEVIEW_HH

#include "wx/event.h"
#include "wx/frame.h"
#include "wx/imaglist.h"
#include "wx/listctrl.h"

#include "wxgui/DefaultTableModel.H"
#include "wxgui/TableViewFilter.H"

#include "util/Preferences.H"


class DefaultTableModel;

#define ID_LISTVIEW_ELEMENTS 21000

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(ewxEVT_TABLE_SELECTION_CHANGED, ID_LISTVIEW_ELEMENTS + 50)
    DECLARE_EVENT_TYPE(ewxEVT_TABLE_ELEMENT_ACTIVATED, ID_LISTVIEW_ELEMENTS + 51)
END_DECLARE_EVENT_TYPES()


class WxTableView: public wxWindow
{

    DECLARE_CLASS(WxTableView)
    DECLARE_EVENT_TABLE()

    //DECLARE_EVENT_TYPE(ewxEVT_TABLE_SELECTION_CHANGED, ID_LISTVIEW_ELEMENTS + 100)


    public:
        // Constructors, Destructor
        WxTableView(      wxWindow* parent,
                            wxWindowID id,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                            long style = 0,
                      const wxString& name = wxPanelNameStr);

        ~WxTableView();

        DefaultTableModel*  getModel();
        void                setModel(DefaultTableModel *model);
        void                update();
        TableElement*       getSelectedElement();
        TableViewFilter    *getFilter();
        void                setFilter(TableViewFilter *filter);
        wxSize              DoGetBestSize() const;
        void                setBackground(const wxColor& color);
        void                addItemImage(wxImage *img);

        void                addRowColor(const wxColour & color);
        void                setGridVisible(bool vsbl);

        //  Event Handlers
        void                elementsListCtrlColumnLeftClickCB(wxListEvent& event);
        void                elementsListCtrlItemSelectedCB(wxListEvent& event);
        void                elementsListCtrlItemActivatedCB(wxListEvent& event);
        void                elementsListCtrlColumnBeginDragCB(wxListEvent& event);
        void                elementsListCtrlColumnEndDragCB(wxListEvent& event);
        void                windowResizeCB(wxSizeEvent& event);


    private:
        //  Data members
        void arrangeColumns(int tableWidth);
        void arrangeColumns(int tableWidth, int idxStart);

        DefaultTableModel   *p_dataModel;
        wxListCtrl          *p_listView;
        long                 p_slctElmntIdx;
        wxListItem*          findItem(long elmntIdx);
        int                  locateItem(long elmntIdx);
        TableViewFilter     *p_viewFilter;

        int                  p_itemImageCount;
        wxImageList         *p_ctrlImages;

        //  Private methods
        void                 fireSelectionChanged();
        void                 fireElementActivated();
        void                 refresh();

        vector<wxColour *>   p_rowColors;

        int                  p_initWidth;


};


#endif  // WXTABLEVIEW_HH
