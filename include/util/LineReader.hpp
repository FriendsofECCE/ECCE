//////////////////////////////////////////////////////////////////////////////
// LineReader
//
// Synopsis:
//   Simple class to read lines that may contain comments or
//   continuationsfrom a stream.
//
// Description:
//   A logical line can cross multiple physical lines so long as it ends
//   with a '\' character.  
//   Comment lines and trailing comments are stripped out.
//////////////////////////////////////////////////////////////////////////////
#ifndef LINEREADER
#define LINEREADER

#include <iostream>
using std::istream;

#include <string>
using std::string;

class LineReader {
  public:
    LineReader(const string& file);
    LineReader(istream& istr);
    virtual ~LineReader();

    void stripTrailingComments(bool);
    bool getLine(string& line);

  protected:
    istream *p_is;
    bool     p_autoClose;
    bool     p_stripTrailingComments;

};

#endif
