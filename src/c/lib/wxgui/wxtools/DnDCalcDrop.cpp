/**
 * @file
 *
 *
 */
#include <iostream>
 using std::cout;
 using std::endl;

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxBitmap.H"
#include "wxgui/DnDCalcDrop.H"

#include <iostream>
using std::cerr;
using std::endl;

/**
 * Constructor.
 * @param btn - the wxBitmapButton used for the drop and drop active pixmaps.
 * @param hanlder - implementer of CalcDropHanlder interface that will be called
 *    when a drop occurs.
 */
DndCalcDrop::DndCalcDrop(wxBitmapButton *btn, CalcDropHandler *handler) 
{ 
   p_btn = btn;
   p_handler = handler;
   resetData();
   //   SetDataObject(new wxURLDataObject); 
}

void DndCalcDrop::OnDropURL(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxString& text)
{
   // of course, a real program would do something more useful here...
   wxMessageBox(text, _T("wxDnD sample: got URL"),
         wxICON_INFORMATION | wxOK);
}


// URLs can't be moved, only copied
wxDragResult DndCalcDrop::OnDragOver(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y),
      wxDragResult WXUNUSED(def))
{
   //cout << "drag over " << endl;
   return wxDragLink;  // At least IE 5.x needs wxDragLink, the
   // other browsers on MSW seem okay with it too.
}


wxDragResult DndCalcDrop::OnEnter(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y),
      wxDragResult WXUNUSED(def))
{
   p_btn->SetBitmapLabel(ewxBitmap("dropValid.xpm",wxBITMAP_TYPE_XPM));
   return wxDragLink;  // At least IE 5.x needs wxDragLink, the
   // other browsers on MSW seem okay with it too.
}


void DndCalcDrop::OnLeave()
{
   p_btn->SetBitmapLabel(ewxBitmap("drop_w.xpm",wxBITMAP_TYPE_XPM));
}

/**
 * Send the url to the CalcDropHandler if defined.  
 * If not defined, nothing will be done.
 */
wxDragResult DndCalcDrop::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
  if ( !GetData() ) {
    resetData();
    return wxDragNone;
  }

  string url;
  if (p_filedata->GetFilenames().Count() == 1) {
    url = p_filedata->GetFilenames()[0];
  }
  else if (p_filedata->GetFilenames().Count() > 1) {
    // @todo Show error that it can't open multiple items at the same time
    resetData();
    return wxDragNone;
  }
  else {
    url = p_urldata->GetURL();
  }

  if (p_handler && url != "") {
    p_handler->processDrop(url);
  }

  //OnDropURL(x, y, ((wxURLDataObject *)m_dataObject)->GetURL());
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
void DndCalcDrop::resetData()
{
  p_data = new wxDataObjectComposite;
  p_urldata = new wxURLDataObject;
  p_data->Add(p_urldata, true);
  p_filedata = new wxFileDataObject;
  p_data->Add(p_filedata);
  SetDataObject(p_data);
}
