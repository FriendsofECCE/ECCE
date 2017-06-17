/**
 * @file
 *
 *
 */

#ifndef QMMMMODELXMLIZER_HH
#define QMMMMODELXMLIZER_HH

#include <string>
#include "dsm/XMLSerializer.H"

class TAtm;
class QMMMParams;
class QMMMModel;


/**
 * Performs conversion of a xml data stream into an object -
 * Deserialization.  Likewise, performs conversion of an
 * object into a xml data stream - Serialization.  Currently,
 * Serialization and Deserialization are performed only for
 * QMMMModel objects (i.e. list of meta-potentials).
 */
class QMMMModelXMLizer : public XMLSerializer
{
   public:
      QMMMModelXMLizer();
      virtual ~QMMMModelXMLizer();

      std::string mimetype() const;

      void deserialize(const std::string& data,
                       Serializable& object, EcceMap& properties) 
                       throw (ParseException);

      void serialize(const Serializable& object, 
                     std::string& data, EcceMap& properties);

   private:
      void deserializeQMMMTerms(QMMMModel *dobject,
                                DOMNodeList *potentials);
      std::string serializeQMMMTerms(vector<QMMMParams *> *potentials);
      std::vector<string> atomStrToVec(const std::string& str) const;
      std::string paramsToStr(const std::vector<double>& g) const;
      std::vector<double> paramStrToVec(const std::string& str) const;

      static const std::string ROOTELEMENT;

};


#endif

