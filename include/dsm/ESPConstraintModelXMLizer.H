/**
 * @file 
 *
 *
 */
#ifndef ESPCONSTRAINTMODELXMLIZER_HH
#define ESPCONSTRAINTMODELXMLIZER_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "dsm/XMLSerializer.H"

class ESPConstraintModel;
class Fragment;
class TAtm;

/**
 * Serializer for ESPConstraintModel class.
 */
class ESPConstraintModelXMLizer : public XMLSerializer
{
   public:
      ESPConstraintModelXMLizer();
      virtual ~ESPConstraintModelXMLizer();

      /**
       * Defines the mimetype of streamed data handled by this serializer.
       * This mimetype applies to the data stream.  Type is undefined for
       * properties.
       */
      string mimetype() const;

      /**
       * Convert a data stream into an object.
       * @param data - the data stream to be converted to an object
       * @param object - the object to create from the stream 
       * @param properties - metadata needed to complete the object.  
       *    This may be empty.  If non-empty, the map containts property
       *    keys to aquire but not the values.
       */
      void deserialize(const string& data,
                       Serializable& object, EcceMap& properties) 
                       throw (ParseException);

      /**
       * Convert an object into its data and metadata.
       * @param object - an object to serialize
       * @param data - the returned data stream - possibly empty but not null
       * @param properties - metadata for the object - possibly empty.
       */
      void serialize(const Serializable& object, 
                     string& data, EcceMap& properties);

   private:
      string groupToStr(const vector<TAtm*>& g) const;
      vector<TAtm*> groupStrToVec(const Fragment& frag, 
                                       const string& str) const;
      static const string ROOTELEMENT;

};


#endif
