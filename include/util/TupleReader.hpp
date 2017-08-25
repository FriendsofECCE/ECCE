//////////////////////////////////////////////////////////////////////////////
// TupleReader
//
// Synopsis:
//   Simple class to read a stream that is structured as tuples (3).
//
// Description:
//   A logical line can cross multiple physical lines so long as it ends
//   with a '\' character.  Each line should contain a three values
//   separated by : (by default)
//////////////////////////////////////////////////////////////////////////////
#ifndef TUPLEREADER
#define TUPLEREADER

#include <iostream>
using std::istream;

#include <string>
using std::string;

class TupleReader 
{
  public:
    TupleReader(const string& file, bool stripWhite=true);
    TupleReader(istream& istr, bool stripWhite=true);
    virtual ~TupleReader();

    void setSeparator(char ch);

    virtual bool gettuple(string& one, string& two, string& three);
  protected:

    istream *p_is;
    bool     p_autoClose;
    bool     p_strip;
    char     p_sep;

};

#endif
