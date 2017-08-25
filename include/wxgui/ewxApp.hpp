#ifndef _EWXAPP_H  
#define _EWXAPP_H  

#include <string>
  using std::string;

#include "wx/app.h"


/**
 * Subclass wxApp to provide common Ecce application behavior.
 * This includes things like setting vendor name, requesting best
 * available visual etc etc.
 *
 * All ecce main classes should subclass this class.
 */
class ewxApp : public wxApp
{

   public:

      virtual ~ewxApp();

      virtual bool OnInit();

      virtual string getName() const { return "noname"; }

      virtual void processGlobalPreferenceChange();


};

#endif
