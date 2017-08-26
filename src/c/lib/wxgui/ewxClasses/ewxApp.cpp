#include <iostream>
  using namespace std;

#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/image.h"

#include "util/Ecce.hpp"
#include "util/Preferences.hpp"
#include "util/Color.hpp"

#include "dsm/DavDebug.hpp"

#include "tdat/TaskApp.hpp"

#include "wxgui/ewxWindowUtils.hpp"
#include "wxgui/ewxApp.hpp"


/**
 * Destructor.
 */
ewxApp::~ewxApp()
{
}


/**
 * Constructor.
 * Sets vendor and app names associated with how applications may
 * be grouped by window managers.
 * Performs other common initializations.
 */
bool ewxApp::OnInit()
{
   wxInitAllImageHandlers();

   SetVendorName("EMSL");
   SetAppName("ECCE");
   SetUseBestVisual(true);

   Ecce::initialize();

   Color::initialize();

   DavDebug::setDebugContext(("/tmp/dav" + getName()).c_str());

   return true;
}

/**
 * Load global preferences and update accordingly.
 * Implementation delegated to ewxWindowUtils method of same name.
 * This method will
 *   <li>find ID_FEEDBACK  and set beep preferences</li>
 *   <li>call setUnitFamily for TaskApp implementations.  This works only
 *   if the top frame can be cast to a TaskApp</li>
 */
void ewxApp::processGlobalPreferenceChange()
{
   wxWindow *top = GetTopWindow();
   ewxWindowUtils::processGlobalPreferenceChange(top);

}
