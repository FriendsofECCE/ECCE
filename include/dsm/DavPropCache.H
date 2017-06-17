///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: DavPropCache.H
//
//
// CLASS SYNOPSIS:
//    Class DavPropCache is an implementation of the IPropCache class.  
//
//
// LOCAL TYPES:
//
// SEE ALSO:
//
// DESCRIPTION:  Provides the means to store calculation properties on a 
//               DAV server.
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DAVPROPCACHE_H
#define _DAVPROPCACHE_H

#include <string>
#include <vector>
#include <map>
  using std::string;
  using std::vector;
  using std::map;

#include "dsm/ActivityLog.H"
#include "dsm/IPropCache.H"
class EcceURL;
class EDSI;
class VDoc;
class PropertyDoc;

typedef   map<const string, int, less<string> > TSSTEPS;

class DavPropCache : public IPropCache {

  public:
    DavPropCache(const EcceURL& url, const bool store,const int MaxFailures); 
   ~DavPropCache();

   void setURL(const EcceURL& url);

   bool        put(istream& propStream, 
                   propertyMapPairToValue& properties,  
                   string& log);

  private:
   void        cleanup(void);
  void          appendToLog(const ActivityLog::EventType type,
                            const char*  name,
                            const char* message,
                            string& log);

   bool          flush( propertyMapPairToValue& properties, string& log); 
   bool          bufferProperty( PropertyDoc *entry,
                                 string& log);
   bool          bufferTSProperty( PropertyDoc *entry,
                                   unsigned long step,
                                   string& log);

   string        propEntryToString(PropertyDoc& entry);
   string        TsPropEntryToString(PropertyDoc& entry, int step);
   unsigned long stepToken(const string& key, const Property_Ref* propRef);
   bool          restoreStepToken(const string& key , 
                                  const Property_Ref* propRef);
   void          appendTsCloseTag(void);
   string        davXMLDocumentString() const;
   bool          storeProperty(QueueEntry& entry, 
                               propertyMapPairToValue& properties,
                               string& log);
   bool          putProp(PropertyDoc& entry);  
   bool          putTSProp(PropertyDoc& entry);  
   bool          storeMetadata(PropertyDoc& entry, string& log,
                          const EcceURL& url, 
                          const string& ns,const string& property);
   bool          initializeXmlUtils();

   EDSI*         p_edsi;                // Interface to EDSI storage
   VDoc*         p_vdoc;                // Virtual document which is set to the
					// url of the calculation root.  There
					// is not provision at present time to
					// change the url after object 
                                        // construction.
   int           p_flushDenied;         // Counter state to keep track of how
                                        // many times a failure to flush was
 					// denied.
   int         p_messageBodyThreshhold; // Maximum message body size
   unsigned long p_dataBufferSavedSize; // last message body saved 

   bool          p_ecceJobLogAll;       // flag set at construction based on 
                                        // existance of environment variable.


   //map<const string, int, less<string> > p_tspropsteps;
   TSSTEPS p_tspropsteps;
                                        // Time step properties do not carry
                                        // their own labels at this time.  Due
                                        // to the stateless nature of the 
                                        // parser scripts an internal map is
                                        // required to track the number of 
                                        // steps processed thus far for each
                                        // property.  The current sum is used
                                        // to create a unique label for the
                                        // step element in the XML document. 
                                   

   vector<string> p_tsproperties;       // Time series data is appended to
                                        // in storage.  As a result the cache
                                        // needs to append a close root tag
                                        // to the XML document before it is 
					// destroyed.  This vector of 
                                        // time step properties is a record
                                        // of all the time step properties 
 					// that have been saved and need
					// a closure tag to make the XML 
                                        // document well-formed. 
                                        // A property must only be saved 
                                        // to this vector if at least two
                                        // time steps exist.  This rule
                                        // is due to the fact that codes
                                        // sometimes send out a single step
                                        // of a property when that time step
                                        // property has not been calculated.
                                        // Since we can't seem to turn this 
                                        // off we need to enforce the rule
                                        // internally.

};

#endif

