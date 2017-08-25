#ifndef BASICDOMPARSER_HH
#define BASICDOMPARSER_HH

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/sax/InputSource.hpp>
  using namespace xercesc;

#include <iostream>
  using std::istream;
#include <string>
  using std::string;

#include "util/ParseException.H"

class SFile;
  

/**
 * BasicDOMParse is a wrapper around DOM document parser.  
 * This parser is non-validating and spits errors and warning to stdout.
 * That is about it.
 */
class BasicDOMParser 
{

  public:
    BasicDOMParser();
    virtual ~BasicDOMParser(); 

    DOMDocument* parse(const SFile& file, const bool& logErrors=true)
                       throw (ParseException);
    DOMDocument* parse(istream& is, const bool& logErrors=true)
                       throw (ParseException);
    DOMDocument* parse(const InputSource& is, const bool& logErrors=true)
                       throw (ParseException);
    DOMDocument* parse(const string& data, const bool& logErrors=true)
                       throw (ParseException);

  protected:

    BasicDOMParser(const BasicDOMParser& rhs);

    void initialize() throw (EcceException);

};

#endif 
