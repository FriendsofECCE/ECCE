#include <string.h>
#include <fstream>
using std::ifstream;

#include "util/STLUtil.H"
#include "util/KeyValueReader.H"

//////////////////////////////////////////////////////////////////////////////
// Constructor.
// When this constructor is used the file and stream are managed
// internally.
//////////////////////////////////////////////////////////////////////////////
KeyValueReader::KeyValueReader(const string& file)
{
  p_sep = ':';
  p_is = new ifstream(file.c_str());
  p_autoClose = true;
  p_stripTrailingComments = true;
}

//////////////////////////////////////////////////////////////////////////////
// Constructor.
// When this constructor is used the caller must manage the stream
// object.
//////////////////////////////////////////////////////////////////////////////
KeyValueReader::KeyValueReader(istream& is)
{
  p_sep = ':';
  p_is = &is;
  p_autoClose = false;
  p_stripTrailingComments = true;
}

KeyValueReader::~KeyValueReader()
{
  if (p_autoClose) {
    ((ifstream*)p_is)->close();
    delete p_is;
  }
}

void KeyValueReader::setSeparator(char sep)
{
  p_sep = sep;
}

void KeyValueReader::stripTrailingComments(bool flag)
{
  p_stripTrailingComments = flag;
}

bool KeyValueReader::getpair(string& key, string& text)
{
  static const int MAXLINE = 2056;
  string line;
  string tmp;
  size_t startText;
  char buf[MAXLINE];  // hardwired message limit
  bool needMore = true;
  while (p_is->getline(buf,MAXLINE-1)) {
    int len = strlen(buf);
    // skip empty lines
    if (len == 0) continue;

    // skip comment lines - can't use strtok
    bool comment = false;
    char *cptr = buf;
    while (cptr != '\0') {
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
    if (p_stripTrailingComments) {
      char *cpos = strchr(buf,'#');
      if (cpos) {
        *cpos='\0';
        len = strlen(buf);
      }
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

    // Strip leading white space (for continuing lines)
    tmp = buf;
    startText = tmp.find_first_not_of(" ");
    if (startText > 0 && startText != string::npos) { 
      tmp = tmp.substr(startText);
    }

    line += tmp;
    if (!needMore) break;
  }
  // Now tokenize
  int pos = line.find(p_sep);
  if (pos) {
    key = line.substr(0,pos);
    text = line.substr(pos+1);

    // Now gotta strip white space from beginning of text 
    STLUtil::stripLeadingWhiteSpace(key);
    STLUtil::stripLeadingWhiteSpace(text);
  }

  return p_is->good();
}




