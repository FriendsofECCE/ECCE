/**
 * @file
 *
 * @author Lisong Sun
 *
 */

#ifndef _FILECHOOSERAPP_H_
#define _FILECHOOSERAPP_H_

// For compilers that support precompilation, includes "wx/wx.h".
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/WxFileChooser.H"

/**
 * App container for the file chooser which is a dialog in wxwidgets so we
 * don't have to create a new dialog in teleuse.
 */
class WxFileChooserApp : public ewxApp
{
   public:

      WxFileChooserApp();
      virtual bool OnInit();

   protected:

      WxFileChooser* p_fileChooser;

};

DECLARE_APP(WxFileChooserApp)

#endif
// _FILECHOOSERAPP_H_
