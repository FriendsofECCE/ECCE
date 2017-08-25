///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: EcceSortedVector.H
//
//
// CLASS SYNOPSIS:
//    A vector that always stays sorted (if you use the push_back function
//    for inserting values).  Use this class to replace RWTValSortedVector.
//    
///////////////////////////////////////////////////////////////////////////////

#ifndef ECCESORTEDVECTOR_H
#define ECCESORTEDVECTOR_H

#include <vector>
#include <algorithm>

using std::vector;
using std::sort;

template <class T, class Comp>

/**
 * NOTE:  Your class must have the less than operator ( < )
 * defined in order to use the sorted vector 
 */
class EcceSortedVector : public vector<T> {

  public:
  
  // Empty constructor
  EcceSortedVector(bool autosort=true) : vector<T>() {
    p_autosort = autosort;
  }
 
  // Construct from existing vector
  EcceSortedVector(const vector<T>& vec) : vector<T>(vec) {

    p_autosort = true;
    sort(this->begin(), this->end(), Comp());    
  }

  // Construct from existing sorted vector
  EcceSortedVector(const EcceSortedVector<T, Comp>& vec) : vector<T>(vec) {
    p_autosort = vec.p_autosort;
  }

  // Overloaded = operator (works on any vector)
  EcceSortedVector& operator=(const vector<T>& vec) {
    vector<T>::operator=(vec);
    p_autosort = true;
    sort(this->begin(), this->end(), Comp());
    return *this;
  }

  EcceSortedVector& operator=(const EcceSortedVector<T, Comp>& vec) {
    vector<T>::operator=(vec);
    p_autosort = vec.p_autosort;
    return *this;
  }

  // Only use this function to insert values if you want it sorted.
  // Note that vector's push_back class is NOT virtual, so you can
  // only access the sorted version of push_back if the pointer is explicitly
  // cast as (EcceSortedVector*)
  void push_back(const T& val) {
    vector<T>::push_back(val);
    if (p_autosort) {
      sort(this->begin(), this->end(), Comp());
    }    
  }

  // Sort on demand
  void sortElements() {
    sort(this->begin(), this->end(), Comp());
  }
  

  private:

  bool p_autosort;  // indicates if we should sort with every call to push_back

};

#endif // ECCESORTEDVECTOR_H


