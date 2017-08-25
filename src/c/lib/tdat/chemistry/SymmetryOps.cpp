/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;
#include <fstream>
  using std::ifstream;
#include <iomanip>
  using std::ios;


#include <vector>
  using std::vector;
#include <algorithm>
  using std::find;

#include "math.h"
#include "string.h"

#include "util/Ecce.H"
#include "util/TempStorage.H"
#include "util/SFile.H"

#include "tdat/SymmetryOps.H"
#include "tdat/LatticeDef.H"

static string cleanCmd = "./cleansym";
static string autosymCmd = "./autosym";
static string getIFrag = "./getfrag";
static string genFrag = "./genmol";
static string genLatticeFrag = "./genmollat";


void SymmetryOps::addGhosts(Fragment& frag) throw(IOException)
{
   string group = frag.pointGroup();

   // Convert to upper case
   for (int i = 0; i<group.size(); i++) group[i] = tolower(group[i]);

   // Generate file name where data is stored
   string path = Ecce::ecceDataPath();
   path += "/client/symghosts/";
   path += group;
   path += ".xyz";

   ifstream *fp = new ifstream((char*)path.c_str(), ios::in);
   if (fp) {
      frag.restoreXYZ(*fp, 1.0, false);
      fp->close();
      delete fp;
   } else {
      throw IOException("Failed open ghost file.", WHERE);
   }
}


void SymmetryOps::removeGhosts(Fragment& frag)
{
   // Delete from back because that is probably where the ghosts are at
   int ncenter = frag.numAtoms();
   for (int idx=ncenter-1; idx>=0; idx--) {
      if (frag.atomRef(idx)->atomicSymbol() == "X") {
         frag.deleteAtom(idx);
      }
   }
}


void SymmetryOps::findIrreducible(Fragment& frag, double threshold, const string& group)
   throw(IOException,InvalidException)
{
   if (frag.numAtoms() == 0) return;

   // collect all ghost atoms and delete them from the original list
   // They'll be added back later
   string ghost_tag = "X" ;
   vector<TAtm*>* ghostatoms = frag.atoms( "X" ) ;
#if (!defined(INSTALL) && defined(DEBUG))
   cout << ghostatoms->length() << " ghost atoms found." << endl ;
   for (int g=0; g< ghostatoms->length(); g++)
      cout << *(ghostatoms->at(g)) << endl;
#endif

   // delete ghost atoms
   vector<TAtm*>* atomlist = frag.atoms() ;
   for ( int a = atomlist->size() -1 ; a > -1; a--) {
      if ( (*atomlist)[a]->atomicSymbol() == "X" ) {
         frag.deleteAtom(a) ;
      }
   }
   delete atomlist;

   // create list of atom indices
   // This will be reduced to the list of we want to toss (the excess atoms)
   vector<int> listofatoms ;
   int ncenter = frag.numAtoms();
   for (int k = 0; k < ncenter; k++)
      listofatoms.push_back( k ) ;


   // write coordinates to a file
   SFile inFile = 
      dumpGetFragCoordinates(frag, 0, threshold, group);

   SFile* outFile = TempStorage::getTempFile();

   // create command line
   string cmd = getIFrag + " < " + inFile.path() + " >" + outFile->path();

   int istatus = system((char*)cmd.c_str()) ;  // execute command
   istatus = ( istatus >> 8 ) ;
   inFile.remove();

   if (istatus == 0) {

      readIrrAtoms(listofatoms, *outFile);
      outFile->remove();
      delete outFile;


      // Delete all atoms except the irreducible indices
      for ( int r = listofatoms.size()-1; r > -1 ; r--) {
         frag.deleteAtom( listofatoms[r] ) ;
      }

      // add the ghost atoms back in
      if ( ghostatoms->size() > 0 ) {
         for ( int t = 0; t < ghostatoms->size(); t++ ) {
            TAtm *atmPtr = (*ghostatoms)[t] ;
            frag.addAtom( new TAtm(*atmPtr), 0 ) ;
         }
      }

      // we're done with this list.  Get rid of it.
      for (int i = 0; i<ghostatoms->size(); i++) {
         delete (*ghostatoms)[i];
      }
      ghostatoms->clear();
      delete ghostatoms;

   } else if ( istatus == 1 ) {
      throw InvalidException("Failed to execute getfrag.", WHERE);
   } else if ( istatus == 2 ) {
      throw InvalidException("Maximum number of atoms exceeded.", WHERE);
   }

}



void SymmetryOps::generateLatticeFragment(Fragment& frag, double threshold)
throw(IOException,InvalidException)
{
   if (frag.numAtoms() == 0) return;
//   if (frag.pointGroup() == "C1") return;

   // save the ghost atoms:
   string ghost_tag = "X";
   vector<TAtm*>* ghostatoms = frag.atoms( ghost_tag );

   // write coordinates to a file
   SFile inFile = dumpLatticeFrag(frag, ghostatoms->size(), 
         threshold, frag.pointGroup());

   SFile* outFile = TempStorage::getTempFile();

   // create command line
   string cmd = genLatticeFrag + " < " + inFile.path() + " >" + outFile->path();
   //cout << "cmd " << cmd << endl;

   int istatus = system((char*)cmd.c_str()) ;  // execute command
   istatus = ( istatus >> 8 ) ;
   inFile.remove();

   if (istatus == 0) {
      ifstream *fp = new ifstream(outFile->path().c_str());
      if (fp) {
         string group = frag.pointGroup();
         // Since the fragment clear will remove the lattice,
         // save a copy here so we can restore it - we want to remove
         // the atoms and bonds but not the lattice.
         LatticeDef *lattice = new LatticeDef(*frag.getLattice());
         frag.clear();
         frag.setLattice(lattice);
         bool status = frag.restoreXYZ(*fp,1.0, false);
         if (!status) {
            throw InvalidException("Unable to parse output.", WHERE);
         }
         // Nwchem puts out fractionals...
         frag.convertFromFractionalCoords();

         frag.addCovalentBonds();

         // reset useSymmetry flag for the benefit of calced
         frag.useSymmetry(false);

         // restore since the clear method clears it
         frag.pointGroup(group);

         fp->close();
         outFile->remove();
         // Add in old ghost atoms
         if ( ghostatoms->size() > 0 )
            for ( int t = 0; t < ghostatoms->size(); t++ ) {
               TAtm *atmPtr = (*ghostatoms)[t] ;
               frag.addAtom( new TAtm( *atmPtr ), 0  ) ;
            }

         // we're done with this atom list
         for (int g=0; g<ghostatoms->size(); g++)
            delete (*ghostatoms)[g];

         ghostatoms->clear();
         delete ghostatoms;

      } else {
         throw IOException("Failed to open output file.", WHERE);
      }

   } else if ( istatus == 1 ) {
      throw InvalidException("Failed to execute generate fragment.", WHERE);
   } else if ( istatus == 2 ) {
      throw InvalidException("Maximum number of atoms exceeded.", WHERE);
   }
}



void SymmetryOps::generateFragment(Fragment& frag, double threshold)
throw(IOException,InvalidException)
{
   if (frag.numAtoms() == 0) return;
   if (frag.pointGroup() == "C1") return;

   // save the ghost atoms:
   string ghost_tag = "X";
   vector<TAtm*>* ghostatoms = frag.atoms( ghost_tag );

   // write coordinates to a file
   SFile inFile = dumpGetFragCoordinates(frag, ghostatoms->size(), 
         threshold, frag.pointGroup());

   SFile* outFile = TempStorage::getTempFile();

   // create command line
   string cmd = genFrag + " < " + inFile.path() + " >" + outFile->path();
   //cout << "cmd " << cmd << endl;

   int istatus = system((char*)cmd.c_str()) ;  // execute command
   istatus = ( istatus >> 8 ) ;
   inFile.remove();

   if (istatus == 0) {
      ifstream *fp = new ifstream(outFile->path().c_str());
      if (fp) {
         string group = frag.pointGroup();
         frag.clear();
         bool status = frag.restoreXYZ(*fp,1.0, false);
         if (!status) {
            throw InvalidException("Unable to parse output.", WHERE);
         }
         frag.addCovalentBonds();

         // reset useSymmetry flag for the benefit of calced
         frag.useSymmetry(false);

         // restore since the clear method clears it
         frag.pointGroup(group);

         fp->close();
         outFile->remove();
         // Add in old ghost atoms
         if ( ghostatoms->size() > 0 )
            for ( int t = 0; t < ghostatoms->size(); t++ ) {
               TAtm *atmPtr = (*ghostatoms)[t] ;
               frag.addAtom( new TAtm( *atmPtr ), 0  ) ;
            }

         // we're done with this atom list
         for (int g=0; g<ghostatoms->size(); g++)
            delete (*ghostatoms)[g];

         ghostatoms->clear();
         delete ghostatoms;

      } else {
         throw IOException("Failed to open output file.", WHERE);
      }

   } else if ( istatus == 1 ) {
      throw InvalidException("Failed to execute generate fragment.", WHERE);
   } else if ( istatus == 2 ) {
      throw InvalidException("Maximum number of atoms exceeded.", WHERE);
   }
}




void SymmetryOps::clean(Fragment& frag, double threshold, const string& group)
   throw(IOException,InvalidException)
{
   if (frag.numAtoms() == 0) return;

   int ncenter = frag.numAtoms();

   string lgroup = group;
   for (int i=0; i<lgroup.size(); i++) lgroup[i] = toupper(lgroup[i]);

   // save indices of atoms that are not ghost atoms and real atoms
   vector<int> atom_indices ;
   vector<int> ghostatoms;
   for (int idx=0; idx<ncenter; idx++) {
      if ( frag.atomRef(idx)->atomicSymbol() == "X" ) {
         ghostatoms.push_back(idx);
      } else {
         atom_indices.push_back(idx);
      }
   }


   // write coordinates to a file
   SFile inFile = 
      dumpCoordinates(frag, ghostatoms.size(), threshold, lgroup);

   SFile* outFile = TempStorage::getTempFile();

   // create command line
   string cmd = cleanCmd + " < " + inFile.path() + " >" + outFile->path();

//   cout << "executing " << cmd << endl;
   int istatus = system((char*)cmd.c_str()) ;  // execute command
   istatus = ( istatus >> 8 ) ;
   inFile.remove();

   if (istatus == 0) {

      loadCoordinates(frag, atom_indices, *outFile);
      outFile->remove();
      delete outFile;

      // Check for duplicates
      if (hasDuplicates(frag)) {
         throw InvalidException("Duplicate atoms detected.", WHERE);
      }

   } else if ( istatus == 1 ) {
      throw InvalidException("Failed to execute cleansym.", WHERE);
   } else if ( istatus == 2 ) {
      throw InvalidException("Maximum number of atoms exceeded.", WHERE);
   }
} 


/**
 * Execute the symmetry find operaton.
 * @return symmetry group name 
 */
string SymmetryOps::find(Fragment& frag, double threshold) 
  throw(IOException, InvalidException)
{
   string group = "C1";

   if (frag.numAtoms() == 0) return group;


   int ncenter = frag.numAtoms();

   // save indices of atoms that are not ghost atoms and real atoms
   vector<int> atom_indices ;
   vector<int> ghostatoms;
   for (int idx=0; idx<ncenter; idx++) {
      if ( frag.atomRef(idx)->atomicSymbol() == "X" ) {
         ghostatoms.push_back(idx);
      } else {
         atom_indices.push_back(idx);
      }
   }

   // write coordinates to a file
   SFile inFile = dumpCoordinates(frag, ghostatoms.size(), threshold);

   SFile* outFile = TempStorage::getTempFile();

   // create command line
   string cmd = autosymCmd + " < " + inFile.path() + " >" + outFile->path();

   int istatus = system((char*)cmd.c_str()) ;  // execute command
   istatus = ( istatus >> 8 ) ;
   inFile.remove();

   if (istatus == 0) {

      group = readSymmetry(frag, atom_indices, *outFile);
      outFile->remove();
      delete outFile;


   } else if ( istatus == 1 ) {
      throw InvalidException("Failed to execute autosym.", WHERE);
   } else if ( istatus == 2 ) {
      throw InvalidException("Maximum number of atoms exceeded.", WHERE);
   }

   return group;

}



/**
 * Method to dump atom coordinates to temporary input file.
 * Ghost atoms are not dumped.
 * Format is:
 *
 * #atoms
 * threshold
 * groupname
 * sym
 * # x y z
 * sym
 * # x y z
 * ...
 */
SFile SymmetryOps::dumpGetFragCoordinates(const Fragment& frag, int nGhosts, 
      double threshold,
      const string& group) throw(IOException)
{
   // Create input file
   SFile* inFile = TempStorage::getTempFile();
   FILE* fp = fopen((char*)inFile->path().c_str(), "w");

   TAtm* aref = 0;
   if (fp) {

      int ncenter = frag.numAtoms();

      fprintf(fp, "%d\n%G\n%-8s\n", ncenter - nGhosts, threshold, group.c_str());
      for (int it=0; it<ncenter; it++) {
         aref = frag.atomRef(it);
         const double *coords = aref->coordinates();
         if ( aref->atomicSymbol() != "X" ) {
            fprintf(fp, "%-16s\n%d %G %G %G\n",
                 (char*)aref->atomicSymbol().c_str(),
                 aref->atomicNumber(),
                 coords[0], coords[1], coords[2]);
         }
      }
      fclose(fp);
   } else {
      throw IOException("Unable to open input file", WHERE);
   }

   // DO this to avoid memory leak if exception is thrown
   SFile ret = *inFile;
   delete inFile;

   return ret;
}


SFile SymmetryOps::dumpLatticeFrag(Fragment& frag, int nGhosts, 
      double threshold,
      const string& group) throw(IOException)
{
   // Check for valid LatticeDef object
   LatticeDef *lattice = frag.getLattice();
   if (lattice == 0) throw IOException("No lattice", WHERE);

   // Create input file
   SFile* inFile = TempStorage::getTempFile();
   FILE* fp = fopen((char*)inFile->path().c_str(), "w");

   TAtm* aref = 0;
   if (fp) {
      frag.convertToFractionalCoords(true);

      int ncenter = frag.numAtoms();

      fprintf(fp, "%d\n%G\n%-8s\n", ncenter - nGhosts, threshold, group.c_str());
      double A, B, C, alpha, beta, gamma;
      lattice->getLattice(A,B,C,alpha,beta,gamma);
      fprintf(fp, "%G %G %G %G %G %G\n",A,B,C,alpha,beta,gamma);

      for (int it=0; it<ncenter; it++) {
         aref = frag.atomRef(it);
         const double *crd = aref->coordinates();

         if ( aref->atomicSymbol() != "X" ) {
            fprintf(fp, "%-16s\n%d %G %G %G\n",
                  (char*)aref->atomicSymbol().c_str(),
                  aref->atomicNumber(), crd[0], crd[1], crd[2]);
         }
      }
      fclose(fp);
      frag.convertFromFractionalCoords();
   } else {
      throw IOException("Unable to open input file", WHERE);
   }

   // DO this to avoid memory leak if exception is thrown
   SFile ret = *inFile;
   delete inFile;

   return ret;
}

/**
 * Method to dump atom coordinates to temporary input file.
 * Ghost atoms are not dumped.
 * Format is:
 *
 * #atoms
 * threshold
 * groupname
 * x y z
 * x y z
 * ...
 */
SFile SymmetryOps::dumpCoordinates(const Fragment& frag, int nGhosts, 
      double threshold,
      const string& group) throw(IOException)
{
   // Create input file
   SFile* inFile = TempStorage::getTempFile();
   FILE* fp = fopen((char*)inFile->path().c_str(), "w");

   TAtm* aref = 0;
   if (fp) {

      int ncenter = frag.numAtoms();

      fprintf(fp, "%d\n%G\n%-8s\n", ncenter - nGhosts, threshold, group.c_str());
      for (int it=0; it<ncenter; it++) {
         aref = frag.atomRef(it);
         const double *coords = aref->coordinates();
         if ( aref->atomicSymbol() != "X" ) {
            fprintf(fp, "%G %G %G\n",coords[0], coords[1], coords[2]);
         }
      }
      fclose(fp);
   } else {
      throw IOException("Unable to open input file", WHERE);
   }

   // DO this to avoid memory leak if exception is thrown
   SFile ret = *inFile;
   delete inFile;

   return ret;
}


/**
 * Method to dump atom coordinates to temporary input file.
 * Used by find method
 * Ghost atoms are not dumped.
 * Format is:
 *
 * #atoms
 * threshold
 * sym 
 * # x y z
 * sym 
 * # x y z
 * ...
 */
SFile SymmetryOps::dumpCoordinates(const Fragment& frag, int nGhosts, 
      double threshold) throw(IOException)
{
   // Create input file
   SFile* inFile = TempStorage::getTempFile();
   FILE* fp = fopen((char*)inFile->path().c_str(), "w");

   TAtm* aref = 0;
   if (fp) {

      int ncenter = frag.numAtoms();

      fprintf(fp, "%d\n%G\n", ncenter - nGhosts, threshold);
      for (int it=0; it<ncenter; it++) {
         aref = frag.atomRef(it);
         const double *coords = aref->coordinates();
         if ( aref->atomicSymbol() != "X" ) {
            fprintf(fp, "%-16s\n%d %G %G %G\n",
                 (char*)aref->atomicSymbol().c_str(),
                 aref->atomicNumber(),
                 coords[0], coords[1], coords[2]);
         }
      }
      fclose(fp);
   } else {
      throw IOException("Unable to open input file", WHERE);
   }

   // DO this to avoid memory leak if exception is thrown
   SFile ret = *inFile;
   delete inFile;

   return ret;
}



/**
 * Load coordinates from temporary file.
 * Format is:
 * 
 * #atoms
 * x y z
 * x y z
 * ...
 */
void SymmetryOps::loadCoordinates(Fragment& frag, const vector<int>& indices,
      const SFile& outFile) throw(IOException) 
{
    // read in new file
    FILE* fp = fopen((char*)outFile.path().c_str(), "r");
    if (fp != (FILE*)0) {
       char buf[256];

       // Read number of atoms
       (void)fgets(buf, 256, fp);
       int natoms = (int)strtol(buf, NULL, 10);

       char* tail;
       // The file only contains non-ghost atom data
       // natoms should be the same as atoms.size()
       int ai;
       TAtm *atm;
       double x, y, z;
       for (int it=0; it<natoms; it++) {
          ai = indices[it] ;
          atm = frag.atomRef(ai);

          (void)fgets(buf, 256, fp);
          x = strtod(buf, &tail);
          y = strtod(tail, &tail);
          z = strtod(tail, &tail);
          atm->coordinates(x,y,z);
       }
       fclose(fp);
    } else {
       throw IOException("Unable to open output file", WHERE);
    }
}


/**
 * Read symmetry group from file
 * Group name is converted to mixed case convention.
 */
string SymmetryOps::readSymmetry(Fragment& frag, const vector<int>& indices, 
        const SFile& outFile) throw(IOException)
{
   char group[8];

   FILE* fp = fopen((char*)outFile.path().c_str(), "r");
   if (fp != (FILE*)0) {
      char buf[256];
      (void)fgets(buf, 256, fp);
      strncpy(group, buf, 8);
      char* space = strchr(group, ' ');
      if (space != NULL)
         *space = '\0';

      int natoms = indices.size();
      int ai;
      TAtm *atm;
      double x, y, z;
      char *tail;
      for (int it=0; it<natoms; it++) {
         ai = indices[it] ;
         atm = frag.atomRef(ai);

         (void)fgets(buf, 256, fp);
         x = strtod(buf, &tail);
         y = strtod(tail, &tail);
         z = strtod(tail, &tail);
         atm->coordinates(x,y,z);
      }

      // make symmetry conform to upper/lower case convention
      for (space=group; *space!='\0'; space++)
         *space = tolower(*space);
      *group = toupper(*group);

      fclose(fp);

   } else {
      throw IOException("Unable to open output file", WHERE);
   }
   return group;
}


bool SymmetryOps::hasDuplicates(const Fragment& frag)
{
   bool ret = false;
   // check for near duplicate atoms (but do not compare with
   // ghost atoms)
   vector<TAtm*>* allatoms = frag.atoms() ;
   int allAtomCnt = allatoms->size();

   for ( int d = 0; d < allAtomCnt; d++ )
   {
      if ( (*allatoms)[d]->atomicSymbol() != "X" )
      {
         const double *ac1 = (*allatoms)[d]->coordinates() ;
         for ( int j = d+1; j < allatoms->size(); j++ )
         {
            if ( (*allatoms)[j]->atomicSymbol() != "X" )
            {
               const double *ac2 = (*allatoms)[j]->coordinates() ;
               double tol = 0.1 ;
               if ( ( fabs(ac1[0] - ac2[0]) < tol ) &&
                     ( fabs(ac1[1] - ac2[1]) < tol ) &&
                     ( fabs(ac1[2] - ac2[2]) < tol ) )
               {
                  ret = true;
                  break;
               }
            }

         }
      }
   }  // end for loop for duplicates
   delete allatoms;
   return ret;
}


/**
 * Read list of atoms that should be kept in the system.
 * The atom list passed in is modified.  
 *
 * The file contains a list of the listofatoms that should remain in the
 * system.  The atoms remaining in the atoms vector are the ones
 * removed from the fragment
 */
void SymmetryOps::readIrrAtoms(vector<int>& listofatoms, 
      const SFile& data) throw(IOException)
{
    // read in new file
   FILE* fp = fopen((char*)data.path().c_str(), "r");
   if (fp != (FILE*)0) {
      char buf[256];

      (void)fgets(buf, 256, fp);
      int nind = (int)strtol(buf, NULL, 10);
      int ind;
      vector<int>::iterator iter;
      for (int it=0; it<nind; it++) {
         (void)fgets(buf, 256, fp);
         ind = (int)strtol(buf, NULL, 10) - 1;
         iter = ::find(listofatoms.begin(),listofatoms.end(),ind);
         if (iter != listofatoms.end()) listofatoms.erase(iter) ;
      }

      fclose(fp);

   } else {
      throw IOException("Unable to open output file", WHERE);
   }
}
