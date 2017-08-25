/**
 * @file
 *
 *
 */
#ifndef _EWXPROGRESSDIALOG_H  
#define _EWXPROGRESSDIALOG_H  

#include "util/ProgressMonitor.H"

#include "wx/progdlg.h"
#include "wxgui/ewxStyledWindow.H"

/**
 * Subclass wxProgressDialog to provide style changes.
 */
class ewxProgressDialog : public wxProgressDialog, 
                          public ewxStyledWindow,
                          public ProgressMonitor
{
   public:

      ewxProgressDialog(const wxString& title, const wxString& message, int maximum=100, wxWindow* parent=NULL, int style=wxPD_AUTO_HIDE|wxPD_APP_MODAL);

      virtual ~ewxProgressDialog();


      /* Use this if you just want to show indeterminate progress */
      virtual bool isInterrupted();

      /* Use this if you want to show indeterminate progress */
      virtual bool isInterrupted(const string& msg, int pct);



};

#endif
