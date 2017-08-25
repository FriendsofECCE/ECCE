/**
 * @file 
 *
 *
 */
#ifndef XMLSERIALIZER_HH
#define XMLSERIALIZER_HH

#include <string>
#include <vector>
  using std::vector;
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMElement.hpp>

using namespace xercesc;

class Serializable;

#include "util/Serializer.H"

/**
 * An abstract implementation of Serializer that adds some
 * XML serialization convenience methods for subclasses.
 */
class XMLSerializer : public Serializer
{
   public:
      /**
       * Convert the XMLCh* to a string and free memory.
       */
       std::string getAsString(const XMLCh *data) const;

      /**
       * Convenience method to get an attribute by name. 
       */
       std::string getAttribute(const DOMElement&, const char *value) const;

      /**
       * Return a DOMElement pointer based on tag name. Good method for drilling
       * down through XML hierarchy
       */
       DOMElement* getNextElementFromTag(const DOMElement *root,
	                                 const string tag) const;

      /**
       * Return a list of DOMElements based on tag name.
       */
       vector<DOMElement*> getNextElementsFromTag(const DOMElement *root,
	                                          const string tag) const;

      /**
       * Return value as a string from a simple tag.
       */
       std::string getTagValue(const DOMElement *root, const string) const;

      /**
       * Return child value as a string from a simple parent/child tag.
       */
       std::string getChildTagValue(const DOMElement *root, const string, const string) const;

      /**
       * Returns the XML header line.
       * Example: <?xml version="1.0" encoding="utf-8" ?>
       */
      virtual std::string xmlHeader() const;

      /**
       * Defines the mimetype of streamed data handled by this serializer.
       * This mimetype applies to the data stream.  Type is undefined for
       * properties.
       */
      virtual std::string mimetype() const = 0;

      /**
       * Convenience function that guarantees that a double value is
       * written in a format containing a decimal point
       * @param value - initial double value that is to be converted
       *    to a string representation.
       */
      std::string writeDoubleAsString(double value);

      /**
       * Convert a data stream into an object.
       * @param data - the data stream to be converted to an object
       * @param object - the object to create from the stream 
       * @param properties - metadata needed to complete the object.  
       *    This may be empty.  If non-empty, the map containts property
       *    keys to aquire but not the values.
       */
      virtual void deserialize(const std::string& data, 
                               Serializable& object, EcceMap& properties) 
                                                throw (ParseException) = 0;

      /**
       * Convert an object into its data and metadata.
       * @param object - an object to serialize
       * @param data - the returned data stream - possibly empty but not null
       * @param properties - metadata for the object - possibly empty.
       */
      virtual void serialize(const Serializable& object, 
                             std::string& data, EcceMap& properties) = 0;

};


#endif
