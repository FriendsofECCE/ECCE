//#define DEBUG

#ifdef DEBUG
#include <iostream>
  using std::cout;
  using std::endl;
#endif
#include <fstream>
  using std::ifstream;
#include <strstream>
  using std::istrstream;
#include <set>
  using std::set;
#include <algorithm>
  using std::sort;
#include <list>
    using std::list;
  using std::find;

#ifdef __GNUC__
  #include <ext/hash_map>
  using __gnu_cxx::hash_map;
  using __gnu_cxx::hash;
#endif

#include "inv/SoDB.H"
#include "inv/nodekits/SoWrapperKit.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoPickAction.H"

#include "util/ETimer.H"         
#include "util/ErrMsg.H"         
#include "util/Transformations.H"
#include "util/CompareOps.H"         
#include "util/STLUtil.H"   
#include "util/StringConverter.H"   
#include "util/StringTokenizer.H"   
#include "util/SFile.H"   

#include "tdat/FragUtil.H"
#include "tdat/LatticeDef.H"
#include "tdat/TBond.H"
#include "tdat/TPerTab.H"
#include "tdat/TAtm.H"
#include "tdat/TResTab.H"
#include "tdat/TResItem.H"
#include "tdat/LinkCellList.H"

#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSI.H"

#include "inv/ChemKit/ChemKit.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemColor.H"
#include "inv/ChemKit/ChemRadii.H"
#include "inv/ChemKit/ChemDisplayParam.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/SparseBits.H"
#include "inv/ChemKit/ChemSSSR.H"

#include "viz/SGFragment.H"   

#define PI 3.14159265358979323846
static const char *NUB_TAG = "Nub";

SO_NODE_SOURCE(SGFragment);

typedef map<int, int>::value_type RRPair;

// File static for convenience
static ShapeData s_shapes;



///////////////////////////////////////////////////////////////////////////
// Description
///////////////////////////////////////////////////////////////////////////
void SGFragment::initClass()
{
   SO_NODE_INIT_CLASS(SGFragment, ChemData ,"ChemData");
}

///////////////////////////////////////////////////////////////////////////
// Descriptions
//  Default constructor - required for NODE but not allowed by
//  ECCE Subject hierary.  That is why we can get away with calling
//  the superclass with a 0 pointer.
///////////////////////////////////////////////////////////////////////////
SGFragment::SGFragment() : Fragment(), ChemData(), p_bondIndices(0)
{
  SO_NODE_CONSTRUCTOR(SGFragment);
  p_hasBioResidues = false;
  touchNumbers();
}

SGFragment::SGFragment(const char *name) : Fragment(name), ChemData(),
                               p_bondIndices(0)
{
  SO_NODE_CONSTRUCTOR(SGFragment);
  p_hasBioResidues = false;
  touchNumbers();
}



///////////////////////////////////////////////////////////////////////////
// Description
//   Construct object from a stream and do it even though the ECCE
//   Subject hierarrcy doesn't support default constructors.  We 
//   construct a tiny dummy object as a work around.  
///////////////////////////////////////////////////////////////////////////
SGFragment::SGFragment(istream& infile, const string& type) 
                        : Fragment(), 
                          ChemData(), 
                          p_bondIndices(0)
{

  SO_NODE_CONSTRUCTOR(SGFragment);
  p_hasBioResidues = false;
  touchNumbers();

}

SGFragment::SGFragment(const SGFragment& frag) 
         : Fragment(frag), ChemData(), p_bondIndices(0)
{
  SO_NODE_CONSTRUCTOR(SGFragment);
  p_hasBioResidues = false;
  m_atomHighLight = frag.m_atomHighLight;
  m_bondHighLight = frag.m_bondHighLight;
  touchNumbers();
  m_atomLabels = frag.m_atomLabels;
  p_mainDisplayStyle = frag.p_mainDisplayStyle;
}

///////////////////////////////////////////////////////////////////////////
// Description
//   Have to initialize display styles.
///////////////////////////////////////////////////////////////////////////
SGFragment::SGFragment(const Fragment& frag) 
         : Fragment(frag), ChemData(), p_bondIndices(0)
{
  SO_NODE_CONSTRUCTOR(SGFragment);
  p_hasBioResidues = false;
  touchNumbers();
}

//===========================================================================
    SGFragment::~SGFragment()
//===========================================================================
{
  if (p_bondIndices) delete p_bondIndices;
}

SGFragment& SGFragment::operator=(const SGFragment& frag)
{
   (void)Fragment::operator=(frag);
   touchNumbers();
   return *this;
}

size_t SGFragment::numBonds() const
{
  return p_bondIndices->mnumBonds;
}

//===========================================================================
//===========================================================================
int32_t   SGFragment::getNumberOfAtoms() const
{
  return ((int32_t)p_atoms.size());
}

//===========================================================================
    int32_t   SGFragment::getNumberOfBonds() const
//===========================================================================
{
  return ((int32_t)p_bondIndices->mnumBonds);
}


//===========================================================================
//===========================================================================
short   SGFragment::getAtomicNumber(int32_t index) const
{
  return (p_atoms[index])->atomicNumber();
}



//===========================================================================
//===========================================================================
int32_t SGFragment::getAtomId(int32_t index) const
{ 
  return index; 
}



///////////////////////////////////////////////////////////////////////////
// Description
//   Virtual function from ChemBaseData.  The returned string is used
//   as the atom label.  Therefore, if the atom is a nub, we return the
//   empty string to reduce clutter.
//
//   There is a hack in here.  I was experiencing problems with order of
//   events when update messages arrived and couldn't get the data
//   structure updated in time within range of the number of atoms.  I
//   finally gave up and decided to make this code safe by returning ""
//   if the index is out of range.  I'm not too proud of this but I've
//   wasted enough time.  The problem was in cvchemsys when the builder
//   changed the subject by increasing the number of atoms.  Before the
//   user can notice, the labels will be re-rendered correctly.
///////////////////////////////////////////////////////////////////////////
SbString  SGFragment::getAtomName(int32_t index) const
{
   //if (index == 0 )
   //cout << "SGFragment::getAtomName " << index << " called" << endl ;
  static SbString name;
  //static char buf[5];
  //static DisplayStyle ds ;
  

  if (index < p_atoms.size()) {
    name = p_atoms[index]->displayLabel().c_str();
  }
  
  return name; 
}


//===========================================================================
//===========================================================================
int32_t SGFragment::getAtomIndex(int32_t index) const
{
#ifdef DEBUG
   cout << "**** getAtomIndex " << index << " = " << p_atoms[index]->atomicNumber() << endl ;
#endif
  return p_atoms[index]->atomicNumber();
}



//===========================================================================
//===========================================================================
SbVec3f  SGFragment::getAtomCoordinates(int32_t index) const
{

   SbVec3f vector;

   EE_RT_ASSERT(index<=p_atoms.size(), EE_WARNING, "index exceeds atoms");

   const double *coords = p_atoms[index]->coordinates();
   vector.setValue((float)coords[0], (float)coords[1], (float) coords[2]);

   /* if we cache coords... doesn't seem to affect perf much
   double *iter = &vcoords[index*3];
    vector.setValue((float)*iter++,(float)*iter++,(float)*iter++);
   */

   return  vector;

}



//=========================================================================== 
//===========================================================================
int32_t SGFragment::getBondFrom(int32_t index) const
{ 

  int32_t val;
  if (p_bondIndices != (IBondList*)0) {
    val = (*p_bondIndices).matomIndices[2*index];
  } else {
    val =  ((Fragment*)this)->atomIndex(*((*bonds())[index]->atom1()));
  }

  return val;
}




//=========================================================================== 
//=========================================================================== 
int32_t SGFragment::getBondTo(int32_t index) const
{ 
  if (p_bondIndices != (IBondList*)0) {
    return (*p_bondIndices).matomIndices[(2*index)+1];
  } else {
    return (int32_t) ((Fragment*)this)->atomIndex(*((*bonds())[index]->atom2()));
  }
}




//===========================================================================
//===========================================================================
int     SGFragment::getBondType(int32_t index) const
{
 double order;
 if (p_bondIndices != (IBondList*)0)
   order = (*p_bondIndices).mbondOrders[index];
 else
   order = (*bonds())[index]->order();
 if (order == 2.0)      return DOUBLE_BOND;
 else if (order == 3.0) return TRIPLE_BOND;
 else if (order == 1.5) return AROMATIC_BOND;
 else if (order < 1.0) return HYDROGEN_BOND;
 return SINGLE_BOND;
}


DisplayStyle
SGFragment::getBondStyle(int index) const
{
 DisplayStyle style;
 if (p_bondIndices != (IBondList*)0)
   style = (*p_bondIndices).mstyle[index];
 else
   style = (*bonds())[index]->displayStyle();
 return style;

}


//===========================================================================
   int32_t SGFragment::getBondIndex(int32_t index) const
//===========================================================================
{  return index;}


//===========================================================================
//===========================================================================
void SGFragment::touchNumbers()
{

  // We Should Really Re-Use the Object
  if (p_bondIndices != (IBondList*)0) delete p_bondIndices;
  p_bondIndices = bondIndices();

  touch();

  touchProteins();

    
}


/**
 * Recompute the protein attributes and remake the SbResidue objects.
 */
void SGFragment::touchProteins()
{
   clearProteinAttributes();
   generateProteinAttributes();

   makeSbResidues();
}

void SGFragment::doAction(SoAction *action)
{
    ChemBaseDataElement::set(action->getState(), this, this);
}

void SGFragment::GLRender(SoGLRenderAction *action)
{
    SGFragment::doAction(action);
}

void SGFragment::pick(SoPickAction *action)
{
    SGFragment::doAction(action);
}

void SGFragment::callback(SoCallbackAction *action)
{
     SGFragment::doAction(action);
}


/**
 * This call reproduces the getBoxDimensions functionality in
 * Fragment but is called by something in ChemData in the OpenMoiv
 * library so it is  being left in
 * Actually NOT USED - perhaps by the old MI
 */
void SGFragment::getBoundBox
    (float &xmin, float &ymin, float &zmin, 
     float &xmax, float &ymax, float &zmax)
{
   vector<bool> atombits;
   vector<bool> bondbits;
   bool selAll = m_atomHighLight.size() == 0;
   if (!selAll) getSelectionBits(atombits,bondbits);

   vector<TAtm*> *atomvec = atoms();
   int nAtoms = atomvec->size() ;

   if (nAtoms > 0 ) {

      // init min and max values
      const double* itcoords = (*atomvec)[0]->coordinates();
      xmin = xmax = itcoords[0] ;
      ymin = ymax = itcoords[1] ;
      zmin = zmax = itcoords[2] ;

      // calculate min and max values
      for (int idx=1; idx<nAtoms; idx++) {
         if (selAll || atombits[idx] == true) {
            TAtm *tatm = (*atomvec)[idx];
            const double *coords = tatm->coordinates() ;

            // x
            if (coords[0] > xmax) xmax = coords[0] ;
            if (coords[0] < xmin) xmin = coords[0] ;

            // y
            if (coords[1] > ymax) ymax = coords[1] ;
            if (coords[1] < ymin) ymin = coords[1] ;

            // z
            if (coords[2] > zmax) zmax = coords[2] ;
            if (coords[2] < zmin) zmin = coords[2] ;
         }
      }

   } else {

      xmin = xmax = 0 ;
      ymin = ymax = 0 ;
      zmin = zmax = 0 ;

   }
   delete atomvec;

}



void SGFragment::addAtom(const string& tag, const double* newCoords)
{
   //cout << "calling SGFragment addAtom1" << endl ;
   TAtm* newAtm = new TAtm( tag.c_str(), newCoords ) ;
   Fragment::addAtom( newAtm ) ;
   newAtm->displayStyle(p_mainDisplayStyle) ;
}

void SGFragment::addAtom(TAtm* atom)
{
   Fragment::addAtom(atom) ;
   atom->displayStyle(p_mainDisplayStyle);
}
void SGFragment::addAtom(TAtm* atom, TAtm *parent)
{
   Fragment::addAtom(atom,parent) ;
   atom->displayStyle(p_mainDisplayStyle);
}
/*
void SGFragment::addBond(TBond* newBond)
{
   newBond->displayStyle(p_mainDisplayStyle) ;
}
*/


void SGFragment::fixAllNubs(float scale, float scale_cpk) 
{
   vector<TAtm*> *atomvec = atoms();
   int nAtoms = atomvec->size() ;

   for (int idx=0; idx<nAtoms; idx++) 
   {
      TAtm *tatm = (*atomvec)[idx];
      if (tatm->atomicSymbol() != NUB_TAG) 
      {
         if (tatm->displayStyle().getStyle() == DisplayStyle::CPK) 
            fixAtomNubs((*atomvec)[idx],scale_cpk); //1.0
         else
            fixAtomNubs((*atomvec)[idx],scale); //.5
      }
   }

  delete atomvec;
}

//    Fix all nub positions so that the nub is sticking 1/2 way out.
void SGFragment::fixAtomNubs(TAtm *atm, float scale)
{
  TPerTab tpt;

  // This is not pretty, but this value must be kept in sync with
  // what is set in VRExpt. 
  float radii = 1.0;
  if (atm->displayStyle().getStyle() == DisplayStyle::CPK) {
    radii = tpt.vwr(atm->atomicNumber());
  } else {
    radii = tpt.covalentRadius(atm->atomicNumber());
  }

  const double *coords = atm->coordinates();

  vector<TAtm*> nubs;
  atm->connectedAtoms(nubs,NUB_TAG);

  TAtm *curAtm;
  float x,y,z;
  int nNubs = nubs.size();
  
  for (int idx=0; idx<nNubs; idx++) 
  {
     curAtm = nubs[idx];
     if (NUB_TAG == curAtm->atomicSymbol()) {

        const double *nubCoords = curAtm->coordinates();

        x = nubCoords[0] - coords[0];
        y = nubCoords[1] - coords[1];
        z = nubCoords[2] - coords[2];

        float factor = scale * (radii / sqrt( pow(x,2)+pow(y,2)+pow(z,2)));

        double newCoords[3];
        newCoords[0] = x * factor + coords[0];
        newCoords[1] = y * factor + coords[1];
        newCoords[2] = z * factor + coords[2];

        curAtm->coordinates(newCoords);
     }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   Fix a particular nub.  This method is similar to fixAtomNubs but is
//   presumably faster in situations where you know you only have one nub
//   to fix.
///////////////////////////////////////////////////////////////////////////////
void SGFragment::fixNub(TAtm *nub, float scale)
{
  TAtm *parent = nubParent(nub);

  if (parent) {
    TPerTab tpt;

    // This is not pretty, but this value must be kept in sync with
    // what is set in VRExpt. 
    float radii = 1.0;
    if (nub->displayStyle().getStyle() == DisplayStyle::CPK) {
      radii = tpt.vwr(parent->atomicNumber());
    } else {
      radii = tpt.covalentRadius(parent->atomicNumber());
    }

    double x,y,z;
    const double *nubCoords = nub->coordinates();
    const double *coords = parent->coordinates();

    x = nubCoords[0] - coords[0];
    y = nubCoords[1] - coords[1];
    z = nubCoords[2] - coords[2];

    float factor = scale * (radii / sqrt( pow(x,2)+pow(y,2)+pow(z,2)));
    double newCoords[3];
    newCoords[0] = x * factor + coords[0];
    newCoords[1] = y * factor + coords[1];
    newCoords[2] = z * factor + coords[2];

    nub->coordinates(newCoords);
  }

}
///////////////////////////////////////////////////////////////////////////////
// Description
//   This function is used to find determine whether an atom is part of a ring.
//   Eventually it will be used to determine how many rings an atom is part of
//   and what size the rings are.
///////////////////////////////////////////////////////////////////////////////
vector<int> SGFragment::findRingAtoms() const
{
  int natoms = numAtoms();
  vector<int> ret(natoms,0);
  ChemSSSR sssr((ChemBaseData*)this);
  int32_t numberOfRings, minRingSize, maxRingSize;
  int32_t maxRingLimit = 7;
  sssr.findRings(maxRingLimit, numberOfRings, minRingSize, maxRingSize);
      //cout << "found rings " << numberOfRings << endl;
  const RWBitVec *bitvec;
  bitvec = sssr.getRingBonds();
  if (bitvec != (RWBitVec*)0) {
    vector<TBond*> *bondvec = this->bonds();
    int nsize = bitvec->size();
    if (nsize == bondvec->size()) {
      int ibnd, iat1, iat2; 
      TBond *bond;
      for (ibnd=0; ibnd<nsize; ibnd++) {
        if (bitvec->testBit(ibnd)) {
          bond = (*bondvec)[ibnd];
          iat1 = bond->atom1()->index();
          iat2 = bond->atom2()->index();
          ret[iat1] = 1;
          ret[iat2] = 1;
        }
      }
    }
    delete bondvec;
  }
  return ret;
}

/**
 * Methods for getting selection based on lists of strings specified by
 * user. This is intended to drive command line selection.
 */
int SGFragment::getAtomIndexFromToken(string token)
{
  string atom_name;
  int atom_idx;
  string res_name;
  int res_pdb_idx;
  char res_chain;
  int res_idx;
  int natoms = numAtoms();
  vector<TAtm*> *latoms = atoms();
  TAtm::parseString(token, atom_name, atom_idx, res_name,
      res_pdb_idx, res_chain, res_idx); 
  Residue *res;
  int ret = -1;
  int idx;
  // Find index corresponding to first atom
  for (idx = 0; idx<natoms; idx++) {
    if (atom_idx >= 0) {
      if (idx == atom_idx) {
        ret = idx;
        break;
      }
    } else if (atom_name != "") {
      if (atom_name == (*latoms)[idx]->atomName()) {
        if (res_name != "" || res_pdb_idx >= 0 || res_chain != ' ' ||
            res_idx >= 0) {
          res = (*latoms)[idx]->getResidue();
          // Residue index is available so use that.
          if (res_idx >= 0) {
            if (res_idx == res->index()) {
              ret = idx;
              // Check to make sure everything else matches, if available
              if (res_pdb_idx >= 0 && res_pdb_idx != res->number()) {
                ret = -1;
              }
              if (res_name != "" && res_name != res->name()) {
                ret = -1;
              }
              if (res_chain != ' ' && res_chain != res->chain()) {
                ret = -1;
              }
              if (idx >= 0) {
                break;
              }
            }
          }
        } else if (res_pdb_idx >= 0) {
          // PDB residue number is available, so use that. Match on other
          // parameters if available.
          if (res_pdb_idx == res->number()) {
            ret = idx;
            if (res_name != "" && res_name != res->name()) {
              ret = -1;
            }
            if (res_chain != ' ' && res_chain != res->chain()) {
              ret = -1;
            }
            if (idx >= 0) {
              break;
            }
          }
        } else if (res_chain != ' ') {
          // match on first atom in chain
          if (res_chain == res->chain()) {
            ret = idx;
            if (res_name != "" && res_name != res->name()) {
              idx = -1;
            }
            if (idx >= 0) {
              break;
            }
          }
        } else if (res_name != "") {
          // match on first atom in residue
          ret = idx;
          break;
        }
      } else {
        // Only information to go on is atom name, so just select first
        // match and use that.
        if ((*latoms)[idx]->atomName() == atom_name) {
          ret = idx;
          break;
        }
      }
    }
  }
  return ret;
}



void SGFragment::getSelectionFromAtomList(vector<string> *list, 
                          vector<bool>& atombits, vector<bool>& bondbits)
{
   int nsize = list->size();
   if (nsize > 0) {
      int item, idx;
      bool hasResidues = numResidues() > 0;
      vector<TAtm*> *latoms = atoms();
      for (item=0; item<nsize; item++) {
//         cout<<"List[item] "<<(*list)[item]<<endl;
         StringTokenizer tokenizer((*list)[item], "-");
         vector<string> tokens = tokenizer.tokenize("-");
         int ntokens = tokens.size();
         int idx1 = -1;
         int idx2 = -1;
         if (ntokens < 2) {
//            cout<< "Token[0]: "<<tokens[0]<<endl;
            if (hasResidues) {
              idx1 = getAtomIndexFromToken(tokens[0]);
            } else {
               sscanf(tokens[0].c_str(),"%d",&idx1);
               idx1--; // Convert to internal indexing
            }
         } else {
            idx1 = getAtomIndexFromToken(tokens[0]);
            idx2 = getAtomIndexFromToken(tokens[1]);
//            cout<< "Token[0]: "<<tokens[0]<<endl;
//            cout<< "Token[1]: "<<tokens[1]<<endl;
         }
//         cout<<"Low atom index: "<<idx1<<endl;
//         cout<<"High atom index: "<<idx2<<endl;

         // add atoms and bonds to highlight vectors
         int last;
         if (idx2 >= 0) {
            last = idx2;
         } else {
            last = idx1;
         }
         if (idx1 >=0) {
            for (idx=idx1; idx<=last; idx++) {
               //cout << "setting some atom bits" << idx << endl;
               atombits[idx] = true;

               // add bond indices
               const vector<TBond*>& atomBonds = (*latoms)[idx]->bondList();
               int numBonds = atomBonds.size();
               for (int b =0; b <numBonds; b++) {
                  TBond *aBond = atomBonds[b];
                  bondbits[aBond->index()] = true;
               }
            }
         }
      }
   }
}

// This is basically a utility function that gets the atoms selected
// represented by an NWChem formatted string segment. It is called by
// the getSelectionFromNWChemString method.
void SGFragment::getAtomsFromNWChemString(string selection, vector<int> &atomsList)
{
  StringTokenizer tokenizer(selection, ":");
  vector<string> tokens = tokenizer.tokenize(":");
  // Parse string at : into residues and atoms
  if (tokens.size() == 2) {
    int i, j, k, n;
    int nres = numResidues();
    const vector<Residue*> &resvec = Fragment::residues();
    string residuelist = tokens[0];
    string atoms = tokens[1];
    // Create list of residue tokens
    StringTokenizer res_tokenizer(residuelist, ",");
    vector<string> res_tokens = res_tokenizer.tokenize(",");
    int nres_tok = res_tokens.size();
    // Create list of atom tokens
    StringTokenizer atm_tokenizer(atoms, ",");
    vector<string> atm_tokens = atm_tokenizer.tokenize(",");
    int natm_tok = atm_tokens.size();
    // Create list of flags to identify wild card strings
    vector<bool> isWild;
    for (i=0; i<natm_tok; i++) {
      isWild.push_back(false);
    }
    // Remove leading underscore from atom name and all white space
    for (i=0; i<natm_tok; i++) {
      for (j=0; j<atm_tokens[i].length(); j++) {
        if (atm_tokens[i][j] == '_') {
          atm_tokens[i][j] = ' ';
        }
      }
      STLUtil::stripLeadingAndTrailingWhiteSpace(atm_tokens[i]);
      // Check for wild card characters
      if (atm_tokens[i].size() > 4) {
        if (atm_tokens[i].compare(atm_tokens[i].size()-4,4,"????")==0) {
          isWild[i] = true;
        }
      }
    }
    // Get list of residue indices
    for (i = 0; i < nres_tok; i++) {
      StringTokenizer intrvl_tokenizer(res_tokens[i], "-");
      vector<string> intrvl = intrvl_tokenizer.tokenize("-");
      int ibeg, iend;
      bool lbeg, lend;
      if (intrvl.size() == 2) {
        lbeg = StringConverter::toInt(intrvl[0], ibeg);
        lend = StringConverter::toInt(intrvl[1], iend);
      } else if (intrvl.size() == 1) {
        lbeg = StringConverter::toInt(intrvl[0], ibeg);
        iend = ibeg;
        lend = lbeg;
      }
      if (lbeg && lend) {
        for (j=0; j<nres; j++) {
          if (resvec[j]->number() >= ibeg && resvec[j]->number() <= iend) {
            int ifirst = resvec[j]->firstAtomPtr()->index();
            int ilast;
            if (j<nres-1) {
              ilast = resvec[j+1]->firstAtomPtr()->index();
            } else {
              ilast = numAtoms();
            }
            for (k=ifirst; k<ilast; k++) {
              string name = p_atoms[k]->atomName();
              STLUtil::stripLeadingAndTrailingWhiteSpace(name);
              for (n=0; n<natm_tok; n++) {
                bool isMatch = false;
                if (!isWild[n]) {
                  if (name == atm_tokens[n]) {
                    isMatch = true;
                  }
                } else {
                  int len;
                  len = atm_tokens[n].size()-4;
                  if (name.substr(0,len) == atm_tokens[n].substr(0,len))
                    isMatch=true;
                }
                if ( isMatch ) {
                  atomsList.push_back(k);
                }
              }
            }
          }
        }
      }
    }
  }
}

// This function takes an NWChem formatted string specifying a selection
// of atoms based on residue and PDB atom name and converts it into a
// selection vector. This is specifically for use in the MD prepare module.

bool SGFragment::getSelectionFromNWChemString(string selection,
                 vector<bool>& atombits, vector<bool>& bondbits)
{
  bool ret = false;
  StringTokenizer tokenizer(selection, " ");
  vector<string> tokens = tokenizer.tokenize(" ");
  int i, j;
  vector<int> atoms;
  for (i=0; i<tokens.size(); i++) {
    atoms.clear();
    getAtomsFromNWChemString(tokens[i], atoms);
    if (atoms.size() > 0) {
      ret = true;
      for (j=0; j<atoms.size(); j++) {
        atombits[atoms[j]] = true;
        // add bond indices
        const vector<TBond*>& atomBonds = p_atoms[atoms[j]]->bondList();
        int numBonds = atomBonds.size();
        for (int b =0; b <numBonds; b++) {
          TBond *aBond = atomBonds[b];
          bondbits[aBond->index()] = true;
        }
      }
    }
  }
  return ret;
}


void SGFragment::getSelectionBits(
                 vector<bool>& atombits, vector<bool>& bondbits)
{
  atombits.clear();
  atombits.resize(numAtoms(), false);
  int nSelAtoms = m_atomHighLight.size();
  int idx;
  for (idx=0; idx<nSelAtoms; idx++) {
     atombits[m_atomHighLight[idx]] = true;
  }

  bondbits.clear();
  bondbits.resize(numBonds(), false);
  int nSelBonds = m_bondHighLight.size();
  for (idx=0; idx<nSelBonds; idx++) {
     bondbits[m_bondHighLight[idx]] = true;
  }

}

// Select all atoms of same element
void SGFragment::getSelectionFromElement(string element, vector<bool>& atombits,
            vector<bool>& bondbits)
{
  TPerTab pertab;
  int atomNumber = pertab.atomicNumber(element);
  int natoms = numAtoms();
  int i, j, nbond;
  for (i=0; i<natoms; i++) {
    if (atomRef(i)->atomicNumber() == atomNumber) {
      atombits[i] = true;
      const vector<TBond*> blist = atomRef(i)->bondList();
      nbond = blist.size();
      for (j=0; j<nbond; j++) {
        bondbits[blist[j]->index()] = true;
      }
    }
  }
}

// Select all atoms of same type
void SGFragment::getSelectionFromAtomType(string type, vector<bool>& atombits,
            vector<bool>& bondbits)
{
  int natoms = numAtoms();
  int i, j, nbond;
  STLUtil::stripLeadingAndTrailingWhiteSpace(type);
  TAtm *atm;
  string atype;
  for (i=0; i<natoms; i++) {
    atm = atomRef(i);
    atype = atm->atomType();
    STLUtil::stripLeadingAndTrailingWhiteSpace(atype);
    if (type == atype) {
      atombits[i] = true;
      const vector<TBond*> blist = atm->bondList();
      nbond = blist.size();
      for (j=0; j<nbond; j++) {
        bondbits[blist[j]->index()] = true;
      }
    }
  }
}

// Select all atoms with the same PDB atom name
void SGFragment::getSelectionFromAtomName(string name, vector<bool>& atombits,
            vector<bool>& bondbits)
{
  int natoms = numAtoms();
  int i, j, nbond;
  STLUtil::stripLeadingAndTrailingWhiteSpace(name);
  TAtm *atm;
  string aname;
  for (i=0; i<natoms; i++) {
    atm = atomRef(i);
    aname = atm->atomName();
    STLUtil::stripLeadingAndTrailingWhiteSpace(aname);
    if (name == aname) {
      atombits[i] = true;
      const vector<TBond*> blist = atm->bondList();
      nbond = blist.size();
      for (j=0; j<nbond; j++) {
        bondbits[blist[j]->index()] = true;
      }
    }
  }
}

// Select all atoms belonging to residues with same name as input
void SGFragment::getSelectionFromResidueName(string name, vector<bool>& atombits,
            vector<bool>& bondbits)
{
  int nres = Fragment::residues().size();
  if (nres > 0) {
    int i, j, nbond;
    STLUtil::stripLeadingAndTrailingWhiteSpace(name);
    int natoms = numAtoms();
    TAtm *atm;
    string aresname;
    for (i=0; i<natoms; i++) {
      atm = atomRef(i);
      aresname = atm->getResidue()->name();
      STLUtil::stripLeadingAndTrailingWhiteSpace(aresname);
      if (aresname == name) {
        atombits[i] = true;
        const vector<TBond*> blist = atm->bondList();
        nbond = blist.size();
        for (j=0; j<nbond; j++) {
          bondbits[blist[j]->index()] = true;
        }
      }
    }
  }
}

// Select all atoms belonging to solvent molecules
void SGFragment::getSolvent(vector<bool>& atombits, vector<bool>& bondbits)
{
   findFirstSolventAtomIndex();
   int ifirst = getFirstSolventAtomIndex();
   if (ifirst >= 0) {  // make sure there is solvent
      int ilast = numAtoms();
      int i, j, nbond;
      TAtm *atm;
      for (i=ifirst; i<ilast; i++) {
         atm = atomRef(i);
         atombits[i] = true;
         const vector<TBond*> blist = atm->bondList();
         nbond = blist.size();
         for (j=0; j<nbond; j++) {
            bondbits[blist[j]->index()] = true;
         }
      }
   }
}

// Select all atoms belonging to residue containing atm
void SGFragment::getFullResidue(TAtm *atm, vector<bool>& atombits, vector<bool>& bondbits)
{
  int nres = numResidues();
  if (nres > 0) {
    Residue *res = atm->getResidue();
    vector<TAtm*> atoms = findResidueAtoms(res);
    int natoms = atoms.size();
    int i, j, nbond;
    for (i=0; i<natoms; i++) {
      atombits[atoms[i]->index()] = true;
      const vector<TBond*> blist = atoms[i]->bondList();
      nbond = blist.size();
      for (j=0; j<nbond; j++) {
        bondbits[blist[j]->index()] = true;
      }
    }
  }
}

// This function takes a selection of atoms based and tries to convert it
// into an NWChem selection specification. This is specifically for use in
// the MD prepare module.

string SGFragment::getNWChemStringFromSelection()
{
  string ret;

  // We want to be able to assume that the high light vector is sorted in
  // ascending order
  sort(m_atomHighLight.begin(),m_atomHighLight.end());
  int natom = m_atomHighLight.size();
  bool stringOK = true;
  bool oneString = true;

  // Bail if no atoms selected
  if (natom == 0) stringOK = false;

  int i, j, iat, residx;

  bool firstResDone = false;
  vector<string> atomNames;
  vector<bool> inventory;
  vector<int> resList;

  // Set parameters for first atom
  if (stringOK) {
    iat = m_atomHighLight[0];
    residx = p_atoms[iat]->getResidue()->number();
    atomNames.push_back(p_atoms[iat]->atomName());
    resList.push_back(residx);
    inventory.push_back(true);
  }

  Residue *res;
  bool check = true;
  for (i=1; i<natom; i++) {
    iat = m_atomHighLight[i];
    res = p_atoms[iat]->getResidue();
    int newres = res->number();

    // Starting a new residue
    if (newres != residx || i == natom-1) {
      check = true;
      if (firstResDone) {
        // If a residue has already been completed, then
        // make sure that all atoms selected on current
        // residue match completed residue
        for (j=0; j<inventory.size(); j++) {
          if (!inventory[j]) check = false;
          inventory[j] = false;
        }
      }
      if (check && (i < natom-1 || (i==natom-1 && newres!=residx))) {
        resList.push_back(newres);
      } else if (!check) {
        oneString = false;
      }
      firstResDone = true;
      residx = newres;
    }

    if (firstResDone) {
      for (j=0; j<inventory.size(); j++) {
        if (p_atoms[iat]->atomName() == atomNames[j]) {
          inventory[j] = true;
        }
      }
    } else {
      inventory.push_back(true);
      atomNames.push_back(p_atoms[iat]->atomName());
    }
  }
  
  // form string
  if (oneString and stringOK) {
    // form residue list
    char buf[120];
    int nres = resList.size();
    int resBegin = resList[0];
    int resEnd = resList[0];
    int rcnt, icnt;
    int ncnt = 0;
    while (ncnt < nres) {
      rcnt = resBegin;
      resEnd = resBegin;
      icnt = ncnt;
      for (i=ncnt+1; i<nres; i++) {
        rcnt++;  
        if (rcnt == resList[i]) {
          resEnd = rcnt;
          icnt = i;
        } else {
          break;
        }
      }
      ncnt = icnt+1;
      if (resBegin == resEnd) {
        sprintf(buf,"%d",resBegin);
      } else {
        sprintf(buf,"%d-%d",resBegin,resEnd);
      }
      ret.append(buf);
      if (ncnt < nres) {
        ret.append(",");
      }
      if (ncnt < nres) {
        resBegin = resList[ncnt];
      }
    }
    ret.append(":");
    // form atom list
    for (i=0; i<atomNames.size(); i++) {
      STLUtil::stripLeadingAndTrailingWhiteSpace(atomNames[i]);
      if (isdigit(atomNames[i][0])) {
        sprintf(buf,"%s",atomNames[i].c_str());
      } else {
        sprintf(buf,"_%s",atomNames[i].c_str());
      }
      ret.append(buf);
      if (i<atomNames.size()-1) {
        ret.append(",");
      }
    }
  } else if (stringOK) {
    // Just list out single atoms and their residues
      char buf[120];
      int ires;
      string name;
    for (i=0; i<natom; i++) {
      iat = m_atomHighLight[i]; 
      name = p_atoms[iat]->atomName();
      ires = p_atoms[iat]->getResidue()->number();
      STLUtil::stripLeadingAndTrailingWhiteSpace(name);
      if (isdigit(name[0])) {
        sprintf(buf,"%d:%s",ires,name.c_str());
      } else {
        sprintf(buf,"%d:_%s",ires,name.c_str());
      }
      ret.append(buf);
      if (i<natom-1) {
      ret.append(" ");
      }
    }
  }
  return ret;
}


/**
 * Find the center of mass for the given NWChem selection string.
 * Returns false if the selection could not be retrieved given the sel string.
 */
bool SGFragment::getNWChemCenterOfMass(string sel,
                                       double &x, double &y, double &z)
{
  int natoms = numAtoms();
  int nbonds = numBonds();
  vector<bool> atombits(natoms,false);
  vector<bool> bondbits(nbonds,false);
  bool isSelectionGood = false;
  TAtm *atom;
  const double *coords;
  double mass = 0.0;
  double massTotal = 0.0;
  TPerTab pertab;

  isSelectionGood = getSelectionFromNWChemString(sel, atombits, bondbits);
  if (isSelectionGood) {
    x = 0.0;
    y = 0.0;
    z = 0.0;
    for (int i = 0; i < natoms; i++) {
      if (atombits[i]) {
        atom = atomRef(i);
        coords = atom->coordinates();
        mass = pertab.atomicWeight(atom->atomicNumber());
        x += mass*coords[0];
        y += mass*coords[1];
        z += mass*coords[2];
        massTotal += mass;
      }
    }
    if (massTotal > 0.0) {
      x /= massTotal;
      y /= massTotal;
      z /= massTotal;
    }
  }

  return isSelectionGood; 
}


/**
 * Get the distance between two groups of atoms specified using NWChem string 
 * selection specifications.
 */
double SGFragment::getNWChemPMFDistance(string sel1, string sel2)
{
  double rdist = 0.0;
  bool grp1, grp2;
  double x1,y1,z1;
  double x2,y2,z2;

  // find center of mass of selection 1
  grp1 = getNWChemCenterOfMass(sel1, x1, y1, z1);
  // find center of mass of selection 2
  grp2 = getNWChemCenterOfMass(sel2, x2, y2, z2);

  if (grp1 && grp2) {
    rdist = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2);
    rdist = sqrt(rdist);
  }
  return rdist;
}


/**
 * Get the angle between three groups of atoms specified using NWChem string 
 * selection specifications. Selection 2 is the center of the angle and value 
 * is returned in radians.
 */
double SGFragment::getNWChemPMFAngle(string sel1, string sel2, string sel3)
{
  double angle = 0.0;
  bool grp1, grp2, grp3;
  double x1,y1,z1;
  double x2,y2,z2;
  double x3,y3,z3;

  // find center of mass of selection 1
  grp1 = getNWChemCenterOfMass(sel1, x1, y1, z1);
  // find center of mass of selection 2
  grp2 = getNWChemCenterOfMass(sel2, x2, y2, z2);
  // find center of mass of selection 3
  grp3 = getNWChemCenterOfMass(sel3, x3, y3, z3);

  if (grp1 && grp2 && grp3) {
    double rn12, rn32, rdot;
    rn12 = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2);
    rn12 = sqrt(rn12);
    rn32 = (x3-x2)*(x3-x2) + (y3-y2)*(y3-y2) + (z3-z2)*(z3-z2);
    rn32 = sqrt(rn32);
    rdot = (x3-x2)*(x1-x2) + (y3-y2)*(y1-y2) + (z3-z2)*(z1-z2);
    angle = rdot/(rn12*rn32);
    angle = acos(angle);
  }
  return angle;
}

/**
 * Get the torsion angle between four groups of atoms specified using NWChem
 * string selection specifications. The torsion is formed from selections 1,
 * 2, 3, 4 in that order, with selections 2 and 3 forming the central bond
 * of the torsion. The return value is in radians.
 */
double SGFragment::getNWChemPMFTorsionAngle(string sel1, string sel2,
                                            string sel3, string sel4)
{
  double angle = 0.0;
  bool grp1, grp2, grp3, grp4;
  double x1,y1,z1;
  double x2,y2,z2;
  double x3,y3,z3;
  double x4,y4,z4;

  // find center of mass of selection 1
  grp1 = getNWChemCenterOfMass(sel1, x1, y1, z1);
  // find center of mass of selection 2
  grp2 = getNWChemCenterOfMass(sel2, x2, y2, z2);
  // find center of mass of selection 3
  grp3 = getNWChemCenterOfMass(sel3, x3, y3, z3);
  // find center of mass of selection 4
  grp4 = getNWChemCenterOfMass(sel4, x4, y4, z4);

  if (grp1 && grp2 && grp3 && grp4) {

    // evaluate cosine(angle);
    double r12[3],r32[3],r43[3];
    double r12p[3],r43p[3];
    int i;

    r12[0] = x1 - x2;
    r12[1] = y1 - y2;
    r12[2] = z1 - z2;
    r32[0] = x3 - x2;
    r32[1] = y3 - y2;
    r32[2] = z3 - z2;
    r43[0] = x4 - x3;
    r43[1] = y4 - y3;
    r43[2] = z4 - z3;

    double dot12_32 = 0.0;
    double dot43_32 = 0.0;
    double rn32i = 0.0;
    for (i=0; i<3; i++) {
      dot12_32 += r12[i]*r32[i];
      dot43_32 += r43[i]*r32[i];
      rn32i += r32[i]*r32[i];
    }
    rn32i = 1.0/rn32i;

    // evaluate components of end vectors perpendicular to
    // torsion bond
    for (i=0; i<3; i++) {
      r12p[i] = r12[i] - dot12_32*rn32i*r32[i];
      r43p[i] = r43[i] - dot43_32*rn32i*r32[i];
    }
    // evaluate lengths of perpendicular components of end
    // vectors and
    // calculate cos(Phi)
    double rn12p = 0.0;
    double rn43p = 0.0;
    double dot12_43 = 0.0;
    for (i=0; i<3; i++) {
      rn12p += r12p[i]*r12p[i];
      rn43p += r43p[i]*r43p[i];
      dot12_43 += r12p[i]*r43p[i];
    }
    rn12p = sqrt(rn12p);
    rn43p = sqrt(rn43p);

    // bail out and return zero if we run into a linear angle
    if (rn12p < 1.e-9 || rn43p < 1.e-9) return 0.0;

    angle = dot12_43/(rn12p*rn43p);
    angle = acos(angle);
  }

  return angle;
}

int SGFragment::getResidueIndexFromToken(string token)
{
  string res_name;
  int res_pdb_idx;
  char res_chain;
  int res_idx;
  int nres = numResidues();
  const vector<Residue*> &lres = Fragment::residues();
  Residue::parseString(token, res_name, res_pdb_idx, res_chain, res_idx); 
  int ret = -1;
  int idx;

  if (res_chain != ' ' && res_pdb_idx == -1 && res_idx == -1) {
    // Only chain and residue name specified
    ret = -3;
  } else if (res_name != "" && res_pdb_idx == -1 && res_idx == -1) {
    // Only residue name specified
    ret = -2;
  } else {
    // Find index corresponding to first atom
    for (idx = 0; idx<nres; idx++) {
      if (res_idx >= 0) {
        if (idx == res_idx) {
          ret = idx;
          break;
        }
      } else if (res_pdb_idx > 0) {
        // PDB residue number is available, so use that. Match on other
        // parameters if available.
        if (res_pdb_idx == lres[idx]->number()) {
          ret = idx;
          if (res_name != "" && res_name != lres[idx]->name()) {
            ret = -1;
          }
          if (res_chain != ' ' && res_chain != lres[idx]->chain()) {
            ret = -1;
          }
          if (idx >= 0) {
            break;
          }
        }
      }
    }
  }
  return ret;
}

void SGFragment::getSelectionFromResidueList(vector<string> *list,
                     vector<bool>& atombits, vector<bool>& bondbits)
{
  int nsize = list->size();
  int nres = numResidues();
  if (nsize > 0 && nres > 0) {
    int item, idx;
    vector<TAtm*> *latoms = atoms();
    int natoms = numAtoms();
    for (item=0; item<nsize; item++) {
      StringTokenizer tokenizer((*list)[item], "-");
      vector<string> tokens = tokenizer.tokenize("-");
      int ntokens = tokens.size();
      int idx1 = -1;
      int idx2 = -1;
      bool doLoop = true;
      if (ntokens < 2) {
        idx1 = getResidueIndexFromToken(tokens[0]);
        if (idx1 < -1) {
          doLoop = false;
        }
      } else {
        idx1 = getResidueIndexFromToken(tokens[0]);
        idx2 = getResidueIndexFromToken(tokens[1]);
      }

      if (idx1 >=0 && doLoop) {
        // add atoms and bonds to highlight vectors
        int first, last;
        Residue *res = residue(idx1);
        TAtm *atm;
        atm = res->firstAtomPtr();
        // Check if residue has any atoms
        if (atm != (TAtm*)0) {
          first = atm->index();
        } else {
          first = -1;
          int ires = idx1+1;
          while (atm == (TAtm*)0 && ires <= idx2) {
            atm = residue(ires)->firstAtomPtr();
            if (atm != (TAtm*)0) {
              first = atm->index();
              break;
            } else {
              ires++;
            }
          }
        }
        if (idx2 >= 0) {
          if (idx2 < nres-1) {
            res = residue(idx2+1);
            // Check if residue has any atoms
            atm = res->firstAtomPtr();
            if (atm != (TAtm*)0) {
              last = res->firstAtomPtr()->index();
            } else {
              last = natoms;
            }
          } else {
            last = natoms;
          }
        } else {
          if (idx1 < nres-1) {
            res = residue(idx1+1);
            atm = res->firstAtomPtr();
            // check if residue has any atoms
            if (atm != (TAtm*)0) {
              last = res->firstAtomPtr()->index();
            } else {
              last = natoms;
              int ires = idx1+1;
              while (atm == (TAtm*)0 && ires <= nres-1){
                atm = residue(ires)->firstAtomPtr();
                if (atm != (TAtm*)0) {
                  last = atm->index();
                  break;
                } else {
                  ires++;
                }
              }
            }
          } else {
            last = natoms;
          }
        }
        if (first >=0) {
          for (idx=first; idx<last; idx++) {
            atombits[idx] = true;

            // add bond indices
            const vector<TBond*>& atomBonds = (*latoms)[idx]->bondList();
            int numBonds = atomBonds.size();
            for (int b =0; b <numBonds; b++) {
              TBond *aBond = atomBonds[b];
              bondbits[aBond->index()] = true;
            }
          }
        }
      } else if (idx1 < -1 && !doLoop) {
        Residue *res;
        TAtm *atm;
        string res_name;
        int res_pdb_idx,res_idx,iat;
        char res_chain;
        Residue::parseString(tokens[0], res_name, res_pdb_idx, res_chain, res_idx); 
        for (idx = 0; idx < nres; idx++) {
          res = residue(idx);
          if ((res_name == res->name() && idx1 == -2) ||
              (res_chain == res->chain() && idx1 == -3)) {
            int first = -1;
            int last = -1;
            atm = res->firstAtomPtr();
            if (atm != (TAtm*)0) {
              first = atm->index();
            }
            if (idx < nres-1) {
              res = residue(idx+1);
              atm = res->firstAtomPtr();
              if (atm != (TAtm*)0) {
                last = atm->index();
              } else {
                last = natoms;
                int ires = idx+1;
                while (atm == (TAtm*)0 && ires <= nres-1){
                  atm = residue(ires)->firstAtomPtr();
                  if (atm != (TAtm*)0) {
                    last = atm->index();
                    break;
                  } else {
                    ires++;
                  }
                }
              }
            } else {
              last = natoms;
            }
            if (first >= 0) {
              for (iat = first; iat < last; iat++) {
                atombits[iat] = true;

                // add bond indices
                const vector<TBond*>& atomBonds = (*latoms)[iat]->bondList();
                int numBonds = atomBonds.size();
                for (int b =0; b <numBonds; b++) {
                  TBond *aBond = atomBonds[b];
                  bondbits[aBond->index()] = true;
                }
              }
            }
          }
        }
      }
    }
  }
}

/**
 * Translate all atoms by the vector x,y,z
 */
void SGFragment::translate(double x, double y, double z, bool all)
{
  if (m_atomHighLight.size() == 0 || all) {
    Fragment::translate(x,y,z);
  } else {
    int i, nsize = m_atomHighLight.size();
    TAtm *atom;
    for (i=0; i<nsize; i++) {
      atom = atomRef(m_atomHighLight[i]);
      const double *coords = atom->coordinates();
      atom->coordinates(coords[0]+x,coords[1]+y,coords[2]+z);
    }
  }
}

/**
 * Rotate all atoms by the angle around an axis specified by iaxis
 */
void SGFragment::rotate(int iaxis, double angle, bool all)
{
  double newcrd[3], r[3][3];
  // Maintain right-hand rule on all rotations
  if (iaxis == 1) {
    r[0][0] = cos(angle);
    r[0][1] = -sin(angle);
    r[0][2] = 0.0;
    r[1][0] = sin(angle);
    r[1][1] = cos(angle);
    r[1][2] = 0.0;
    r[2][0] = 0.0;
    r[2][1] = 0.0;
    r[2][2] = 1.0;
  } else if (iaxis == 2) {
    r[0][0] = cos(angle);
    r[0][1] = 0.0;
    r[0][2] = sin(angle);
    r[1][0] = 0.0;
    r[1][1] = 1.0;
    r[1][2] = 0.0;
    r[2][0] = -sin(angle);
    r[2][1] = 0.0;
    r[2][2] = cos(angle);
  } else if (iaxis == 3) {
    r[0][0] = 1.0;
    r[0][1] = 0.0;
    r[0][2] = 0.0;
    r[1][0] = 0.0;
    r[1][1] = cos(angle);
    r[1][2] = -sin(angle);
    r[2][0] = 0.0;
    r[2][1] = sin(angle);
    r[2][2] = cos(angle);
  }
  int i,j,k;
  if (m_atomHighLight.size() == 0 || all) {
    int natoms = numAtoms();
    for (i=0; i<natoms; i++) {
      const double *coords = p_atoms[i]->coordinates();
      for (j=0; j<3; j++) {
        newcrd[j] = 0;
        for (k=0; k<3; k++) {
          newcrd[j] += r[j][k]*coords[k];
        }
      }
      p_atoms[i]->coordinates(newcrd[0],newcrd[1],newcrd[2]);
    }
  } else {
    int nsize = m_atomHighLight.size();
    TAtm *atom;
    for (i=0; i<nsize; i++) {
      atom = atomRef(m_atomHighLight[i]);
      const double *coords = atom->coordinates();
      for (j=0; j<3; j++) {
        newcrd[j] = 0;
        for (k=0; k<3; k++) {
          newcrd[j] += r[j][k]*coords[k];
        }
      }
      atom->coordinates(newcrd[0],newcrd[1],newcrd[2]);
    }
  }
}

/**
 * Compute center of mass. Overwrites method in Fragment class.
 */
void SGFragment::centerOfMass(double& cmx, double& cmy, double& cmz, bool all)
{
  TPerTab pertab;
  int natoms = numAtoms();
  cmx = 0.0;
  cmy = 0.0;
  cmz = 0.0;
  double mtot = 0.0;
  const double *coords;
  double mass;
  TAtm* atom;
  if (m_atomHighLight.size() == 0 || all) {
    for (int i=0; i<natoms; i++) {
      atom = atomRef(i);
      if (atom->atomicNumber()>0) {
        mass =
          (double)pertab.atomicWeight(atom->atomicNumber());
        mtot += mass;
        coords =
          atom->coordinates();
        cmx += mass*coords[0];
        cmy +=
          mass*coords[1];
        cmz +=
          mass*coords[2];
      }
    }
  } else {
    int nsize = m_atomHighLight.size();
    for (int i=0; i<nsize; i++) {
      atom = atomRef(m_atomHighLight[i]);
      if (atom->atomicNumber()>0) {
        mass =
          (double)pertab.atomicWeight(atom->atomicNumber());
        mtot += mass;
        coords =
          atom->coordinates();
        cmx += mass*coords[0];
        cmy +=
          mass*coords[1];
        cmz +=
          mass*coords[2];
      }
    }
  }
  cmx = cmx/mtot;
  cmy = cmy/mtot;
  cmz = cmz/mtot;
}

/**
 * Get all atoms connected to theAtom,
 * This method currently returns the total number of fragments.
 * It superceedes the Fragment class methods to pass in the global
 * bit vec for performance reasons.
 */
bool SGFragment::getFragments(vector<bool>& globalatoms,
                             vector<bool>& vatoms, vector<bool>& vbonds,
                             const TAtm *theAtom,
                             const TAtm* lastAtom,
                             const TAtm* firstAtom) const
{
   bool foundCycle = false;
   const vector<TBond*>& bonds = theAtom->bondList();
   int cnt = bonds.size();
   TBond *theBond = (TBond*)0;
   for (int idx=0; idx<cnt; idx++) {
      theBond = bonds[idx];
      const TAtm* nextAtom;
      if ((theBond->atom1()) == theAtom) {
         nextAtom = theBond->atom2();
      } else {
         nextAtom = theBond->atom1();
      }
      if (nextAtom != lastAtom) {
         vbonds[theBond->index()] = true;
         if (nextAtom == firstAtom) {
            foundCycle = true;
            break;
         } else if (!(vatoms[nextAtom->index()])) {
            vatoms[nextAtom->index()] = true;
            globalatoms[nextAtom->index()] = true;
            if ((foundCycle=getFragments(globalatoms,vatoms,vbonds,nextAtom,theAtom,firstAtom)))
               break;
         }
      }
   }

   return foundCycle;

}


/**
 * Determine whether amino acids in residue are part of an Alpha helix are a
 * Beta sheet, based on the criteria defined in Kabsch and Sander, Biopolymers,
 * vol. 22, pp. 2577-2637 (1983)
 */
void SGFragment::generateProteinAttributes()
{
  const vector<Residue*>& resList = Fragment::residues();
  int nRes = resList.size();
  int i,j,k,i1,i2;

  p_hasBioResidues = false;
  if (nRes == 0) return;

  // Find Hydrogen bonds and evaluate whether N or O atom in amino acid is
  // hydrogen bonded to another amino acid
  vector<int> NHbond_B(nRes);
  vector<int> NHbond_T(nRes);
  vector<int> OHbond_B(nRes);
  vector<int> OHbond_T(nRes);
  vector<int> NHbondN_B(nRes);
  vector<int> NHbondN_T(nRes);
  vector<int> OHbondN_B(nRes);
  vector<int> OHbondN_T(nRes);
  for (i=0; i<nRes; i++) {
    NHbond_B[i] = -2;
    NHbond_T[i] = -2;
    OHbond_B[i] = -2;
    OHbond_T[i] = -2;
    NHbondN_B[i] = -2;
    NHbondN_T[i] = -2;
    OHbondN_B[i] = -2;
    OHbondN_T[i] = -2;
  }
  hash_map <unsigned long, int, hash<unsigned long>, equint> resMap(nRes);
  for (i=0; i<nRes; i++) {
    resMap[(unsigned long)resList[i]] = i;
  }
  TResTab restab;

  // Find amino acid residues that are hydrogen bonded to each other. Construct
  // list of both H bonds from N-H to 0=C and C=0 to H-N so that we can figure
  // out parallel and anti-parallel sequences.

  // Must have fragment and at least two atoms to start the processing
  if (numAtoms() > 1) {

    vector<TAtm*> *atoms = this->atoms();

    int nAtoms = atoms->size();

    vector<bool> atomList;

    atomList.resize(nAtoms, true);

    // _B and _T are cutoffs associated with bridges and turns, respectively.
    // The N refers to cutoffs on the O-N distance, as opposed to O-H distance.
    double maxR_B = 2.45;
    double maxR_T = 2.35;
    double maxNR_B = 3.30;
    double maxNR_T = 3.40;

    // Get a spatial partition of atoms (LinkCellList) 
    double maxC = maxR_B;
    if (maxR_T > maxC) maxC = maxR_T;
    if (maxNR_B > maxC) maxC = maxNR_B;
    if (maxNR_T > maxC) maxC = maxNR_T;
    LinkCellList cellList(atoms, maxC);

    maxR_B = maxR_B*maxR_B;
    maxR_T = maxR_T*maxR_T;
    maxNR_B = maxNR_B*maxNR_B;
    maxNR_T = maxNR_T*maxNR_T;
    double maxR = maxR_B;
    if (maxR_T > maxR) maxR = maxR_T;
    double maxNR = maxNR_B;
    if (maxNR_T > maxNR) maxNR = maxNR_T;
    const int *link_list = cellList.getLinkCellList();
    const int *header = cellList.getHeader();

    vector<int> cells;
    int cellIdx;
    int atomIdx;
    string atsym;
    MPoint p1, p2;
    int j;

    TAtm *atm1, *atm2;
    for (int i = 0; i < nAtoms; i++) {

      if ( !atomList[i]) continue;

      cells.clear();

      atm1 = (*atoms)[i];

      string atom1Name = atm1->atomName();

      if (!p_hasBioResidues && 
         restab.verify(atm1->getResidue()->name().c_str()) != TResItem::Other) {
        p_hasBioResidues = true;
      }

      // If atom is not the H hydrogen on an amino acid, skip to next atom.
      if ((atom1Name != " H  " && atom1Name != " N  ") ||
          !restab.verify(atm1->getResidue()->name().c_str())
          == TResItem::AminoAcid) continue;
      if (atom1Name == " H  ") {
        j = resMap[(unsigned long)(atm1->getResidue())];
        if (NHbond_B[j] < 0) NHbond_B[j] = -1;
        if (NHbond_T[j] < 0) NHbond_T[j] = -1;
      }

      // Get list of cells that contain "atm1" and border it
      cellList.getCells(cells, i);

      p1.xyz(atm1->coordinates());

      // Iterate over all cells in cell list
      for (j = 0; j < cells.size(); j++) {

        // Determine cell index
        cellIdx = cells[j];

        // Use cell index to get first atom from header array 
        atomIdx = header[cellIdx];

        // End of cell list is indicated by -1
        while (atomIdx >= 0) {

          // Only do check if atom 2 is O atom on an amino acid 
          atm2 = (*atoms)[atomIdx];
          string atom2Name = atm2->atomName();
          if (atom2Name == " O  " &&
              restab.verify(atm2->getResidue()->name().c_str())
              == TResItem::AminoAcid) {

            p2.xyz(atm2->coordinates());
            p2.subtract(p1);

            double r2 = p2.lengthSqr();

            if (atom1Name == " H  " && r2<=maxR) {
              bool doit = false;
              vector<TAtm*> *connected = atm1->connectedAtoms();
              // Mark residue so that it its possble to tell that it had an H hydrogen
              if (connected && connected->size() > 0) {
                TAtm *parent = (*connected)[0];
                double angle = fabs(atm1->angleBetween(parent,atm2));
                if (angle >= 120. && angle <= 240.) doit = true;
                delete connected;
              }
              if (doit) {
                i1 = resMap[(unsigned long)(atm1->getResidue())];
                i2 = resMap[(unsigned long)(atm2->getResidue())];
                if (i1 != i2 && r2 <= maxR_B) {
                  NHbond_B[i1] = i2;
                  OHbond_B[i2] = i1;
                }
                if (i1 != i2 && r2 <= maxR_T) {
                  NHbond_T[i1] = i2;
                  OHbond_T[i2] = i1;
                }
              }
            } else if (atom1Name == " N  " && r2<=maxNR) {
              i1 = resMap[(unsigned long)(atm1->getResidue())];
              i2 = resMap[(unsigned long)(atm2->getResidue())];
              // Consecutive residues will have an N and O atom close together.
              // Prevent this by making sure the residues are not consecutive.
              if (i1 != i2 && r2 <= maxNR_B && abs(i1-i2) > 2) {
                NHbondN_B[i1] = i2;
                OHbondN_B[i2] = i1;
              }
              if (i1 != i2 && r2 <= maxNR_T && abs(i1-i2) > 2) {
                NHbondN_T[i1] = i2;
                OHbondN_T[i2] = i1;
              }
            }
          }
          // Get next atom in cell
          atomIdx = link_list[atomIdx]; 
        }
      }
    }
    delete atoms;
  }

  // Find beta sheets. Start by marking anything that forms a hydrogen
  // bonded bridge as a SHEET.
  Residue* res;
  for (i=0; i<nRes; i++) {
    res = resList[i];
    if (restab.verify(res->name().c_str()) == TResItem::AminoAcid) {
      // Check for bridges
      k = OHbond_B[i];
      // Parallel bridge
      if (k >= 2 && NHbond_B[i] == k-2 && res->getProteinAttribute() == Residue::NONE) {
        res->setProteinAttribute(Residue::SHEET);
      }
      // Anti-parallel bridge
      if (k >= 0 && OHbond_B[k] == i && res->getProteinAttribute() == Residue::NONE) {
        res->setProteinAttribute(Residue::SHEET);
      }
      // If no hydrogens are on N, then use bonds between N and O.
      if (NHbond_B[i] == -2) {
        j = OHbondN_B[i];
        if (j >= 2 && NHbondN_B[i] == j-2 && res->getProteinAttribute() == Residue::NONE) {
          res->setProteinAttribute(Residue::SHEET);
        }
        if (j >= 0 && OHbondN_B[j] == i && res->getProteinAttribute() == Residue::NONE) {
          res->setProteinAttribute(Residue::SHEET);
        }
      }
    }
  }

  // Look for alpha helices. Start by finding 3 and 4 turns and
  // marking these as HELIX if they are not already labeled as SHEET.
  for (i=0; i<nRes; i++) {
    res = resList[i];
    if (restab.verify(res->name().c_str()) == TResItem::AminoAcid) {
      // Check for alpha helices
      k = OHbond_T[i];
      if (k >= 0) {
        if (k-i >=3 && k-i <= 4 && res->getProteinAttribute() != Residue::SHEET) {
          res->setProteinAttribute(Residue::HELIX);
        }
      } else if (NHbond_T[i] == -2) {
        j = OHbondN_T[i];
        if (j-i >=3 && j-i <= 4 && res->getProteinAttribute() != Residue::SHEET) {
          res->setProteinAttribute(Residue::HELIX);
        }
      }
    }
  }

  // Look for two consecutive turns to mark a HELIX
  bool condition1, condition2;
  Residue *res2;
  for (i=0; i<nRes; i++) {
    condition1 = true;
    condition2 = true;
    res = resList[i];
    if (res->getProteinAttribute() == Residue::HELIX) {
      if (i>0) {
        res2 = resList[i-1];
        if (res2->getProteinAttribute() == Residue::HELIX) {
          condition1 = false;
        }
      }
      if (i<nRes-1) {
        res2 = resList[i+1];
        if (res2->getProteinAttribute() == Residue::HELIX) {
          condition2 = false;
        }
      }
    } else {
      condition1 = false;
      condition2 = false;
    }
    if (condition1 && condition2) {
      res->setProteinAttribute(Residue::NONE);
    }
  }
  // now set all residues in a turn to be marked as HELIX
  vector<Residue::Attribute> oldAttr(nRes);
  for (i=0; i<nRes; i++) {
    oldAttr[i] = resList[i]->getProteinAttribute();
  }
  for (i=0; i<nRes; i++) {
    res = resList[i];
    if (oldAttr[i] == Residue::HELIX) {
      k = OHbond_T[i];
      if (k<0) k = OHbondN_B[i];
      for (j=i+1;j<=k;j++) {
        resList[j]->setProteinAttribute(Residue::HELIX);
      }
    }
  }
  // Look for two consecutive bridges to mark a SHEET
  for (i=0; i<nRes; i++) {
    condition1 = true;
    condition2 = true;
    res = resList[i];
    if (res->getProteinAttribute() == Residue::SHEET) {
      if (i>0) {
        res2 = resList[i-1];
        if (res2->getProteinAttribute() == Residue::SHEET) {
          condition1 = false;
        }
      }
      if (i<nRes-1) {
        res2 = resList[i+1];
        if (res2->getProteinAttribute() == Residue::SHEET) {
          condition2 = false;
        }
      }
    } else {
      condition1 = false;
      condition2 = false;
    }
    if (condition1 && condition2) {
      res->setProteinAttribute(Residue::NONE);
    }
  }
}

void SGFragment::clear()
{
   Fragment::clear();
   clearSelection();
}

void SGFragment::clearSelection()
{
   m_atomHighLight.clear();
   m_bondHighLight.clear();
}

/**
 * Clear all residue attributes
 */
void SGFragment::clearProteinAttributes()
{
  const vector<Residue*>& resList = Fragment::residues();
  int nRes = resList.size();
  for (int i=0; i<nRes; i++) {
    resList[i]->setProteinAttribute(Residue::NONE);
  }
}

bool SGFragment::hasBiologicalResidues()
{
  return p_hasBioResidues;
}

/**
 * Get a listing of all atoms in the backbone of an amino acid
 * chain. Specifically, find the C, CA, and N atoms in all
 * amino acids.
 */
void SGFragment::getBackboneAtoms(vector<bool>& vatoms, vector<bool>& vbonds)
{
  const vector<Residue*>& resList = Fragment::residues();
  int nRes = resList.size();
  int nAtom = Fragment::numAtoms();
  TAtm *atom, *atom2;
  if (nRes != 0) {
    vector<TAtm*> *atoms = this->atoms();
    int i, iat, ibond, nbond, ifirst, ilast;
    Residue *res;
    TResTab restab;
    std::set<string, less<string> > aa;
    aa.insert(" C  ");
    aa.insert(" CA ");
    aa.insert(" HA ");
    aa.insert("3HA ");
    aa.insert("2HA ");
    aa.insert(" O  ");
    aa.insert(" OXT");
    aa.insert(" H  ");
    aa.insert(" N  ");
    std::set<string, less<string> > dna;
    dna.insert(" P  ");
    dna.insert("3H5*");
    dna.insert("2H5*");
    dna.insert(" H4*");
    dna.insert(" H3*");
    dna.insert(" H3T");
    dna.insert("3H2*");
    dna.insert("2H2*");
    dna.insert("3HO*");
    dna.insert(" H1*");
    dna.insert(" O5*");
    dna.insert(" O4*");
    dna.insert(" O3*");
    dna.insert(" O2*");
    dna.insert(" O2P");
    dna.insert(" O1P");
    dna.insert(" C5*");
    dna.insert(" C4*");
    dna.insert(" C3*");
    dna.insert(" C2*");
    dna.insert(" C1*");
    std::set<string, less<string> >::iterator it_aa;
    std::set<string, less<string> >::iterator it_dna;
    for (i=0; i<nRes; i++) {
      res = resList[i];
      TResItem::Classification resType = restab.verify(res->name().c_str());
      if (resType != TResItem::Other) {
        ifirst = res->firstAtomPtr()->index();
        if (i+1 < nRes) {
          ilast = resList[i+1]->firstAtomPtr()->index();
        } else {
          ilast = nAtom;
        }
        for (iat=ifirst; iat<ilast; iat++) {
          atom = (*atoms)[iat];
          if (resType == TResItem::AminoAcid) {
            it_aa = aa.find(atom->atomName());
            it_dna = dna.end();
          } else if (resType != TResItem::Other) {
            it_dna = dna.find(atom->atomName());
            it_aa = aa.end();
          }
          if (it_aa != aa.end() || it_dna != dna.end()) {
            vatoms[atom->index()] = true;
            const vector<TBond*>& bonds = atom->bondList();
            nbond = bonds.size();
            for (ibond=0; ibond < nbond; ibond++) {
              atom2 = bonds[ibond]->atom1();
              if (atom2 == atom) {
                atom2 = bonds[ibond]->atom2();
              }
              if (resType == TResItem::AminoAcid) {
                it_aa = aa.find(atom->atomName());
                it_dna = dna.end();
              } else if (resType != TResItem::Other) {
                it_dna = dna.find(atom->atomName());
                it_aa = aa.end();
              }
              if (it_aa != aa.end() || it_dna != dna.end()) {
                vbonds[bonds[ibond]->index()] = true;
              }
            }
          }
        }
      }
    }
    delete atoms;
  }
}


void SGFragment::getSelectionFromResidueStatus(const string& status,
          vector<bool>& atombits, vector<bool>& bondbits)
{
   Residue::ResidueStatus statuscode = Residue::statusFromString(status);

   const vector<Residue*>& resList = Fragment::residues();
   Residue *res;
   vector<int> atomindices;
   for (int idx=0; idx<resList.size(); idx++) {
      res = residue(idx);
      if (res->residueStatus() == statuscode) {
         atomindices = findResidueAtomIndices(idx);
         for (int i=0; i<atomindices.size(); i++) {
            atombits[atomindices[i]] = true;

            // Add connected bonds
            const vector<TBond*>& bonds = atomRef(atomindices[i])->bondList();
            int nbond = bonds.size();
            for (int ibond=0; ibond < nbond; ibond++) {
               bondbits[bonds[ibond]->index()] = true;
            }
         }

      }
   }
}


/**
 * Get list of atoms and bonds represented by the specified chain ids.
 */
void SGFragment::getChainAtoms(const vector<string>& chains, 
                               vector<bool>& vatoms, vector<bool>& vbonds)
{
  const vector<Residue*>& resList = Fragment::residues();
  Residue *res;
  vector<int> atomindices;
  for (int cdx=0; cdx<chains.size(); cdx++) {
     char chain = chains[cdx][0];
     for (int idx=0; idx<resList.size(); idx++) {
        res = residue(idx);
        if (res->chain() == chain) {
           atomindices = findResidueAtomIndices(idx);
           for (int i=0; i<atomindices.size(); i++) {
              vatoms[atomindices[i]] = true;

              // Add connected bonds
              const vector<TBond*>& bonds = atomRef(atomindices[i])->bondList();
              int nbond = bonds.size();
              for (int ibond=0; ibond < nbond; ibond++) {
                 vbonds[bonds[ibond]->index()] = true;
              }
           }

        }
     }
  }
}

/**
 * Get a listing of all atoms in the side chains of an amino
 * acid chain. Specifically, find all atoms that are not C, CA,
 * or N in all amino acids.
 */
void SGFragment::getSideChainAtoms(vector<bool>& vatoms, vector<bool>& vbonds)
{
  const vector<Residue*>& resList = Fragment::residues();
  int nRes = resList.size();
  int nAtom = Fragment::numAtoms();
  TAtm *atom, *atom2;
  if (nRes != 0) {
    vector<TAtm*> *atoms = this->atoms();
    int i, iat, ibond, nbond, ifirst, ilast;
    Residue *res;
    TResTab restab;
    std::set<string, less<string> > aa;
    aa.insert(" C  ");
    aa.insert(" CA ");
    aa.insert(" HA ");
    aa.insert("3HA ");
    aa.insert("2HA ");
    aa.insert(" O  ");
    aa.insert(" OXT");
    aa.insert(" H  ");
    aa.insert(" N  ");
    std::set<string, less<string> > dna;
    dna.insert(" P  ");
    dna.insert("3H5*");
    dna.insert("2H5*");
    dna.insert(" H4*");
    dna.insert(" H3*");
    dna.insert(" H3T");
    dna.insert("3H2*");
    dna.insert("2H2*");
    dna.insert("3HO*");
    dna.insert(" H1*");
    dna.insert(" O5*");
    dna.insert(" O4*");
    dna.insert(" O3*");
    dna.insert(" O2*");
    dna.insert(" O2P");
    dna.insert(" O1P");
    dna.insert(" C5*");
    dna.insert(" C4*");
    dna.insert(" C3*");
    dna.insert(" C2*");
    dna.insert(" C1*");
    std::set<string, less<string> >::iterator it_aa;
    std::set<string, less<string> >::iterator it_dna;
    for (i=0; i<nRes; i++) {
      res = resList[i];
      TResItem::Classification resType = restab.verify(res->name().c_str());
      if (resType != TResItem::Other) {
        ifirst = res->firstAtomPtr()->index();
        if (i+1 < nRes) {
          ilast = resList[i+1]->firstAtomPtr()->index();
        } else {
          ilast = nAtom;
        }
        for (iat=ifirst; iat<ilast; iat++) {
          atom = (*atoms)[iat];
          if (resType == TResItem::AminoAcid) {
            it_aa = aa.find(atom->atomName());
            it_dna = dna.end();
          } else if (resType != TResItem::Other) {
            it_dna = dna.find(atom->atomName());
            it_aa = aa.end();
          }
          if ( it_aa == aa.end() &&  it_dna == dna.end()) {
            vatoms[atom->index()] = true;
            const vector<TBond*>& bonds = atom->bondList();
            nbond = bonds.size();
            for (ibond=0; ibond < nbond; ibond++) {
              atom2 = bonds[ibond]->atom1();
              if (atom2 == atom) {
                atom2 = bonds[ibond]->atom2();
              }
              if ( it_aa == aa.end() &&  it_dna == dna.end()) {
                vbonds[bonds[ibond]->index()] = true;
              }
            }
          }
        }
      }
    }
    delete atoms;
  }
}

/**
 * Determine if two selected atoms are on the same molecule (i.e. connected
 * to each other through a network of bonds)
 */
bool SGFragment::onSameMolecule()
{
  bool ret = true;
  int nAtoms = numAtoms();
  int nBonds = numBonds();
  vector<bool> atombits(nAtoms, false);
  vector<bool> bondbits(nBonds, false);
  if (m_atomHighLight.size() == 2) {
    int idx2;
    TAtm *atm1 = atomRef(m_atomHighLight[0]);
    getConnected(atombits,bondbits,atm1,0,0);
    idx2 = m_atomHighLight[1];
    if (atombits[idx2]) {
      ret = true;
    } else {
      ret = false;
    }
  }
  return ret;
}


/**
 * Makes SbResidues from our Residue objects for display in the scene
 * graph.  
 */
void SGFragment::makeSbResidues()
{
//#define timertest
#ifdef timertest
   ETimer t;
   t.start();
#endif

   // Reset to no residues
   ChemData *moiv = (ChemData*)this;
   moiv->residues.setNum(0);
   moiv->residueName.setNum(0);

   Fragment *efrag = (Fragment*)this;
   int numr = efrag->numResidues();

   // Build these up before sending to inventor for performance reasons.
   // See documentation on SoMField
   // ie need to use setValues instead of set1Value
   SbResidue *fastResidues = new SbResidue[numr];
   SbString   *fastNames = new SbString[numr];

   const vector<Residue*>& eres = efrag->residues();
   int xnumResidues = 0;
   int oldAttr = SbResidue::RESIDUE_TYPE_COIL;
   int chainLength = 0;
   for (int idx=0; idx<numr; idx++) {

      vector<TAtm*> ratoms = efrag->findResidueAtoms(eres[idx]);
      int ca = -1;
      int p = -1;
      int o = -1;
      int o1p = -1;
      int resAttr = SbResidue::RESIDUE_TYPE_COIL;
      if (eres[idx]->getProteinAttribute() == Residue::HELIX) {
         resAttr = SbResidue::RESIDUE_TYPE_HELIX;
         //bjp         cout<<"Residue "<<idx+1<<" is HELIX"<<endl;
      } else if (eres[idx]->getProteinAttribute() == Residue::SHEET) {
         resAttr = SbResidue::RESIDUE_TYPE_SHEET;
         //bjp         cout<<"Residue "<<idx+1<<" is SHEET"<<endl;
      } else {
         //bjp           cout<<"Residue "<<idx+1<<" is COIL"<<endl;
      }
      if (oldAttr != resAttr) {
         //bjp           cout <<"Chain end at "<<idx+1<<" old: "<<oldAttr<<" new: "
         //bjp                <<resAttr<<" chain length: "<<chainLength<<endl;
         chainLength=0;
         oldAttr = resAttr;
      } else {
         chainLength++;
      }
      for (int adx=0; adx<ratoms.size(); adx++) {
         TAtm *atm = ratoms[adx];
         string name = atm->atomName();
         STLUtil::stripLeadingAndTrailingWhiteSpace(name);
         //cout << "name " << name << "!" << endl;
         if (name == "CA") {
            // OpenMoiv looks for CA or C3* ???
            ca = atm->index();
         } else if (name == "P") {
            p = atm->index();
         } else if (name == "O") {
            // OpenMoiv looks for O or C4* ???
            o = atm->index();
         } else if (name == "O1P") {
            o1p = atm->index();
         }
      }

      // Create SbResiude
      fastNames[xnumResidues] = SbString(eres[idx]->name().c_str());
      SbResidue residue;
      residue.setIndex(xnumResidues);
      residue.setNumber(xnumResidues);
      residue.setChain(eres[idx]->chain());
      if (resAttr >= 0) {
         residue.residueType.setValue(resAttr);
      }

      // Set up control points for cartoon (if found)
      if ((o > 0 || o1p > 0) && (ca > 0 || p > 0)) {
         if (o  == -1) o = o1p;
         if (ca  == -1) ca = p;

         //cout << "ca/o " << ca << "/" << o << endl;
         residue.addAtomIndex(ca, SbResidue::RESIDUE_CONTROL_POINT_CA);
         residue.addAtomIndex(o, SbResidue::RESIDUE_CONTROL_POINT_O);
         for (int adx=0; adx<ratoms.size(); adx++) {
            TAtm *atm = ratoms[adx];
            int index = atm->index();
            if (index != ca && index != o) {
               residue.addAtomIndex(index);
            }
         }

      } else {
         for (int adx=0; adx<ratoms.size(); adx++) {
            residue.addAtomIndex(ratoms[adx]->index());
         }
      }
      fastResidues[xnumResidues] = residue;
      xnumResidues++;
   }
   moiv->numberOfResidues.setValue(numr);
   moiv->residues.setValues(0,numr,fastResidues);

   moiv->residueName.setValue(numr);
   moiv->residueName.setValues(0, numr, fastNames);
   delete [] fastResidues;
   delete [] fastNames;
#ifdef timertest
   t.stop();
   cout << "time to make residues " << t.elapsedTime() << endl;
#endif

}



void SGFragment::addFragment(const string& type,
      const string& memstream, const string& filename,
      const string& units, int conatom,
      double x, double y, double z,
      bool genBondOrders,
      int modelNum, const string& altLoc, const string& chainId)
throw(EcceException)
{
   double mult = 1.0;
   if (units == "Bohr") mult = 0.529177;
   if (units == "Picometers") mult = 0.01;
   if (units == "Nanometers") mult = 10.0;

   SGFragment tmpfrag;
   tmpfrag.setMainDisplayStyle(getMainDisplayStyle());

   FragUtil fragutil(&tmpfrag);

   bool loadStatus = false;
   if (filename != "") { // load from file
      SFile sfile(filename.c_str());
      if (!sfile.exists()) throw EcceException("Unable to open " + filename, WHERE);
      ifstream data(filename.c_str());
      if (data) {
         loadStatus = fragutil.load(data,type,mult,genBondOrders,
               modelNum, altLoc, chainId);

         tmpfrag.name(sfile.filename());
         if (numAtoms() != 0) {
            name("");
         }
         data.close();
      }
   } else {   // in memory stream
      istrstream data(memstream.c_str());
      if (data) {
         loadStatus = fragutil.load(data,type,mult,genBondOrders);
      }
   }


   if (!loadStatus) throw EcceException("Unable to parse input data", WHERE);

   // Not sure if this is needed.  Definitely not needed.
   tmpfrag.touchNumbers();

   // if there was already a lattice, leave it.  Otherwise take from
   // new system if exists
   LatticeDef *lattice1 = getLattice();
   LatticeDef *lattice2 = tmpfrag.getLattice();
   if (lattice1 == 0 && lattice2) {
      // Force copy to be made
      setLattice(new LatticeDef(*lattice2));
   }

   int fragconatom = -1;
   if (m_atomHighLight.size() > 0) {
      fragconatom = m_atomHighLight[0];
   }

   TAtm *mainNub, *mainAtom, *fragAtom, *fragNub;
   if (fragconatom==-1 || bondingAtoms(tmpfrag, conatom,fragconatom,
            &mainNub,&mainAtom, &fragAtom, &fragNub)) {
      mergeFragment(tmpfrag, conatom, fragconatom, x, y, z);
      // Is this really needed????
      for (int idx=tmpfrag.numAtoms()-1; idx>=0; idx--) {
         tmpfrag.removeAtom(idx);
      }
   } else {
      throw EcceException("Cannot unambigously connect fragments. Please select the sites that should be bonded.",WHERE);
   }

}

void SGFragment::generateDNA(string &sequence)
  throw(EcceException)
{
  EDSIServerCentral sc;

  //
  // Some hardwired but relative paths.
  //
  string bpbegin = "/DNA_Bases/Base_Pairs_Termini";
  string bp3end = "/DNA_Bases/3_Terminus";
  string bp5end = "/DNA_Bases/5_Terminus";
  string bpinterior = "/DNA_Bases/Interior";

  // Multiplier for units. Since they are Angstroms, just set to 1.0
  double mult = 1.0;

  if (numAtoms() > 0) {
      throw EcceException("Fragment must be empty before DNA "
                          "can be generated", WHERE);
  }
  if (sequence.size()<2) {
      throw EcceException("DNA sequence must contain at least two base "
                          "pairs", WHERE);
  }
  int idx, nsize = sequence.size();
  string sequence2;
  // Create complementary part of DNA sequence
  for (idx=0; idx<nsize; idx++) {
    if (sequence[idx] == 'A') {
      sequence2.append(1,'T');
    } else if (sequence[idx] == 'T') {
      sequence2.append(1,'A');
    } else if (sequence[idx] == 'C') {
      sequence2.append(1,'G');
    } else if (sequence[idx] == 'G') {
      sequence2.append(1,'C');
    } else {
      throw EcceException("Illegal DNA base specified", WHERE);
    }
  }
  // Do first base pair
  string path = sc.getDefaultStructureLibrary().toString() + bpbegin;
  path += "/";
  path.append(1,sequence[0]);
  path += "_5-";
  path.append(1,sequence2[0]);
  path += "_3.mvm";
  EDSI *edsi = EDSIFactory::getEDSI(EcceURL(path));
  istream *data = edsi->getDataSet();
  restoreMVM(*data, mult, true);
  delete edsi;
  delete data;

  // Modify residue numbers and chain IDs so that it all works out
  vector<Residue*> resList = ((Fragment*)this)->residues();
  if (resList.size() == 2) {
    resList[0]->number(1);
    resList[0]->chain('A');
    resList[1]->number(nsize);
    resList[1]->chain('B');
  }
  
  // Create left chain
  for (idx=1; idx<nsize; idx++) {
    if (idx<nsize-1) {
      path = sc.getDefaultStructureLibrary().toString() + bpinterior;
    } else {
      path = sc.getDefaultStructureLibrary().toString() + bp3end;
    }
    if (sequence[idx] == 'A') {
      path += "/Deoxyadenosine.mvm";
    } else if (sequence[idx] == 'T') {
      path += "/Deoxythymidine.mvm";
    } else if (sequence[idx] == 'G') {
      path += "/Deoxyguanosine.mvm";
    } else if (sequence[idx] == 'C') {
      path += "/Deoxycytidine.mvm";
    } else  {
      throw EcceException("Unexpected base",WHERE);
    }

    EDSI *newedsi = EDSIFactory::getEDSI(EcceURL(path));
    istream *newdata = newedsi->getDataSet();
    SGFragment tmpfrag;
    bool loadStatus;
    loadStatus = tmpfrag.restoreMVM(*newdata, mult, true);
    delete newedsi;
    delete newdata;
    
    // Look for O3* atom and then find the nub attached to it
    Residue *res = residue(idx-1);
    vector<TAtm*> atomList = findResidueAtoms(res);
    int i, iatm, natoms;
    iatm = -1;
    TAtm* atom;
    natoms = atomList.size();
    for (i=0; i<natoms; i++) {
      atom = atomList[i];
      if (atom->atomName() == " O3*") {
        iatm = i;
        break;
      }
    }
    atom = atomList[iatm];
    // get attached nub
    vector<TAtm*>* alist = atom->connectedAtoms();
    natoms = alist->size();
    for (i=0; i<natoms; i++) {
      atom = (*alist)[i];
      if (atom->atomicSymbol() == "Nub") {
        iatm = atom->index();
        break;
      }
    }
    delete alist;
    if (iatm == -1) {
      string errmsg = "No bonding nubs found.  Check structures in ";
      errmsg += "Structure Library";
      throw EcceException(errmsg, WHERE);
    }
    // Search tmpfrag for P atom and then find nub attached to it
    res = tmpfrag.residue(0);
    atomList = tmpfrag.findResidueAtoms(res);
    int jatm = -1;
    natoms = atomList.size();
    for (i=0; i<natoms; i++) {
      atom = atomList[i];
      if (atom->atomName() == " P  ") {
        jatm = i;
        break;
      }
    }
    atom = atomList[jatm];
    // get attached nub
    alist = atom->connectedAtoms();
    natoms = alist->size();
    for (i=0; i<natoms; i++) {
      atom = (*alist)[i];
      if (atom->atomicSymbol() == "Nub") {
        jatm = atom->index();
        break;
      }
    }
    delete alist;
    if (jatm == -1) {
      string errmsg = "No bonding nubs found.  Check structures in ";
      errmsg += "Structure Library";
      throw EcceException(errmsg, WHERE);
    }
    // Merge new base to existing structure
    editResidueMode(false,-1);
    mergeFragment(tmpfrag, jatm, iatm, 0.0, 0.0, 0.0);

    if (loadStatus) {
      // Is this really needed????
      for (int idx=tmpfrag.numAtoms()-1; idx>=0; idx--) {
        tmpfrag.removeAtom(idx);
      }

    } else {
      throw EcceException("Unable to parse input data", WHERE);
    }
  }

  // Create right chain
  for (idx=1; idx<nsize; idx++) {
    if (idx<nsize-1) {
      path = sc.getDefaultStructureLibrary().toString() + bpinterior;
    } else {
      path = sc.getDefaultStructureLibrary().toString() + bp5end;
    }
    if (sequence2[idx] == 'A') {
      path += "/Deoxyadenosine.mvm";
    } else if (sequence2[idx] == 'T') {
      path += "/Deoxythymidine.mvm";
    } else if (sequence2[idx] == 'G') {
      path += "/Deoxyguanosine.mvm";
    } else if (sequence2[idx] == 'C') {
      path += "/Deoxycytidine.mvm";
    } else  {
      throw EcceException("Unexpected base",WHERE);
    }

    EDSI *newedsi = EDSIFactory::getEDSI(EcceURL(path));
    istream *newdata = newedsi->getDataSet();
    SGFragment tmpfrag;
    bool loadStatus;
    loadStatus = tmpfrag.restoreMVM(*newdata, mult, true);
    delete newedsi;
    delete newdata;
    
    // Look for P atom and then find the nub attached to it
    Residue *res = residue(nsize);
    vector<TAtm*> atomList = findResidueAtoms(res);
    int i, iatm, natoms;
    iatm = -1;
    TAtm* atom;
    natoms = atomList.size();
    for (i=0; i<natoms; i++) {
      atom = atomList[i];
      if (atom->atomName() == " P  ") {
        iatm = i;
        break;
      }
    }
    atom = atomList[iatm];
    // get attached nub
    vector<TAtm*>* alist = atom->connectedAtoms();
    natoms = alist->size();
    for (i=0; i<natoms; i++) {
      atom = (*alist)[i];
      if (atom->atomicSymbol() == "Nub") {
        iatm = atom->index();
        break;
      }
    }
    delete alist;
    if (iatm == -1) {
      string errmsg = "No bonding nubs found.  Check structures in ";
      errmsg += "Structure Library";
      throw EcceException(errmsg, WHERE);
    }
    // Search tmpfrag for P atom and then find nub attached to it
    res = tmpfrag.residue(0);
    atomList = tmpfrag.findResidueAtoms(res);
    int jatm = -1;
    natoms = atomList.size();
    for (i=0; i<natoms; i++) {
      atom = atomList[i];
      if (atom->atomName() == " O3*") {
        jatm = i;
        break;
      }
    }
    atom = atomList[jatm];
    // get attached nub
    alist = atom->connectedAtoms();
    natoms = alist->size();
    for (i=0; i<natoms; i++) {
      atom = (*alist)[i];
      if (atom->atomicSymbol() == "Nub") {
        jatm = atom->index();
        break;
      }
    }
    delete alist;
    if (jatm == -1) {
      string errmsg = "No bonding nubs found.  Check structures in ";
      errmsg += "Structure Library";
      throw EcceException(errmsg, WHERE);
    }
    // Merge new base to existing structure
    mergeFragment(tmpfrag, jatm, iatm, 0.0, 0.0, 0.0);

    if (loadStatus) {
      // Is this really needed????
      for (int idx=tmpfrag.numAtoms()-1; idx>=0; idx--) {
        tmpfrag.removeAtom(idx);
      }

    } else {
      throw EcceException("Unable to parse input data", WHERE);
    }
  }
  reIndexResidues();
  recenterFrag();
}

void SGFragment::generatePeptide(string &sequence)
  throw(EcceException)
{
  //
  // Validate sequence and verify that it can be translated into
  // a viable peptide chain
  //

  STLUtil::stripLeadingAndTrailingWhiteSpace(sequence);
  STLUtil::toUpper(sequence);
  StringTokenizer tokenizer(sequence,"-");
  vector<string> bases = tokenizer.tokenize("-");

  // Create map of residue names
  map<string,string> aa_map;
  aa_map.insert(pair<string,string>("ALA","/Alanine.mvm"));
  aa_map.insert(pair<string,string>("ARG","/Arginine.mvm"));
  aa_map.insert(pair<string,string>("ASN","/Asparagine.mvm"));
  aa_map.insert(pair<string,string>("ASP","/Aspartate.mvm"));
  aa_map.insert(pair<string,string>("CYS","/Cysteine.mvm"));
  aa_map.insert(pair<string,string>("CYX","/Cysteine-X.mvm"));
  aa_map.insert(pair<string,string>("GLN","/Glutamine.mvm"));
  aa_map.insert(pair<string,string>("GLU","/Glutamate.mvm"));
  aa_map.insert(pair<string,string>("GLY","/Glycine.mvm"));
  aa_map.insert(pair<string,string>("HID","/Histidine-D.mvm"));
  aa_map.insert(pair<string,string>("HIE","/Histidine-E.mvm"));
  aa_map.insert(pair<string,string>("HIP","/Histidine-P.mvm"));
  aa_map.insert(pair<string,string>("HIS","/Histidine-E.mvm"));
  aa_map.insert(pair<string,string>("ILE","/Isoleucine.mvm"));
  aa_map.insert(pair<string,string>("LEU","/Leucine.mvm"));
  aa_map.insert(pair<string,string>("LYS","/Lysine.mvm"));
  aa_map.insert(pair<string,string>("MET","/Methionine.mvm"));
  aa_map.insert(pair<string,string>("PHE","/Phenylalanine.mvm"));
  aa_map.insert(pair<string,string>("PRO","/Proline.mvm"));
  aa_map.insert(pair<string,string>("SER","/Serine.mvm"));
  aa_map.insert(pair<string,string>("THR","/Threonine.mvm"));
  aa_map.insert(pair<string,string>("TRP","/Tryptophan.mvm"));
  aa_map.insert(pair<string,string>("TYR","/Tyrosine.mvm"));
  aa_map.insert(pair<string,string>("VAL","/Valine.mvm"));

  // Create second map of reside names with single character symbols
  map<char,string> ch_map;
  ch_map.insert(pair<char,string>('A',"ALA"));
  ch_map.insert(pair<char,string>('R',"ARG"));
  ch_map.insert(pair<char,string>('N',"ASN"));
  ch_map.insert(pair<char,string>('D',"ASP"));
  ch_map.insert(pair<char,string>('C',"CYS"));
  ch_map.insert(pair<char,string>('Q',"GLN"));
  ch_map.insert(pair<char,string>('E',"GLU"));
  ch_map.insert(pair<char,string>('G',"GLY"));
  ch_map.insert(pair<char,string>('H',"HIS"));
  ch_map.insert(pair<char,string>('I',"ILE"));
  ch_map.insert(pair<char,string>('L',"LEU"));
  ch_map.insert(pair<char,string>('K',"LYS"));
  ch_map.insert(pair<char,string>('M',"MET"));
  ch_map.insert(pair<char,string>('F',"PHE"));
  ch_map.insert(pair<char,string>('P',"PRO"));
  ch_map.insert(pair<char,string>('S',"SER"));
  ch_map.insert(pair<char,string>('T',"THR"));
  ch_map.insert(pair<char,string>('W',"TRP"));
  ch_map.insert(pair<char,string>('Y',"TYR"));
  ch_map.insert(pair<char,string>('V',"VAL"));

  int idx, nsize = bases.size();
  if (nsize > 1) {
    // Several substrings delimited by "-" were found. Check that
    // each substring contains 3 characters.
    for (idx=0; idx<nsize; idx++) {
      STLUtil::stripLeadingAndTrailingWhiteSpace(bases[idx]);
      if (bases[idx].length() != 3 || aa_map.find(bases[idx]) == aa_map.end()) {
        string error = "Unknown amino acid ";
        error.append(bases[idx]);
        error.append(" in peptide sequence");
        throw EcceException(error, WHERE);
      }
    }
  } else {
    if (bases[0].length() == 3 && aa_map.find(bases[0]) == aa_map.end()) {
      bases.clear();
      nsize = sequence.length();
      map<char,string>::iterator cbase;
      for (idx=0; idx<nsize; idx++) {
        cbase = ch_map.find(sequence[idx]);
        if (cbase != ch_map.end()) {
          bases.push_back(cbase->second);
        } else {
          string error = "Unknown amino acid ";
          error.append(1,sequence[idx]);
          error.append(" in peptide sequence");
          throw EcceException(error, WHERE);
        }
      }
    }
  }

  EDSIServerCentral sc;
  //
  // Some hardwired but relative paths.
  //
  string aaNend = "/Amino_Acids/N_Terminus";
  string aaCend = "/Amino_Acids/C_Terminus";
  string aainterior = "/Amino_Acids/Interior";

  // Multiplier for units. Since they are Angstroms, just set to 1.0
  double mult = 1.0;

  if (numAtoms() > 0) {
      throw EcceException("Fragment must be empty before peptide "
                          "can be generated", WHERE);
  }
  if (bases.size()<2) {
      throw EcceException("Peptide sequence must contain at least two amino "
                          "acids", WHERE);
  }
  // Do first amino acid
  string path = sc.getDefaultStructureLibrary().toString() + aaNend;
  map<string,string>::iterator base;
  base = aa_map.find(bases[0]);
  if (base != aa_map.end()) {
    path += base->second;
  }
  EDSI *edsi = EDSIFactory::getEDSI(EcceURL(path));
  istream *data = edsi->getDataSet();
  restoreMVM(*data, mult, true);
  delete edsi;
  delete data;

  // Set initial residue numbers to 1
  vector<Residue*> resList = ((Fragment*)this)->residues();
  if (resList.size() == 1) {
    resList[0]->number(1);
  }
  
  // Create remainder of chain
  for (idx=1; idx<nsize; idx++) {
    if (idx<nsize-1) {
      path = sc.getDefaultStructureLibrary().toString() + aainterior;
    } else {
      path = sc.getDefaultStructureLibrary().toString() + aaCend;
    }
    base = aa_map.find(bases[idx]);
    if (base != aa_map.end()) {
      path += base->second;
    } else  {
      throw EcceException("Unexpected base",WHERE);
    }

    EDSI *newedsi = EDSIFactory::getEDSI(EcceURL(path));
    istream *newdata = newedsi->getDataSet();
    SGFragment tmpfrag;
    bool loadStatus;
    loadStatus = tmpfrag.restoreMVM(*newdata, mult, true);
    delete newedsi;
    delete newdata;
    
    // Look for C atom and then find the nub attached to it
    Residue *res = residue(idx-1);
    vector<TAtm*> atomList = findResidueAtoms(res);
    int i, iatm, natoms;
    iatm = -1;
    TAtm* atom;
    natoms = atomList.size();
    for (i=0; i<natoms; i++) {
      atom = atomList[i];
      if (atom->atomName() == " C  ") {
        iatm = i;
        break;
      }
    }
    atom = atomList[iatm];
    // get attached nub
    vector<TAtm*>* alist = atom->connectedAtoms();
    natoms = alist->size();
    for (i=0; i<natoms; i++) {
      atom = (*alist)[i];
      if (atom->atomicSymbol() == "Nub") {
        iatm = atom->index();
        break;
      }
    }
    delete alist;
    if (iatm == -1) {
      string errmsg = "No bonding nubs found.  Check structures in ";
      errmsg += "Structure Library";
      throw EcceException(errmsg, WHERE);
    }
    // Search tmpfrag for N atom and then find nub attached to it
    res = tmpfrag.residue(0);
    atomList = tmpfrag.findResidueAtoms(res);
    int jatm = -1;
    natoms = atomList.size();
    for (i=0; i<natoms; i++) {
      atom = atomList[i];
      if (atom->atomName() == " N  ") {
        jatm = i;
        break;
      }
    }
    atom = atomList[jatm];
    // get attached nub
    alist = atom->connectedAtoms();
    natoms = alist->size();
    for (i=0; i<natoms; i++) {
      atom = (*alist)[i];
      if (atom->atomicSymbol() == "Nub") {
        jatm = atom->index();
        break;
      }
    }
    delete alist;
    if (jatm == -1) {
      string errmsg = "No bonding nubs found.  Check structures in ";
      errmsg += "Structure Library";
      throw EcceException(errmsg, WHERE);
    }
    // Merge new base to existing structure
    mergeFragment(tmpfrag, jatm, iatm, 0.0, 0.0, 0.0);

    if (loadStatus) {
      // Is this really needed????
      for (int idx=tmpfrag.numAtoms()-1; idx>=0; idx--) {
        tmpfrag.removeAtom(idx);
      }

    } else {
      throw EcceException("Unable to parse input data", WHERE);
    }
  }
  reIndexResidues();
  recenterFrag();
}

/**
 * Add or change an atom based on what is selected and provided params.
 */
bool SGFragment::addAtom(const string& elem, const string& geom,
    double x, double y, double z, double order)
{

   bool ret = true;  // add operation

   FragUtil fragutil(this);
   if (m_atomHighLight.size() > 1) {
      // We only allow one atom to be selected.  We could just use the
      // first one in the array but we rather make them be explicit.
      throw EcceException("Select only one atom for this action.",WHERE);

   } else if (m_atomHighLight.size() == 0) {
      // Here we have a free space click.
      fragutil.addAtLocation(elem.c_str(),geom.c_str(),x, y,z,0);
   } else {

      // Here we will add to the selected atom which may be a nub or an
      // atom that has open nubs.
      int selIndex = m_atomHighLight[0];
      TAtm *selAtom = atomRef(selIndex);

      if (selAtom->atomicSymbol() == "Nub") {
         // This is good, we know just where to add it.
         TAtm* parentAtom = nubParent(selAtom);
         ret = fragutil.addToNub(parentAtom,selAtom,elem.c_str(),
               geom.c_str(),order);
      } else {
#ifdef autonubselect
         // ok so we have to find a nub
         TAtm* nubAtom = childNub(selAtom);
         if (nubAtom == 0) {
            throw EcceException("No free nubs.",WHERE);
         } else {
            addToNub(selAtom,nubAtom,elem,geom,order);
         }
#else

         // If they selected a non Nub they must want to change the
         // geometry.  We can only do this if we don't have to remove bonds.
         // First make sure they provided a new shape or atom type
         string oldTag = selAtom->atomicSymbol();
         string oldGeom = selAtom->shapeString();
         if (STLUtil::compareString(oldTag,elem) == 0 &&
               STLUtil::compareString(oldGeom,geom) == 0) {
            ret = false;
            throw EcceException("The new atom and shape are identical to "
                  "the current atom and shape.  Request ignored.",WHERE);
         } else if (oldTag != elem && oldGeom == geom) {
            // Here we are changing the type but not the shape.  This is
            // easy.  We want to change things like symbol, number, instance
            // name, and mass number but not partial charge, charge, shape,
            // or coordinates.
            ret = false;  // atom change
            TAtm tmp(elem.c_str());
            selAtom->atomicSymbol(elem.c_str());
            selAtom->atomicNumber(tmp.atomicNumber());
            selAtom->atomName(tmp.atomName().c_str());
         } else {
            ret = false;  // atom change

            // Alright, they want to change the shape.  
            // But we can't take away bonds
            // and metal pi bonds don't count for anything.
            vector<TBond*> theBonds = selAtom->bondList();
            TBond *bond = 0;
            int ignoreCnt = 0;
            for (int idx=theBonds.size()-1; idx>=0; idx--) {
               bond = theBonds[idx];
               if (bond->order() == TBond::Metal ||
                     bond->atom1()->atomicSymbol() == "Nub" ||
                     bond->atom2()->atomicSymbol() == "Nub") {
                  ignoreCnt++;
               }
            }

            // So how many bonds are currently in use?
            int curBonds = theBonds.size() - ignoreCnt;

            int numAtoms = s_shapes.getNumberOfAtoms(geom.c_str());

            // We need to get the data for the requested shape.
            if (curBonds > numAtoms) {
               throw EcceException("Bonds cannot be taken away from "
                      "existing atoms using this method.  "
                      "Ignoring request to change atom.",WHERE);
            } else {
               DisplayStyle keepDisplayStyle = selAtom->displayStyle();
               fragutil.changeGeometry(selAtom,elem, geom);
               //frag->changeTag(selIndex,elem);
               changeTag(selAtom->index(),elem);
               selAtom->shapeString(geom);

               ret = false;
            }
            // Due to the complexities of how shapes are changed, the atoms
            // need to be reordered.
            renumberAtoms();
         }
#endif
      }
   }
   return ret;
}


ostream& operator<<(ostream& os, const SGFragment& rhs)
{
  os << (Fragment&) rhs;
  return os;
}


// The following two getter methods (getAtomHighlight and
// getBondHighlight) were added to avoid a core dump.
// For some reason, directly accessing m_atomHighlight from
// the GeomTable was causing a crash.  At this point not 
// sure what the underlying cause is, but this fixes
// the problem.
vector<int> *SGFragment::getAtomHighlight()
{
  return &m_atomHighLight;
}


vector<int> *SGFragment::getBondHighlight()
{
  return &m_bondHighLight;
}



/**
 * If a nub is selected but its atom is not, clear nub selection and
 * if an atom is selected, ensure its nubs are.
 */
bool SGFragment::syncNubSelection(vector<bool>& atomsToDelete)
{
   bool ret = false;


   int cnt = atomsToDelete.size();
   TAtm *parent, *atm;
   int parentIdx;

   for (int idx=0; idx<cnt; idx++) {
      if (atomsToDelete[idx] == true) {
         atm = atomRef(idx);
         if (atm->atomicSymbol() == "Nub") {
            parent = nubParent(atm);
            parentIdx = parent->index();
            if (atomsToDelete[parentIdx] == false) {
               atomsToDelete[idx] = false;
               ret = true;
            }
         } else {
            // Here add any nubs that aren't selected
            vector<TAtm*> *conAtoms;
            conAtoms = atm->connectedAtoms();
            int numConnectedAtoms = conAtoms->size();
            TAtm  *child;
            for (int adx=0; adx<numConnectedAtoms; adx++) {
               child = (*conAtoms)[adx];
               if (child->atomicSymbol() == "Nub") {
                  atomsToDelete[child->index()] = true;
                  ret = true;
               }
            }
            delete conAtoms;
         }
      }
   }

   return ret;
}


/**
 * Deletes selected bonds.
 */
bool SGFragment::deleteSelectedBonds()
{
   bool ret = true;

   vector<TBond*> *allBonds = bonds();

   int nBonds = m_bondHighLight.size();
   TBond *bond;
   TAtm *atm1, *atm2, *nub;
   for (int idx=0; idx<nBonds; idx++) {
      bond = (*allBonds)[m_bondHighLight[idx]];

      atm1 = bond->atom1();
      atm2 = bond->atom2();

      //Attach nub to atm1
      nub = new TAtm("Nub",atm2->coordinates());
      nub->shape(ShapeData::terminal);
      nub->displayStyle(atm1->displayStyle());
      //cout << "new nub display style: " << nub->displayStyle() << endl;
      new TBond(atm1,nub);
      addAtom(nub,atm1);
      nub->displayStyle(atm1->displayStyle());
//      fixNub(nub,sg->getScaleFactor(nub->displayStyle()));

      //Attach nub the atm2
      nub = new TAtm("Nub",atm1->coordinates());
      nub->shape(ShapeData::terminal);
      nub->displayStyle(atm2->displayStyle());
      //cout << "new nub display style: " << nub->displayStyle() << endl;
      new TBond(atm2,nub);
      addAtom(nub,atm2);
      nub->displayStyle(atm2->displayStyle());
//      fixNub(nub,sg->getScaleFactor(nub->displayStyle()));

      /* note: the display style has to be set twice.  The first time it is
       * set ensures the bond is the right display style type.  The second
       * time overrides the "addAtom" ability to set the display
       * style.  This is the only class that we have to force the display
       * style setting.
       */

      delete bond;

   }
   delete allBonds;

   return ret;

}



/**
 *
 * Assumptions
 *   a) bitmask reflects current selection of atoms AND
 *   b) bitmask includes all nubs for selected atoms 
 *      and excludes nubs for unselected atoms
 *   c) called after bonds have been broken (deleteSelectedAtomsANdBonds)
 *
 * For histerical purposes, see documentation in DeleteCmd for 
 * info on processing step numbers.
 */
bool SGFragment::deleteSelectedAtoms(const vector<bool>& atomsToDelete)
{
   bool ret = false;
   TPerTab tpt;
   int idx;

   int curEditResIndex = editResidueIndex();
   int curMode = editResidueMode();
   editResidueMode(false,-1);


   int numAtoms = this->numAtoms();

   // make list of atom pointers to delete
   vector<TAtm*> ptrsToDelete;
   for (idx=0; idx<numAtoms; idx++) {
      if (atomsToDelete[idx] == true) {
         ptrsToDelete.push_back(atomRef(idx));
      }

   }

   // 3. Create a vector to keep track of 1st atom pointers so we can repair
   // the first atom pointers.
   vector<bool> firstAP(numAtoms);
   int numRes = numResidues();
   Residue *res=NULL;
   TAtm *first = NULL;
   for (idx=0; idx<numRes; idx++) {
      res = findResidue(idx);
      if (res) {
         first = res->firstAtomPtr();
         if (first) {
            firstAP[first->index()] = true;
         }
      }
   }


   // 3a. Figure out how to fix firstAtomPtrs after all the deleting is
   // done
   vector<int> resDelList;
   vector<TAtm*> firsts(numRes);
   int iatom;
   for (idx=0; idx < numRes; idx++) {
      res = findResidue(idx);
      firsts[idx] = res->firstAtomPtr();
      if (firsts[idx]) {
         iatom = firsts[idx]->index();
         if (atomsToDelete[iatom] == true) {
            // Ok here is a 1st atom pointer that gets deleted
            bool fixed = false;
            for (int jdx=iatom+1; jdx<numAtoms && firstAP[jdx]==false; jdx++) {
               if (firstAP[jdx] == false && atomsToDelete[jdx] == false &&
                     atomRef(jdx)->getResidue() == res) {
                  firsts[idx] = atomRef(jdx);
                  fixed = true;
                  break;
               }
            }
            if (fixed == false) {
               // no new pointer found so mark residue for delete
               resDelList.push_back(idx);
            }
         }
      }
   }
   /*
   if (numRes > 0) {
      //    int curres = -1;
      int curres;
      for (idx=0; idx<numAtoms; idx++) {
         //cout << "FirstAP["<<idx<<"] is "<<firstAP[idx]<<endl;
         if (firstAP[idx] == true) {
            //        curres++;
            curres = atomRef(idx)->getResidue()->number();
            //cout << "Found first atom pointer for residue "<<curres<<endl;
            firsts[curres] = findResidue(curres)->firstAtomPtr();
            if ((*p_atomsToDelete)[idx] == true) {
               // Ok here is a 1st atom pointer that go deleted
               bool fixed = false;
               for (int jdx=idx+1; jdx<numAtoms && firstAP[jdx]==false; jdx++) {
                  if (firstAP[jdx] == false && (*p_atomsToDelete)[jdx] == false) {
                     firsts[curres] = atomRef(jdx);
                     fixed = true;
                     break;
                  }
               }
               if (fixed == false) {
                  // no pointers so set to null and mark residue for delete
                  firsts[curres] = NULL;
                  resDelList.push_back(curres);
               }
            }
         }
      }
   }
   */

   // 4.
   vector<TAtm*> *conAtoms;
   vector<TAtm*> nubList;
   vector<TAtm*> nubParentList;
   TAtm *src;
   int numToDelete = ptrsToDelete.size();
   for (idx=0; idx<numToDelete; idx++) {
      src = ptrsToDelete[idx];

      // get src's connected atoms
      conAtoms = src->connectedAtoms();
      int numConnectedAtoms = conAtoms->size();

      // loop through all the connected atoms
      TAtm *conatm, *nub;
      for (int cdx = 0; cdx < numConnectedAtoms; cdx++) {

         conatm = (*conAtoms)[cdx];
         if (atomsToDelete[conatm->index()] == true) {
            // The bond between these atoms can be eliminated
            // do nothing - bonds go away with atom later.
         } else {
            // The bond between these atoms must be exchanged for a
            // nub/atom bond on the conatm.
            // create a nub
            nub = new TAtm("Nub",src->coordinates());
            nub->shape(ShapeData::terminal);

            // give it a display style so the bond is created correctly
            nub->displayStyle(conatm->displayStyle());

            // create a bond and set atom1 and atom2 ptrs
            new TBond(conatm,nub);  

            if (conatm->residueName() != "" ) {
               // copy all the residue info from the atom to the nub since
               // we want to make sure the nub has the same properties as
               // its atom
               nub->copyResidueInfo(conatm);
            }

            // will need to add the atom to our fragment
            nubList.push_back(nub);
            nubParentList.push_back(conatm);

         }
      }
      // POSSIBLE MEMORY LEAK
      delete conAtoms;
   }

   // 5.
   int numNubs = nubList.size();
   TAtm *nub;
   for (idx=0; idx<numNubs; idx++) {
      nub = nubList[idx];
      if (nub->residueName() == "") {
         // use the Fragment's method to skip over the display
         // style setting in SGFragment
         Fragment::addAtom(nub,nubParentList[idx]);
      } else {
         // go get the Residue that the nub belongs to
         res = Fragment::findResidue
            (nub->residueName().c_str(), nub->residueNumber(), 
             nub->icode(), nub->chain());
         EE_ASSERT(res!=NULL, EE_FATAL, "CmdDelete, findResidue failed");
         // insert our new nub with its other residue atoms 
         Fragment::addAtom(nub,nubParentList[idx]);
      }
//      fixNub(nub, sg->getScaleFactor(nub->displayStyle()));

   }

   // 6.
   renumberAtoms(0);

   // 7. delete atoms
   list<int> delAtomIndices ;
   for (int j = 0; j < numToDelete; j++) {
      delAtomIndices.push_back(ptrsToDelete[j]->index());
   }
   // Do all the deletes at the end to avoid messing up index orders.
   delAtomIndices.sort() ;

   list<int>::reverse_iterator delAtom_riter = delAtomIndices.rbegin();
   TAtm *delatom;
   while (delAtom_riter != delAtomIndices.rend()) {
      delatom = atomRef(*delAtom_riter);
      removeAtom(*delAtom_riter);
      delete delatom;
      delAtom_riter++;
   }

   // 8.
   for (idx=0; idx<numRes; idx++) {
      findResidue(idx)->firstAtomPtr(firsts[idx]);
   }

   // 9. now delete residues
   int numResDel = resDelList.size();
   for (idx=numResDel-1; idx>=0; idx--) {
      deleteResidue(resDelList[idx]) ;
   }

   // 10. done deleting.  renumber
   renumberAtoms(0);

   editResidueMode(curMode,curEditResIndex);

   return ret;
}



bool SGFragment::deleteSelectedAtomsAndBonds(vector<bool>& atomsToDelete)
{
   bool ret = true;

   vector<TBond*> *allBonds = bonds();

   // Copy atom highlight vector
   vector<TAtm*> saveHighLight;
   int nAtoms = m_atomHighLight.size();
   int iat, i;
   for (i=0; i<nAtoms; i++) {
      iat = m_atomHighLight[i];
      saveHighLight.push_back(atomRef(iat));
   }

   int nBonds = m_bondHighLight.size();
   TBond *bond;
   TAtm *atm1, *atm2, *nub;
   int iatm1, iatm2;
   for (int idx=0; idx<nBonds; idx++) {
      bond = (*allBonds)[m_bondHighLight[idx]];

      atm1 = bond->atom1();
      atm2 = bond->atom2();
      if (atm1->atomicSymbol() != "Nub" && atm2->atomicSymbol() != "Nub") {
         //iatm1 = atomIndex(*atm1);
         //iatm2 = atomIndex(*atm2);
         iatm1 = atm1->index();
         iatm2 = atm2->index();
         bool conts1 = (find(m_atomHighLight.begin(),
                  m_atomHighLight.end(),
                  iatm1) == m_atomHighLight.end());
         bool conts2 = (find(m_atomHighLight.begin(),
                  m_atomHighLight.end(),
                  iatm2) == m_atomHighLight.end());
         if (conts1 && conts2) {

            nub = new TAtm("Nub",atm2->coordinates());
            nub->shape(ShapeData::terminal);
            nub->displayStyle(atm1->displayStyle());
            new TBond(nub,atm1);
            addAtom(nub,atm1);  
            nub->displayStyle(atm1->displayStyle());
            //            fixNub(nub,sg->getScaleFactor(nub->displayStyle()));

            nub = new TAtm("Nub",atm1->coordinates());
            nub->shape(ShapeData::terminal);
            nub->displayStyle(atm2->displayStyle());
            new TBond(atm2,nub);
            addAtom(nub,atm2);  
            nub->displayStyle(atm2->displayStyle());
            //            fixNub(nub,sg->getScaleFactor(nub->displayStyle()));

            /*  See note in "deleteOnlyBonds" for display style setting
             *  explanation.
             */

            delete bond;
         }

      }
   }
   delete allBonds;

   // reset atom highlight vector
   m_atomHighLight.clear();
   for (i=0; i<nAtoms; i++) {
      (void)m_atomHighLight.push_back(saveHighLight[i]->index());
   }
   // now need to reset atomsToDelete vector so that it reflects
   // new ordering
   atomsToDelete.clear();

   int nTotAtoms = numAtoms();
   for (i=0; i<nTotAtoms; i++) {
      atomsToDelete.push_back(false);
   }
   for (i=0; i<nAtoms; i++) {
      atomsToDelete[m_atomHighLight[i]] = true;
   } 
   ret |= deleteSelectedAtoms(atomsToDelete);


   return ret;
}


/** 
 * Turn list of selected atoms into list of selected residues.
 */
void SGFragment::atomsToResidues(const vector<int>& atoms, vector<bool>& resbits)
{
   int numRes = numResidues();
   if (numRes == 0) return;

   int numAtoms = atoms.size();
   resbits.resize(numRes,false);
   for (int idx=0; idx<numAtoms; idx++) {
      TAtm *atm = atomRef(atoms[idx]);
      resbits[atm->getResidue()->index()] = true;
   }
}


/** 
 * Turn list of selected atoms into list of selected residues.
 */
void SGFragment::atomsToResidues(const vector<int>& atoms, vector<int>& residues)
{
   residues.clear();
   int numAtoms = atoms.size();
   int numRes = numResidues();
   vector<bool> resbits(numRes,false);
   for (int idx=0; idx<numAtoms; idx++) {
      TAtm *atm = atomRef(atoms[idx]);
      resbits[atm->getResidue()->index()] = true;
   }

   for (int rdx=0; rdx<numRes; rdx++) {
      if (resbits[rdx] == true) {
         residues.push_back(rdx);
      }
   }
}
