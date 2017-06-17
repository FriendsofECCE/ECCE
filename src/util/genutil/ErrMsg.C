//#define DEBUG
///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: ErrMsg.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <fstream>
using std::ifstream;
#include <iostream>
using std::cout;
using std::endl;

#include <stdlib.h>  // exit()
#include <stdio.h>  // sprintf() 
#include <stdarg.h>  // va_start()
#include <string.h>

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/EcceMap.H"
#include "util/KeyValueReader.H"
#include "util/EcceRegexp.H"

///////////////////////////////////////////////////////////////////////////////
// class statics
///////////////////////////////////////////////////////////////////////////////
EcceMap        *ErrMsg::p_refMsgs=0;
vector<string>  ErrMsg::p_ids;
vector<string>  ErrMsg::p_msgs;
vector<string>  ErrMsg::p_file;
vector<int>     ErrMsg::p_severities;
vector<int>     ErrMsg::p_line;

const char *ErrMsg::p_sevStr[] = {"EE_INFO","EE_WARNING","EE_FATAL"};


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
ErrMsg::ErrMsg(void)
{
  ensureInitialization();
}


 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
///  man
//
//  Description
//    Nothing to do.  Calling program should flush messages before
//    the program terminates.
//
///////////////////////////////////////////////////////////////////////////////
ErrMsg::~ErrMsg(void)
{
}


// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Add the message to the queue.  The caller is responsible for 
//    using a defined id (see the system errmsg file).  If an unknown
//    id is used, the message will be "Undefined Message ID".  The caller
//    is also responsible for supplying the correct number of args that
//    are of the right type for the message.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
void ErrMsg::message(const string& id,ErrSeverity sev,const char *file,
		     int line...)
{
  static char str[1024];
  string msg;
  EcceMap::iterator it;

  if ( (it = p_refMsgs->find(id)) == p_refMsgs->end()) {
    msg = "Undefined Message ID: " + id;
  } else {

    msg = (*it).second;
    EcceRegexp re("%[l]*[cdsf]");

    //  Rigamaro to get variable arguments into message
    if ( msg.length() > 0 ) {
      va_list ap;
      string type;
      int idx=0;
      int foundIdx;
      int substrLength;
      va_start(ap, line);

      // Use a regular expression to search for a printf()-like format string
      while((type = re.match(msg,foundIdx,substrLength, idx)).length() >0 ) {

        // Get the index to the printf()-like format string
        idx = foundIdx;

        // Create a string corresponding to the format and the dta passed in
        if ( type == "%d" )       sprintf(str, "%d", va_arg(ap, int));
        else if ( type == "%ld" ) sprintf(str, "%ld", va_arg(ap, long));
        else if ( type == "%f" )  sprintf(str, "%lf", va_arg(ap, double));
        else if ( type == "%lf" ) sprintf(str, "%lf", va_arg(ap, double));
        else if ( type == "%c" ) {
          sprintf(str, "%c", va_arg(ap, int));
        } else if ( type == "%s" ) {
          // This is recursive and won't work so die the hard way!
          //EE_RT_ASSERT(va_arg(ap,char*),EE_UTIL|EE_WARNING,"Null pointer");
          char *cptr = va_arg(ap,char*);
          if (cptr == (char*)0) {
            // If we get here, either the caller passed a 0 pointer for
            // a string or there weren't enough arguments on the stack.
            // The fatal assert will cause an abort.
            cerr << "Null pointer passed in var args -  gotta die!" << endl;
            EE_RT_ASSERT(cptr,EE_FATAL,msg);
          } else {
            sprintf(str, "%s", cptr);
          }
        } else {
          cerr << "Invalid format type:  '" << type << "'" << endl;
        }

        // Replace the printf()-like substring with the corresponding string
        msg = msg.replace(idx, type.length(), str);
        idx += strlen(str);
      }
      va_end(ap);
    }
  }

  p_ids.push_back(id);
  p_severities.push_back(sev);
  p_msgs.push_back(msg);
  p_file.push_back(file);
  p_line.push_back(line);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//     Removes all messages from the queue.
//
///////////////////////////////////////////////////////////////////////////////
void ErrMsg::flush(void)
{

if (p_ids.size() > 0) {

#if (!defined(INSTALL) & defined(DEBUG))
cout << "Dumping queue because it is flushed in bulk: " << endl;
dump();
#endif

    p_ids.clear();
    p_msgs.clear();
    p_severities.clear();
    p_file.clear();
    p_line.clear();
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Removes the last instance of the specified id from the queue.  It
//    the id is not found, no action is taken.
//
///////////////////////////////////////////////////////////////////////////////
void ErrMsg::flush(const string& id)
{
  for (int idx=p_ids.size()-1; idx>=0; idx--) {
    if (p_ids[idx] == id) {
      flush(idx);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Removes the message at the specfied id.  If the idx is out of
//    range, no action is taken.
//
///////////////////////////////////////////////////////////////////////////////
void ErrMsg::flush(size_t idx)
{
  if (idx < p_ids.size()) {
    vector<string>::iterator i_ids = p_ids.begin();
    vector<string>::iterator i_msgs = p_msgs.begin();
    vector<string>::iterator i_file = p_file.begin();
    vector<int>::iterator i_line = p_line.begin();
    vector<int>::iterator i_severities = p_severities.begin();
    i_ids += idx;
    i_msgs += idx;
    i_file += idx;
    i_line += idx;
    i_severities += idx;
    p_ids.erase(i_ids);
    p_msgs.erase(i_msgs);
    p_file.erase(i_file);
    p_line.erase(i_line);
    p_severities.erase(i_severities);
  }
}

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//     Returns the number of messages currently in the queue.
//
///////////////////////////////////////////////////////////////////////////////
int ErrMsg::count(void) const
{
  return p_ids.size();
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the specified id exists within the queue.
//
///////////////////////////////////////////////////////////////////////////////
bool ErrMsg::check(const string& id) const
{
  for (size_t i = 0; i<p_ids.size(); i++) {
    if (p_ids[i] == id) return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the specified error severity level exists 
//    within the queue.
//
///////////////////////////////////////////////////////////////////////////////
bool ErrMsg::check(ErrSeverity sev) const
{
  for (size_t i = 0; i<p_severities.size(); i++) {
    if (p_severities[i] == sev) return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns the message text for the last occuring instance of the
//    specified message id.
///////////////////////////////////////////////////////////////////////////////
string ErrMsg::messageText(const string& id) const
{
  string ret;
  for (int idx=p_ids.size()-1; idx>=0; idx--) {
    if (p_ids[idx] == id) {
      ret = p_msgs[idx];
      break;
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//     Returns the id of the last message in the queue.  An empty
//     string is returned if the queue is empty.
//
///////////////////////////////////////////////////////////////////////////////
string ErrMsg::last(void) const
{
  string ret;
  if (p_ids.size() > 0) ret = p_ids[p_ids.size()-1];
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns the id of the error message at the specified index.
//
///////////////////////////////////////////////////////////////////////////////
string ErrMsg::getId(size_t idx) const
{
  EE_ASSERT(idx<p_ids.size(),EE_WARNING,"Index out of Range");
  return p_ids[idx];
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//     Returns the id of the first message in the queue.  An empty
//     string is returned of the queue is empty.
//
///////////////////////////////////////////////////////////////////////////////
string ErrMsg::first(void) const
{
  string ret;
  if (p_ids.size() > 0) ret = p_ids[0];
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Dumps the contents of the message queue (fifo).  If no file name
//    is specified, the contents are dumped to stderr and the append
//    argument is not used.
//
///////////////////////////////////////////////////////////////////////////////
void ErrMsg::dump(void) const
{
  for (size_t idx=0; idx< p_ids.size(); idx++) {
    cerr << endl;
    cerr << "Message:	" << p_ids[idx] << endl;
    cerr << "Location:	" << p_file[idx] << ":" << p_line[idx] << endl;
    cerr << "Severity:	" << severityToString((ErrSeverity)p_severities[idx]) << endl;
    cerr << "Contents:	" << p_msgs[idx] << endl;
  }
}

// ---------- General Methods  ----------



// --------------------------
// Protected Member Functions
// --------------------------
ErrMsg::ErrMsg(const ErrMsg& XXX)
{
  // Hidden copy constructor
}


///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
void ErrMsg::ensureInitialization(void)
{
  if (p_refMsgs == (EcceMap*)0) {

    // This is a leak
    p_refMsgs = new EcceMap;

    string msgFile = Ecce::ecceHome();
    msgFile += "/data/client/config/errmsg";
    ifstream istr(msgFile.c_str());
    if (!istr) {
      cerr << "Unable to open error message file: " << msgFile << endl;
    } else {
      KeyValueReader reader(istr);
      string key,value;
      while (reader.getpair(key,value)) {
        (*p_refMsgs)[key] = value;
      }
      istr.close();
    }
  }
}


// ------------------------
// Private Member Functions
// ------------------------

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Convert ErrSeverity to a string representation.
//
///////////////////////////////////////////////////////////////////////////////
string ErrMsg::severityToString(ErrSeverity sev)
{
  string ret = "Invalid";
  if (sev >= 0 && (size_t)sev < sizeof(p_sevStr)/(sizeof(char*))) {
    ret = p_sevStr[sev];
  }
  return ret;
}


// ---------- Encapsulated Behavior ----------

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
