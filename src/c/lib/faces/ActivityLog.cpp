#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
  using namespace xercesc;

#include <vector>
#include <string>

using std::string;
using std::vector;

#include "util/ErrMsg.H"
#include "util/EcceURL.H"
#include "util/TDateTime.H"

#include "dsm/ActivityLog.H"
#include "dsm/BasicDOMWriter.H"

ActivityLog::ActivityLog() {

}

ActivityLog::~ActivityLog() {
}

/**
 *
 *  createContainer() uses the Property_Ref property identity to construct
 *  a shell Property XML document.  The step element creation has been 
 *  separated out to add more versatility to Time Series Property document
 *  implementations.
 *
**/
string ActivityLog::startLog(const string& rootString) {
  string ret;
  ret = BasicDOMWriter::XMLDocumentDeclaration();
  ret += "<" +rootString + ">\n"; 
  return ret;
}


/**
 *
 *  eventTypeToStr() returns a string equivalent of the enumerated
 *  type. 
 *
**/


string  ActivityLog::eventTypeToStr (ActivityLog::EventType eventType)
{
  string ret = "General";
  switch (eventType)
  {
    case ActivityLog::INFO    : ret = "Info";       break;
    case ActivityLog::MESSAGE : ret = "Message";    break;
    case ActivityLog::WARNING : ret = "Warning";    break;
    case ActivityLog::ERROR   : ret = "Error";      break;
    case ActivityLog::FATAL   : ret = "Fatal";      break;
    case ActivityLog::GENERAL : ret = "General";    break;

  }
  return ret;
}

ActivityLog::EventType ActivityLog::strToEventType( const string& str)
{
  ActivityLog::EventType ret = ActivityLog::GENERAL;

  if (str.find("nfo"))           ret = ActivityLog::INFO;
  else if (str.find("essage"))   ret = ActivityLog::MESSAGE; 
  else if (str.find("arning"))   ret = ActivityLog::WARNING; 
  else if (str.find("rror"))     ret = ActivityLog::ERROR;    
  else if (str.find("atal"))     ret = ActivityLog::FATAL;
  else if (str.find("eneral"))   ret = ActivityLog::GENERAL;
  return ret;

}

/***
*
*  Add a log entry 
*  
*  <event type="" name="" date="">
*    notes      
*  </event>
***/

string ActivityLog::entry(ActivityLog::EventType eventType, const char* event,  const TDateTime&  date, const char* description)
{
  string ret;
  string eventStartTag = "     <event ";
  string eventEndTag   = "     </event>\n";
  string typeName = " type=\"";
  string nameName = " name=\"";
  string dateName = " date=\"";
  string typeData = ActivityLog::eventTypeToStr(eventType);
         typeData += "\" ";
  string nameData = event;
         nameData += "\" ";
  string dateData = date.toLocaleString();
         dateData += "\" ";
  string descriptionData = description;

  ret += eventStartTag + typeName + typeData
                       + nameName + nameData
                       + dateName + dateData + ">";
  ret += description;
  ret += eventEndTag;  
  return ret;
 
}



string ActivityLog::endLog(const string& rootString) {
  string ret;
  ret += "</" +rootString + ">\n"; 
  return ret;
}
