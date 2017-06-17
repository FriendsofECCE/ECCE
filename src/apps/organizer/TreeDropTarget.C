/**
 * @file
 *
 *
 */

#include <wx/dnd.h>
#include <wx/dataobj.h>
#include <wx/tokenzr.h>

#include "TreeDropTarget.H"
#include "CalcMgr.H"

#include <iostream>
using std::cerr;
using std::endl;

/**
 * Default constructor. Pass in pointer to CalcMgr obj.
 */
TreeDropTarget::TreeDropTarget(CalcMgr * calcMgr)
{
  p_calcMgr = calcMgr;
  resetData();
}


/**
 * Destructor.
 * No need to delete p_data since wxDropTarget will do it.
 */
TreeDropTarget::~TreeDropTarget()
{
}


wxDragResult TreeDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
  p_calcMgr->enableTreeDropHighlight(false);

  if (!GetData()) {
    resetData();
    return wxDragCancel;
  }

  wxArrayString filenames = p_filedata->GetFilenames();
  vector<EcceURL> urls;
  for (int i=0; i<filenames.GetCount(); ++i)
    urls.push_back(filenames[i].c_str());

  if (filenames.GetCount() == 0) {
    wxStringTokenizer tkz(p_urldata->GetText(), wxT("\n"));
    while (tkz.HasMoreTokens()) {
      string str = tkz.GetNextToken().c_str();
      if (str != "")
        urls.push_back(str);
    }
  }

  p_calcMgr->dropInTree( urls, x, y, def);
  resetData();
  return def;
}


/**
 * Call the CalcMgr to confirm target node is dropable
 */
bool TreeDropTarget::OnDrop(wxCoord x, wxCoord y)
{
  p_calcMgr->enableTreeDropHighlight(false);
  return p_calcMgr->canDropOnTree(x, y);
}


/**
 * Based on the coord, highlight the tree item under mouse
 */
wxDragResult TreeDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
  p_calcMgr->enableTreeDropHighlight(true, x, y);
  return def;
}


void TreeDropTarget::OnLeave()
{
  p_calcMgr->enableTreeDropHighlight(false);
}


/**
 * Seems like the GetData won't reset data in other formats and this version of
 * wxWidgets doesn't have GetReceivedFormat yet, so the solution is to clear
 * out data in all formats after each useage. According to the wx doc, the
 * original data will be deleted when SetDataObject is called. So I guess I
 * don't need to delete them myself.
 */
void TreeDropTarget::resetData()
{
  p_data = new wxDataObjectComposite;
  p_urldata = new wxTextDataObject;
  p_data->Add(p_urldata, true);
  p_filedata = new wxFileDataObject;
  p_data->Add(p_filedata);
  SetDataObject(p_data);
}
