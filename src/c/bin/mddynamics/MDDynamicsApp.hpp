#ifndef _MDDYNAMICSAPP_H
#define _MDDYNAMICSAPP_H

#include "wxgui/ewxApp.H"

class MDDynamics;

class MDDynamicsApp : public ewxApp
{
   public:
      MDDynamicsApp();
      virtual ~MDDynamicsApp();

      virtual string getName() const;

      /**
       *  Initialize application
       */
      virtual bool OnInit();


   protected:

      MDDynamics *p_frame;
};

DECLARE_APP(MDDynamicsApp)

#endif
