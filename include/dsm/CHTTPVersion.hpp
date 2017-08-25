// CHTTPVersion.H
// Created: 05/19/2000
// Version: 05/19/2000
//
// Description:
//
// Modifications:

#if !defined( __CHTTPVERSION_H )
#define __CHTTPVERSION_H    // prevent multiple includes

// include files ***********************************************************

#include <string>

// class: CHTTPVersion *****************************************************

class CHTTPVersion
{
public :

  typedef std::string string_type;

  struct ParseError {};

  // Constructors
  CHTTPVersion(
    unsigned major = 1,
    unsigned minor = 1) : major_(major), minor_(minor) {}
  CHTTPVersion(const string_type&) throw(CHTTPVersion::ParseError);
  CHTTPVersion(const CHTTPVersion& other)
    : major_(other.major_), minor_(other.minor_) {}

  // Destructor
  virtual ~CHTTPVersion(void) {}

  // Assignment operator
  CHTTPVersion& operator = (const CHTTPVersion& other)
  {
    if (&other != this)
    {
      major_ = other.major_;
      minor_ = other.minor_;
    }
    return *this;
  }

  // Get/Set the major version number
  virtual unsigned getMajor(void) const { return major_; }
  virtual void     setMajor(unsigned v) { major_ = v; }

  // Get/Set the minor version number
  virtual unsigned getMinor(void) const { return minor_; }
  virtual void     setMinor(unsigned v) { minor_ = v; }

  // Parse a result string into self and return true.
  // Return false on failure.
  virtual bool parse(const string_type&);

  // Return self as a string.
  // If <full> is true, return HTTP/major.minor.
  // Return major.minor otherwise
  virtual string_type toString(bool full = true) const;

protected :

  unsigned  major_, // major version number
            minor_; // minor version number
};

#endif  // __CHTTPVERSION_H

// end of CHTTPVersion.H ***************************************************

