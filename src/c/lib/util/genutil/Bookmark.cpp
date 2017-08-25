/**
 * @file
 * @author Lisong Sun
 * 
 * Replacement for EDSIMount
 *
 */
#include "util/Bookmark.H"


Bookmark::Bookmark()
{}


Bookmark::Bookmark(const string & name, const string & url) :
  p_name(name), p_url(url) {}


Bookmark::Bookmark(const Bookmark & rhs)
{
  *this = rhs;
}


Bookmark::~Bookmark()
{}


string Bookmark::getName()
{
  return p_name;
}


string Bookmark::getUrl()
{
  return p_url;
}


EcceURL Bookmark::getEcceUrl()
{
  return EcceURL(p_url);
}


void Bookmark::setName(const string & name)
{
  p_name = name;
}


void Bookmark::setUrl(const string & url)
{
  p_url = url;
}


bool Bookmark::operator==(const Bookmark& rhs) const
{
  return (p_name == rhs.p_name && p_url == rhs.p_url);
}


Bookmark& Bookmark::operator=(const Bookmark& rhs)
{
  if (this != &rhs) {
    p_name = rhs.p_name;
    p_url = rhs.p_url;
  }
  return *this;
}
