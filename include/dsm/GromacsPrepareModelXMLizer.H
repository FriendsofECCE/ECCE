/**
 * @file
 */
#ifndef GROMACSPREPAREMODELXMLIZER_HH
#define GROMACSPREPAREMODELXMLIZER_HH

#include <string>

#include "dsm/XMLSerializer.H"

/**
 * <GromacsPrepareModel> with one element per recipe field (Source,
 * ForceField, WaterModel, Histidine, IgnoreHydrogens, MergeChains, BoxType,
 * BoxDistance, IonConcentration, PositiveIon, NegativeIon), always all
 * written; md.gmxprep reads it.
 */
class GromacsPrepareModelXMLizer : public XMLSerializer
{
   public:
      GromacsPrepareModelXMLizer();
      virtual ~GromacsPrepareModelXMLizer();

      std::string mimetype() const;
      void deserialize(const std::string& data,
                       Serializable& object, EcceMap& properties);
      void serialize(const Serializable& object,
                     std::string& data, EcceMap& properties);

   private:
      static const std::string ROOTELEMENT;
};

#endif
