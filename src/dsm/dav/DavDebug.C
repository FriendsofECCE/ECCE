#include <iostream>
  using std::ios;
  using std::endl;
  using std::flush;

#include <fstream>
  using std::ofstream;

#include <stdlib.h>   // free
#include <string.h>

#include <xercesc/util/PlatformUtils.hpp>
using namespace xercesc;

#include "util/TDateTime.H"
#include "dsm/DavDebug.H"

char *DavDebug::s_context = NULL;

DavDebug::DavDebug(const char *file)
{
  s_context = strdup("/tmp/davtraffic");

  p_file = 0;
  if (file) {
    p_file = strdup(file);
  } else {
    p_file = strdup(s_context);
  }
  p_lasttime = 0;
}

DavDebug::~DavDebug()
{
  if (p_file) free(p_file);
}

void DavDebug::setDebugContext(const char *context)
{
  if (context) {
    if (s_context) free(s_context);
    s_context = strdup(context);
  } else {
    s_context = strdup("/tmp/davtraffic");
  }
}

void DavDebug::dump(const string& value, const char *description)
{
 ofstream davdump(p_file,ios::app);
 davdump << "\n";
 davdump << "----------- \n";
 davdump << "DUMP START: " << description << " " << TDateTime().toString() << "\n";
 davdump << "----------- " << "\n";
 davdump << value;
 davdump << "--------- " << "\n";
 davdump << "DUMP END: " << "\n";
 davdump << "--------- " << "\n";
 davdump.flush();
 davdump.close();
}

void DavDebug::startTimer()
{
  p_lasttime = XMLPlatformUtils::getCurrentMillis();
}

void DavDebug::endTimer(const char * description)
{
  ofstream davdump(p_file,ios::app);
  if (description) davdump << description;
  unsigned long now = XMLPlatformUtils::getCurrentMillis();
  davdump <<  " " << now - p_lasttime << endl;
  davdump.close();
  p_lasttime = now;  // maybe?
}


