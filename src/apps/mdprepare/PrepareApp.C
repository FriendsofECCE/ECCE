#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;
#include <strstream>
  using std::ostrstream;

 
#include "util/JMSMessage.H"

#include "wxgui/WxJMSSubscriber.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "MDPrep.H"
#include "PrepareApp.H"

IMPLEMENT_APP(PrepareApp)


/**
 * Constructor.
 */
PrepareApp::PrepareApp()
{
   p_prepare = 0;
}


/**
 * Destructor.
 */
PrepareApp::~PrepareApp()
{
}

string PrepareApp::getName() const
{
   return "MDPrepare";
}


bool PrepareApp::OnInit()
{
   ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1],"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2]);
   }

   p_prepare = new MDPrep((wxFrame*)NULL);
   p_prepare->Show(TRUE);
   SetTopWindow(p_prepare);

   processGlobalPreferenceChange();

   return true;
}
