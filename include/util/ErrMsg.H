///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ErrMsg.H
//
//
// CLASS SYNOPSIS:
//   Class for communicating errors between code modules.
//
// EXPORTED TYPES:
//    Class ErrMsg
//    typedef ErrSeverity
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//    An ordered queue of messages to communicate between modules of code.
//    Currently predefined messages are required.  These are stored in
//    $ECCE_HOME/data/config/errmsg.  These messages are intended to
//    be suitable to be sent directly to a user.
//
//    EE_ASSERT and EE_RT_ASSERT are ecce's assertion macros.  EE_ASSERT
//    will be compiled out when INSTALL is defined and so can be 
//    liberally inserted in code to ensure correct programming.
//    Fatal asserts (EE_FATAL) call abort(3) which will generate a
//    a core if possible.
//
//    GDB 1/30/02
//    Don't want core files for FATAL RT_ASSERTs because INSTALL builds
//    don't have debug information anyway.
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ERRMSG_HH
#define ERRMSG_HH

#include <stdlib.h>        // abort
#include <stdarg.h>
#include <iostream>
  using std::cerr;
  using std::endl;


#include <string>
  using std::string;
#include <vector>
  using std::vector;

#define  WHERE   __FILE__, __LINE__

class ErrMsg;
class EcceMap;



enum ErrSeverity { EE_INFO=0, EE_WARNING, EE_FATAL};

class ErrMsg
{

public:
     ErrMsg(void);
     virtual ~ErrMsg(void);

     // Modifiers
          //file/line are the WHERE macro
     void message(const string& id,ErrSeverity sev,const char *file,int line...);
     void flush(void);
     void flush(const string& id);
     void flush(size_t idx);

     // Accessors
     int count(void) const;
     bool check(const string& id) const;
     bool check(ErrSeverity sev) const;
     string getId(size_t idx) const;
     string last(void) const;
     string first(void) const;
     string messageText(const string& id) const;

     void dump(void) const;

protected:

     ErrMsg(const ErrMsg&);
     void ensureInitialization(void);

private:
     static EcceMap *p_refMsgs;

     static vector<string>            p_ids;
     static vector<string>            p_msgs;
     static vector<string>            p_file;
     static vector<int>               p_severities;
     static vector<int>               p_line;

     static const char *p_sevStr[];

     static string severityToString(ErrSeverity sev);


}; // ErrMsg


////////
// EE_ASSERTS are setup to be compiled out at install by defining
// -DINSTALL at build time.  EE_RT_ASSERTS are never compiled out.
////////
#ifdef INSTALL
#define  EE_ASSERT(expr, sev, txt)
#else
#define  EE_ASSERT(expr, sev, txt) \
if ( !(expr) ) \
{ \
  cerr << "ASSERTION " << __FILE__ << ":" << __LINE__; \
  cerr << endl;                   \
  cerr << "Assertion ("#expr") failed " << txt << "\n";    \
  if (sev == EE_FATAL) abort(); \
}
#endif /* INSTALL */

#ifdef INSTALL
#define  EE_RT_ASSERT(expr, sev, txt) \
if ( !(expr) ) \
{ \
  cerr << "ASSERTION " << __FILE__ << ":" << __LINE__; \
  cerr << "\n";                   \
  cerr << "Assertion ("#expr") failed " << txt << "\n";    \
  if (sev == EE_FATAL) exit(1);                            \
}
#else
#define  EE_RT_ASSERT(expr, sev, txt) \
if ( !(expr) ) \
{ \
  cerr << "ASSERTION " << __FILE__ << ":" << __LINE__; \
  cerr << "\n";                   \
  cerr << "Assertion ("#expr") failed " << txt << "\n";    \
  if (sev == EE_FATAL) abort();                            \
}
#endif /* INSTALL */




#endif /* ERRMSG_HH */
