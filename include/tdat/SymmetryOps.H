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
      static void clean(Fragment& frag, double threshold, const string& group) throw(IOException,InvalidException);
      static string find(Fragment& frag, double threshold) throw(IOException,InvalidException);
      static void findIrreducible(Fragment& frag, double threshold, const string& group) throw(IOException,InvalidException);
      static void generateFragment(Fragment& frag, double threshold) throw(IOException,InvalidException);
      static void generateLatticeFragment(Fragment& frag, double threshold) throw(IOException,InvalidException);
      static void addGhosts(Fragment& frag) throw(IOException);
      static void removeGhosts(Fragment& frag);

   protected:

      static SFile dumpCoordinates(const Fragment& frag, int nGhosts, 
            double threshold, const string& group) 
         throw(IOException);
      static SFile dumpGetFragCoordinates(const Fragment& frag, int nGhosts, 
            double threshold, const string& group) 
         throw(IOException);
      static SFile dumpLatticeFrag(Fragment& frag, int nGhosts, 
            double threshold, const string& group) 
         throw(IOException);
      static SFile dumpCoordinates(const Fragment& frag, int nGhosts, 
            double threshold) throw(IOException);
      static void loadCoordinates(Fragment& frag, const vector<int>& atoms, 
            const SFile& data) 
         throw(IOException);
      static string readSymmetry(Fragment& frag, const vector<int>& atoms,
            const SFile& data) throw(IOException);
      static void readIrrAtoms(vector<int>& extras,
            const SFile& data) throw(IOException);
      static bool hasDuplicates(const Fragment& frag);

};


#endif
