#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxNotebook.H"


ewxNotebook::ewxNotebook()
  : wxNotebook(),
    ewxStyledWindow()
{
}


ewxNotebook::ewxNotebook(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos, const wxSize& size, long style,
                         const wxString& name)
  : wxNotebook(),
    ewxStyledWindow()
{
   Create(parent, id, pos, size, style, name);
}


bool ewxNotebook::Create(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos, const wxSize& size, long style,
                         const wxString& name)
{
   if (!wxNotebook::Create(parent, id, pos, size, style, name)) {
      wxFAIL_MSG( wxT("ewxNotebook creation failed") );
      return false;
   }

   setStyles(this, true);

   return true;
}


/**
 * Return the maximum size required by any of the panels inside the notebook.
 * This method considers the panel sizes and the notebook sizes.
 * The size can then be used to compute the size of the overall window.  This
 * is useful because notebooks don't properly size themselves with gtk.
 * I noticed that one of the size methods for notebook says it doesn't work
 * with gtk.
 */
wxSize ewxNotebook::computeSize() 
{
   int numPages = GetPageCount();

   int iw, ih, wmax, hmax;
   iw = ih = wmax = hmax = 0;
   for (int idx=0; idx<numPages; idx++) {
      wxNotebookPage *page = GetPage(idx);
      page->GetSize(&iw,&ih);
      if (iw > wmax)
         wmax = iw;
      if (ih > hmax)
         hmax = ih;
   }

   // Not sure why this would be useful but it seems to be
   GetSize(&iw,&ih);
   if (iw > wmax) {
      wmax = iw;
   }
   if (ih > hmax) {
      hmax = ih;
   }

   return wxSize(wmax, hmax);

}

