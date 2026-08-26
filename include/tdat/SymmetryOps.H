/**
 * @file
 *
 *
 */
#ifndef SYMMETRYOPS_H
#define SYMMETRYOPS_H

#include <iostream>
  using std::ostream;
  using std::istream;

#include <string>
#include <vector>
  using std::string;
  using std::vector;

#include "util/IOException.H"
#include "util/InvalidException.H"
#include "util/SFile.H"

#include "tdat/Fragment.H"

class SymmetryOps
{
   public:
      static void clean(Fragment& frag, double threshold, const string& group);
      static string find(Fragment& frag, double threshold);
      static void findIrreducible(Fragment& frag, double threshold, const string& group);
      static void generateFragment(Fragment& frag, double threshold);
      static void generateLatticeFragment(Fragment& frag, double threshold);
      static void addGhosts(Fragment& frag);
      static void removeGhosts(Fragment& frag);

   protected:

      static SFile dumpCoordinates(const Fragment& frag, int nGhosts, 
            double threshold, const string& group) 
;
      static SFile dumpGetFragCoordinates(const Fragment& frag, int nGhosts, 
            double threshold, const string& group) 
;
      static SFile dumpLatticeFrag(Fragment& frag, int nGhosts, 
            double threshold, const string& group) 
;
      static SFile dumpCoordinates(const Fragment& frag, int nGhosts, 
            double threshold);
      static void loadCoordinates(Fragment& frag, const vector<int>& atoms, 
            const SFile& data) 
;
      static string readSymmetry(Fragment& frag, const vector<int>& atoms,
            const SFile& data);
      static void readIrrAtoms(vector<int>& extras,
            const SFile& data);
      static bool hasDuplicates(const Fragment& frag);

};


#endif
