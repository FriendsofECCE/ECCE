#ifndef  SGFRAGMENT_HH
#define  SGFRAGMENT_HH

#include <iostream>
  using std::istream;

//#include <utility>
//    using std::pair;

#include <vector>
  using std::vector;

#include <map>
  using std::map;

#include "util/EcceException.H"

#include "tdat/Fragment.H"
#include "tdat/DisplayStyle.H"

#include "inv/ChemKit/ChemData.H"

class SoAction;
class SoCallbackAction;
class SoGLRenderAction;
class SoPickAction;
class SoGetBoundingBoxAction;
class TAtm;
class SbResidue;


class  SGFragment : public Fragment, public ChemData 
{
  
    SO_NODE_HEADER(SGFragment);


public:

    SGFragment();
    SGFragment(const char *name);
    SGFragment(istream& istr, const string& type);
    SGFragment(const SGFragment& frag);
    SGFragment(const Fragment& frag);
    virtual  ~SGFragment(void);

    SGFragment& operator=(const SGFragment& frag);

    // Fragment virtuals
    virtual size_t     numBonds(void) const;

    bool getFragments(vector<bool>& globalatoms,
                     vector<bool>& vatoms, vector<bool>& vbonds,
                     const TAtm *theAtom,
                     const TAtm* lastAtom,
                     const TAtm* firstAtom) const;

    // Implement actions - Inventor
    virtual void    doAction(SoAction *action);
    virtual void    callback(SoCallbackAction *action);
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    pick(SoPickAction *action);

    /**
     * This call reproduces the getBoxDimensions functionality in
     * Fragment but is called by something in ChemData in the OpenMoiv
     * library so it is  being left in
     */
    virtual void getBoundBox(float &xmin, float &ymin, float &zmin,
	                       float &xmax, float &ymax, float &zmax);


    virtual void addAtom(const string& tag, const double* newCoords);
    virtual void addAtom(TAtm* atom);
    virtual void addAtom(TAtm* atom,TAtm *parent);
    virtual bool addAtom(const string& elem, const string& geom, 
    double x, double y, double z, double bondOrder);

    virtual void addFragment(const string& type, 
          const string& memstream, const string& filename,
          const string& units, int conatom=1,
          double x=0.0, double y=0.0, double z=0.0,
          bool genBondOrders=true,
          int modelNum=1, const string& altLoc=" ", const string& chainId=" ") 
       throw(EcceException);
    virtual void generateDNA(string &sequence) throw(EcceException);
    virtual void generatePeptide(string &sequence) throw(EcceException);

    virtual bool deleteSelectedBonds();
    virtual bool deleteSelectedAtoms(const vector<bool>& selAtoms);
    virtual bool deleteSelectedAtomsAndBonds(vector<bool>& selAtoms);
    virtual bool syncNubSelection(vector<bool>& selAtoms);

SoINTERNAL public:
    static void		initClass();


  SoEXTENDER public:
    // Required functions to "get" the data from a ChemBaseData-derived node
    virtual int32_t   getNumberOfAtoms() const;
    virtual int32_t   getNumberOfBonds() const;
    virtual short   getAtomicNumber(int32_t index) const;
    virtual int32_t getAtomId(int32_t index) const;
    virtual SbString getAtomName(int32_t index) const;
    virtual int32_t getAtomIndex(int32_t index) const;
    virtual SbVec3f getAtomCoordinates(int32_t index) const;
    virtual vector<int> *getAtomHighlight();
    virtual vector<int> *getBondHighlight();
    virtual int32_t getBondFrom(int32_t index) const;
    virtual int32_t getBondTo(int32_t index) const;
    virtual int     getBondType(int32_t index) const;
    virtual int32_t getBondIndex(int32_t index) const;

    // Convenience function to support fast selection filtering
    DisplayStyle getBondStyle(int index) const;
    //DisplayStyle  getBondStyle(int index) const;

    // For performance reasons, if you make changes to the bonding of a 
    // Fragment, you need to call this method to make sure that all
    // caches get updated.  We do this to reduce the overhead of
    // inventor calls as much as possible.
    // We could do the same with atoms my tests show that it isn't
    // necessary - maybe it will be for 50000 atoms?
    virtual void touchNumbers();  
    virtual void touchProteins();  

    virtual void   fixAllNubs(float scale, float scale_cpk);
    virtual void   fixAtomNubs(TAtm *atm, float scale);
    virtual void   fixNub(TAtm *nub, float scale);
    virtual void clear();
    virtual void clearSelection();

    // WARNING: This function is being invoked by a Fragment pointer that
    // has been derived from an SGFragment pointer. This is a highly bogus
    // kluge to get around the hierarchy of libraries in Ecce and should
    // be fixed at some point.
    virtual vector<int> findRingAtoms(void) const;

    void generateProteinAttributes();
    void clearProteinAttributes();
    bool hasBiologicalResidues();
    void getBackboneAtoms(vector<bool>& vatoms, vector<bool>& vbonds);
    void getSideChainAtoms(vector<bool>& vatoms, vector<bool>& vbonds);
    void getChainAtoms(const vector<string>& chains, vector<bool>& vatoms, vector<bool>& vbonds);
    bool onSameMolecule();

    // Methods to get selection using a list of atoms or residues. Used
    // for driving command line selection
    int getAtomIndexFromToken(string token);
    int getResidueIndexFromToken(string token);
    void getSelectionBits(
             vector<bool>& atombits, vector<bool>& bondbits);
    void getSelectionFromAtomList(vector<string> *list,
             vector<bool>& atombits, vector<bool>& bondbits);
    void getSelectionFromResidueList(vector<string> *list,
             vector<bool>& atombits, vector<bool>& bondbits);
    void atomsToResidues(const vector<int>& atoms, vector<int>& residues);
    void atomsToResidues(const vector<int>& atoms, vector<bool>& residues);
    void getAtomsFromNWChemString(string selection, vector<int>& atoms);
    // Return false if no atoms selected
    bool getSelectionFromNWChemString(string selection, vector<bool>& atombits,
                                      vector<bool>& bondbits);
    string getNWChemStringFromSelection();
    void getSelectionFromResidueStatus(const string& status, 
          vector<bool>& atombits, vector<bool>& bondbits);
    void getSelectionFromElement(string element, vector<bool>& atombits,
	vector<bool>& bondbits);
    void getSelectionFromAtomType(string type, vector<bool>& atombits,
	vector<bool>& bondbits);
    void getSelectionFromAtomName(string name, vector<bool>& atombits,
	vector<bool>& bondbits);
    void getSelectionFromResidueName(string name, vector<bool>& atombits,
	vector<bool>& bondbits);
    void getSolvent(vector<bool>& atombits, vector<bool>& bondbits);
    void getFullResidue(TAtm *atm, vector<bool>& atombits, vector<bool>& bondbits);

    // Methods to get parameters using NWChem string selections
    bool   getNWChemCenterOfMass(string sel, double &x, double &y, double &z);
    double getNWChemPMFDistance(string sel1, string sel2);
    double getNWChemPMFAngle(string sel1, string sel2, string sel3);
    double getNWChemPMFTorsionAngle(string sel1, string sel2, string sel3,
                                    string sel4);

    // Methods that act on whole scene graph
    void translate(double x,double y,double z, bool all=false);
    void rotate(int iaxis, double angle, bool all=false);
    void centerOfMass(double &x, double&y, double &z, bool all=false);

    // Instead of using friend, make this public
    vector<int> m_atomHighLight;
    vector<int> m_bondHighLight;


    // This is for customized atom labels.  If not empty, then its
    // expected to have one entry per atom to serve as the label used
    // when labels are turned on.
    vector<string> m_atomLabels;

protected:

   
    // Cached info for faster rendering
    IBondList *p_bondIndices;
    bool p_hasBioResidues;

    void makeSbResidues();

private:

    friend ostream& operator<< (ostream& os, const SGFragment&);

};








#endif /* SGFRAGMENT__HH */

