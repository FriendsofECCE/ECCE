/**
 * @file 
 *
 *
 */
#ifndef NWCHEMMDMODELXMLIZER_HH
#define NWCHEMMDMODELXMLIZER_HH

#include <string>

#include <iostream>
  using std::ostream;

#include "dsm/XMLSerializer.H"

class NWChemMDModel;
class InteractionModel;
class ConstraintModel;
class OptimizeModel;
class ControlModel;
class DynamicsModel;
class ThermodynamicsModel;
class FilesModel;
class RecordModel;

/**
 *  The format for this XML document is
 *
 *  <NWChemMDModel>
 *    <Interactions>
 *         :
 *    </Interactions>
 *         :
 *         :
 *    <Optimize>
 *         :
 *    </Optimize>
 *  </NWChemMDModel>
 */
class NWChemMDModelXMLizer : public XMLSerializer
{
   public:
      NWChemMDModelXMLizer();
      virtual ~NWChemMDModelXMLizer();

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
      /**
       *  Convenience methods for dumping out the contents of the different
       *  models associated with each of the panels in the MD task GUI.
       */
      void dumpInteraction(InteractionModel *model, ostream &os);
      void dumpConstraint(ConstraintModel *model, ostream &os);
      void dumpOptimize(OptimizeModel *model, ostream &os);
      void dumpControl(ControlModel *model, ostream &os);
      void dumpDynamics(DynamicsModel *model, ostream &os);
      void dumpThermodynamics(ThermodynamicsModel *model, ostream &os);
      void dumpFiles(FilesModel *model, ostream &os);

};


#endif
