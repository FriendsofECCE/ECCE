/**
 * @file
 *
 *
 */

#include <iostream>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxProgressDialog.H"


/**
 * Constructor.
 * Override styles with ecce styles.
 */
ewxProgressDialog::ewxProgressDialog(const wxString& title, const wxString& message, int maximum, wxWindow* parent, int style)
  :  wxProgressDialog(title, message, maximum, parent, style),
     ewxStyledWindow()
{
  setStyles(this, true);
}


/**
 * Destructor.
 */
ewxProgressDialog::~ewxProgressDialog()
{
}

bool ewxProgressDialog::isInterrupted()
{
   bool ret = false;
   wxYieldIfNeeded();
   ret = !Pulse();
   return ret;
}


bool ewxProgressDialog::isInterrupted(const string& msg, int pct)
{
   bool ret = false;
   wxYieldIfNeeded();
   ret = !Update(pct,msg.c_str());
   return ret;

}
