#include <iostream>
#include <strstream>
  using namespace std;

#include "util/StringTokenizer.H"

int main(int argc, char** argv)
{
   static string tests[]= { "1 2 3 4, 5",
             "Ununoctium,7,18,synthetic,metallic,#efe7ad,n.a.,0,0,0.00,0.00,1.90,1.2",
             "justone",
             " \t",
             "fred flinstone rules the cratacious",
             "thishas, two",
             "with \r\n, return"
   };

   for (int idx=0; idx<sizeof(tests)/sizeof(char*); idx++) {
      StringTokenizer tokenizer(tests[idx],", \t\n");
      //StringTokenizer tokenizer(tests[idx]);
      cout << "***tokenizing string " << tests[idx] << endl;
      while (tokenizer.hasMoreTokens()) {
         cout << "token:" << tokenizer.next() << "!" << endl;
         //cout << "token:" << tokenizer.next(", \t\n") << "!" << endl;
      }
   }

  return 0;
}
