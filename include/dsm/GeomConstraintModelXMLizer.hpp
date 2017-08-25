/**
 * @file
 *
 *
 */

#ifndef GEOMCONSTRAINTMODELXMLIZER_HH
#define GEOMCONSTRAINTMODELXMLIZER_HH

#include <string>
#include "dsm/XMLSerializer.H"

class TAtm;
class GeomConstraint;
class GeomConstraintModel;


/**
 * Performs conversion of a xml data stream into an object -
 * Deserialization.  Likewise, performs conversion of an
 * object into a xml data stream - Serialization.  Currently,
 * Serialization and Deserialization are performed only for
 * GeomConstraintModel objects (i.e. list of geometry
 * constraints).
 */
class GeomConstraintModelXMLizer : public XMLSerializer
{
   public:
      GeomConstraintModelXMLizer();
      virtual ~GeomConstraintModelXMLizer();

      std::string mimetype() const;

      void deserialize(const std::string& data,
                       Serializable& object, EcceMap& properties) 
                       throw (ParseException);

      void serialize(const Serializable& object, 
                     std::string& data, EcceMap& properties);

   private:
      void deserializeConstraints(GeomConstraintModel *dobject,
                                 DOMNodeList *constraints);
      std::string serializeConstraints(vector<GeomConstraint *> *constraints);
      std::string typeToStr(const GeomConstraint& constraint) const;
      std::string groupToStr(const std::vector<TAtm*>& g) const;
      std::vector<int> groupStrToVec(const Fragment& frag, 
                                       const std::string& str) const;

      static const std::string ROOTELEMENT;

};


#endif

