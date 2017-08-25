/**
 * @file
 *
 *
 */
#ifndef FRAGMENT_HH
#define FRAGMENT_HH


#include <iostream>
  using std::ostream;
  using std::istream;

#include <string>
#include <vector>
#include <map>
  using std::string;
  using std::vector;
  using std::map;
  using std::less;

#include "util/Transformations.H"
#include "util/EcceSortedVector.H"
#include "util/EcceException.H"

#include "tdat/TAtm.H"
#include "tdat/Residue.H"
#include "tdat/TResChain.H"
#include "tdat/IBondList.H"


class TBond;
class TAtm;
class Residue;
class TResChain;
class TTagCountPair;
class GeomConstraintModel;
class MetaPotentialModel;
class QMMMModel;
class LatticeDef;

typedef map<string, int, less<string> > TagCountMap;
typedef vector<int> SortOrder;

/**
 * The Core class for manipulating molecular structures.
 * Fragments have atoms (TAtm) with have bonds (TBond).  They
 * may optionally have residues.  And they may hold on to an
 * association of geometry constraints.
 */
class Fragment
{
   public:

      // Constructors
      Fragment(void);
      Fragment(const string& name);
      Fragment(const string& name,
            const vector<string>& tags,
            const double* coordinates,
            const int numBonds = 0,
            const int *bonds = (int*)0);
      Fragment(const string& name,
            const EcceSortedVector<TTagCountPair, less<TTagCountPair> >& tags,
            const double* coordinates,
            const int numBonds = 0,
            const int *bonds = (int*)0);
      Fragment(const Fragment& frag);
      virtual Fragment      *clone() const;

      // Virtual Destructor
      virtual ~Fragment(void);

      // Operators
      Fragment& operator=(const Fragment& simpleFrag);
      bool      operator==(const Fragment& simpleFrag) const;
      bool      operator!=(const Fragment& simpleFrag) const;
      bool      operator<(const Fragment& rhs) const;


      // Modifiers
      void reIndexResidues(int start=0);
      void renumberAtoms(const int& start=0);
      void clear();
      void name(const string& value);
      void charge(const double& value);
      void charge(const int& value);
      void editResidueMode(int on_off, int index = -1);
      void pointGroup(const string& group);
      void useSymmetry(const bool& flag);
      virtual void merge(const Fragment& chemSys);
      virtual void absorbAtoms(Fragment& chemSys,bool clear);
      virtual void absorb(Fragment& chemSys,bool clear);
      virtual void absorb(Fragment& tmpfrag, TAtm *at, bool normalOrder);
      virtual void coordinates(const double* coords, size_t numCoords);
      virtual void addAtom(TAtm* atom, TAtm *parent);
      virtual void addAtom(const string& tag, const double* newCoords,
            const string& suffix="");
      virtual void addBonds(const int* bonds, int numBonds,
            const double* orders = (const double*)0,
            const DisplayStyle *styles = (const DisplayStyle*)0);
      virtual void addCovalentBonds(bool overrideIsDisplayed=false);
      virtual void addHydrogenBonds(void);
      virtual bool changeBond(TAtm *atm1, TAtm *atm2, float order);
      virtual void bondAtoms(TAtm *atm1, TAtm *atm2, float order,
            const DisplayStyle& style) throw(EcceException);
      virtual void setEnvIndices(void);
      virtual void generateShapes(void);
      virtual bool generateShape(TAtm *atm);
      virtual void generateBondOrders(void);
      void generateFullMolecule(void);
      void generateIrreducibleFragment(void);

      virtual void setLattice(LatticeDef *lattice);
      void resizeToUnitCell();
      virtual void addResidue(Residue* residue);
      virtual void addResidues(vector<Residue*>& residues);
      virtual void deleteResidue(int index);
      virtual void deleteResidues(vector<int>& residues);
      virtual void insertResidue(int ires,Residue* residue);
      void setChainIDs(TResChain* rchain);
      void setMainDisplayStyle(DisplayStyle mainStyle);

      // Methods for managing sort order of atoms in.
      //void sort(onwhat)
      void clearAtomSortOrder();
      void setAtomSortOrder(const SortOrder& order);
      const SortOrder& getAtomSortOrder() const;


      // Builder Modifiers - Index Based
      void deleteNubs(TAtm *atom);
      void deleteAtom(TAtm *atom);
      void deleteAtom(int index1, bool renumber = true );
      void resize(int size);
      void changeTag(int index, const string& tag);
      void changeCoords(int index, const double* newCoords);
      TBond *addBond(int index1, int index2, double bondOrder);
      void deleteBond(int index1, int index2);
      void deleteAllBonds(void);
      void removeAtom(int index);
      vector<string> generateLabels(int start=1);

      // Builder functions
      void  transform(const vector<TAtm*>& atm, MMatrix& matrix);
      bool bondLength(TAtm& atom1, TAtm& atom2, double length, 
                      bool moveConnected=true);
      bool angle(TAtm& atom1, TAtm& atom2, TAtm& atom3, double angle, 
                 bool moveConnected=true);
      bool dihedral(TAtm *atom1, TAtm *atom2, TAtm *atom3, TAtm *atom4,
            double angle);
      bool dihedralAll(TAtm *atom1, TAtm *atom2, TAtm *atom3, TAtm *atom4,
            double angle);
      bool rotateAboutBond(TAtm *atom1, TAtm *atom2, double angle);
      void mergeFragment(Fragment& tmpfrag, int tmpfragconatom, int fragconatom,
            double x, double y, double z);
      bool bondingAtoms(const Fragment& tmpfrag,
            int tmpfragcatm, int fragconatom,
            TAtm **mainNub, TAtm **mainAtom,
            TAtm **fragAtom, TAtm **fragNub);

      //void      fixGeoms(vector<TAtm*>& atoms);
      virtual void translate(double x, double y, double z);


      // Accessors
      string name() const;
      string pointGroup() const;
      bool useSymmetry() const;
      int charge() const;
      int model() const;
      string altLoc() const;
      string chain() const;
      int editResidueMode() ;
      int editResidueIndex() ;
      int editResidueMode() const ;
      int editResidueIndex() const ;
      virtual string generateStructureString(void);
      virtual string generateEmpiricalFormula(void);

      virtual string                formula(void) const;
      virtual string                uniqueTagStr() const; // used for basis tool
      virtual vector<TTagCountPair>* tagCounts(void) const;
      virtual TagCountMap*          tagCountsSTL(void) const;
      virtual size_t                numAtoms(void) const;
      virtual size_t                numNonNubs(void) const;
      virtual size_t                numBonds(void) const;
      virtual size_t                numResidues(void) const;
      virtual int                   nuclearCharge(void) const;
      virtual int                   count(const string& tag) const;
      virtual int                   numElectrons(void) const;
      virtual double*               coordinates(void) const;
      virtual const vector<Residue*>& residues(void);
      virtual Residue*              residue(const int ires) const;
      virtual vector<TAtm*>        *atoms(void) const;
      virtual vector<TAtm*>        *atoms(const string& tag) const;
      virtual vector<string>       *uniqueTags(void) const;
      virtual vector<TBond*>       *bonds(void) const;
      virtual IBondList*            bondIndices(void) const;
      int                           getFirstSolventAtomIndex() const;
      // WARNING: This is a dummy routine that will be used to invoke a method
      // in SGFragment from a Fragment pointer that originates as a SGFragment
      // pointer
      virtual vector<int>           findRingAtoms(void) const;
      Residue*                      findResidue(const char* name,
            const int ires, const char icode,
            const char iseg) const;
      Residue*                      findResidue(const int ires,
            const char icode, const char iseg) const;
      Residue*                      findResidue(const int index) const; 
      int                           getResidueIndex(Residue *residue) const;
      Residue*                      findResidue(const TAtm *atom) const; 
      vector<TAtm*>                 findResidueAtoms(const Residue *res) const;
      vector<int>                   findResidueAtomIndices(int index); 
      TResChain*                    getChainIDs(void);
      DisplayStyle                  getMainDisplayStyle() const;
      LatticeDef *                  getLattice() const;

      // Builder Accessors
      virtual TBond*                bond(int index1, int index2) const;
      virtual vector<TAtm*>*        connectedAtoms(int index) const;
      virtual TAtm*                 atomRef(int index) const;
      virtual int                   atomIndex(const TAtm& atom) const;
      virtual const double*         atomCoords(int index) const;
      bool                          contains(TAtm *atom) const;
      TAtm                         *nubParent(TAtm *nub) const;
      TAtm                         *childNub(TAtm *atom) const;

      // Aggregate Information
      virtual double*  covalentRadii(void) const;
      virtual int* atomicNumbers(void) const;
      int checkResidueStatus(void) const;

      // General methods

      bool  restoreMVM(istream& infile,double mult=1.0, bool genBonds=true);
      bool  restoreXYZ(istream& infile, double mult=1.0, bool genBonds=true);
      bool  restoreCAR(istream& infile, double mult=1.0, bool genBonds=true);
      bool prescanPDB(istream& infile, int& numModels,
            vector<string>& altLocVec,
            vector<bool>& groupFlgVec,
            vector<string>& chainVec,
            int &TotAtoms);
      bool prescanXYZ(istream& infile, int &TotAtoms);
      bool prescanMVM(istream& infile, int &TotAtoms);
      bool  restorePDB(istream& infile, double mult=1.0, bool genBonds=true,
            const int& selectMdl=1, const string& selectAlt=" ",
            const bool& groupFlg=true, bool readBonds=false,
	    const string& chainID=" ");
      bool  dumpXYZ(ostream& outfile);
      bool  dumpMVM(ostream& outfile, bool dumpAll=false);
      bool  dumpPDB(ostream& outfile, bool writeBonds=false);
      bool  dumpCAR(ostream& outfile);

      bool getConnected(vector<bool>& atoms, const TAtm *theAtom,
            const TAtm* lastAtom,
            const TAtm* firstAtom) const;
      bool getConnected(vector<bool>& vatoms, vector<bool>& vbonds,
            const TAtm *theAtom,
            const TAtm* lastAtom,
            const TAtm* firstAtom) const;
      virtual void centerOfMass(double& x, double& y, double& z);
      void residueCenterOfMass(int idx, double& x, double& y, double& z);
      bool containsNubs(void);
      vector<int> * getNubsIndex();
      void recenterFrag(int iop=0);
      void orientFrag();
      bool isLinear();
      void alignFrag(int atm1, int atm2, int atm3);
      void renumberResidues(void);
      void clearResidueInfo(void);
      int insertResidue(TAtm* atom);
      int prependResidue(TAtm* atom);
      void resetAtomList(vector<TAtm*> &list);
      void getBoxDimensions(double *dims);
      double getTotalCharge();
      Fragment* createSubFragment(vector<int>, bool complement = false);
      Fragment* clipFragment(vector<int> atoms);
      Fragment* clipSelectedResidue(int index);
      void setReactionIndices();
      void setReactionIndicesIfUnset();
      void findFirstSolventAtomIndex();
      void setFirstSolventAtomIndex(int idx);
      void clearFirstSolventAtomIndex();
      void setSolventNumAtoms(int size);
      int  getSolventNumAtoms() const;
      bool slice(MPoint normal, MPoint origin, double displ1, double displ2);
      void convertToFractionalCoords(const bool& unshifted);
      void convertFromFractionalCoords();

      // Constraint stuff
      void setConstraints(GeomConstraintModel *constraints);
      GeomConstraintModel *getConstraints() const;

      // Meta-Potential stuff
      void setPotentials(MetaPotentialModel *potentials);
      MetaPotentialModel *getPotentials() const;

      // QM/MM stuff
      bool checkFixedCharges(void) const;
      vector<int> getZeroCharges(void);
      void clearAtomBehavior(void);
      void setQMMMModel(QMMMModel *model);
      QMMMModel *getQMMMModel() const;
      int getNumDoF(void) const;
      void setNumDoF(int DoF);

      void fixBondStyles(const DisplayStyle& style);

   protected:

      void adjustCoordinates(double mult);
      void clearState(void);
      virtual void addAtom(TAtm* atom);
      void addAtoms(const char** tags, const double* coords, int numAtoms);
      void addAtoms(const vector<string>& tags,
            const double* coords);
      void addAtoms(const EcceSortedVector<TTagCountPair,
            less<TTagCountPair> >& tags,
            const double* coords);
      void addAtoms(const vector<TAtm*>& atoms);

      void copyAtoms(const vector<TAtm*>& atoms);


   protected:
      string                         p_name;
      int                            p_charge;
      string                         p_pointGroup;
      bool                           p_useSymmetry;
      vector<TAtm*>                  p_atoms;
      vector<Residue*>               p_residueSequence;
      LatticeDef *                   p_lattice;
      int                            p_firstSolventAtomIndex;
      int                            p_solventNumAtoms;
      int                            p_DoF;

      // when editResidyeMode is true, we add atoms to the appropriate atom
      // group.  Not to the end of the list
      int                            p_editResidueMode;
      int                            p_editResidueIndex;

      // TResChain object to keep track of chain label usage
      TResChain p_chains;

      // display style information
      DisplayStyle                   p_mainDisplayStyle ;

      /**
       * Keep track of any atom sorting requested by the user.
       * If standard sort order is used, SortOrder will be of size 0.
       * For now, the Fragment class doesn't have methods to do the sort - they
       * are done by the GUI but they should be made into Fragment methods
       * in the future so the functionality is separate from the GUI.
       */
      SortOrder                      p_atomSortOrder;

      /**
       * A fragment can have constraints on its geometry.
       * The memory for this model is not managed by the Fragment class.
       * That is, it's a shared pointer that must be created and deleted outside
       * the Fragment class and merely associates the constraints with
       * the Fragment. The meta-potentials QMMM terms are managed in the same way.
       */
      GeomConstraintModel           *p_constraints;
      MetaPotentialModel            *p_potentials;
      QMMMModel                     *p_QMMMTerms;


   private:


      friend ostream& operator<<(ostream& os, const Fragment& frag);

};


#endif
