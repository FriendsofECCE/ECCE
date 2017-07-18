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

 

#include "PolyrateApp.H"
#include "Polyrate.H"

IMPLEMENT_APP(PolyrateApp)


/**
 * Constructor.
 */
PolyrateApp::PolyrateApp() 
{
   p_polyrate = 0;
}


/**
 * Destructor.
 */
PolyrateApp::~PolyrateApp()
{
}

string PolyrateApp::getName() const
{
   return "POLYRATE";
}


bool PolyrateApp::OnInit()
{
   ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1],"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2]);
   }

   p_polyrate= new Polyrate((wxFrame*)NULL);
   p_polyrate->Show(TRUE);
   SetTopWindow(p_polyrate);
   processGlobalPreferenceChange();

   return true;
}


