#include <iostream>
  using namespace std;

#include <unistd.h>

#include "util/EcceURL.H"

int main(int argc, char** argv)
{
  static const char *tests[] = {
    "https://emsl.pnl.gov/webdav/collection/myfrag.frag",
    "http://emsl.pnl.gov:2080/webdav/collection?key1=value1&key2=value2",
    "/",
    "http://emsl.pnl.gov:1080/",
    "http://emsl.pnl.gov:1080/Ecce/users&fred=wilma",
    "ssh://fringe.emsl.pnl.gov",
    "http://emsl.pnl.gov",
    "http://emsl.pnl.gov/",
    "http://emsl.pnl.gov:80/",
    "https://emsl.pnl.gov:2080/",
    "https://emsl.pnl.gov/webdav",
    "https://emsl.pnl.gov:2080/webdav",
    "https://emsl.pnl.gov:2080/webdav/",
    "https://emsl.pnl.gov:2080/webdav/collection",
    "/file",
    "/file/",
    "//file",  // malformed
    "junk"
  };
  static int NUMTEST = sizeof(tests)/sizeof(char*);
  for (int idx=0; idx<NUMTEST; idx++) {
    cout << "\n--URL-- " << tests[idx] << endl;
    EcceURL url(tests[idx]);
    cout << "RESULT: " << url.toString() << endl;
    cout << "Protocol: " << url.getProtocol() << endl;
    cout << "Server: " << url.getHost() << endl;
    cout << "Port: " << url.getPort() << endl;
    cout << "File: " << url.getFile() << endl;
    cout << "Root: " << url.getFilePathRoot() << endl;
    cout << "Tail: " << url.getFilePathTail() << endl;
    EcceURL child = url.getChild("achild");
    cout << "child " << child.toString() << endl;
    EcceURL parent = url.getParent();
    cout << "parent: " << parent.toString() << endl;

  }


  return 0;
}
