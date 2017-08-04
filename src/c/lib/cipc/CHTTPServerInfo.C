// CHTTPServerInfo.C
// Created: 05/10/2000
// Version: 05/23/2000
//
// Description:
//
//   Class CHTTPServerInfo can be used to hold basic server information
//   from a set of headers returned from a server request
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPRequest.H"
#include "dsm/CHTTPUtils.H"
#include "dsm/CHTTPServerInfo.H"

#include <cstdlib>

// Constructors: CHTTPServerInfo *******************************************

CHTTPServerInfo::CHTTPServerInfo(void)
  : chunkedTE_(false), keepAlive_(false),
    auth_(AUTH_NONE), timeout_(0), max_(0),
    realm_(""), server_(""), methods_()
// Pre:
// Modifies:
// Post: Create a new instance
{
}

CHTTPServerInfo::CHTTPServerInfo(const CHTTPHeader& h)
  : chunkedTE_(false), keepAlive_(false),
    auth_(AUTH_NONE), timeout_(0), max_(0),
    realm_(""), server_(""), methods_()
// Pre: Assigned(h)
// Modifies:
// Post: Create a new instance from <h>
{
  getInfo(h);
}

CHTTPServerInfo::CHTTPServerInfo(const CHTTPServerInfo& other)
  : chunkedTE_(other.chunkedTE_), keepAlive_(other.keepAlive_),
    auth_(other.auth_), timeout_(other.timeout_), max_(other.max_),
    realm_(other.realm_), server_(other.server_), methods_(other.methods_)
// Pre: Assigned(other)
// Modifies:
// Post: Create a new instance identical to <other>
{
}

// CHTTPServerInfo::operator = *********************************************

CHTTPServerInfo& CHTTPServerInfo::operator = (const CHTTPServerInfo& other)
// Pre: Assigned(other)
// Modifies:
// Post: Copy other to *this and return *this
{
  if (&other != this)
  {
    chunkedTE_ = other.chunkedTE_;
    keepAlive_ = other.keepAlive_;
    auth_      = other.auth_;
    timeout_   = other.timeout_;
    max_       = other.max_;
    realm_     = other.realm_;
    server_    = other.server_;
    methods_   = other.methods_;
  }
  return *this;
}

// CHTTPServerInfo::clear **************************************************

void CHTTPServerInfo::clear(void)
// Pre:
// Modifies:
// Post: Clear current values
{
  chunkedTE_ = false;
  keepAlive_ = false;
  auth_      = AUTH_NONE;
  timeout_   = 0;
  max_       = 0;
  realm_     = "";
  server_    = "";
  methods_.clear();
}

// CHTTPServerInfo::getInfo ************************************************

void CHTTPServerInfo::getInfo(const CHTTPHeader& h)
// Pre: Assigned(h)
// Modifies:
// Post: Get server information from headers
{
  // Clear current values
  clear();
  if (!h.size())
    return;

  string_type value;

  // Look for Transfer-Encoding
  value = h.header(CHTTPHeader::TRANSFER_ENCODING);
  if (value.length())
  {
    if (to_upper(value).find("CHUNKED") != string_type::npos)
      chunkedTE_ = true;
  }

  // Look for Connection
  value = h.header(CHTTPHeader::CONNECTION);
  if (value.length())
  {
    if (to_upper(value).find("KEEP-ALIVE") != string_type::npos)
      keepAlive_ = true;
  }

  // Look for Keep-Alive
  value = h.header(CHTTPHeader::KEEP_ALIVE);
  if (value.length())
  {
    string_type             tmp  = to_upper(value);
    string_type::size_type  spos = tmp.find("TIMEOUT="),
                            epos;
    if (spos != string_type::npos)
    {
      spos += 8;
      epos  = tmp.find(" ", spos);
      if (epos != string_type::npos)
        timeout_ = std::atoi(value.substr(spos, epos - spos).c_str() );
    }
    spos = tmp.find("MAX=");
    if (spos != string_type::npos)
    {
      spos += 4;
      epos  = tmp.find(" ", spos);
      if (epos != string_type::npos)
        epos = value.length();
      max_ = std::atoi(value.substr(spos, epos - spos).c_str() );
    }
  }

  // Look for authorization
  value = h.header(CHTTPHeader::WWW_AUTHENTICATE);
  if (value.length())
  {
    string_type             tmp  = to_upper(value);
    string_type::size_type  spos = tmp.find("BASIC"),
                            epos;
    if (spos != string_type::npos)
      auth_ = (int) AUTH_BASIC;
    else if ((spos = tmp.find("DIGEST") ) != string_type::npos)
      auth_ = (int) AUTH_DIGEST;
    spos = tmp.find("REALM=");
    if (spos != string_type::npos)
    {
      spos   += 6;
      epos    = value.find(" ", spos);
      if (epos == string_type::npos)
        epos = value.length();
      realm_ = value.substr(spos, epos - spos);
    }
  }
  // TODO else check PROXY_AUTHENTICATE

  // Look for supported methods
  value = h.header(CHTTPHeader::ALLOW);
  if (value.length())
  {
    string_type::size_type  len  = value.length(),
                            spos = 0,
                            epos = value.find(", ", spos);
    while (spos < len)
    {
      if (epos == string_type::npos)
        epos = len;
      int m = methodToInt(value.substr(spos, epos - spos) );
      if (m != -1)
        methods_.push_back(m);
      spos = epos + 2;
      epos = value.find(", ", spos);
    }
  }

  // Look for Server value
  value = h.header(CHTTPHeader::SERVER);
  if (value.length())
    server_ = value;
}

// CHTTPServerInfo::methodToInt ********************************************

int CHTTPServerInfo::methodToInt(const string_type& m) const
// Pre: Assigned(m)
// Modifies:
// Post: If <m> is a valid method string, return it's numeric value.
//       Return -1 if invalid.
// NOTE: This version uses CHTTPRequest::stringToMethod()
{
  return CHTTPRequest().stringToMethod(m);
}

// end of CHTTPServerInfo.C **********************************************

