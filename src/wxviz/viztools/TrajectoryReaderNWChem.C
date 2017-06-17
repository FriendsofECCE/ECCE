#include <fstream>
  using std::ifstream;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#define MAX_LINE 256

#define VELOCITY 1
#if VELOCITY
//#include "inv/nodes/SoSwitch.H"
#endif

#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/VRVector.H"


#include "wxviz/TrajectoryReaderNWChem.H"


/**
 * Constructor.
 * The goal of this class was to separate reading from user interaction
 * but this proved too much work for now at least.
 */
TrajectoryReaderNWChem::TrajectoryReaderNWChem(SGContainer * sg)
  : TrajectoryReader(sg),
    p_useSolvent(false),
    p_soluteAtoms(0),
    p_soluteBonds(0),
    p_solventAtoms(0),
    p_solventBonds(0),
    p_maxSolvMol(0),
    p_useAtom(),
    p_numUsedAtoms(0),
    p_showVector(),
    p_isHeaderRead(false),
    p_stream(),
    p_fileName(""),
    p_stepToFileName(),
    p_stepToSeekPosition()
{
}


/**
 * Destructor.
 */
TrajectoryReaderNWChem::~TrajectoryReaderNWChem() 
{
}


int TrajectoryReaderNWChem::getSoluteAtomsCount()
{
  return p_numUsedAtoms;
}


long TrajectoryReaderNWChem::getStepCount()
{
  return p_stepToFileName.size();
}


string TrajectoryReaderNWChem::getCurrentFileName()
{
  return p_fileName;
}


bool TrajectoryReaderNWChem::readStep(const long& step,
        const bool& readVelocities, const bool& readForces)
        throw (IOException)
{
   if (step >= getStepCount()) {
      // don't bother if the requested step is outside valid range
      return false;
   }

   changeStreamForStep(step);

   SGFragment * frag = p_sg->getFragment();

   char buf[MAX_LINE];
   char* cptr ;
   int i;
   /**
    *  The solvent coordinates, etc. are written out first in the NWChem
    *  format trajectory files (if they are written out at all), followed by
    *  the solute coordinates. The read step method reverses the location
    *  of these coordinates in the fragment so that the solute coordinates
    *  come first, followed by the solvent coordinates. This gives a nicer
    *  structure to the MD and geometry tables.
    */

   cptr = buf;

   // Skip next four lines (for now).

   p_stream.getline(buf,MAX_LINE);
   if (!p_stream.good()) {
      frag->clear();
      throw IOException("EOF reading over line 1 for trajectory step", WHERE);
      return 0;
   }
   p_stream.getline(buf,MAX_LINE);
   if (!p_stream.good()) {
      frag->clear();
      throw IOException("EOF reading over line 2 for trajectory step", WHERE);
      return 0;
   }
   p_stream.getline(buf,MAX_LINE);
   if (!p_stream.good()) {
      frag->clear();
      throw IOException("EOF reading over line 3 for trajectory step", WHERE);
      return 0;
   }
   p_stream.getline(buf,MAX_LINE);
   if (!p_stream.good()) {
      frag->clear();
      throw IOException("EOF reading over line 4 for trajectory step", WHERE);
      return 0;
   }

   // Find out whether or not to read in solvent and/or solute
   // molecules and whether or not to include velocities. Also
   // find out how many solvent molecules are present in this frame.

   char i10[11];
   char r8[9];
   i10[10] = '\0';
   r8[8] = '\0';
   p_stream.getline(buf,MAX_LINE);
   if (!p_stream.good()) {
      frag->clear();
      throw IOException("EOF reading count line for trajectory step", WHERE);
      return 0;
   }
   cptr = buf;

   bool solventCoords;
   bool solventVel;
   bool solventFrc;
   bool solventPol;
   bool soluteCoords;
   bool soluteVel;
   bool soluteFrc;
   bool solutePol;
   // Check to see of frame is in new format.
   if (toupper(*(cptr+7)) == 'T' || toupper(*(cptr+7) == 'F')) {
      solventCoords = toupper(*cptr++) == 'T';
      solventVel =    toupper(*cptr++) == 'T';
      solventFrc =    toupper(*cptr++) == 'T';
      solventPol =    toupper(*cptr++) == 'T';
      soluteCoords =  toupper(*cptr++) == 'T';
      soluteVel =     toupper(*cptr++) == 'T';
      soluteFrc =     toupper(*cptr++) == 'T';
      solutePol =     toupper(*cptr++) == 'T';
   } else if (toupper(*(cptr+5)) == 'T' || toupper(*(cptr+5) == 'F')) {
      // Maintain compatibility with old trajectory files.
      solventCoords = toupper(*cptr++) == 'T';
      solventVel =    toupper(*cptr++) == 'T';
      solventFrc =    toupper(*cptr++) == 'T';
      solventPol =    false;
      soluteCoords =  toupper(*cptr++) == 'T';
      soluteVel =     toupper(*cptr++) == 'T';
      soluteFrc =     toupper(*cptr++) == 'T';
      solutePol =     false;
   } else {
      // Maintain compatibility with really old trajectory files.
      solventCoords = toupper(*cptr++) == 'T';
      solventVel =    toupper(*cptr++) == 'T';
      solventFrc =    false;
      solventPol =    false;
      soluteCoords =  toupper(*cptr++) == 'T';
      soluteVel =     toupper(*cptr++) == 'T';
      soluteFrc =     false;
      solutePol =     false;
   }

   strncpy(i10,cptr,10);
   int solventMol = atoi(i10);
   int solventAtoms = solventMol*p_solventAtoms;

   // Read in solvent coordinates and multiply by 10 to convert
   // from nanometers to angstroms

   double coords[3];
   int offset = p_numUsedAtoms;

   int numSolventAtoms = 0;

   bool newDisplay = false;
   bool oldDisplay;
#if VELOCITY
   p_sg->getTopAtomVectors()->removeAllChildren();
   if (p_showVector.size() == 0) {
     int natoms = solventAtoms + p_soluteAtoms;
     for (int i=0; i<natoms; i++) {
       p_showVector.push_back(true);
     }
   }
#endif
   if (solventCoords || solventVel || solventFrc || solventPol) {
      for (i=0; i<solventAtoms; i++) {
         p_stream.getline(buf,MAX_LINE);
         if (!p_stream.good()) {
            frag->clear();
            throw IOException("EOF reading solvent for trajectory step", WHERE);
            return 0;
         }

         if (solventCoords) {
            // read in solvent coordinates and multiply by 10 to convert
            // from nanometers to angstroms
            cptr = buf;
            strncpy(r8,cptr,8);
            coords[0] = 10.0*atof(r8);
            cptr += 8;
            strncpy(r8,cptr,8);
            coords[1] = 10.0*atof(r8);
            cptr += 8;
            strncpy(r8,cptr,8);
            coords[2] = 10.0*atof(r8);
            if (p_useSolvent){
               TAtm* atm = frag->atomRef(i+offset);
               DisplayStyle style = atm->displayStyle();
               oldDisplay = style.isDisplayed();
               if (!oldDisplay)
                  newDisplay = true;
               style.setDisplayed(true);
               atm->displayStyle(style);
               atm->coordinates(coords);
               numSolventAtoms++;
#if VELOCITY
               if (solventVel) {
                 if (readVelocities) {
                   double vel[3];
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   vel[0] = p_vScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   vel[1] = p_vScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   vel[2] = p_vScale*atof(r8);
                   if (p_showVector[i+offset]) {
                     VRVector *vec = new VRVector;
                     vec->setName("Velocity");
                     vec->position(coords[0],coords[1],coords[2]);
                     vec->direction(vel[0],vel[1],vel[2]);
                     vec->setColor(0.0,0.0,1.0);
                     p_sg->getTopAtomVectors()->addChild(vec);
                   }
                 } else {
                   cptr += 24;
                 }
               }
               if (solventFrc) {
                 if (readForces) {
                   double frc[3];
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   frc[0] = p_fScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   frc[1] = p_fScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   frc[2] = p_fScale*atof(r8);
                   if (p_showVector[i+offset]) {
                     VRVector *vec = new VRVector;
                     vec->setName("Force");
                     vec->position(coords[0],coords[1],coords[2]);
                     vec->direction(frc[0],frc[1],frc[2]);
                     vec->setColor(1.0,1.0,0.0);
                     p_sg->getTopAtomVectors()->addChild(vec);
                   }
                 } else {
                   cptr += 24;
                 }
               }
#endif
            }
            if (solventPol) {
               p_stream.getline(buf,MAX_LINE);
               if (!p_stream.good()) {
                  frag->clear();
                  throw IOException("EOF reading solvent polarizability for "
                                    "trajectory step", WHERE);
                  return 0;
               }
            }
         }
      }
   }
   if (p_useSolvent) {
      offset = numSolventAtoms + p_soluteAtoms;
   } else {
      offset = p_soluteAtoms;
   }
   // Set unused solvent atoms display style to false
   int natoms = frag->numAtoms();
   for (i=offset; i<natoms; i++) {
      TAtm* atm = frag->atomRef(i);
      DisplayStyle style = atm->displayStyle();
      if (style.isDisplayed()) newDisplay = true;
      style.setDisplayed(false);
      atm->displayStyle(style);
   }

   // Read in solute coordinates and multiply by 10 to convert
   // from nanometers to angstroms

   if (soluteCoords || soluteVel || soluteFrc || solutePol) {
      int atomcnt = 0;
      for (i=0; i<p_soluteAtoms; i++) {
         p_stream.getline(buf,MAX_LINE);
         if (!p_stream.good()) {
            frag->clear();
            throw IOException("EOF reading solute for trajectory step", WHERE);
            return 0;
         }
         if (soluteCoords) {
            cptr = buf;
            strncpy(r8,cptr,8);
            coords[0] = 10.0*atof(r8);
            cptr += 8;
            strncpy(r8,cptr,8);
            coords[1] = 10.0*atof(r8);
            cptr += 8;
            strncpy(r8,cptr,8);
            coords[2] = 10.0*atof(r8);
            if (p_useAtom[i]) {
               TAtm* atm = frag->atomRef(atomcnt);
               atm->coordinates(coords);
               atomcnt++;
#if VELOCITY
               if (soluteVel) {
                 if (readVelocities) {
                   double vel[3];
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   vel[0] = p_vScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   vel[1] = p_vScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   vel[2] = p_vScale*atof(r8);
                   if (p_showVector[i]) {
                     VRVector *vec = new VRVector;
                     vec->setName("Velocity");
                     vec->position(coords[0],coords[1],coords[2]);
                     vec->direction(vel[0],vel[1],vel[2]);
                     vec->setColor(0.0,0.0,1.0);
                     p_sg->getTopAtomVectors()->addChild(vec);
                   }
                 } else {
                   cptr += 24;
                 }
               }
               if (soluteFrc) {
                 if (readForces) {
                   double frc[3];
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   frc[0] = p_fScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   frc[1] = p_fScale*atof(r8);
                   cptr += 8;
                   strncpy(r8,cptr,8);
                   frc[2] = p_fScale*atof(r8);
                   if (p_showVector[i]) {
                     VRVector *vec = new VRVector;
                     vec->setName("Force");
                     vec->position(coords[0],coords[1],coords[2]);
                     vec->direction(frc[0],frc[1],frc[2]);
                     vec->setColor(1.0,1.0,0.0);
                     p_sg->getTopAtomVectors()->addChild(vec);
                   }
                 } else {
                   cptr += 24;
                 }
               }
#endif
            }
            if (solutePol) {
               p_stream.getline(buf,MAX_LINE);
               if (!p_stream.good()) {
                  frag->clear();
                  throw IOException("EOF reading solute polarizability for "
                                    "trajectory step", WHERE);
                  return 0;
               }
            }
         }
      }
   }
   // Check to make sure that bond display style is okay
   vector<TBond*> *bonds = frag->bonds();
   TBond* bond = 0;
   int nbonds = bonds->size();
   for (i=0; i<nbonds; i++) {
      bond = (*bonds)[i];
      DisplayStyle style = bond->displayStyle();
      if (bond->atom1()->displayStyle().isDisplayed() ||
            bond->atom2()->displayStyle().isDisplayed()) {
         style.setDisplayed(true);
      } else {
         style.setDisplayed(false);
      }
      bond->displayStyle(style);
   }
   delete bonds;

   if (!p_stream.good()) {
      throw IOException("Stream is invalid", WHERE);
   }

   return newDisplay;
}


void TrajectoryReaderNWChem::setFileNames(const vector<string>& fileNames,
                                          const bool& useSolvent)
{
  TrajectoryReader::setFileNames(fileNames);
  p_isHeaderRead = false;
  p_useSolvent = useSolvent;
  indexFiles();
  readHeader();
}


void TrajectoryReaderNWChem::processVectors(const vector<bool>& atoms)
{
   p_showVector = atoms;
}


/**
 * Read the header section of an nwchem trajectory file.
 */
void TrajectoryReaderNWChem::readHeader() throw (IOException)
{
  if (p_isHeaderRead) {
    return;
  }

  changeStreamForStep(0);

  p_isHeaderRead = true;

   SGFragment * frag = p_sg->getFragment();

   char buf[MAX_LINE];
   char* cptr; 
   char i10[11];
   char i8[9];
   char i6[7];
   char i1[2];
   char a4[5];
   char aa4[5];
   int i,j,k,l;
   int iresold = -1;
   int ires,iuse;
   p_numUsedAtoms = 0;
   p_solventAtoms = 0;
   p_useAtom.clear();
   vector<string> solventSymbol;
   vector<string> solventAtomName;
   vector<int> solventIbnd;
   vector<int> solventJbnd;
   vector<int> atomMap;

   i10[10] = '\0';
   i8[8] = '\0';
   i6[6] = '\0';
   i1[1] = '\0';
   a4[4] = '\0';
   aa4[4] = '\0';

   // go to beginning of file, skip "header" line
   p_stream.seekg(0);
   p_stream.getline(buf,MAX_LINE);

   p_stream.getline(buf,MAX_LINE);
   if (!p_stream.good()) {
      frag->clear();
      throw IOException("EOF reading atom/bond count for trajectory header",
                        WHERE);
   }
   cptr = buf;
   strncpy(i10,cptr,10);
   p_solventAtoms = atoi(i10); 
   cptr += 10;
   strncpy(i10,cptr,10);
   p_soluteAtoms = atoi(i10); 
   cptr += 10;
   strncpy(i10,cptr,10);
   p_soluteBonds = atoi(i10); 
   cptr += 10;
   strncpy(i10,cptr,10);
   p_solventBonds = atoi(i10); 
   cptr += 10;
   strncpy(i10,cptr,10);
   p_maxSolvMol = atoi(i10);   // max number of solvent molecules

   // Read in atomic name for solvent atoms, extract atomic symbol,
   // and convert to atomic number. Assume that atom name conforms
   // to PDB format. Get residue name and number at the same time.

   // Actually talked to Erich about this.  The atom names can look like
   // the following examples:
   //  CA2
   //  3HA2
   //  C
   // ...
   // This means that it is not always possible to tell if the second
   // letter is part of the symbol or not CA2 could be C/A2 or Ca2
   // We decided that for now we would impose a requirement that lower
   // case be used after the first symbol if the letter is part of the
   // symbol name.

   char a2[3];
   char *nameptr;
   string solventName;
   int loc;
   bool hasTwoCharName;

   for (i=0; i<p_solventAtoms; i++) {
      p_stream.getline(buf,MAX_LINE);
      if (!p_stream.good()) {
         frag->clear();
         throw IOException("EOF reading solvent for trajectory header", WHERE);
      }
      cptr = buf;
      strncpy(a4,cptr,4);
      solventName = a4;
      cptr += 5;

      strncpy(a4,cptr,4);  // Bruce, why 4 instead of 5 should be format A5?
      nameptr = a4;

      // skip first char if number or space
      if (isdigit(*nameptr) || isspace(*nameptr)) {
         nameptr++;  
         hasTwoCharName = false;
      } else {
         hasTwoCharName = true;
      }

      // nameptr now points to start of symbol.
      loc=0;
      a2[loc++] = toupper(*nameptr);
      nameptr++;
      // We decided second symbol if first letter is in first column
      if (hasTwoCharName) a2[loc++] = tolower(*nameptr);
      a2[loc] = '\0';

      string astr(a2);
      solventSymbol.push_back(astr);
      string bstr(a4);
      solventAtomName.push_back(bstr);
   }

   // Read in atomic name for solute atoms, extract atomic symbol,
   // and use this to create a TAtm pointer. Then add pointer to
   // atom list in fragment.

   double coords[3];
   coords[0] = 0.0;
   coords[1] = 0.0;
   coords[2] = 0.0;
   Residue* res;
   for (i=0; i<p_soluteAtoms; i++) {
      p_stream.getline(buf,MAX_LINE);
      if (!p_stream.good()) {
         frag->clear();
         throw IOException("EOF reading solute for trajectory header", WHERE);
      }
      cptr = buf;
      strncpy(aa4,cptr,4);
      if (aa4[3] == '_') {
        aa4[3] = ' ';
      }
      if (aa4[2] == '_') {
        aa4[2] = ' ';
        aa4[3] = ' ';
      }

      cptr += 5;

      strncpy(a4,cptr,4);  // Bruce, why 4 instead of 5 should be format A5?
      nameptr = a4;

      // skip first char if number or space
      if (isdigit(*nameptr) || isspace(*nameptr)) {
         nameptr++;  
         hasTwoCharName = false;
      } else {
         hasTwoCharName = true;
      }

      // nameptr now points to start of symbol.
      loc=0;
      a2[loc++] = toupper(*nameptr);
      nameptr++;
      // We decided to only accept second symbol if lower case
      if (hasTwoCharName) a2[loc++] = tolower(*nameptr);
      a2[loc] = '\0';

      cptr += 5;
      strncpy(i6,cptr,6);
      ires = atoi(i6);

      // Check to see if we are actually going to display this residue.
      if (strlen(cptr) > 20) {
         cptr += 20;
         strncpy(i1,cptr,1);
         iuse = atoi(i1);
      } else {
         iuse = 1;
      }
      if (iuse == 1) {
         TAtm* atm = new TAtm(a2,coords);
         frag->addAtom(atm);
         atm->atomName(a4);
         if (ires != iresold) {
            Residue *residue = new Residue(aa4,ires);
            res = residue;
            frag->addResidue(residue);
            residue->firstAtomPtr(atm);
            iresold = ires;
         }
         atm->atomResidue(res);
         p_useAtom.push_back(true);
         atomMap.push_back(p_numUsedAtoms);
         p_numUsedAtoms++;
      } else {
         atomMap.push_back(-1);
         p_useAtom.push_back(false);
      }
   }

   // Create TAtm pointers for maximum number of solvent atoms and
   // add these to atom list in fragment.

   if (p_maxSolvMol <= 0) {
      p_useSolvent = false;
      p_hasSolvent = false;
   } else {
      p_hasSolvent = true;
   }

   if (p_useSolvent) {
      // add solvent starts here 
      for (i=0; i<p_maxSolvMol; i++) {
         ires++;
         Residue *residue = new Residue(solventName.c_str(),ires);
         frag->addResidue(residue);
         for (j=0; j<p_solventAtoms; j++) {
            TAtm* atm = new TAtm(solventSymbol[j].c_str(),coords);
            frag->addAtom(atm);
            atm->atomName(solventAtomName[j].c_str());
            atm->atomResidue(residue);
            if (j == 0) residue->firstAtomPtr(atm);
         }
      }
   } // add solvent ends here

   // Read in solvent bond indices and store them temporarily.

   for (i=0; i<p_solventBonds; i++) {
      p_stream.getline(buf,MAX_LINE);
      if (!p_stream.good()) {
         frag->clear();
         throw IOException("EOF reading solvent bonds for trajectory header",
                           WHERE);
      }
      cptr = buf;
      strncpy(i8,cptr,8);
      solventIbnd.push_back(atoi(i8)-1); 
      cptr += 8;
      strncpy(i8,cptr,8);
      solventJbnd.push_back(atoi(i8)-1); 
   }

   // Read in solute bond indices and use them to create a TBond pointer.
   // Then add them to bond list in fragment.
   TBond *thebond;
   for (i=0; i<p_soluteBonds; i++) {
      p_stream.getline(buf,MAX_LINE);
      if (!p_stream.good()) {
         frag->clear();
         throw IOException("EOF reading solute bonds for trajectory header",
                           WHERE);
      }
      cptr = buf;
      strncpy(i8,cptr,8);
      j = atoi(i8) - 1;
      cptr += 8;
      strncpy(i8,cptr,8);
      k = atoi(i8) - 1;
      if (p_useAtom[j] && p_useAtom[k]) {
         j = atomMap[j];
         k = atomMap[k];
         thebond = frag->addBond(j,k,1.0);
         thebond->displayStyle(DisplayStyle(DisplayStyle::CPK));
      }
   }

   // Add bonds to account for maximum number of solvent molecules

   if (p_useSolvent) {
      int offset = p_numUsedAtoms;
      for (i=0; i<p_maxSolvMol; i++) {
         for (j=0; j<p_solventBonds; j++) {
            k = solventIbnd[j] + offset;
            l = solventJbnd[j] + offset; 
            thebond = frag->addBond(k,l,1.0);
            thebond->displayStyle(DisplayStyle(DisplayStyle::WIRE));
         }
         offset += p_solventAtoms;
      }
   }
   
   if (p_solventAtoms>0) {
     frag->setFirstSolventAtomIndex(p_numUsedAtoms);
   }
}


void TrajectoryReaderNWChem::indexFiles()
{
  // clear old indices
  p_stepToFileName.clear();
  p_stepToSeekPosition.clear();
  p_hasVelocities = false;
  p_hasForces = false;

  char buf[MAX_LINE];
  ifstream stream;
  size_t i;
  char* cptr;
  //bool solventCoords;
  bool solventVel;
  bool solventFrc;
  //bool solventPol;
  //bool soluteCoords;
  bool soluteVel;
  bool soluteFrc;
  //bool solutePol;
  vector<string>::iterator fileName;

  for (fileName = p_fileNames.begin();
       fileName != p_fileNames.end();
       ++fileName) {
    stream.open(fileName->c_str());
    while (stream.good()) {
      stream.getline(buf, 256);
      if (strncmp(buf, "frame", 5) == 0) {
        p_stepToFileName.push_back(*fileName);
        p_stepToSeekPosition.push_back(stream.tellg());
        // skip next four lines
        for (i=0; i<4; ++i) {
          //stream.getline(buf, 256);
          stream.ignore(256, '\n');
        }

        // Find out whether or not to read in solvent and/or solute
        // molecules and whether or not to include velocities. Also
        // find out how many solvent molecules are present in this frame.
        stream.getline(buf,MAX_LINE);
        cptr = buf;
       
        // Check to see if frame is in new format.
        if (toupper(*(cptr+7)) == 'T' || toupper(*(cptr+7) == 'F')) {
           //solventCoords = toupper(*cptr++) == 'T';
           cptr++;
           solventVel =    toupper(*cptr++) == 'T';
           solventFrc =    toupper(*cptr++) == 'T';
           //solventPol =    toupper(*cptr++) == 'T';
           cptr++;
           //soluteCoords =  toupper(*cptr++) == 'T';
           cptr++;
           soluteVel =     toupper(*cptr++) == 'T';
           soluteFrc =     toupper(*cptr++) == 'T';
           //solutePol =     toupper(*cptr++) == 'T';
           cptr++;
        } else if (toupper(*(cptr+5)) == 'T' || toupper(*(cptr+5) == 'F')) {
           // Maintain compatibility with old trajectory files.
           //solventCoords = toupper(*cptr++) == 'T';
           cptr++;
           solventVel =    toupper(*cptr++) == 'T';
           solventFrc =    toupper(*cptr++) == 'T';
           //solventPol =    false;
           //soluteCoords =  toupper(*cptr++) == 'T';
           cptr++;
           soluteVel =     toupper(*cptr++) == 'T';
           soluteFrc =     toupper(*cptr++) == 'T';
           //solutePol =     false;
        } else {
           // Maintain compatibility with really old trajectory files.
           //solventCoords = toupper(*cptr++) == 'T';
           cptr++;
           solventVel =    toupper(*cptr++) == 'T';
           solventFrc =    false;
           //solventPol =    false;
           //soluteCoords =  toupper(*cptr++) == 'T';
           cptr++;
           soluteVel =     toupper(*cptr++) == 'T';
           soluteFrc =     false;
           //solutePol =     false;
        }
        p_hasVelocities |= soluteVel | solventVel;
        p_hasForces |= soluteFrc | solventFrc;
      }
    }
    stream.close();
    stream.clear();
  }
}


void TrajectoryReaderNWChem::changeStreamForStep(const long& step)
        throw (IOException)
{
  if (p_fileName != p_stepToFileName[step]) {
    p_fileName = p_stepToFileName[step];
  }
  if (p_stream.is_open()) {
    p_stream.close();
  }
  p_stream.open(p_fileName.c_str());
  if (!p_stream.good()) {
    throw IOException("could not open file", WHERE);
  }
  p_stream.seekg(p_stepToSeekPosition[step]);
  if (!p_stream.good()) {
    throw IOException("could not seek file", WHERE);
  }
}

