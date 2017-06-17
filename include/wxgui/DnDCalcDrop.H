/**
 * @file
 *
 *
 */
#ifndef DNDCALCDROP_H
#define DNDCALCDROP_H

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dnd.h"
#include "wxgui/CalcDropHandler.H"

class wxBitmapButton;

/**
 * A drag and drop target that takes urls and swaps our drop active icon
 * in and out.
 *
 * This class is copied from the wxWidgets dnd sample with the addition
 * of onEnter and onLeave handling.
 */
class DndCalcDrop : public wxDropTarget
{
   public:
      DndCalcDrop(wxBitmapButton *btn, CalcDropHandler *handler);

      virtual void OnDropURL(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxString& text);

      virtual wxDragResult OnEnter(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y),
            wxDragResult WXUNUSED(def));

      virtual void OnLeave();

      // URLs can't be moved, only copied
      virtual wxDragResult OnDragOver(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y),
            wxDragResult WXUNUSED(def));

      // translate this to calls to OnDropURL() just for convenience
      virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);

      virtual void resetData();

   protected:

      wxBitmapButton *p_btn;
      CalcDropHandler *p_handler;

      wxDataObjectComposite * p_data;
      wxURLDataObject * p_urldata;
      wxFileDataObject * p_filedata;
};

#endif
