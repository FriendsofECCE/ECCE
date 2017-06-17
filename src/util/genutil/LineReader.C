#include <string.h>
#include <fstream>
using std::ifstream;

#include "util/LineReader.H"

//////////////////////////////////////////////////////////////////////////////
// Constructor.
// When this constructor is used the file and stream are managed
// internally.
//////////////////////////////////////////////////////////////////////////////
LineReader::LineReader(const string& file)
{
  p_is = new ifstream(file.c_str());
  p_autoClose = true;
  p_stripTrailingComments = true;
}

//////////////////////////////////////////////////////////////////////////////
// Constructor.
// When this constructor is used the caller must manage the stream
// object.
//////////////////////////////////////////////////////////////////////////////
LineReader::LineReader(istream& is)
{
  p_is = &is;
  p_autoClose = false;
  p_stripTrailingComments = true;
}

LineReader::~LineReader()
{
  if (p_autoClose) {
    ((ifstream*)p_is)->close();
    delete p_is;
  }
}

void LineReader::stripTrailingComments(bool flag)
{
  p_stripTrailingComments = flag;
}

bool LineReader::getLine(string& line)
{
  static const int MAXLINE = 256;
  line = "";
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

    line += buf;
    if (!needMore) break;
  }

  return p_is->good();
}




