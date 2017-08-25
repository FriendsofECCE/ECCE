/**
 * @file
 *
 *
 */

#ifndef METAPOTENTIALMODELXMLIZER_HH
#define METAPOTENTIALMODELXMLIZER_HH

#include <string>
#include "dsm/XMLSerializer.H"

class TAtm;
class MetaPotential;
class MetaPotentialModel;


/**
 * Performs conversion of a xml data stream into an object -
 * Deserialization.  Likewise, performs conversion of an
 * object into a xml data stream - Serialization.  Currently,
 * Serialization and Deserialization are performed only for
 * MetaPotentialModel objects (i.e. list of meta-potentials).
 */
class MetaPotentialModelXMLizer : public XMLSerializer
{
   public:
      MetaPotentialModelXMLizer();
      virtual ~MetaPotentialModelXMLizer();

      std::string mimetype() const;

      void deserialize(const std::string& data,
                       Serializable& object, EcceMap& properties) 
                       throw (ParseException);

      void serialize(const Serializable& object, 
                     std::string& data, EcceMap& properties);

   private:
      void deserializePotentials(MetaPotentialModel *dobject,
                                 DOMNodeList *potentials);
      std::string serializePotentials(vector<MetaPotential *> *potentials);
      std::string typeToStr(const MetaPotential& potential) const;
      std::string groupToStr(const std::vector<int>& g) const;
      std::vector<int> groupStrToVec(const Fragment& frag, 
                                       const std::string& str) const;
      std::string rGroupToStr(const std::vector<double>& g) const;
      std::vector<double> rGroupStrToVec(const std::string& str) const;

      static const std::string ROOTELEMENT;

};


#endif

