// CCookie.H
// Created: 06/02/1998
// Version: 05/10/2000
//
// Description: 
//
//   HTTP Cookie and cookie map classes
//
// Modifications: 

#if !defined( __CCOOKIE_H )
#define __CCOOKIE_H  // prevent multiple includes

// include files ***********************************************************

#include <string>
#include <map>

// class: CCookie **********************************************************

class CCookie
{
public :

  typedef std::string string_type;
  struct ParseError {};

  // Constructors
  CCookie(void);
  CCookie(const string_type&) throw(CCookie::ParseError);
  CCookie(const CCookie&);

  // Destructor
  virtual ~CCookie(void);

  // Assignment operator
  CCookie& operator = (const CCookie&);

  // Relational operators
  bool operator == (const CCookie& other) const
  { return compare(other) == 0; }
  bool operator != (const CCookie& other) const
  { return compare(other) != 0; }
  bool operator <  (const CCookie& other) const
  { return compare(other) < 0; }
  bool operator <= (const CCookie& other) const
  { return compare(other) <= 0; }
  bool operator >  (const CCookie& other) const
  { return compare(other) > 0; }
  bool operator >= (const CCookie& other) const
  { return compare(other) >= 0; }

  // Compare self to <other> on path(), then name(), then value():
  //   Return -1 if self is less than other
  //   Return  0 if self is equal to other
  //   Return  1 if self is greater than other
  virtual int compare(const CCookie& other) const;

  // Get/Set the domain of the cookie.
  virtual const string_type& domain(void) const           { return domain_; }
  virtual void               domain(const string_type& s) { domain_ = s; }

  // Get/Set the expire date of the cookie.
  // The date format is: Wdy, DD-Mon-YYYY HH:MM:SS GMT
  virtual const string_type& expires(void) const
  { return expires_; }
  virtual void               expires(const string_type& s)
  { if (validateDate(s)) expires_ = s; }

  // Initialize self's values.
  virtual void init(void);

  // Get/Set the name of the cookie.
  virtual const string_type& name(void) const           { return name_; }
  virtual void               name(const string_type& s) { name_ = s; }

  // Parse a cookie value string into self and return true.
  // Return false on failure.
  virtual bool parse(const string_type&);

  // Get/Set the path of the cookie.
  virtual const string_type& path(void) const           { return path_; }
  virtual void               path(const string_type& s) { path_ = s; }

  // Get/Set if the cookie is secure.
  virtual bool secure(void) const { return secure_; }
  virtual void secure(bool n)     { secure_ = n; }

  // Return self as a string in proper HTTP format:
  //   name=value;expires=value;path=value;domain=value;secure
  virtual string_type toString(void) const;

  // Get/Set the value of the cookie.
  virtual const string_type& value(void) const           { return value_; }
  virtual void               value(const string_type& s) { value_ = s; }

protected :

  string_type domain_,
              expires_,
              name_,
              path_,
              value_;
  bool        secure_;

  // Strip leading/trailing whitespace and trailing newline/CRLF if any
  static string_type strip(const string_type&);

  // Validate a date string.
  static bool validateDate(const string_type&);
};

// class: CCookieMap *******************************************************

class CCookieMap
{
public :

  typedef CCookie::string_type                  string_type;

  // We use a multi map to associate multiple cookies with a single path
  // <Key> is the cookie's path, <Value> is the cookie instance
  typedef std::multimap<string_type, CCookie>   cookie_map;
  typedef cookie_map::size_type                 size_type;

  // Constructors
  CCookieMap(void) : cookiesByName_(), cookiesByPath_() {}
  CCookieMap(const CCookieMap& other)
    : cookiesByName_(other.cookiesByName_),
      cookiesByPath_(other.cookiesByPath_) {}

  // Destructor
  virtual ~CCookieMap(void) { clear(); }

  // Assignment operator
  CCookieMap& operator = (const CCookieMap& other)
  {
    if (&other != this)
    {
      cookiesByName_ = other.cookiesByName_;
      cookiesByPath_ = other.cookiesByPath_;
    }
    return *this;
  }

  // Clear cookies
  virtual void clear(void);

  // Return the current cookies (read only)
  virtual const cookie_map& cookies(void) const { return cookiesByPath_; }

  // Erase the specified cookie and return true.
  // Return false if not found.
  virtual bool erase(const CCookie&);

  // Add a new cookie to self
  // Cookies with the same path and name/value overwrite each other, with
  // the latest instance replacing the older.
  virtual void insert(const CCookie&);

  // Return the number of cookies in self
  virtual size_type size(void) const { return cookiesByPath_.size(); }

  // Return self as a string in proper HTTP format:
  //   cookie1[; cookie2; ... cookieN]
  // If the bool argument is false, write only the cookie name=value,
  // use CCookie::toString() otherwise
  virtual string_type toString(bool = false) const;

protected :

  cookie_map  cookiesByName_,   // key on CCookie::name()
              cookiesByPath_;   // key on CCookie::path()
};

#endif  // __CCOOKIE_H

// end of CCookie.H ********************************************************

