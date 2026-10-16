#ifndef _GMXPREPAREAPP_H
#define _GMXPREPAREAPP_H

#include "wxgui/ewxApp.H"

class GmxPrepare;

class GmxPrepareApp : public ewxApp
{
   public:
      GmxPrepareApp();
      virtual ~GmxPrepareApp();

      virtual string getName() const;

      /**
       *  Initialize application
       */
      virtual bool OnInit();


   protected:

      GmxPrepare *p_frame;
};

DECLARE_APP(GmxPrepareApp)

#endif
