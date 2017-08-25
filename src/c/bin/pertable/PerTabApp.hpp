/**
 * @file
 *
 * @author Lisong Sun
 *
 */

#ifndef _PERTABAPP_H_
#define _PERTABAPP_H_

// For compilers that support precompilation, includes "wx/wx.h".
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/WxAppJMSMessageDispatch.H"
#include "wxgui/PerTabFrame.H"


/**
 * App container for periodic table application.
 */
class PerTabApp : public WxAppJMSMessageDispatch
{
   public:

      PerTabApp();
      virtual bool OnInit();

      virtual string identify();

      void preferenceMCB(JMSMessage& msg);
      void authMCB(JMSMessage& msg);

   protected:

      PerTabFrame * p_perTabFrame;

};

DECLARE_APP(PerTabApp)

#endif
// _PERTABAPP_H_
