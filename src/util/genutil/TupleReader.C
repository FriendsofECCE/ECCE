#include <string.h>
#include <vector>
using std::vector;

#include <fstream>
using std::ifstream;

#include "util/STLUtil.H"
#include "util/TupleReader.H"

//////////////////////////////////////////////////////////////////////////////
// Constructor.
// When this constructor is used the file and stream are managed
// internally.
//////////////////////////////////////////////////////////////////////////////
TupleReader::TupleReader(const string& file, bool strip)
{
  p_sep = ':';
  p_is = new ifstream(file.c_str());
  p_autoClose = true;
  p_strip = strip;
}

//////////////////////////////////////////////////////////////////////////////
// Constructor.
// When this constructor is used the caller must manage the stream
// object.
//////////////////////////////////////////////////////////////////////////////
TupleReader::TupleReader(istream& is, bool strip)
{
  p_sep = ':';
  p_is = &is;
  p_autoClose = false;
  p_strip = strip;
}

TupleReader::~TupleReader()
{
  if (p_autoClose) {
    ((ifstream*)p_is)->close();
    delete p_is;
  }
}

void TupleReader::setSeparator(char sep)
{
  p_sep = sep;
}

// Not an error if the line doesn't contain 3 values but return values
// set to empty string.
bool TupleReader::gettuple(string& one, string& two, string& three)
{
  one = two = three = "";

  string line;
  string rawLine;
  bool needMore = true;
  while (std::getline(*p_is, rawLine)) {
    vector<char> bufv(rawLine.begin(), rawLine.end());
    bufv.push_back('\0');
    char* buf = &bufv[0];
    int len = strlen(buf);
    // skip empty lines
    if (len == 0) continue;

    // skip comment lines - can't use strtok
    bool comment = false;
    char *cptr = buf;
    while (*cptr != '\0') {
      if (*cptr == ' ' ||  *cptr == '\t') {
        ;
      } else if (*cptr == '#') {
        comment = true;
        break;
      } else {
        break;
      }
      cptr++;
    }
    if (comment) continue;

    // Eliminate trailing comments
    char *cpos = strchr(buf,'#');
    if (cpos) {
      *cpos='\0';
      len = strlen(buf);
    }

    // Strip trailing white space and decide if we need to read more
    for (int idx=len-1; idx>=0; idx--) {
      if (buf[idx] == ' ' || buf[idx] == '\t') {
        buf[idx] = '\0';
      } else if (buf[idx] == '\\') {
        buf[idx] = '\0';
        break;
      } else {
        needMore = false;
        break;
      }
    }

    line += buf;
    if (!needMore) break;
  }

  // Now tokenize
  size_t pos = line.find(p_sep);
  if (pos != string::npos) {
    one = line.substr(0,pos);
    two = line.substr(pos+1);

    pos = two.find(p_sep);
    if (pos != string::npos) {
      // Order important here...
      three = two.substr(pos+1);
      two = two.substr(0,pos);
    } else {
    }
  } else {
    one = line;
  }

  if (p_strip) {
    STLUtil::stripLeadingWhiteSpace(one);
    STLUtil::stripLeadingWhiteSpace(two);
    STLUtil::stripLeadingWhiteSpace(three);
  }

  // Its currently not an error if the line doesn't contain 3 values...
  return p_is->good();
}

