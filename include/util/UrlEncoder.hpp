#ifndef URLENCODER_HH
#define URLENCODER_HH

#include <string>
  using std::string;

#include "util/EcceURL.hpp"


class UrlEncoder 
{

  public: 

    static EcceURL encode(const EcceURL& url);

};

#endif
