#ifndef _POLYRATEAPP_H
#define _POLYRATEAPP_H

#include "wxgui/ewxApp.hpp"
#include "wxgui/WxJMSMessageDispatch.hpp"

class Polyrate;
class JMSMessage;

class PolyrateApp : public ewxApp
{
   public:
      PolyrateApp();
      virtual ~PolyrateApp();

      virtual string getName() const;

      /**
       *  Initialize application
       */
      virtual bool OnInit();


   protected:

      Polyrate *p_polyrate;
};

DECLARE_APP(PolyrateApp)

#endif
