///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PrepareModelModelXMLizer.H
//
//
// CLASS SYNOPSIS:
//
// DESCRIPTION:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PREPAREMODELXMLIZER_HH
#define PREPAREMODELXMLIZER_HH

#include <string>

#include "dsm/XMLSerializer.H"

class XMLable;
class PrepareModel;
/**
 *  The format for this XML document is
 *
 *  <PrepareModel>
 *    <PDBSource> string </PDBSource>
 *    <PDBModel> string </PDBModel>
 *    <PDBAltLoc> string </PDBAltLoc>
 *    <PDBChain> string </PDBChain>
 *    <Histidine> string </Histidine>
 *    <CoordCommands>
 *       <Command> string </Command>
 *    </CoordCommands>
 *  </PrepareModel>
 */
class PrepareModelXMLizer : public XMLSerializer
{
   public:
      PrepareModelXMLizer();
      virtual ~PrepareModelXMLizer();

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
