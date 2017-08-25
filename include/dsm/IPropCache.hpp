///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: IPropCache.H
//
//
// CLASS SYNOPSIS:
//    Class IPropCache is an abstract class used to define the interface for 
//    storing calculation properties.     
//
//
// LOCAL TYPES:
//
// SEE ALSO:  DavPropCache.H
//
// DESCRIPTION:
//    The class puts property data in a cache.  The function flush() is used
//    to manually store unsaved cached data. 
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IPROPCACHE_H
#define _IPROPCACHE_H

#include <string>
#include <vector>
#include <deque>
#include <map>
  using std::string;
  using std::vector;
  using std::map;
  using std::deque;
  using std::pair;

#  include "dsm/PropertyDoc.H"

class EcceURL;
class EDSI;

//
//  Type definition made to make code a little cleaner.  This definition is 
//  propogated to comm/jobparser, apps/eccejobstore, and the dsm tree.
//
typedef map< pair <string, int> , string, 
             less<pair < string , int > > > propertyMapPairToValue;

//
// QueueEntry is basically a struct used in a stl deque to provide FIFO
// order for flushing properties to the DAV Server.  A map was not used
// because order couldn't be guarenteed. For non-time step data the step
// value will always be "0".
//
class QueueEntry
{
  public:
  QueueEntry() { step=0; document=0; }
  int step;
  PropertyDoc *document;
};

class IPropCache {

  public:
   
   IPropCache() {  m_storageFlag = true; m_maxStorageFailures = 10; }
   IPropCache(bool storage, int maxStorageFailures = 10) 
                {m_storageFlag = storage; m_maxStorageFailures = maxStorageFailures;}
   virtual ~IPropCache() {}
   virtual void setURL(const EcceURL& url) = 0; 
   virtual bool put(istream& propStream, propertyMapPairToValue& properties,  string& log) = 0;

  protected:
    deque<QueueEntry>  p_entries;	// FIFO structure for buffering
					// processed properties ready for
					// storage and messaging.
		
    map<string, 
        PropertyDoc *, 
        less<string> >  p_tsbuffer;    	// Temporary buffer used to hold the
					// first step of a time step property
					// until the second step arrives. Once
					// the second step arrives the first
					// step is removed from the p_tsbuffer
					// and stored in p_entries for eventual
					// storage. 

    int m_maxStorageFailures; 		// Maximum allowable failures that 
					// can occur before the cache
					// gives up attempting to store. 

    bool m_storageFlag;                 // if false data is not stored, merely
					// sent out as a tooltalk message.
 					// if true data is stored and sent out
					// as a tooltalk message.
};

#endif

