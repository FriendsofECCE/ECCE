/**
 * @file
 *
 *
 */
#ifndef TREEDROPTARGET_H
#define TREEDROPTARGET_H

#include <wx/dnd.h>
#include <wx/dataobj.h>

class CalcMgr;

class TreeDropTarget: public wxDropTarget
{
public:

  TreeDropTarget(CalcMgr * calcMgr);

  ~TreeDropTarget();

  virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);

  virtual bool OnDrop(wxCoord x, wxCoord y);

  virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);

  virtual void OnLeave();

  virtual void resetData();

protected:

  CalcMgr * p_calcMgr;

  wxDataObjectComposite * p_data;
  wxTextDataObject * p_urldata;
  wxFileDataObject * p_filedata;
};

#endif
