///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropertyDoc.H
//
//
// CLASS SYNOPSIS:
//    Class PropertyDoc stores property structure in an XML document. 
//
//
// LOCAL TYPES:
//
// SEE ALSO:
//
// DESCRIPTION:
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PROPERTYDOC_H
#define _PROPERTYDOC_H

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
  using namespace xercesc;


class Property_Ref;

class PropertyDoc {

  public:

    //Constructor 
      PropertyDoc(const Property_Ref* ref);
      PropertyDoc(char* doc, const bool& logErrors=true);
      PropertyDoc(const char* doc, const bool& logErrors=true);
      PropertyDoc();
      ~PropertyDoc();

    //Accessors
    const Property_Ref*   reference(void); 
    DOMDocument*          container(void);
    unsigned long         lastStep(void);
    unsigned long         lastMatrix(void);
    char*                 name(void);
    bool                  empty(void);
    char*                 units(void);
    char*                columnLabel(void);
    char*                rowLabel(void);
    char*                vectorLabel(void);
    char*                vectorLabels(void);
    char*                columnLabels(void);
    char*                rowLabels(void);
    char*                vectors(void);
    char*                rows(void);
    char*                columns(void);
    char*                 values(unsigned long dataIndex = 0);
    vector<double>        doubleVector();
    vector<double>        doubleVector(unsigned long dataIndex);
    vector<vector<double> >* doubleVecTable(); 



    //Modifiers
    void                  parse(char*, const bool& logErrors=true);
    void                  parse(const char*, const bool& logErrors=true);
    void                  units(const char*);
    void                  vectorLabels(const char* );
    void                  columnLabels(const char* );
    void                  rowLabels(const char* );
    void                  vectors(const char* );
    void                  rows(const char* );
    void                  columns(const char* );
    void                  values(const char* );
    void                  createStep(unsigned long step);
    void                  createMatrix(unsigned long matrix);

  protected:
    DOMElement* valueElement(void);
    bool        createContainer(void);
    void        createDataPartition(unsigned long partitionNum, XMLCh* type);
    void        setVectorLabel(void);
    void        setColumnLabel(void);
    void        setRowLabel(void);
    char*       attribute(const char*  name, DOMElement& element);
    bool        parseXmlDoc(char*  propertyDoc, const bool& logErrors);
    bool        parseXmlDoc(const char*  propertyDoc, const bool& logErrors);
    void        parseValues(char*  values, vector<double>& vdouble);


    //State Info
    const Property_Ref*    p_ref;
    DOMDocument*           p_container;

};

#endif

