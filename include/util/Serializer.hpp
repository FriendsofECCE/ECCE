///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Serializer.H
//
//
// CLASS SYNOPSIS: Abstract class that defines serialization for an object.
//
// DESCRIPTION:
//    All objects that my be streamed will also have an implementation of
//    the Serializer interface for streaming data into and out of objects.
//    All such data objects must subclass Serializable.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef SERIALIZER_HH
#define SERIALIZER_HH

class Serializable;
class EcceMap;

#include "util/ParseException.H"

class Serializer
{
   public:

      /**
       * Defines the mimetype of streamed data handled by this serializer.
       * This mimetype applies to the data stream.  Type is undefined for
       * properties.
       */
      virtual std::string mimetype() const = 0;

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
       * @param data - the returned data stream - possibly null
       * @param properties - metadata for the object - possibly empty.
       */
      virtual void serialize(const Serializable& object, 
                             std::string& data, EcceMap& properties) = 0;

};


#endif
