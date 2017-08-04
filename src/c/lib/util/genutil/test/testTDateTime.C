#include <iostream>
  using namespace std;

#include <time.h>
#include <unistd.h>
#include <locale.h>

#include "util/TDateTime.H"

// Test some basic things we need out of date/time
//   . parse Z time used in Dav Creation time.
//   . print in standard format (GMT)
//   . print in local format.
int main(int argc, char** argv)
{
   char *l;
   float f;
   l = setlocale(LC_NUMERIC, "");
   sscanf("100.987","%f",&f);
   printf("%f\n",f);
   cout << "local is " << l << endl;
   cout << "french value " << f << endl;

   l = setlocale(LC_NUMERIC, "C");
   sscanf("100.987","%f",&f);
   printf("%f\n",f);
   cout << "local is " << l << endl;
   cout << "C value " << f << endl;

   l= setlocale(LC_TIME, "japanese");
   cout << "local is " << l << endl;

  TDateTime now;
  string str = now.toString();
  cout << "now is " << str << endl;

  long lnow = TDateTime::toSeconds(str.c_str());

  string test1 = " Mon, 22 Jan 2001 21:06:20 GMT";
  long ltest1 = TDateTime::toSeconds(test1.data());
  cout << lnow << " " << ltest1 << " " << lnow-ltest1 << endl;

  TDateTime ver1(test1);
  cout << "verify == " << ver1.toSeconds() << " " << ltest1 << endl;

   struct tm ltm, gtm, mytm;

    time_t clk = time(0);
    gmtime_r(&clk,&gtm);
    localtime_r(&clk,&ltm);
    char buf[256];
  TDateTime t2;
  cout << t2.toString() << endl;
  cout << "locally " << t2.toLocaleString() << endl;

  // Here is something I was working on when I decided to move on.


  cout << "starting with Mon, 22 Jul 2001 21:06:20 PDT" << endl;
  TDateTime t3 = string("Mon, 22 Jul 2001 21:06:20 PDT");
  cout << "in GMT " << t3.toString() << endl;



  return 0;
}
