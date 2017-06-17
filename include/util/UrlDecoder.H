#ifndef URLDECODER_HH
#define URLDECODER_HH

#include <string>
  using std::string;

#include "util/EcceURL.H"


class UrlDecoder 
{

  public: 

    static EcceURL decode(const EcceURL& url);

};

#endif
