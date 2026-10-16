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

 
#include "GmxPrepare.H"
#include "GmxPrepareApp.H"

IMPLEMENT_APP(GmxPrepareApp)


/**
 * Constructor.
 */
GmxPrepareApp::GmxPrepareApp()
{
   p_frame = 0;
}


/**
 * Destructor.
 */
GmxPrepareApp::~GmxPrepareApp()
{
}

string GmxPrepareApp::getName() const
{
   return GMXPREPARE;
}


bool GmxPrepareApp::OnInit()
{
   ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1].ToStdString().c_str(),"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2].ToStdString());
   }

   p_frame = new GmxPrepare((wxFrame*)NULL);
   p_frame->Show(TRUE);
   SetTopWindow(p_frame);

   processGlobalPreferenceChange();

   return true;
}

