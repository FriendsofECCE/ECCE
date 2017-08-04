#ifndef _MDENERGYAPP_H
#define _MDENERGYAPP_H

#include "wxgui/ewxApp.H"

class MDEnergy;

class MDEnergyApp : public ewxApp
{
   public:
      MDEnergyApp();
      virtual ~MDEnergyApp();

      virtual string getName() const;

      /**
       *  Initialize application
       */
      virtual bool OnInit();

   protected:

      MDEnergy *p_frame;
};

DECLARE_APP(MDEnergyApp)

#endif
