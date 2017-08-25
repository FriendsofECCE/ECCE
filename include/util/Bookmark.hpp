/**
 * @file
 * @author Lisong Sun
 * 
 */

#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <string>
using std::string;

#include "util/EcceURL.H"

/**
 * A small container class that contains a name/url pair.
 */
class Bookmark
{
public:
  Bookmark();
  Bookmark(const string & name, const string & url);
  Bookmark(const Bookmark & rhs);

  virtual ~Bookmark();

  string getName();
  string getUrl();
  EcceURL getEcceUrl();

  void setName(const string & name);
  void setUrl(const string & url);

  bool operator==(const Bookmark & rhs) const;
  Bookmark & operator=(const Bookmark & rhs);

protected:
  string p_name;
  string p_url;
};
#endif
