#include "dsm/EDSI.H"

EDSI::EDSI() 
{
}

EDSI::EDSI(const EcceURL& url) 
{
  p_url = url;       
}

EDSI::EDSI(const EDSI& rhs) 
{
  p_url = rhs.p_url;
  // Not copying message stack
}

EDSI::~EDSI() 
{
}

EcceURL EDSI::getURL() const
{
  return p_url; 
}

void EDSI::setURL(const EcceURL & url) 
{
  p_url = url;
}
