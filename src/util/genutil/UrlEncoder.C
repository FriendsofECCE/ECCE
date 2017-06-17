#include <iostream>
  using std::ends;

#include <strstream>
  using std::ostrstream;

#include <iomanip>
  using std::setw;
  using std::setfill;
  using std::hex;
#include <string>
  using std::string;

#include <string.h>

#include "util/EcceURL.H"
#include "util/UrlEncoder.H"


EcceURL UrlEncoder::encode(const EcceURL& url)
{
  string path = url.getPathWithParams();
  ostrstream encodedPath;
  EcceURL ret;

  // Encode path and query string components
  string::iterator pathIt = path.begin();
  while (pathIt != path.end()) {
    if ( !isalnum(*pathIt) && !strchr("$-_.+!*'(),:@&=/~?", *pathIt) ) {
      encodedPath << '%'
                  << setw(2) 
                  << setfill('0') 
                  << hex
                  << static_cast<int>(*pathIt);
    }
    else {
      encodedPath << *pathIt;
    }
    pathIt++;
  }

  encodedPath << ends;
  // Return encoded URL
  ret.set(url.getProtocol().c_str(),
          url.getHost().c_str(),
          url.getPort(),
          encodedPath.str());
  return ret;
}


