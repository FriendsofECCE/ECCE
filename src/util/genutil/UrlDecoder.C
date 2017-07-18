#include <iostream>
  using std::ends;

#include <strstream>
  using std::ostrstream;

#include <string>
  using std::string;

#include "util/EcceURL.H"
#include "util/UrlDecoder.H"


EcceURL UrlDecoder::decode(const EcceURL& url)
{
  string path = url.getPathWithParams();
  ostrstream decodedPath;
  EcceURL ret;

  // Decode path and query string components
  int i = 0;
  int pathSize = path.size();  
  int hexDigit;
  int stepSize;
  char c;
  while (i < pathSize) {
    c = path[i];
    stepSize = 1;
    if ((path[i] == '%') && ((i+2) < pathSize)) { 
      if ((isxdigit(path[i+1])) && (isxdigit(path[i+2]))) {
        hexDigit = (path[i+1] >= 'A' ? ((path[i+1] & 0xDF) - 'A') + 10 
                                     : (path[i+1] - '0'));
        hexDigit *= 16;
        hexDigit += (path[i+2] >= 'A' ? ((path[i+2] & 0xDF) - 'A') + 10 
                                      : (path[i+2] - '0'));
        c = static_cast<char>(hexDigit);
        stepSize = 3;
      }
    }
    decodedPath << c;
    i += stepSize;
  }

  decodedPath << ends;
  // Return decoded URL
  ret.set(url.getProtocol().c_str(),
          url.getHost().c_str(),
          url.getPort(),
          decodedPath.str());

  return ret;
}


