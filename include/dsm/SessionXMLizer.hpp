/**
 * @file
 *
 *
 */

#ifndef SESSIONXMLIZER_HH
#define SESSIONXMLIZER_HH

#include <string>
using std::string;

#include "util/EcceMap.H"
#include "dsm/XMLSerializer.H"



/**
 * Performs serialization and deserialization tasks.  Serializes 
 * a Session object into an XLink Linkbase.  Deserializes an
 * Xlink Linkbase into a Session object.  A Linkbase is a well 
 * formed XML file contianing XLink attributes that define
 * out-of-line links.  The Linkbase is currently being used
 * to identify and describe relationships between a Session's members.
 */
class SessionXMLizer : public XMLSerializer
{
public:
  SessionXMLizer();

  virtual ~SessionXMLizer();

  string mimetype() const;

  void deserialize(const string & data,
                   Serializable & object,
                   EcceMap & properties) throw (ParseException);

  void serialize(const Serializable& object,
                 string& data,
                 EcceMap& properties);

private:

  Resource * deserializeMember(string & memberLabel,
                               const DOMElement & locatorElement,
                               EcceURL baseUrl);

  void deserializeLink(string& sourceLabel,
                       string& targetLabel,
                       string& linkType,
                       string& linkName,
                       const DOMElement& arcElement);

  string serializeMember(Resource * member, string memberLabel);

  string serializeLink(string sourceLabel,
                       string targetLabel,
                       string linkType,
                       string linkName);

  static string memberLabel(int index);

  static const string ROOT_ELEMENT;  /**< XML root element name in Linkbase */

  static const string MEMBER_ELEMENT;  /**< XML task element name in Linkbase,
                                        this is an XLink Locator element */

  static const string LINK_ELEMENT;  /**< XML link element name in Linkbase,
                                        this is an XLink Arc element */

  static const string LINK_NAME_ELEMENT; /**< Link element name in Linkbase,
                                         this is an XLink title-type element */
  static const string MEMBER_LABEL;
};

#endif
