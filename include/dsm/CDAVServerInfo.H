// CDAVServerInfo.H
// Created: 05/10/2000
// Version: 05/10/2000
//
// Description:
//
// Modifications:

#if !defined( __CDAVSERVERINFO_H )
#define __CDAVSERVERINFO_H    // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPServerInfo.H"
#include "dsm/CDAVHeader.H"

// class: CDAVServerInfo ***************************************************

class CDAVServerInfo : public CHTTPServerInfo
{
public :

  typedef CHTTPServerInfo parent;

  // Constructors
  CDAVServerInfo(void)
    : parent(), isDAVServer_(false), supportLocks_(false) {}
  CDAVServerInfo(const CDAVHeader& h)
    : parent(), isDAVServer_(false), supportLocks_(false) { getInfo(h); }
  CDAVServerInfo(const CDAVServerInfo& other)
    : parent(), isDAVServer_(other.isDAVServer_),
      supportLocks_(other.supportLocks_) {}

  // Destructor
  virtual ~CDAVServerInfo(void) {}

  // Assignment operator
  CDAVServerInfo& operator = (const CDAVServerInfo& other)
  {
    parent::operator = (other);
    isDAVServer_  = other.isDAVServer_;
    supportLocks_ = other.supportLocks_;
    return *this;
  }

  // Clear current values
  virtual void clear(void)
  { parent::clear(); isDAVServer_ = supportLocks_ = false; }

  // Get server information from headers
  virtual void getInfo(const CDAVHeader&);
  virtual void getInfo(const CHTTPHeader& h) { parent::getInfo(h); }

  // Return true is the server is known to be a DAV server.
  // Return false otherwise.
  virtual bool isDAVServer(void) const { return isDAVServer_; }

  // Return true is the server supports DAV locks
  // Return false otherwise.
  virtual bool supportLocks(void) const { return supportLocks_; }

protected :

  bool  isDAVServer_,   // is this really a DAV server?
        supportLocks_;  // does it support DAV locks?

  // If <m> is a valid method string, return it's numeric value.
  // Return -1 if invalid.
  // NOTE: This version uses CDAVRequest::stringToMethod()
  virtual int methodToInt(const string_type& m) const;
};

#endif  // __CDAVSERVERINFO_H

// end of CDAVServerInfo.H *************************************************

