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

 
#include "MDDynamics.H"
#include "MDDynamicsApp.H"

IMPLEMENT_APP(MDDynamicsApp)


/**
 * Constructor.
 */
MDDynamicsApp::MDDynamicsApp()
{
   p_frame = 0;
}


/**
 * Destructor.
 */
MDDynamicsApp::~MDDynamicsApp()
{
}

string MDDynamicsApp::getName() const
{
   return MDDYNAMICS;
}


bool MDDynamicsApp::OnInit()
{
   ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1],"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2]);
   }


   p_frame = new MDDynamics((wxFrame*)NULL);
   p_frame->Show(TRUE);
   SetTopWindow(p_frame);

   processGlobalPreferenceChange();

   return true;
}

