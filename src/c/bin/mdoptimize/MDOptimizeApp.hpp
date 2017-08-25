#ifndef _MDOPTIMIZEAPP_H
#define _MDOPTIMIZEAPP_H

#include "wxgui/ewxApp.H"

class MDOptimize;

class MDOptimizeApp : public ewxApp
{
   public:
      MDOptimizeApp();
      virtual ~MDOptimizeApp();

      virtual string getName() const;

      /**
       *  Initialize application
       */
      virtual bool OnInit();


   protected:

      MDOptimize *p_frame;
};

DECLARE_APP(MDOptimizeApp)

#endif
