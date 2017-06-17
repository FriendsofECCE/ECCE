#include <iostream>
#include <strstream>
#include <fstream>
  using namespace std;
#include "util/StringTokenizer.H"

int main(int argc, char** argv)
{
// Convert your istream to an ostrstream
   ifstream test("./input.txt");
   if (test.good()) {
   ostrstream temp;
   temp << test.rdbuf(); 

// Use the str() method to convert ostrstream to a string 
// (only ostrstream has this method)
   char* tmpStr = temp.str();  // we have to delete the return value
                               // of this method
   string fileStr(tmpStr);
   delete tmpStr;
   cout << "File conents are: " << endl;
   cout << fileStr << endl;
   cout << "---------------------------------------------" << endl;

/* Make a string tokenizer to parse based on your criteria 
   ("\n" for readLine, " \n\t" for readToken) */
   StringTokenizer token(fileStr, " \t\n");

// Now just call token.next() instead of readToken()
   string curTok;
   while ((curTok = token.next()) != "") {
     cout << curTok << endl;
  }
  } else {
    cout << "No input file input.txt" << endl;
    cout << "create this file with whitespace tokenized data" << endl;
  }

  return 0;
}
