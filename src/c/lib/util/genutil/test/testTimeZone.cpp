#include <iostream>
#include <time.h>
using namespace std;

int main() {
  time_t a;
  cout << "empty time_t\t" << a << endl;
  time(&a);
  tm * lt = localtime(&a);
  cout << "time\t" << a << endl;
  cout << "tm_sec\t" << lt->tm_sec << endl;
  cout << "tm_min\t" << lt->tm_min << endl;
  cout << "tm_hour\t" << lt->tm_hour << endl;
  cout << "tm_mday\t" << lt->tm_mday << endl;
  cout << "tm_mon\t" << lt->tm_mon << endl;
  cout << "tm_year\t" << lt->tm_year << endl;
  cout << "tm_wday\t" << lt->tm_wday << endl;
  cout << "tm_yday\t" << lt->tm_yday << endl;
  cout << "tm_isdst\t" << lt->tm_isdst << endl;
  cout << "tm_gmtoff\t" << lt->tm_gmtoff << endl;
  cout << "tm_zone\t" << lt->tm_zone << endl;
  cout << timezone << endl;
  cout << - lt->tm_gmtoff + (lt->tm_isdst?3600:0) << endl;
}
