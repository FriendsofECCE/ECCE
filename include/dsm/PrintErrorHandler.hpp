#ifndef PRINTERRORHANDLER_HH
#define PRINTERRORHANDLER_HH
/////////////////////////////////////////////////////////////////////////////
// Class: PrintErrorHandler
//
// Synopsis: Reports errors from XML document printing.
//
// Description:
//   Report them to stderr at this time.
/////////////////////////////////////////////////////////////////////////////


#include <string>
  using std::string;

#include <xercesc/dom/DOMErrorHandler.hpp> 
using namespace xercesc;

class PrintErrorHandler : public DOMErrorHandler
{
  public:
    PrintErrorHandler();
    virtual ~PrintErrorHandler();

    virtual bool handleError (const DOMError& domError);

    string getMessages() const;

  private:
    // Unimplemented constructor and operators
    PrintErrorHandler(const DOMErrorHandler&);
    void operator=(const DOMErrorHandler&);

    string p_msgs;

};

#endif 
