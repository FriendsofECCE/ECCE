///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ActivityLog.H
//
//
// CLASS SYNOPSIS:
//    Class ActivityLog stores log structure in an XML document. 
//
//
// LOCAL TYPES:
//
// SEE ALSO:
//
// DESCRIPTION:
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ACTIVITYLOG_H
#define _ACTIVITYLOG_H

#include <string>

  using std::string;

class TDateTime;

class ActivityLog {

  public:

    //Constructor 
      ActivityLog(const char* doc);
      ActivityLog();
      ~ActivityLog();


      enum EventType 
      {
      // Write once: these types are not updated one unit at a time
         INFO,                 // Useful information 
         MESSAGE,              // Communique from local or remote process. 
         WARNING,              // Condition that may cause future problems.
         ERROR,                // Condition that is a currrent problem, but
			       // not fatal.
         FATAL,                // Condition that is a currrent problem, and 
			       // is fatal.
         GENERAL               // Misc. type, but is useful reporting.   
       };

    //Accessors
    static string   eventTypeToStr (ActivityLog::EventType eventType);
    static ActivityLog::EventType   strToEventType (const string& str);
    static string startLog(const string& rootString);
    static string endLog(const string& rootString);
    static string entry(ActivityLog::EventType type, const char* event, 
                        const TDateTime& date, const char* description);



};

#endif

