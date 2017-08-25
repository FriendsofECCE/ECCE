#include <iostream>
  using namespace std;

#include <map>
#include "util/EcceSortedVector.H"
#include "util/CompareOps.H"
#include "tdat/TTCPair.H"

int main(int argc, char** argv)
{
   
  /*  TEST DEFAULT CONSTRUCTOR 
  EcceSortedVector<string, less<string> > vec;

  vec.push_back("monkey");
  vec.push_back("cat");
  vec.push_back("apple");
  vec.push_back("banana");
  vec.push_back("dog");

  for (int i = 0; i < vec.size(); i++) {
    
    cout << vec[i] << endl;
  }
  */  

  /* TEST COPY CONSTRUCTOR 
  vector<string> vec;
  vec.push_back("monkey");
  vec.push_back("cat");
  vec.push_back("apple");
  vec.push_back("banana");
  vec.push_back("dog");

  EcceSortedVector<string, less<string> > sVec(vec);

  for (int i = 0; i < sVec.size(); i++) {
    
    cout << sVec[i] << endl;
  }
  */

  /* TEST NON-STL CLASS 
  TTagCountPair test;
  EcceSortedVector<TTagCountPair, less<TTagCountPair> > vec;

  test.tag = "H";
  test.count = 2;
  vec.push_back(test);
  
  test.tag = "C";
  test.count = 5;
  vec.push_back(test);
  
  test.tag = "Al";
  test.count = 1;
  vec.push_back(test);

  for (int i = 0; i < vec.size(); i++) {

    cout << vec[i].tag << endl;
  }
  */

  /* TEST = OPERATOR 
  //EcceSortedVector<string, less<string> > vec;
  vector<string> vec;

  vec.push_back("one");
  vec.push_back("two");
  vec.push_back("three");
  
  EcceSortedVector<string, less<string> > vec2(vec);
  
  for (int i = 0; i < vec2.size(); i++) {
    cout << vec2[i] << endl;
  }
  */

  /* TEST POINTER COMPARISON OPERATOR */
  EcceSortedVector<string*, ltstringPtr> vec;
  
  vec.push_back(new string("monkey"));
  vec.push_back(new string("cat"));
  vec.push_back(new string("apple"));
  vec.push_back(new string("banana"));
  vec.push_back(new string("dog"));

  for (int i = 0; i < vec.size(); i++) {  
    cout << vec[i]->c_str() << endl;
  }
  return 0;
}
