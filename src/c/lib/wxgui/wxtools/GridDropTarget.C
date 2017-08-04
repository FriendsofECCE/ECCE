/**
 * @file
 *
 *
 */
#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/tokenzr.h>

#include "dsm/Resource.H"
#include "dsm/EDSIFactory.H"

#include "util/JMSPublisher.H"

#include "wxgui/ewxGrid.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxStyledWindow.H"
#include "wxgui/GridDropTarget.H"
#include "wxgui/GridPropElement.H"
#include "wxgui/GridModel.H"
#include "wxgui/WxGridView.H"


/**
 * Constructor
 */
GridDropTarget::GridDropTarget(WxGridView *gridView)
{
  p_gridView = gridView;
  resetData();
}


/**
 *  Destructor
 */
GridDropTarget::~GridDropTarget()
{}


wxDragResult GridDropTarget::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
  p_prevRow = -1;
  p_isPrevRowHighlighted = false;
  def = wxDragNone;
  return def;
}


wxDragResult GridDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
  // Get Row dragging over, if any
  int row = p_gridView->yToRow(y);

  // Need to unhighlight previous row, if dragging accross rows
  if (p_prevRow != row) {
    if (p_isPrevRowHighlighted) {
      // unhighlight previous row
      wxGridCellAttr *attr = p_gridView->getModel()->GetAttr(p_prevRow,0,wxGridCellAttr::Row);
      if (attr != 0) {
        attr->SetBackgroundColour(p_gridView->getGrid()->GetDefaultCellBackgroundColour());
        p_gridView->getGrid()->ForceRefresh();
      }
      else {
        attr = new wxGridCellAttr();
        attr->SetBackgroundColour(p_gridView->getGrid()->GetDefaultCellBackgroundColour());
        p_gridView->getGrid()->SetRowAttr(p_prevRow,attr);
        p_gridView->getGrid()->ForceRefresh();
      }
    }
  }

  // Get rows associated resource, if any
  Resource *resource = 0;
  GridModel *model = p_gridView->getModel();
  if (model != 0) {
    GridPropElement *element =
      dynamic_cast<GridPropElement*>(model->GetValueAsGridElement((row), 0));
    if (element != 0) {
      resource = element->getResource();
    }
  }

  // Dragging over a row
  if (resource != 0) {
    // Dragging over a selected row or a non-collection
    if ((p_gridView->isRowSelected(row)) ||
        ((resource->getResourceType() != ResourceDescriptor::RT_COLLECTION) &&
         (resource->getResourceType() != ResourceDescriptor::RT_VIRTUAL_DOCUMENT))) {
      def = wxDragNone;
      p_prevRow = row;
      p_isPrevRowHighlighted = false;
    }
    // Dragging over a valid drop row
    else {
      // highlight row
      wxGridCellAttr *attr = p_gridView->getModel()->GetAttr(row,0, wxGridCellAttr::Row);
      if (attr != 0) {
        attr->SetBackgroundColour(ewxStyledWindow::getUnfocusedSelectionColor());
        p_gridView->getGrid()->ForceRefresh();
      }
      else {
        attr = new wxGridCellAttr();
        attr->SetBackgroundColour(ewxStyledWindow::getUnfocusedSelectionColor());
        p_gridView->getGrid()->SetRowAttr(row,attr);
        p_gridView->getGrid()->ForceRefresh();
      }
      p_prevRow = row;
      p_isPrevRowHighlighted = true;
    }
  }
  // Not dragging over a row, for now
  // don't allow a drop unless over
  // a row.  May later want to allow a drop
  // while still over the drop target, but 
  // not over a row
  else {
    p_prevRow = -1;
    p_isPrevRowHighlighted = false;
    def = wxDragNone;
  }
  
  return def;
}


void GridDropTarget::OnLeave()
{
  if (p_isPrevRowHighlighted) {

    // unhighlight previous row
    wxGridCellAttr *attr = p_gridView->getModel()->GetAttr(p_prevRow,0,wxGridCellAttr::Row);
    if (attr != 0) {
      attr->SetBackgroundColour(p_gridView->getGrid()->GetDefaultCellBackgroundColour());
      p_gridView->getGrid()->ForceRefresh();
    }
    else {
      attr = new wxGridCellAttr();
      attr->SetBackgroundColour(p_gridView->getGrid()->GetDefaultCellBackgroundColour());
      p_gridView->getGrid()->SetRowAttr(p_prevRow,attr);
      p_gridView->getGrid()->ForceRefresh();
    }
  }
  p_prevRow = -1;
  p_isPrevRowHighlighted = false;
}


bool GridDropTarget::OnDropText(wxCoord x, wxCoord y, const wxString& data) 
{
  return true;
}


/**
 * Gets the data from the drop and performs suggested 
 * wxDragResult (ie copy or move).  It's assumed that the 
 * OnDragOver performs necessary checks to make sure it is a 
 * valid drop.
 */
wxDragResult GridDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
  // Get data from drop
  if (!GetData()) {
    resetData();
    return wxDragCancel;
  }

  wxArrayString filenames = p_filedata->GetFilenames();
  vector<EcceURL> urls;
  for (size_t i=0; i<filenames.GetCount(); ++i)
    urls.push_back(filenames[i].c_str());

  if (filenames.GetCount() == 0) {
    wxStringTokenizer tkz(p_urldata->GetText(), wxT("\n"));
    while (tkz.HasMoreTokens()) {
      string str = tkz.GetNextToken().c_str();
      if (str != "")
        urls.push_back(str);
    }
  }

  // Get row
  int row = p_gridView->yToRow(y);

  // Get target resource
  Resource *target = 0;
  GridModel *model = p_gridView->getModel();
  if (model != 0) {
    GridPropElement *element =
      dynamic_cast<GridPropElement*>(model->GetValueAsGridElement((row), 0));
    if (element != 0) {
      target = element->getResource();
    }
  }

  // Perform the move or copy for each source resource
  if (target != 0) {

    EcceURL targetUrl = target->getURL();

    for (size_t i = 0; i < urls.size(); i++) {

      Resource *source =  EDSIFactory::getResource(urls[i]);

      if (source != 0) {
        EcceURL newUrl = targetUrl.getChild(source->getName());
        EcceURL oldUrl = source->getURL();
        if (def == wxDragCopy) {
          if (!source->copy(newUrl)) {
            ewxMessageDialog dlg(p_gridView, "Copy " + source->getName() + " failed: \n" + source->messages(),
                                 "Copy Failed", wxOK|wxICON_EXCLAMATION,
                                 wxDefaultPosition);
            dlg.ShowModal();
          }
          else {
            JMSPublisher publisher(p_gridView->getAppName());
            JMSMessage *msg = publisher.newMessage();
            msg->addProperty("url",newUrl.toString());
            publisher.publish("ecce_url_created",*msg);
            delete msg;
          }
        } 
        if (def == wxDragMove) {
          if (!source->move(newUrl)) {
            ewxMessageDialog dlg(p_gridView, "Move " + source->getName() + " failed: \n" + source->messages(),
                                 "Move Failed", wxOK|wxICON_EXCLAMATION,
                                 wxDefaultPosition);
            dlg.ShowModal();
          }
          else {
            JMSPublisher publisher(p_gridView->getAppName());
            JMSMessage *msg = publisher.newMessage();
            if (oldUrl.isLocal()) {
              msg->addProperty("url",newUrl.toString());
              publisher.publish("ecce_url_created",*msg);
            }
            else {
              msg->addProperty("newurl",newUrl.toString());
              msg->addProperty("oldurl",oldUrl.toString());
              publisher.publish("ecce_url_renamed",*msg);
            }
            delete msg;
          }
        }
      }
    }
    target->getChildren(true);
    p_gridView->selectRow(row);
    p_gridView->setCursor(row,0);
  }
  else {
    def = wxDragError;
  }

  resetData();
  return def;
}


/**
 * Seems like the GetData won't reset data in other formats and this version of
 * wxWidgets doesn't have GetReceivedFormat yet, so the solution is to clear
 * out data in all formats after each useage. According to the wx doc, the
 * original data will be deleted when SetDataObject is called. So I guess I
 * don't need to delete them myself.
 */
void GridDropTarget::resetData()
{
  p_data = new wxDataObjectComposite;
  p_urldata = new wxTextDataObject;
  p_data->Add(p_urldata, true);
  p_filedata = new wxFileDataObject;
  p_data->Add(p_filedata);
  SetDataObject(p_data);
}
