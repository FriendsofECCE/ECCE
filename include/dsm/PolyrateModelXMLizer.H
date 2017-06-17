/**
 * @file
 *
 *
 */
#ifndef POLYRATEMODELXMLIZER_HH
#define POLYRATEMODELXMLIZER_HH

#include <string>

#include "dsm/XMLSerializer.H"


/**
 * Performs serialization and deserialization of NWDirdyModel
 */
class PolyrateModelXMLizer : public XMLSerializer
{
   public:
      PolyrateModelXMLizer();

      virtual ~PolyrateModelXMLizer();

      /**
       * Defines the mimetype of streamed data handled by this serializer.
       * This mimetype applies to the data stream.  Type is undefined for
       * properties.
       */
      std::string mimetype() const;

      /**
       * Convert a data stream into an object.
       * @param data - the data stream to be converted to an object
       * @param object - the object to create from the stream 
       * @param properties - metadata needed to complete the object.  
       *    This may be empty.  If non-empty, the map containts property
       *    keys to aquire but not the values.
       */
      void deserialize(const std::string& data,
                       Serializable& object, EcceMap& properties) 
                       throw (ParseException);

      /**
       * Convert an object into its data and metadata.
       * @param object - an object to serialize
       * @param data - the returned data stream - possibly empty but not null
       * @param properties - metadata for the object - possibly empty.
       */
      void serialize(const Serializable& object, 
                     std::string& data, EcceMap& properties);

   private:
      static const std::string ROOTELEMENT;

};


#endif
