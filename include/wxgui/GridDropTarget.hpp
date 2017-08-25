/**
 * @file
 *
 *
 */

#ifndef GRIDDROPTARGET_HH
#define GRIDDROPTARGET_HH


#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif

#include "wxgui/WxGridView.H"


class GridDropTarget: public wxDropTarget
{
  public:

    GridDropTarget(WxGridView *gridView);

    virtual ~GridDropTarget();

    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual void OnLeave();
    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& data);
    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
    virtual void resetData();

  private:

    WxGridView *p_gridView;
    int p_prevRow;
    bool p_isPrevRowHighlighted;

    wxDataObjectComposite * p_data;
    wxTextDataObject * p_urldata;
    wxFileDataObject * p_filedata;

};

#endif



