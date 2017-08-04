#include <iostream>
  using namespace std;

#include "util/EcceRegexp.H"

int main(int argc, char** argv)
{
  /*
  string jobIdParseExpression = ".*";
  string jobIdParseLeadingText =  "<";
  string jobIdParseTrailingText = ">:";

  EcceRegexp re(jobIdParseLeadingText + jobIdParseExpression + 
		jobIdParseTrailingText);

  EcceRegexp re2 ("[ \t]*$");
  
  string test = "<test>:blahstuff  \t\t\n";
  int foundIdx, length;

  cout << "test= " << endl << test;

  string substr = re.match(test, foundIdx, length);
  test.replace(foundIdx, length, "<R>");
  cout << "foundidx = " << foundIdx << endl
       << "length = " << length << endl;
  cout << "replacement = " << endl << test;


  string substr2 = re2.match(test, foundIdx, length);
  cout << "***" << substr2 << "***" << endl;
  */
  /*
  EcceRegexp re ("[^ \t]");
  string line("    thisIsATest   *");

  int pos, substrLen;
  re.match(line,pos,substrLen);
  cout << "pos= " << pos << endl;

  cout << line.replace(0,pos,"") << endl;
  */

  EcceRegexp re("stuff....trj");
  string line("stuff01.trj");

  int pos, substrLen;
  string substring = re.match(line,pos,substrLen);
  
  cout << substring << endl;
  cout << "pos = " << pos << " length = " << substrLen << endl;

  return 0;
}
