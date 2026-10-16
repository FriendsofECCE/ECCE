#include <sstream>
#include <stdlib.h>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>

#include "util/EcceMap.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/GromacsPrepareModel.H"
#include "dsm/GromacsPrepareModelXMLizer.H"

using std::string;

const string GromacsPrepareModelXMLizer::ROOTELEMENT = "GromacsPrepareModel";

GromacsPrepareModelXMLizer::GromacsPrepareModelXMLizer() : XMLSerializer()
{
}

GromacsPrepareModelXMLizer::~GromacsPrepareModelXMLizer()
{
}

string GromacsPrepareModelXMLizer::mimetype() const
{
   return "text/xml/ecce-gromacspreparemodel";
}

void GromacsPrepareModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
{
   GromacsPrepareModel *m = dynamic_cast<GromacsPrepareModel*>(&object);
   if (m == 0) return;

   BasicDOMParser parser;
   std::istringstream is(data);
   DOMDocument *doc = parser.parse(is);
   DOMElement *root = doc->getDocumentElement();

   string v;
   if (!(v = getTagValue(root, "Source")).empty()) m->source(v);
   if (!(v = getTagValue(root, "ForceField")).empty()) m->forceField(v);
   if (!(v = getTagValue(root, "WaterModel")).empty()) m->waterModel(v);
   if (!(v = getTagValue(root, "Histidine")).empty()) m->histidine(v);
   if (!(v = getTagValue(root, "IgnoreHydrogens")).empty())
      m->ignoreHydrogens(atoi(v.c_str()) != 0);
   if (!(v = getTagValue(root, "MergeChains")).empty())
      m->mergeChains(atoi(v.c_str()) != 0);
   if (!(v = getTagValue(root, "BoxType")).empty()) m->boxType(v);
   if (!(v = getTagValue(root, "BoxDistance")).empty())
      m->boxDistance(atof(v.c_str()));
   if (!(v = getTagValue(root, "IonConcentration")).empty())
      m->ionConcentration(atof(v.c_str()));
   if (!(v = getTagValue(root, "PositiveIon")).empty()) m->positiveIon(v);
   if (!(v = getTagValue(root, "NegativeIon")).empty()) m->negativeIon(v);
}

void GromacsPrepareModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)
{
   data = "";
   const GromacsPrepareModel *m =
      dynamic_cast<const GromacsPrepareModel*>(&object);
   if (m == 0) return;

   std::ostringstream os;
   os << xmlHeader() << "\n<" << ROOTELEMENT << ">\n"
      << "<Source>" << m->source() << "</Source>\n"
      << "<ForceField>" << m->forceField() << "</ForceField>\n"
      << "<WaterModel>" << m->waterModel() << "</WaterModel>\n"
      << "<Histidine>" << m->histidine() << "</Histidine>\n"
      << "<IgnoreHydrogens>" << (m->ignoreHydrogens() ? 1 : 0)
      << "</IgnoreHydrogens>\n"
      << "<MergeChains>" << (m->mergeChains() ? 1 : 0) << "</MergeChains>\n"
      << "<BoxType>" << m->boxType() << "</BoxType>\n"
      << "<BoxDistance>" << m->boxDistance() << "</BoxDistance>\n"
      << "<IonConcentration>" << m->ionConcentration()
      << "</IonConcentration>\n"
      << "<PositiveIon>" << m->positiveIon() << "</PositiveIon>\n"
      << "<NegativeIon>" << m->negativeIon() << "</NegativeIon>\n"
      << "</" << ROOTELEMENT << ">\n";
   data = os.str();
}
