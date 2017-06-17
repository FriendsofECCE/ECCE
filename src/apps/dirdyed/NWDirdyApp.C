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

 
#include "NWDirdy.H"
#include "NWDirdyApp.H"

IMPLEMENT_APP(NWDirdyApp)


/**
 * Constructor.
 */
NWDirdyApp::NWDirdyApp() 
{
   p_nwdirdy = 0;
}


/**
 * Destructor.
 */
NWDirdyApp::~NWDirdyApp()
{
}

string NWDirdyApp::getName() const
{
   return NWDIRDY;
}


bool NWDirdyApp::OnInit()
{
   ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1],"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2]);
   }

   p_nwdirdy= new NWDirdy((wxFrame*)NULL);
   p_nwdirdy->Show(TRUE);
   SetTopWindow(p_nwdirdy);
   processGlobalPreferenceChange();

   return true;
}


