//////////////////////////////////////////////////////////////////////////////
// KeyValueReader
//
// Synopsis:
//   Simple class to read a stream that is structured as key/value pairs.
//
// Description:
//   A logical line can cross multiple physical lines so long as it ends
//   with a '\' character.  Each line should contain a key followed by a
//   colon ':' followed by the message text.
//
//   Comment char is '#'.
//////////////////////////////////////////////////////////////////////////////
#ifndef KEYVALUEREADER
#define KEYVALUEREADER

#include <iostream>
using std::istream;

#include <string>
using std::string;

class KeyValueReader {
  public:
    KeyValueReader(const string& file);
    KeyValueReader(istream& istr);
    virtual ~KeyValueReader();

    void setSeparator(char ch);

    void stripTrailingComments(bool);
    virtual bool getpair(string& key, string& value);

  protected:
    istream *p_is;
    bool     p_autoClose;
    bool     p_stripTrailingComments;
    char     p_sep;

};

#endif
