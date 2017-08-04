#include <iostream>
  using namespace std;

#include <unistd.h>

#include "util/StringTokenizer.H"

int main(int argc, char** argv)
{
  static const char *tests[] = {
    "/",
    "http://emsl.pnl.gov:1080/",
    "ssh://fringe.emsl.pnl.gov",
    "http://emsl.pnl.gov",
    "http://emsl.pnl.gov/",
    "http://emsl.pnl.gov:80/",
    "https://emsl.pnl.gov:2080/",
    "https://emsl.pnl.gov/webdav",
    "https://emsl.pnl.gov:2080/webdav",
    "https://emsl.pnl.gov:2080/webdav/",
    "https://emsl.pnl.gov:2080/webdav/collection",
    "https://emsl.pnl.gov/webdav/collection/myfrag.frag",
    "",
    "junk"
  };
  static int NUMTEST = sizeof(tests)/sizeof(char*);

  string token;
  for (int idx=0; idx<NUMTEST; idx++) {
    StringTokenizer tokenizer(tests[idx],":/.");
    cout << "Tokensizing: " << tests[idx] << endl;
    while (!(token = tokenizer.next()).empty()) {
      cout << "\t" << token << endl;
    }

  }

  char *str = "one|two|three:a:b|c";
  cout << "Special test on: " << str << endl;
  StringTokenizer tokenizer(str);
  token = tokenizer.next("|");
  if (!token.empty()) cout << "\t" << token << endl;
  cout << "has more tokens " << tokenizer.hasMoreTokens() << endl;

  token = tokenizer.next("|");
  if (!token.empty()) cout << "\t" << token << endl;
  cout << "has more tokens " << tokenizer.hasMoreTokens() << endl;

  token = tokenizer.next(":");
  if (!token.empty()) cout << "\t" << token << endl;
  cout << "has more tokens " << tokenizer.hasMoreTokens() << endl;

  token = tokenizer.next("|");
  if (!token.empty()) cout << "\t" << token << endl;
  cout << "has more tokens " << tokenizer.hasMoreTokens() << endl;

  token = tokenizer.next();
  if (!token.empty()) cout << "\t" << token << endl;
  cout << "has more tokens " << tokenizer.hasMoreTokens() << endl;


  return 0;
}
