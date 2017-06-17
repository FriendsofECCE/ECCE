#include <iostream.h>
#include <fstream.h>
#include <unistd.h>

#include "dsm/HistoryList.H"

int main(int argc, char** argv)
{
  static const char *tests[] = {
    "/",
    "http://emsl.pnl.gov:1080/",
    "ssh://fringe.emsl.pnl.gov",
    "http://emsl.pnl.gov",
    "http://emsl.pnl.gov/",
    "http://emsl.pnl.gov:80/last",
    "https://emsl.pnl.gov:2080/",
    "https://emsl.pnl.gov/webdav",
    "https://emsl.pnl.gov:2080/webdav",
    "https://emsl.pnl.gov:2080/webdav/",
    "https://emsl.pnl.gov:2080/webdav/collection",
    "https://emsl.pnl.gov/webdav/collection/myfrag.frag",
    "/file/eight",
    "/file/nine",
    "junk"
  };
  static int NUMTEST = sizeof(tests)/sizeof(char*);


  HistoryList hlist(10);

  int cnt = NUMTEST;
  for (int idx=0; idx<cnt; idx++) {
    EcceURL url = tests[idx];
    hlist.add(url);
  }

  cnt = hlist.size();
  cout << "size is " << cnt << endl;

  cout << hlist.get(0).toString() << endl;
  cout << hlist.get(cnt-1).toString() << endl;

  cout << hlist.pop(7).toString() << endl;;
  cnt = hlist.size();
  cout << "size is " << cnt << endl;

  hlist.setMaxSize(5);
  cnt = hlist.size();
  cout << "size is " << cnt << endl;

  cout << hlist.get(0).toString() << endl;
  cout << hlist.get(cnt-1).toString() << endl;

  EcceURL storetest = "junk";
  bool status;
  status = hlist.save(storetest);
  cout << "save? " << (int) status << endl;

  hlist.clear();
  cnt = hlist.size();
  cout << "size is " << cnt << endl;

  hlist.load(storetest);
  cnt = hlist.size();
  cout << "size is " << cnt << endl;
  cout << hlist.get(0).toString() << endl;
  cout << hlist.get(cnt-1).toString() << endl;

  cout << hlist.canMoveForward() << endl;
  cout << hlist.canMoveBack() << endl;


  return 0;
}
