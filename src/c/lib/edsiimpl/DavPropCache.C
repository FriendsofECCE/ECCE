//#define GEOMBUFFERING

#include <stdio.h>  // sprintf

#include <strstream>
  using std::ostrstream;
  using std::istrstream;

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ends;
#include <string>
  using std::string;
#include <vector>
  using std::vector;
#include <map>
  using std::map;
#include <deque>
  using std::deque;
  using std::pair;

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
using namespace xercesc;

#include "util/TDateTime.H"
#include "util/ETimer.H"
#include "util/ErrMsg.H"
#include "util/EcceURL.H"

#include "dsm/ActivityLog.H"
#include "dsm/VDoc.H"
#include "dsm/EDSI.H"
#include "dsm/PropFactory.H"
#include "dsm/PropertyDoc.H"
#include "dsm/IPropCache.H"
#include "dsm/IPropCalculation.H"
#include "dsm/DavPropCache.H"
#include "dsm/EDSIFactory.H"
#include "dsm/PropertyInterpreter.H"
#include "dsm/BasicDOMWriter.H"

#include "tdat/Fragment.H"


/**
 *
 *
 *  Initializes the Cache with a calculation URL, sets the storage flag, and
 *  the maximum allowable failures counter.  In the beginning the Xerces XML
 *  singleton is initialized for all subsequent use of the Xerces library.
 *
 * @todo EE_FATAL must be changed so that we can do a signal handle
 *
 */
DavPropCache::DavPropCache(const EcceURL& url, const bool store,const int maxFailures) : IPropCache(store, maxFailures) 
{
   EE_ASSERT(initializeXmlUtils(),EE_FATAL,"initializeXmlUtils not initialized!!! ");
   p_ecceJobLogAll = false;
   p_messageBodyThreshhold = 2048;
   if (getenv("ECCE_JOB_MESSAGE_BODY"))
   {
      string messageBodySize = getenv("ECCE_JOB_MESSAGE_BODY");
      if (!messageBodySize.empty())
      {
         p_messageBodyThreshhold = atoi(messageBodySize.c_str());
      }
   }

   if (getenv("ECCE_JOB_LOGALL")) {
      p_ecceJobLogAll = true;
   }

   p_vdoc = EDSIFactory::getVdoc(url);
   EE_ASSERT(p_vdoc,EE_FATAL,"Virtual document could not be retrieved.");


   p_edsi = EDSIFactory::getEDSI(url);
   p_flushDenied = 0;
}

void DavPropCache::setURL(const EcceURL& url) {
  p_vdoc->setURL(url);
}



DavPropCache::~DavPropCache() 
{
  cleanup();
}



/**
 *
 *				restoreStepToken
 *
 * A token is needed to create a unique attribute name for a step.
 * This attribute name is created by taking the sum of all the steps processed
 * thus far and using that sum as the tag.  We calculate the sum before we
 * actually parse the property stream.  If for some reason the step 
 * cannot be parsed we return the token to its original value. If the token
 * was "1" then we remove the token from the p_tspropsteps map.  
 *
 */
bool DavPropCache::restoreStepToken(const string& key ,
                                       const Property_Ref* propRef)
{
  bool ret = false;
  if ( propRef->classType == TProperty::PROPVALUE     ||
       propRef->classType == TProperty::PROPSTRING    ||
       propRef->classType == TProperty::PROPVECTOR    ||
       propRef->classType == TProperty::PROPVECSTRING ||
       propRef->classType == TProperty::PROPTABLE     ||
       propRef->classType == TProperty::PROPVECTABLE )
    return ret;

  ret = true;
  map <string, int, less<string> >::iterator cur =
                                 p_tspropsteps.find(key);
  if (p_tspropsteps[key] < 2)
    p_tspropsteps.erase(cur);
  else if (p_tspropsteps[key] >= 2)
    p_tspropsteps[key] = p_tspropsteps[key] - 1 ;
  return ret;
}


/**
 *
 * A token is needed to create a unique attribute name for a step.
 * This attribute name is created by taking the sum of all the steps processed
 * thus far and using that sum as the tag.  We calculate the sum before we
 * actually parse the property stream.   The original value is set to "1".
 * 
 */
unsigned long DavPropCache::stepToken(const string& key , 
                                       const Property_Ref* propRef)
{
  unsigned long ret = 0;
  if ( propRef->classType == TProperty::PROPVALUE     ||
       propRef->classType == TProperty::PROPSTRING    || 
       propRef->classType == TProperty::PROPVECTOR    || 
       propRef->classType == TProperty::PROPVECSTRING ||
       propRef->classType == TProperty::PROPTABLE     || 
       propRef->classType == TProperty::PROPVECTABLE )
    return ret; 

  map <string, int, less<string> >::iterator cur =
                                 p_tspropsteps.find(key);
  if ( cur == p_tspropsteps.end())
    p_tspropsteps[key] = 1;
  else
    p_tspropsteps[key] = p_tspropsteps[key] + 1;
  ret =  p_tspropsteps[key] ;
  return ret;
}


/**
 *
 *  After an entry has been parsed into a DOMDocument it needs to be
 *  serialized into a character string so that can be sent in a tool
 *  talk message to other applications such as the Calc Viewer.
 *  
 *  The string calls upon BasicDOMWriter to get the document header
 *  and then recursively serializes the DOMDocument body into a 
 *  string.  This is appended to the XML document declaration 
 *  and returned. 
 *
 */
string DavPropCache::propEntryToString(PropertyDoc& entry)
{
  ostrstream os;
  BasicDOMWriter::write(*(entry.container()),os,false,true);
  os << ends;
  string ret = os.str();
  if (ret.size() > p_messageBodyThreshhold) 
  {
    EcceURL* tSUrl = new EcceURL(p_vdoc->getProperty(entry.reference()->short_name));
    p_edsi->setURL(*tSUrl);
    ret = tSUrl->toString();
    delete tSUrl;
  }    
  return ret;
}


string DavPropCache::TsPropEntryToString(PropertyDoc& entry, int step)
{
  string ret;
  ostrstream os;

  if (  p_dataBufferSavedSize > p_messageBodyThreshhold ) {
    // GDB 12/3/02 Hacked out the "step" logic to get some kind of valid URL
    // sent in the property message.  Doesn't seem to actually need any kind
    // of offset for calcviewer to get the most recent geometry.
#if 0000
//This first seven lines of code are not being used - btd
    DOMElement *root =  entry.container()->getDocumentElement();
    DOMElement *child = (DOMElement *) root->getFirstChild();
    BasicDOMWriter::write(*child,os,false);
    os << ends;
    string body = os.str();
    if (step > 1) 
    {
      EcceURL* tSUrl = new EcceURL(p_vdoc->getPropertyURL().getChild(entry.reference()->short_name.c_str()));
      p_edsi->setURL(*tSUrl);
      unsigned long dataSetSize = p_edsi->getDataSetSize();
      unsigned long stepStart = dataSetSize - body.size() - 1;
      unsigned long stepLength = strlen(body.c_str());
      char startBuf[80];
      char lengthBuf[80];
      sprintf(startBuf, "start=%u&length=%u", stepStart, stepLength);
      string params(startBuf);
      ostrstream local_os;

      DOMElement root =  entry.container().getDocumentElement();
      DOMElement *child = (DOMElement *)root->getFirstChild();
      BasicDOMWriter::writeStartTag(root,local_os);
      BasicDOMWriter::write(domnode,local_os,false);
      local_os << ends;
      char *local_cptr = local_os.str();
      string xmlChunk = "&header=";
      xmlChunk += local_cptr;
      params += xmlChunk;
      EcceURL* paramUrl = new EcceURL(tSUrl->getChild("?"));
      ret = paramUrl->toString();
      ret += params;
      delete tSUrl;
      delete paramUrl;
    } else if ( step == 1)
    {
#endif
      EcceURL* tSUrl = new EcceURL(p_vdoc->getProperty(entry.reference()->short_name));
      p_edsi->setURL(*tSUrl);
      ret = tSUrl->toString();
      delete tSUrl;
#if 0000
    }
#endif
  } else { // p_dataBufferSavedSize <= p_messageBodyThreshhold
    ostrstream local_os;
    BasicDOMWriter::write(*(entry.container()),local_os,false,true);
    local_os << ends;
    ret = local_os.str();
  }

  return ret;
}


/**
*
*				bufferTSProperty
*
* After a property has been parsed and stored in a DOM document the
* property is buffered waiting to be flushed to the server and/or a tool talk 
* message. 
*
* Note that due to the fact that we don't store time step data until the second
* step arrives we have a special map of first step time step data that is
* anxiously awaiting the second step so that it can be stored in the buffer.
* When the second step arrives the ts_buffer gets the first step, stores it
* in the buffer and then buffers the second step (so that they remain in order)
* .  The ts_buffer removes the first step once the first step as been buffered.
* All subsequent steps are buffered without question.  
*
**/
bool DavPropCache::bufferTSProperty( PropertyDoc *entry, 
                                     unsigned long step, 
                                     string& log)
{
  // Change this to turn buffering of first geom step on/off
#ifdef GEOMBUFFERING
  bool doGeomBuffering = true;
#else
  bool doGeomBuffering = false;
#endif

  bool ret = false;
  string message;
  char buf[80];
  sprintf(buf, "%lu", step);
  string stepString(buf);

  if (!entry || step < 1)
  {
    appendToLog(ActivityLog::ERROR, 
                "Cache buffering", "Attempted to buffer NULL entry.", log);
    return ret;
  }

  char* ename = entry->name();

  if (!ename)
  {
    appendToLog(ActivityLog::ERROR, 
                "Cache buffering", 
                "property name is not found in DOM entry, cannot buffer", log);
    delete [] ename;
    return ret;
  }

  QueueEntry queE;
  string entryName(ename);
  if (doGeomBuffering && step == 1)
  {
    if (p_ecceJobLogAll)
    {
      message = "First step of time step property (";
      message += entryName;
      message += ") buffered in temporary buffer.  Step #1 will not be ";
      message += " slotted for permanent storage until second step is parsed.";
      appendToLog(ActivityLog::INFO, 
                 "Cache buffering", 
                  message.c_str(), log);
    }
    p_tsbuffer[entryName] = entry;
  }
  else if (doGeomBuffering && step == 2)
  {
    //
    //We add property name to p_tsproperties to record time step properties
    //that are being saved to storage.  Since time step property steps are
    //appended to the XML storage document separately they need a closure
    //root document element to make the XML document well-formed. 
    //
    p_tsproperties.push_back(entryName);

    map<string, PropertyDoc*, less<string> >::iterator cursor;
    cursor = p_tsbuffer.find(entryName);
   
    if (cursor != p_tsbuffer.end())
    {
      queE.document = (*cursor).second;
      queE.step = 1; 
      p_entries.push_back(queE);
      p_tsbuffer.erase(cursor);
      if ( p_ecceJobLogAll)
      {
        message = "Step #1 of the time step property (";
        message += entryName;
        message += ") is being inserted into permanent storage buffer";
        appendToLog(ActivityLog::INFO, 
                   "Cache buffering", 
                    message.c_str(), log);
      }
      queE.document = entry;
      queE.step = 2; 
      p_entries.push_back(queE);
      if (p_ecceJobLogAll)
      {
        message = "Step #2 of the time step property (";
        message += entryName;
        message += ") is being inserted into permanent storage buffer";
        appendToLog(ActivityLog::INFO, 
                   "Cache buffering", 
                    message.c_str(), log);
      }
    } else
    {
      message = "Step #1 of time step property (";
      message += entryName;
      message += ") not buffered in temporary buffer.  Yet ";
      message += " step #2 is parsed.  A programming logic";
      message += " error exists?  The second property will not be ";
      message += "stored.";
      appendToLog(ActivityLog::ERROR, 
                 "Cache buffering", 
                  message.c_str(), log);
    }
     
  } else

  {
    queE.document = entry;
    queE.step = step;
    p_entries.push_back(queE);

    if ( p_ecceJobLogAll)
    {
      message = "Step #";
      message += stepString;
      message += " of time step property (";
      message += entryName;
      message += ") is in permanent storage buffer";

      appendToLog(ActivityLog::INFO, 
                 "Cache buffering", 
                  message.c_str(), log);
    }
  }
  delete [] ename;
  return ret;
}
/**
*
*                               bufferProperty
*
* After a property has been parsed and stored in a DOM document the
* property is buffered waiting to be flushed to the server and/or a tool talk
* message.
*
**/
bool DavPropCache::bufferProperty( PropertyDoc *entry, 
                                   string& log)
{
  bool ret = true;
  char* cp = NULL;
  QueueEntry queE;
  string message;
  queE.step = 0;
  queE.document = entry;
  if (!entry)
  {
    appendToLog(ActivityLog::ERROR, 
                "Cache buffering", "Attempted to buffer NULL entry.", log);
    ret = false;
  }
  else
  {
    cp = entry->name(); 
    if (cp) 
    {
      p_entries.push_back(queE);
      if ( p_ecceJobLogAll)
      {
        message += "Property (";
        message += cp;
        message += ") is in permanent storage buffer";

        appendToLog(ActivityLog::INFO, 
                   "Cache buffering", 
                    message.c_str(), log);
      }
    }
    else
    {
     appendToLog(ActivityLog::ERROR, 
                 "Cache buffering", 
                 "Property name is not found in DOM entry, cannot buffer", log);
     ret = false;
    }

  }
  delete [] cp;
  return ret;
}

/**
 *
 *				put
 *
 *  This is the interface that jobparser uses when it as prepared a 
 *  stream for parsing.  The stream is a series of key/value pairs beginning
 *  with "KEY:" and ending with "END".   The property interpreter is called
 *  here to actually create the PropertyDoc DOM document from the stream.  A
 *  failure to do so will yield an empty result.  The stream may contain one
 *  (which is usaually the case) or many properties.  
 * 
 *  If the property is successfully parsed it is then checked first to determine
 *  if it a special case attribute of the Calculation Chemical System PNTGRP.
 *  PNTGRP is not a calculation result, but is a setting that the model 
 *  overrides when symmetry is determined.
 *
 *  If property truly is a calculation property the property is buffered in
 *  the cache to be eventually flushed.  The first time step is not actually
 *  buffered, but stored in a temporary cache until the second time step 
 *  arrives.  
 *  
 *  All property data is buffered in a FIFO queue (stl deque) awaiting the 
 *  flush attempt at the end of the put() function.  
 *
 *  If the property data is either successfully stored or if the storage
 *  setting was set to false (see constructor) the property entry will 
 *  be inserted into the propertyMapPairToValue so that a tooltalk message
 *  can be eventually created to announce the new property to other applications.* 
 *  Log messages are created using the ActivityLog class they are concatenated
 *  together and then returned by the put method.
 *
 *  Put will only return false when the maximum number of attempts to store
 *  data on the DAV server have been exceeded throughout the entire lifetime
 *  of the DavPropCache object.
 *
 * If this is not called, no memeory is leaked.
 */
bool DavPropCache::put(istream& propStream, propertyMapPairToValue& properties,  string& log) {
  
//#define timertest
#ifdef timertest
  ETimer timer;
  timer.start();
#endif
  bool ret = true;
  string newvalue;
  string token, key, value;
  PropertyDoc *entry = 0;

  unsigned long step = 0;
  PropertyInterpreter pi;
  string message;
  properties.clear();
  while (propStream.good()) {
  
    while (propStream.good() && (token.rfind("key:") == string::npos)) propStream >> token;
    if (propStream.good()) {
      propStream >> token;
      newvalue = token;

//  If value is a size, label, key from the script 
//  instead of a property key abort. 

      if (newvalue.find(":") != string::npos) 
      {
        message = "Missing property name in stream token value is (";
        message += newvalue; 
        message += ")";
        appendToLog(ActivityLog::ERROR, "Cache parsing", message.c_str(), log);

        continue;
      }
      const Property_Ref* propRef = PropFactory::getPropRef(token);
      if (!propRef)
      {
        message = "Unable to find property name in reference table token ";
        message += "name is (";
        message += token; 
        message += ")"; 
        appendToLog(ActivityLog::ERROR, "Cache parsing", message.c_str(), log);

        continue;
      }
      key = token;
      step = stepToken(key, propRef);
      entry = pi.parse(propStream, token, log, step);
#ifdef timertest
  timer.stop();
  cout << "time to parse " << timer.elapsedTime() << endl;
  timer.start();
#endif
      //
      // Couldn't parse token so if step property setting step
      // value - 1 so that we don't have holes in the step sequence.  
      //
      if (!entry)
      {
        message = "Unable to transform property into cache entry.  No ";
        message += "entry create for property (";
        message += token; 
        message += ")"; 
        appendToLog(ActivityLog::ERROR, "Cache parsing", message.c_str(), log);
        restoreStepToken(key, propRef);
        break; 
      }

      if (key.find("PNTGRP") != string::npos)
      {
         EcceURL url = p_vdoc->getConfiguration(p_vdoc->getMoleculeName());

         if (!storeMetadata(*entry, log, 
		      url, VDoc::getEcceNamespace(),(string)":symmetrygroup"))
          break;
      }

      if (key.find("VERSION") != string::npos)
      {
         EcceURL url = p_vdoc->getBaseURL();

         if (!storeMetadata(*entry, log, 
		      url, VDoc::getEcceNamespace(),":version"))
          break;
      }

      switch(propRef->classType)
      {
        case TProperty::PROPVALUE         :
        case TProperty::PROPSTRING        :
        case TProperty::PROPVECTOR        :
        case TProperty::PROPVECSTRING     :
        case TProperty::PROPTABLE         : 
        case TProperty::PROPVECTABLE      : 
                         bufferProperty(entry,  log); break;
        case TProperty::PROPTSTABLE       :
        case TProperty::PROPTSVECTABLE    :
        case TProperty::PROPTSVECSTRING   :
        case TProperty::PROPTSVECTOR      :
        case TProperty::PROPTSVECVECTOR   : 
                          bufferTSProperty(entry, step, log); break;
        case TProperty::PROPVECGRID       : 
        case TProperty::SUPER             : 
                          break;

      }
#ifdef timertest
  timer.stop();
  cout << "time to buffer " << timer.elapsedTime() << endl;
  timer.start();
#endif
    }

    flush(properties, log);
#ifdef timertest
  timer.stop();
  cout << "time to flush " << timer.elapsedTime() << endl;
  timer.start();
#endif
  }
  //This false return value will send a message to the application that
  //the maxStorageFailures has been exceeded.  The application will probably
  //need to bail at this point.
  if (p_flushDenied >  m_maxStorageFailures)
    ret = false; 
  return ret;
} 

/**
*
*		appendToLog
*
* Appends a message to the log string.  This string will be returned from
* the put() function and be stored on the server once put is done parsing
* the property stream.
*
**/
void DavPropCache::appendToLog(const ActivityLog::EventType type, 
                               const char* name,
                               const char* message, 
                               string& log)
{
  TDateTime date;
  log += ActivityLog::entry(type, name, date ,message);       
}

/**
 *
 *				flush
 *
 * Attempt to save data to storage system if the m_storageFlag has been set.
 * If successful flushing then add properties to propertyMapPairToValue
 * map.  Note that if the m_storageFlag is set to false then storage will
 * always return true and a property will always be added to the 
 * propertyMapPairToValue. 
 *
 * If dav errors occur, flush will be called repeatedly and chew up the
 * CPU.  We haven't seen this problem so have not attempted to fix it but
 * having a delay of some sort would be useful in this case.
 *
 */
bool DavPropCache::flush(propertyMapPairToValue& properties, string& log ) 
{
  bool ret = true;
  deque<QueueEntry>::iterator entryIt = p_entries.begin(); 
  string message;
  string entryName;
  entryIt = p_entries.begin();
  //PropertyDoc *tempProp;
  while (entryIt != p_entries.end())
  {
    // NOTE: storeProperty does a pop_front to remove item from
    // queue. Seems like a bad way to implement but not changing now.
    if (!storeProperty(*entryIt, properties, log))
    {
      ret = false;
      break;
    }
    // GDB 3/12/2012 deleting here seemed to work fine under RHEL, but
    // causes eccejobstore to go out to lunch on OpenSUSE and likely
    // other platforms.  I found that if I called delete in the
    // storeProperty method instead it would work properly.  So my
    // guess is that there a horse before the cart issue and the
    // delete has to happen prior to the pop_front() call in
    // storeProperty.
    // Very twilight zone that I happened also to be making a fix to this
    // class on 12/3/2002 and it's now 3/12/2012...
    // clean up allocated PropertyDoc - btd 10/27/2003
    //tempProp = (*entryIt).document;
    entryIt++;
    //delete tempProp;
  }
  return ret;

} 


/**
 * Store a metadata property on specified document.
 *
 * This method exists to handle our outlyer cases - data that is not
 * considered properties but is extracted from the output file.
 *
 * @returns false if storing it failed.
 */
bool DavPropCache::storeMetadata(PropertyDoc& entry, string& log,
		const EcceURL& url, 
		const string& ns, const string& property)
{ 
	//cout << "STORE " << url.toString() << ns << " " << property << endl;
  bool ret = true;
  p_edsi->setURL(url);

  vector<MetaDataResult> results(1);
  results[0].name = ns  + property;
  char* pv = entry.values(entry.lastStep()-1);
  results[0].value = pv;
  delete [] pv; 
  if (p_edsi->putMetaData(results)) 
    {
      if ( p_ecceJobLogAll)
      {
        appendToLog(ActivityLog::INFO,
                    "Cache parsing",
                    (property + " successfully saved").c_str(), log);
      }
      ret = false;
    }
    else
    {
      appendToLog(ActivityLog::ERROR,
                  "Cache parsing",
                  (property + " parsed but was unable to save").c_str(), log);
    }

  return ret;
}


/**
 *
 * stores both time series and non-time series properties based on their
 * TProperty enumerated classType.  A failure to store yields a false
 * return value;
 *
 */
bool DavPropCache::storeProperty(QueueEntry& entry,
                                 propertyMapPairToValue& properties, 
                                 string& log)
{
  bool ret = true;
  string message;
  string entryName;
  if (m_storageFlag)
  {
    switch(entry.document->reference()->classType)
    {
      case TProperty::PROPVALUE         :
      case TProperty::PROPSTRING        :
      case TProperty::PROPVECTOR        :
      case TProperty::PROPVECSTRING     :
      case TProperty::PROPTABLE         :
      case TProperty::PROPVECTABLE      :  
      {
        // fix memory leadk
        char *crap = entry.document->name();
        entryName = crap;
        delete [] crap;
        ret = putProp(*entry.document); 
        if (ret)
        {
          properties[ pair<string, int>(entryName, entry.step)  ] = 
            propEntryToString(*entry.document);
          // GDB 3/12/2012  Calling delete here seems OK (after
          // the pop_front() call sends it out to lunch)
          delete entry.document;
          p_entries.pop_front();
        }
        break;
      }
      case TProperty::PROPTSTABLE       :
      case TProperty::PROPTSVECTABLE    :
      case TProperty::PROPTSVECSTRING   :
      case TProperty::PROPTSVECTOR      :
      case TProperty::PROPTSVECVECTOR   :  
      {
        // fix memory leadk
        char *crap = entry.document->name();
        entryName = crap;
        delete [] crap;
        ret = putTSProp(*entry.document); 
        if (ret)
        {
          properties[ pair<string, int>(entryName, entry.step)  ]  = 
            TsPropEntryToString(*entry.document, entry.step);
          // GDB 3/12/2012  Calling delete here seems OK (after
          // the pop_front() call sends it out to lunch)
          delete entry.document;
          p_entries.pop_front();
        }
        break;
      }
      case TProperty::PROPVECGRID       :  
      case TProperty::SUPER             :  
        break;
    }
  }


  if (!ret) 
  {
    char flushNumBuf[32];
    p_flushDenied++;
    sprintf(flushNumBuf, "%u", p_flushDenied);
    message = "Unable to flush property (";
    message += entryName;
    message += ") to permanent storage flush failure count is ";
    message += flushNumBuf;
    appendToLog(ActivityLog::WARNING,
                "Cache storage",
                message.c_str(), log);
    ret = false;
  }
  return ret;

}

/**
*
*				putProp
*
*  Serializes the DOM document into a stream and saves the stream out to the
*  EDSI.  If unsuccessful putProp returns false;
*
**/
bool DavPropCache::putProp(PropertyDoc& entry) {
  bool ret = true;

#ifdef timertest
  ETimer timer;
  cout << "getting it from BasicDOMWriter" << endl;
  timer.start();
#endif

  ostrstream os;
  BasicDOMWriter::write(*(entry.container()),os,false,true);
  os << ends;
#ifdef timertest
  timer.stop();
  cout << "time to dump xml " << timer.elapsedTime() << endl;
  timer.start();
#endif

  char *cptr = os.str();
  p_dataBufferSavedSize = strlen(cptr);
  EcceURL* tSUrl = new EcceURL;
  istrstream is(cptr);
  try {
    *tSUrl = p_vdoc->addProperty(entry.reference()->short_name, &is);
    p_edsi->setURL(*tSUrl);
  }
  catch (DavException& davException) {
    ret = false;
  }


#ifdef timertest
  timer.stop();
  cout << "time to do put" << p_dataBufferSavedSize << " "  << timer.elapsedTime() << endl;
  timer.start();
#endif


  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":mime-type";
  results[0].value = "text/xml";
  if (ret)
  {
     ret = p_edsi->putMetaData(results);  
  }

  delete tSUrl; 

#ifdef timertest
  timer.stop();
  cout << "time for metadata" << timer.elapsedTime() << endl;
#endif

return ret;
}

/**
*
*				putTSProp
*
*  Serializes the DOM document into a stream and saves the stream out to the
*  EDSI.  If the document is currently available on the server the document
*  is appended to.  If unsuccessful putTSProp returns false;
*
*  Note that when appending data that the root node is not rewritten only the
*  step that was processed.  This of course depends on the condition that only
*  one step is being parsed at a time which is true;
*
**/
bool DavPropCache::putTSProp(PropertyDoc& entry) {
  ostrstream os;
  bool ret = true;
  string propName = entry.reference()->short_name;
  EcceURL* tSUrl = new EcceURL(p_vdoc->getProperty(propName));
  p_edsi->setURL(*tSUrl);
  if (!p_edsi->exists()) {
    BasicDOMWriter::write(*(entry.container()),os,false,true);
    os << ends;
    char *cptr = os.str();
    p_dataBufferSavedSize = strlen(cptr);
    istrstream is(cptr);
    try {
      *tSUrl = p_vdoc->addProperty(propName, &is);
      p_edsi->setURL(*tSUrl);
    }
    catch (DavException& davException) {
      ret = false;
    }
    vector<MetaDataResult> results(1);
    results[0].name = VDoc::getEcceNamespace() + ":mime-type";
    results[0].value = "text/xml";
    if (ret) {
      ret = p_edsi->putMetaData(results);  
    }
  } else {
    //In order to get consistent formatting for Time Series data we have to
    //manually append the data, and close tag it property, so construct the 
    // close tag, enclosed in </ and >
    const Property_Ref *propRef = entry.reference();
    string endTag = "</";
    endTag += propRef->storagetype();
    endTag += ">"; 

    DOMElement *root =  entry.container()->getDocumentElement(); 
    DOMElement *child = (DOMElement *)root->getFirstChild();
    BasicDOMWriter::write(*child,os,false,false);

    //pretty-print formatting
    os << endl << endl << endTag << endl << ends;  
    string xmlDoc = "  ";  //to indent the line like pretty-print
    xmlDoc += os.str();
    //cout << "trying to write timestep property " << xmlDoc << endl;

    // Since the data in the db already has a close tag, we need to
    // append to the end minus the close tag length and the newlines, so 
    // the existing close tag and pretty-printing is overwritten
    ret = p_edsi->appendDataSet(xmlDoc.c_str(), endTag.length()+2);

    p_dataBufferSavedSize = xmlDoc.length();
  }
  delete tSUrl;
  return ret;
}

/**
*
*				cleanup
*
* public function which the application can call to finish up any writing.
* Currently cleanup calls apppendTsCloseTag() to add the closing tag of 
* the document root element.  We do this because time step data is appended
* one step at a time through separate call invocations.  The close tag
* is added to make the property XML document well formed.
*
**/
  //BTD TODO - remove this method
void DavPropCache::cleanup(void) {
//ERIC This goes away because you are always writing the close tag with
//each append.
  //appendTsCloseTag();
}
/**
*
*				appendTsCloseTag
*
* Time step data is appended one step at a time through separate call 
* invocations.  The close tag is added to make the property XML document 
* well formed.
*
**/
  //BTD TODO - remove this method
void DavPropCache::appendTsCloseTag(void) {
  string log;
  int numProperties =  p_tsproperties.size();
  string message;
  for (int index=0; index < numProperties; index++) {

    const Property_Ref* propRef = 
                 PropFactory::getPropRef(p_tsproperties[index]);
    

    string endTag = "</";
    EcceURL* tSUrl = new EcceURL(p_vdoc->getProperty(p_tsproperties[index]));
    p_edsi->setURL(*tSUrl);
    delete tSUrl;
    endTag += propRef->storagetype();
    endTag += ">"; 

    if (p_edsi->appendDataSet(endTag.c_str()) )
    {
      if (p_ecceJobLogAll)
      {
        message = "Appended endTag on time step property (";
        message += p_tsproperties[index];
        message += ")";
        appendToLog(ActivityLog::INFO,
                  "Cache storage",
                   message.c_str(), log);
      }
    }
    else 
    {
      message = "Unable to append endTag on time step property (";
      message += p_tsproperties[index];
      message += ")";
      appendToLog(ActivityLog::ERROR,
                "Cache storage",
                 message.c_str(), log);

    }

    //
    //I Have to store this directly since it's being called from
    //the destructor.  I really only have one shot at this.
    //
    EcceURL runLogName = p_vdoc->getOutput(p_vdoc->getRunLogName());
    p_edsi->setURL(runLogName); 
    p_edsi->appendDataSet(log.c_str());
  } 
 
}



/**
 *
 * Set up Xerces XML environment to use library.
 *
 */
bool DavPropCache::initializeXmlUtils()
{
  bool ret = true;
  try
  {
    XMLPlatformUtils::Initialize();
  }

  catch (const XMLException& toCatch)
  {
    string msg = "Error during initialization! :\n";
    char *tmpMsg = XMLString::transcode(toCatch.getMessage());
    msg +=  tmpMsg;
    delete [] tmpMsg;
    EE_RT_ASSERT(false, EE_FATAL, msg);
    ret = false;
  }
  return ret;
}

