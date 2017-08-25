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

 
#include "MDOptimize.H"
#include "MDOptimizeApp.H"

IMPLEMENT_APP(MDOptimizeApp)


/**
 * Constructor.
 */
MDOptimizeApp::MDOptimizeApp()
{
   p_frame = 0;
}


/**
 * Destructor.
 */
MDOptimizeApp::~MDOptimizeApp()
{
}

string MDOptimizeApp::getName() const
{
   return MDOPTIMIZE;
}


bool MDOptimizeApp::OnInit()
{
   ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1],"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2]);
   }

   p_frame = new MDOptimize((wxFrame*)NULL);
   p_frame->Show(TRUE);
   SetTopWindow(p_frame);

   processGlobalPreferenceChange();

   return true;
}

