#ifndef _PREPAREAPP_H
#define _PREPAREAPP_H

#include "wxgui/ewxApp.H"

class MDPrep;

class PrepareApp : public ewxApp 
{
   public:
      PrepareApp();
      virtual ~PrepareApp();

      virtual string getName() const;

      /**
       *  Initialize application
       */
      virtual bool OnInit();


   protected:

      MDPrep *p_prepare;
};

DECLARE_APP(PrepareApp)

#endif
