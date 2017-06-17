#ifndef BASICDOMWRITER_HH
#define BASICDOMWRITER_HH

#include <xercesc/dom/DOMNode.hpp>
using namespace xercesc;

#include <iostream>
  using std::ostream;
#include <string>
  using std::string;

#include "util/EcceException.H"
  

/**
 * Wrapper for using DOMWriter to format at DOMNode (document)
 * This writer is non-validating and spits errors and warning to stderr.
 * That is about it.
 */
class BasicDOMWriter 
{

  public:
    static string XMLDocumentDeclaration();
    static void write(const DOMNode& dn, ostream& os, bool escaped,
                      bool prettyPrint=false) throw (EcceException);


};

#endif 
