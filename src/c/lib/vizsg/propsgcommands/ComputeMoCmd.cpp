///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: ComputeMoCmd.C
//
//
// DESIGN:
//   This file contains methods dealing with the computation of a 
//   MO-type field.
//
// Verification:
//   There is a test page at http://www.emsl.pnl.gov/ecce/testing/MO.html
//   which outlines the verification process.
//
// IMPLEMENTATION NOTES:
//   (from Theresa)
//     ToDo:
//     1. accommodating sphericals
//
//   (from Hugh)
//   The radial versus angular stuff does not appear in the current code, it
//   is just the functional form of a basis function.  
//   So a Cartesian dxy function, which would have the form 
//       A * x * y * SUM(i) Ci exp(ai * r * r)
//   where A is some normalization constant and Ci is the ith contraction
//   coefficient, and ai is the associated "exponent" coefficient, could be
//   written as a 
//      "radial part"  SUM(i) Ci exp(ai * r * r) * angular part x * y.  
//   The normalization A could go into either term, as convenient.
//   This is important because 
//      -all of the cost is in computing the contracted radial part, and it
//       is the same for all functions in the shell
//      - the problem of Cartesians vs Sphericals only involves the angular
//        part of the problem.
//   So if you are calculating a d shell, the radial part is done first, then
//   the 5 or six angular parts, then they can be combined.
//   (from Theresa)
//   The radial versus angular part is in place.  The spherical part still
//   needs to be taken care of.
//
/////////////////////////////////////////////////////////////////////////////
//#define DEBUG

#include <math.h>

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"
#include "util/InternalException.H"
#include "util/CancelException.H"
#include "util/Receiver.H"
#include "util/ProgressMonitor.H"

#include "tdat/TGBSAngFunc.H"
#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/SingleGrid.H"

#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSConfig.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"

#include "viz/ComputeMoCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

//
// Last remaining macro 
//
#define  MAX(x,y)        (((x) > (y)) ? (x) : (y))

////////////////////////////////////////////////////////////////////////////
// Class Statics
////////////////////////////////////////////////////////////////////////////

double ComputeMoCmd::pi = acos(-1.0);
double ComputeMoCmd::pi32 = ComputeMoCmd::pi * sqrt(ComputeMoCmd::pi);
double ComputeMoCmd::sNormFactor= pow(2.0,0.75)*pow(5.568327996831707,-0.5);
double ComputeMoCmd::pNormFactor= pow(2.0,1.75)*pow(5.568327996831707,-0.5);
double ComputeMoCmd::dNormFactor= pow(2.0,2.75)*pow(3*5.568327996831707,-0.5);
double ComputeMoCmd::fNormFactor= pow(2.0,3.75)*pow(15*5.568327996831707,-0.5);
double ComputeMoCmd::gNormFactor= pow(2.0,4.75)*pow(105*5.568327996831707,-0.5);
double ComputeMoCmd::hNormFactor= pow(2.0,5.75)*pow(945*5.568327996831707,-0.5);
double ComputeMoCmd::iNormFactor= pow(2.0,6.75)*pow(10395*5.568327996831707,-0.5);



/**
 * Constructor.
 * Calls init method.
 * @param name - command name
 */
ComputeMoCmd::ComputeMoCmd(
      const string& name, 
      Receiver * receiver, 
      IPropCalculation *calc,
      ProgressMonitor *monitor) 
: PropCmd(name, receiver, calc)
{
   p_monitor = monitor;
   init();
}



/**
 * Destructor.
 */
ComputeMoCmd::~ComputeMoCmd()
{ 
}



/**
 * Add command parameters with their default values.
 */
void ComputeMoCmd::init(void)
{
   addParameter(new CommandParameter("FieldType", "MO"));

   addParameter(new CommandParameter("SelectedMO", 0));

   addParameter(new CommandParameter("FieldMin", 0.0));
   addParameter(new CommandParameter("FieldMax", 0.0));

   addParameter(new CommandParameter("resX", 10));
   addParameter(new CommandParameter("resY", 10));
   addParameter(new CommandParameter("resZ", 10));

   addParameter(new CommandParameter("fromX", -1.0));
   addParameter(new CommandParameter("fromY", -1.0));
   addParameter(new CommandParameter("fromZ", -1.0));

   addParameter(new CommandParameter("toX", 1.0));
   addParameter(new CommandParameter("toY", 1.0));
   addParameter(new CommandParameter("toZ", 1.0));

   addParameter(new CommandParameter("CoefCutoff", 0.0));

   // Not sure how this differs from FieldType but both are used - KLS Feb 08
   addParameter(new CommandParameter("Type", "MO"));
   addParameter(new CommandParameter("Code", "unknown"));

}



/** 
 * Makes a field with all values set to zero.
 * @param int size of field.
 * @return non-zero field pointer
 */
float *ComputeMoCmd::makeField(int size)
{
  float *ret = new float[size];
  zeroField(ret,size);
  return ret;
}



/**
 * zero out the grid field.
 * @param field - the field to reset
 * @param length - size of the field array
 */
void ComputeMoCmd::zeroField(float *field, int length)
{
   for (int idx=0; idx<length; idx++) {
      field[idx] = 0.0;
   }
}



/////////////////////////////////////////////////////////////////////////////
// Description
//   Report progress - handle interrupt.
//
//   TODO - this probably should be per shell not just per atom or
//          some smaller hunk of the computation.
/////////////////////////////////////////////////////////////////////////////
bool ComputeMoCmd::moCheckInterrupt
(
  const char *type, 
  const char *atom,
  const char *shell,
  int cur, 
  int max,
  unsigned long MO
)
{
  bool ret=false;

  static char msg[120];
  // Check for user interrupt
  if (p_monitor != 0) {

    // Only show work on these shells since they are the only ones
    // we do.
    // TODO remove this when the rest are implemented.
    if (strcmp(shell,"s")==0 || strcmp(shell,"p")==0 ||
        strcmp(shell,"d")==0 || strcmp(shell,"f")==0 ||
        strcmp(shell,"g")==0 ) {
      sprintf(msg,"%s: AO %d%s(%s) - %d of %d for MO %lu",
              type,cur+1,atom,shell,cur+1,max,MO);
    }
    ret = p_monitor->isInterrupted(msg, (int)((cur+1)*p_percentFraction));
  }

  return ret;
}



/**
 *
 */
bool ComputeMoCmd::squareCheckInterrupt
(
  const char *type, 
  int max,
  int MO
)
{
   bool ret=false;

   static char msg[120];
   // Check for user interrupt
   if (p_monitor != 0) {

      sprintf(msg,"%s: MO %d/%d", type,MO,max);
      ret = p_monitor->isInterrupted(msg, (int)ceil(MO*p_percentFraction));
   }

   return ret;
}



/**
 *
 */
double ComputeMoCmd::computeFieldValue (
      float const *field,
      int size,
      float deltaX,
      float deltaY,
      float deltaZ)
{
   double ret=0.0;
   for (int idx = 1; idx < size; idx++) {
      ret += field[idx] * field[idx];
   }
   ret = ret * (deltaX * deltaY * deltaZ);
#if (!defined(INSTALL) && defined(DEBUG))
   cout << "Field Value " << ret << endl;
#endif
   return ret;
}



/**
 *   Compute min,max,absmax of a field.
 */
void ComputeMoCmd::computeMinMax (
      float const *field,
      int size,
      float& min,
      float& max,
      float& absMax)
{
  min = max = field[0];
  for (int idx = 1; idx < size; idx++) {
    if (field[idx] < min) min = field[idx];
    if (field[idx] > max) max = field[idx];
  }

  // Determine absolute value max
  absMax = fabs(min);
  if (fabs(max) > fabs(min))
  absMax = fabs(max);
#if (!defined(INSTALL) && defined(DEBUG))
	cout<<"field min,max = "<<min<<", "<<max<<endl;
#endif //DEBUG
}



/**
 * Description
 *   Computes the grid values for either an MO or for the density.
 *
 *   Got rid of all of the macros except MAX.
 *   Code has been thoroughly tested against Gaussian cube files.
 *   Sphericals are not implemented yet.
 */
bool ComputeMoCmd::execute() throw(EcceException)
{
  bool ret = false ;

  SGContainer *cvsg = dynamic_cast<SGContainer*>(getReceiver());

  unsigned long idx,idy,idz;
  double xval,yval,zval;
  double xrel,yrel,zrel;
  double xrel2,yrel2,zrel2;
  double rr;

  // Field type to compute  {MO, Density, Spin Density}
  string fieldType = getParameter("FieldType")->getString();

  // Selected MO - currently a single MO
  int selectedMO = getParameter("SelectedMO")->getInteger();

  // What type of MO selected: MO or MOBETA
  string moType = getParameter("Type")->getString();

  string codename = getParameter("Code")->getString();

  double coefCutoff = getParameter("CoefCutoff")->getDouble();

  //cout << "fieldType " << fieldType << " " << selectedMO << " " << moType << " " << codename << " " << coefCutoff << endl;

  // Generate a name/unique id for caching purposes
  char key[64];
  if (fieldType == "MO") {
    sprintf(key, "%s %s %d", fieldType.c_str(), moType.c_str(), selectedMO);
  } else {
    sprintf(key, "%s", fieldType.c_str() );
  }

  bool doComputation = false;

  SingleGrid *grid = cvsg->getMOGrid(key);
  if (grid == 0) {
    doComputation = true;
    grid = new SingleGrid();
    grid->name(key);
  } else {
    if (grid->gridSize() != getRequestedGridSize()) {
      doComputation = true;
    }
  }

  if (doComputation) {
    // Create new grid and add it to the cache right away
    cvsg->addMOGrid(grid);

    int idxField = 0;
    unsigned long gridRes = 0;
    float xDelta, yDelta, zDelta;

    // Initialize new grid
    setGrid(grid,gridRes,xDelta,yDelta,zDelta);
    grid->type(fieldType);

    bool doingDensity = (fieldType=="Density");
    bool doingSpinDensity = (fieldType=="Spin Density");

    // Alloc the field.  Use the local field variable as short-hand 
    // and to reduce pointer dereferencing. 
    grid->setFieldData(makeField(gridRes));
    float *field = grid->fieldData();

    // For the density, we need to have at least one other field to hold
    // temporary results.
    float *density = (float*)0;
    float *fieldBeta = (float*)0;

    IPropCalculation *calc = getCalculation();
    INTERNALEXCEPTION(calc, "No calculation object!");

    ICalculation *escalc = dynamic_cast<ICalculation*>(calc);
    INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");

    // Use this for checking if we should bail on the computation
    bool interrupted = false ;

    if (calc != 0) 
    {
      ret = true;

      const JCode *code = escalc->application();
      INTERNALEXCEPTION(escalc, "No code - will be crashing...");

/* WHAT?? */
      // copy fragment so we can  overwrite with the coordinates for the
      // appropriate step.
      const SGFragment *sgfrag = cvsg->getFragment();

      // The calculation is dependent on the atoms' coordinates
      unsigned long  numAtoms    = sgfrag->numAtoms();
      double*  atomCoords  = sgfrag->coordinates();

      // Get MOs and orbital occupation values
      PropTable *moCoefs = (PropTable*) calc->getProperty("MO");

      // ORBOCC may not be available but its only needed for
      // density calc anyway.
      PropVector *orbOcc = (PropVector*) calc->getProperty("ORBOCC");
      vector<double> occ ;
      if (orbOcc) occ = orbOcc->values();

      // Find out if we have a UHF calculation.  If we do,
      // Get Beta MOs and orbital occupation values
      PropTable *moCoefsBeta = (PropTable*) calc->getProperty("MOBETA");
      bool uhfCalc = (moCoefsBeta != (PropTable*)0);
      PropVector *orbOccBeta;
      vector <double> occBeta;
      if (uhfCalc && (doingDensity||doingSpinDensity)) {
        orbOccBeta = (PropVector*) calc->getProperty("ORBOCCBETA");
        if (orbOccBeta) occBeta = orbOccBeta->values();
      } else if (moType == "beta") {
        // If we are doing UHF and a BETA orbital has been chosen,
        // change the default values to the beta ones. - TLW
        moCoefs = (PropTable*) calc->getProperty("MOBETA");
        orbOcc = (PropVector*) calc->getProperty("ORBOCCBETA");
        if (orbOcc) occ = orbOcc->values();
      }

      ///////////////////////////////////////////////////////////////////////
      // NWChem can return fewer MOs, MO energies, ... than the number of
      // basis functions if the basis is somehow redundant.  In this case,
      // the number of basis functions is still the full size, but the number
      // of MOs is less than the number of orbitals.  So we get the number
      // of MOs from the number of energies returned, not the row dimension
      // of the MO Coefficient matrix.  The latter will be the wrong size,
      // since the Perl parse script does not have this information
      ///////////////////////////////////////////////////////////////////////

      // Figure out which molecular orbitals to compute
      unsigned long startMO = 0, endMO = 0;
      if (fieldType=="Density" || fieldType=="Spin Density") {
        startMO = 0;
        // Find the HOMO
        bool foundHOMO = false;
        idx = 0;
        int nrows = orbOcc->rows();
        while ((!foundHOMO) && (idx<nrows)) {
          if (occ[idx] == 0) { 
            foundHOMO = true;
            endMO = --idx;
          } 
          idx++;
        }
        if (!foundHOMO) endMO = --idx; // all orbitals are occupied

        density = makeField(gridRes);

        if (uhfCalc) fieldBeta = makeField(gridRes);
      } else  // This is an MO calculation.
        endMO = startMO = selectedMO - 1;  

#if (!defined(INSTALL) && defined(DEBUG))
      cout << "endMO is " << endMO << endl;
#endif

      //=========================================================
      // Get basis set config info for this calculation:
      //=========================================================
      TGBSConfig *gbsConfig = escalc->gbsConfig();
      //     bool usingCartesian = gbsConfig->cartesian();

      // Assign order based on the "code dependence"
      // This is currently done by getting the angle function orders
      // from the .edml file corresponding to the code for this
      // calculation.
      const JCode *jcode = CodeFactory::lookup(codename.c_str());
      EE_RT_ASSERT(jcode, EE_FATAL, "Unrecognized code");
      TGBSAngFunc *angfunc = jcode->getAngFunc(gbsConfig->coordsys());
      int maxShell = angfunc->maxShells();
#if 000
      int i, cshell[5];
      for (i = 0; i<maxShell; i++) {
        cshell[i] = angfunc->numFuncs(i);
      }
#endif

      ////////////////////////////////////////////////////////////////////////
      // shell lengths are 2l+1 for spherical and (l+1)(l+2)/2 for Cartesian
      // Also set the beginning value of the shells - TLW
      ////////////////////////////////////////////////////////////////////////

      const char *shells[] = {"s","p","d","f","g","h","i"};

      int length_shell[7] = { 1, 3, 5, 7, 9, 11, 13 };
      if (gbsConfig->coordsys() == TGaussianBasisSet::Cartesian) {
        length_shell[2] = 6;    length_shell[3] = 10;
        length_shell[4] = 15;   length_shell[5] = 21;
        length_shell[6] = 28;
      }

      unsigned long ialpha;

      // Shouldn't be here if we don't have one of these.
      if (gbsConfig != (TGBSConfig *)0) {


        // Get the atom list
        vector<TAtm*> *atoms = sgfrag->atoms();

        // The "% complete" feedback slider is updated on a per atom
        // fashion.
        // This should be redone on a finer scale in the future.
        // Hugh, is this your comment?  If so what did you have in mind?
        p_percentFraction = 100.0/numAtoms;

        bool moupdates = doingDensity || doingSpinDensity;
        if (moupdates) p_percentFraction = 100.0/(endMO+1);

        string atomID;

        // Loop over the MOs
        for (unsigned long idxMO = startMO; idxMO <= endMO; idxMO++) {
#if (!defined(INSTALL) && defined(DEBUG))
          cout << "idxMO is " << idxMO << endl;
#endif
          //Experimental - only update for each MO 
          if (moupdates) {
            interrupted = squareCheckInterrupt
              (fieldType.c_str(), endMO+1,idxMO+1);

            if (interrupted) {
              zeroField(field,gridRes);
              break;
            }
          }

          zeroField(field,gridRes);
          if (uhfCalc && (doingDensity||doingSpinDensity))
            zeroField(fieldBeta,gridRes);

          //=========================================================
          // Look at basis set for this calculation
          //=========================================================


          unsigned long idxCoef = 0;
          unsigned long idxAtomCoord = 0;

          /////////////////////////////////////////////////////////////
          // -- Loop - over atoms
          /////////////////////////////////////////////////////////////
          int i_basis_func = 0;
          for (unsigned long idxAtom=0; 
              idxAtom<numAtoms && !interrupted; idxAtom++) {
            // Get the symbol - it'll come in handy in many places
            atomID = (*atoms)[idxAtom]->atomicSymbol();
#if (!defined(INSTALL) && defined(DEBUG))
            cout << "MO atom: "<<atomID<<"("<<idxAtom+1<<")"<<endl;
#endif

            /**
             * skip atom if it is a fixed point charge
             */
            if ((*atoms)[idxAtom]->getBehaviorType() == TAtm::POINT) {
              idxAtomCoord+=3;
              continue;
            }

            // make a list of GBS pointers for the current atom
            vector <const TGaussianBasisSet*> gbslist = 
              gbsConfig->getGBSList(atomID);

#if (!defined(INSTALL) && defined(DEBUG))
            // Print out exponents and coefficients in basis
            for (int n_gbs = 0; n_gbs<gbslist.size(); n_gbs++) {
              for (unsigned long n_ics=0; 
                  n_ics<gbslist[n_gbs]->num_contracted_sets(atomID.c_str()); n_ics++)  {
                Contraction_ *cont_tmp = gbslist[n_gbs]->getContraction(atomID.c_str(),n_ics);
                vector<double> n_alpha =
                  gbslist[n_gbs]->exponents(atomID.c_str(),n_ics);
                int nAlpha = n_alpha.size();

                vector<TGaussianBasisSet::AngularMomentum> nFuncTypes = 
                  gbslist[n_gbs]->func_types(atomID.c_str(),n_ics);
                int n_FuncTypes = nFuncTypes.size();
                for (int ncol=0; ncol<n_FuncTypes; ncol++) {
                  for (int nalpha=0; nalpha<nAlpha; nalpha++) {
                    cout << "Exponent: "<<n_alpha[nalpha]<<" Contraction coefficient: "
                      <<cont_tmp->coefficient(nalpha,ncol)<<endl;
                  }
                }
              }
            }
#endif
            // Do the normalization step
            vector <TGaussianBasisSet*> normalized = 
              normalize(atomID, gbslist, *gbsConfig, code);

            /////////////////////////////////////////////////////////////
            // Loop over basis sets on current atom (orbital, polarization,.)
            /////////////////////////////////////////////////////////////
            int gbsSize = normalized.size() ;
            for(int gbs_index = 0; gbs_index < gbsSize; gbs_index++) {
              const TGaussianBasisSet* gbs = normalized[gbs_index];

              int numContractedSets = 
                gbs->num_contracted_sets(atomID.c_str());

              // For each contracted basis set:
              for (unsigned long ics=0; 
                  ics<numContractedSets && !interrupted; ics++) 
              {
                vector<double> alpha = gbs->exponents(atomID.c_str(),ics);
                int numAlpha = alpha.size();

                // funcTypes: rf. ChemTypes.h enum: s_shell,p_shell, etc.
                vector<TGaussianBasisSet::AngularMomentum> funcTypes = 
                  gbs->func_types(atomID.c_str(),ics);
                int numFuncTypes = funcTypes.size();
                Contraction_ *cont=gbs->getContraction(atomID.c_str(),ics);

                ///////////
                // For each orbital in the contracted set:
                ///////////
                for (int icol=0; icol<numFuncTypes; icol++) {
#if (!defined(INSTALL) && defined(DEBUG))
                  cout << "inside the icol loop with icol of " << icol << endl;
#endif

                  int shell_type = funcTypes[icol];

                  // In this case update more frequently
                  if (!moupdates) {
                    interrupted = moCheckInterrupt
                      (fieldType.c_str(),atomID.c_str(),
                       shells[shell_type],
                       idxAtom,numAtoms,idxMO+1);

                    if (interrupted) {
                      zeroField(field,gridRes);
                      break;
                    }
                  }

                  // Take care of not handling sphericals
                  if (shell_type > maxShell-1) {
                    idxCoef += length_shell[shell_type];
                    continue;
                  }

#if (!defined(INSTALL) && defined(DEBUG))
                  // Get constant information for the given shell
                  double NormFactor = getNormFactor(shell_type);
                  double alphapow = getalphapow(shell_type);
                  cout << "NormFactor and alphapow are " << NormFactor << " " <<
                    alphapow << endl;
#endif

                  double maxCoef = 0.0;
                  for (int moidx=idxCoef; 
                      moidx<(idxCoef+length_shell[shell_type]); 
                      moidx++) 
                    maxCoef = MAX
                      ( fabs(moCoefs->value(idxMO,moidx)), maxCoef);


                  // Only compute contributions from shells whose maximum
                  // coefficient is greater than the cutoff. Otherwise, skip the
                  // shell
                  if (maxCoef > coefCutoff) {
#if (!defined(INSTALL) && defined(DEBUG))
                    cout << "Ready to start calculating on the grid" << endl;
#endif
                    // Calculate over the grid points
                    // It would be somewhat useful to move this xrel...zrel
                    // stuff out to the outer atom loop and store the
                    // values.
                    double atob = 1/0.52917724924; 
                    idxField = 0; 
                    double radialPart = 0.0;
                    double alphar = 0.0;
#if (!defined(INSTALL) && defined(DEBUG))
                    if (idxCoef==0) {
                      cout << "origin_x: "<<grid->origin()[0]<<endl;
                      cout << "origin_y: "<<grid->origin()[1]<<endl;
                      cout << "origin_z: "<<grid->origin()[2]<<endl;
                      cout << "corner_x: "<<grid->corner()[0]<<endl;
                      cout << "corner_y: "<<grid->corner()[1]<<endl;
                      cout << "corner_z: "<<grid->corner()[2]<<endl;
                      cout << "grid_x: "<<grid->dimensions()[0]<<" delta_x: "<<xDelta<<endl;
                      cout << "grid_y: "<<grid->dimensions()[1]<<" delta_y: "<<yDelta<<endl;
                      cout << "grid_z: "<<grid->dimensions()[2]<<" delta_z: "<<zDelta<<endl;
                    }
#endif
                    for (idz=0; idz<grid->dimensions()[2]; idz++) { 
                      zval = grid->origin()[2] + idz*zDelta; 
                      zrel = zval - atomCoords[idxAtomCoord+2]; 
                      zrel = zrel * atob;
                      zrel2 = zrel * zrel; 
                      for (idy=0; idy<grid->dimensions()[1]; idy++) { 
                        yval = grid->origin()[1] + idy*yDelta; 
                        yrel = yval - atomCoords[idxAtomCoord+1]; 
                        yrel = yrel * atob;
                        yrel2 = yrel * yrel; 
                        for (idx=0; idx<grid->dimensions()[0]; idx++) { 
                          xval = grid->origin()[0] + idx*xDelta; 
                          xrel = xval - atomCoords[idxAtomCoord]; 
                          xrel = xrel * atob;
                          xrel2 = xrel * xrel;
                          // Compute R^2
                          rr = xrel2 + yrel2 + zrel2; 

                          // Calculate the radial part for the shell.
                          // Check argument of exponent first and don't
                          // bother to evaluate exponential function if
                          // value is going to be too small (Using 10e-30
                          // for now). Might want to eventually extend this
                          // to account for angular momentum prefactors.
                          radialPart = 0.0;
                          for (ialpha=0; ialpha<numAlpha; ialpha++) {
                            alphar = alpha[ialpha]*rr;
                            if (alphar < 70.) {
                              radialPart += cont->coefficient(ialpha,icol) * 
                                exp(-alphar);
                            } 
#if (!defined(INSTALL) && defined(DEBUG))
                            if (idx==0 && idy == 0 && idz ==0) {
                              cout << "Exponent: "<<alpha[ialpha]<<" Contraction coefficient: "
                                <<cont->coefficient(ialpha,icol)<<" Shell length: "
                                <<length_shell[shell_type]<<endl;
                            }
#endif
                          }


                          // Get angular part and put into the field
                          // This runs over the degeneracy of the shell.
                          // This will be different for sphericals, but currently
                          // works for cartesians. - TLW

                          for (int deg=0;deg<length_shell[shell_type];
                              deg++,idxCoef++) {
                            double angularPart;
                            if (shell_type < maxShell) {
                              angularPart = angfunc->evaluateAMFunc(shell_type,
                                  deg, xrel, yrel, zrel);
                              // have to do a bit more normalization for special cases
                              double oddNormalize = 
                                getoddNormalize(shell_type, deg, angfunc);
                              angularPart *= oddNormalize;
                            } else {
                              angularPart = 0.0;
                            }

                            // Now contract with MO coefficients
                            float contrib;
#if (!defined(INSTALL) && defined(DEBUG))
                            if (idxField == 62) {
                              cout << "MOcoef, radialPart, angularPart " <<
                                moCoefs->value(idxMO,idxCoef) << " " <<
                                radialPart << " " << angularPart << endl;
                            }
                            if (idx==0 && idy == 0 && idz ==0) {
                              i_basis_func++;
                              cout << "     MO["<<i_basis_func<<"] coef: "
                                << moCoefs->value(idxMO,idxCoef)<<endl;
                            }
#endif
#if (!defined(INSTALL) && defined(DEBUG))
                            if (idx==22 && idy == 22 && idz ==20) {
                              i_basis_func++;
                              if (fabs(radialPart*angularPart) > 1.0e-6) {
                                cout << " Contrib["<<i_basis_func<<"]: "
                                  << radialPart*angularPart<< " angular: "<< angularPart<<
                                  " radial: "<< radialPart<< endl;
                              }
                            }
#endif

                            contrib = moCoefs->value(idxMO,idxCoef) * radialPart *
                              angularPart;
                            field[idxField] += contrib;
                            if (uhfCalc && (doingDensity || doingSpinDensity)) {
                              contrib = moCoefsBeta->value(idxMO,idxCoef) * 
                                radialPart * angularPart;
                              fieldBeta[idxField] += contrib;
                            }

                          } // for deg

                          idxCoef -= length_shell[shell_type];    
                          idxField++;
                        }   // x grid
                      }     // y grid
                    }       // z grid
                  }         // if cutoff
                  else {
                             i_basis_func += length_shell[shell_type];
                  }
#if (!defined(INSTALL) && defined(DEBUG))
                  cout << "at the end of the grid" << endl;
#endif
                  idxCoef += length_shell[shell_type];    

                }  // end  icol < numFuncTypes
              }  // end  ics < numContractedSets
            } // end for gbs_index < normalized.size()

            idxAtomCoord+=3;

            // clean up all our pointers
            for (int g = 0; g < gbsSize; g++)
              delete normalized[g] ;

          } // end for (idxAtom...


          // The above gets the MO on the grid. Now we have to handle the
          // the squaring of the MO to get the density if appropriate.
          if (doingDensity) {
            double occnum = occ[idxMO];
            for (idxField=0; idxField<gridRes; idxField++) 
              density[idxField] += 
                occnum * field[idxField] * field[idxField];
            if (uhfCalc) {
              occnum = occBeta[idxMO];
              for (idxField=0; idxField<gridRes; idxField++) 
                density[idxField] += occnum * fieldBeta[idxField] *
                  fieldBeta[idxField];
            }
          } else if (doingSpinDensity) {
            double occnum = occ[idxMO];
            for (idxField=0; idxField<gridRes; idxField++)
              density[idxField] +=
                occnum * field[idxField] * field[idxField];
            if (uhfCalc) {
              occnum = occBeta[idxMO];
              for (idxField=0; idxField<gridRes; idxField++)
                density[idxField] -= occnum * fieldBeta[idxField] *
                  fieldBeta[idxField];
            }
          }  // if (doingDensity)
        }  // end for (idxMO...
        delete atoms;
        delete angfunc;

        // move the density over to the expected field
        if (doingDensity || doingSpinDensity) {
          for (idxField=0; idxField<gridRes; idxField++)  {
            field[idxField] = density[idxField];
          }
        }

        if (!interrupted) {

          grid->findMinMax();
          grid->absFieldMax(grid->fieldMax());
          computeFieldValue(field,gridRes,xDelta,yDelta,zDelta);

        } else {
          cvsg->removeMOGrid(grid->name());
        }

        // Do some cleanup
        if (doingDensity) {
          delete [] density;
        }
        if ((doingDensity||doingSpinDensity) && uhfCalc) {
          delete [] fieldBeta;
        }

#if (!defined(INSTALL) && defined(DEBUG))
        // dump grid 
        cout << "new grid structure" << endl;
        int inc = grid->resolution[0];
        cout << "dumping grid " << gridRes << " " << inc;
        for (idx=0; idx<gridRes; idx++) {
          if ((idx % inc) == 0) cout << endl;
          cout << field[idx] << " "; // was %8.6lf w/printf
        }
        cout << endl;
#endif

      } // we have a GBSConfig

    }

    if (interrupted) {
       throw CancelException(WHERE);
    }
  }

  cvsg->setCurrentGrid(grid);

  return ret;
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   Set up the initial information for the grid and the resolution variables.  
//   This routine does NOT create the field.
/////////////////////////////////////////////////////////////////////////////
void ComputeMoCmd::setGrid(SingleGrid *grid, unsigned long& gridRes, 
                           float& xDelta, float& yDelta, float& zDelta)
{
   int resX = getParameter("resX")->getInteger();
   int resY = getParameter("resY")->getInteger();
   int resZ = getParameter("resZ")->getInteger();

   double xStart = getParameter("fromX")->getDouble();
   double yStart = getParameter("fromY")->getDouble();
   double zStart = getParameter("fromZ")->getDouble();

   double xEnd = getParameter("toX")->getDouble();
   double yEnd = getParameter("toY")->getDouble();
   double zEnd = getParameter("toZ")->getDouble();

  //cout << "grid " << resX << " " << resY << " " << resZ << ":" << xStart << " " << yStart << " " << zStart << ":" << xEnd << " " << yEnd << " " << zEnd << endl;
   grid->dimensions()[0] = resX;
   grid->dimensions()[1] = resY;
   grid->dimensions()[2] = resZ;
   grid->origin()[0] = xStart;
   grid->origin()[1] = yStart;
   grid->origin()[2] = zStart;
   grid->corner()[0] = xEnd;
   grid->corner()[1] = yEnd;
   grid->corner()[2] = zEnd;
   gridRes = grid->dimensions()[0] *
      grid->dimensions()[1] *
      grid->dimensions()[2];

   // Compute deltas based on bounding box and resolution
   xDelta = (grid->corner()[0]-grid->origin()[0])/(grid->dimensions()[0]-1);
   yDelta = (grid->corner()[1]-grid->origin()[1])/(grid->dimensions()[1]-1);
   zDelta = (grid->corner()[2]-grid->origin()[2])/(grid->dimensions()[2]-1);

}

/**
 * Get the size of the requested grid.
 * Used to determine if cache entry should be recomputed.
 */
int ComputeMoCmd::getRequestedGridSize() 
{
   int resX = getParameter("resX")->getInteger();
   int resY = getParameter("resY")->getInteger();
   int resZ = getParameter("resZ")->getInteger();
   return resX * resY * resZ;
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Normalize the atomic basis set coefficients.
//
// Always uses an optimized set of contractions for each basis set.
// (This is accomplished by using a special copy constructor for 
// TGaussianBasisSet that copies contractions as optimized.)
/////////////////////////////////////////////////////////////////////////////

vector <TGaussianBasisSet*> ComputeMoCmd::normalize
(
  string atomID, 
  vector <const TGaussianBasisSet*> gbslist,
  TGBSConfig& gbsConfig, // I had to make this non-const, but it should be
  const JCode* code
)
{
  vector <double> alpha;
  vector<TGaussianBasisSet::AngularMomentum> funcTypes;
  unsigned long ialpha;
  vector <TGaussianBasisSet*> normalized;
  GBSToContInfoMap* infoMap;
  string uniqueKey;
  GBSToContInfoMap::iterator infoIt;
  ContractionInfo* contInfo;

  // TEST - Make a new config for testing:
  TGBSConfig test;
  test.optimize(true);
  TGBSGroup* group = new TGBSGroup();
  //

  // Do the normalization step
  vector<const TGaussianBasisSet*>::const_iterator gbscurs1 ; 
  for (gbscurs1 = gbslist.begin(); 
        gbscurs1!=gbslist.end(); gbscurs1++)
  {
    const TGaussianBasisSet *gbs = *gbscurs1 ;
    
    // Get the delete/uncontract meta data for that element and basis set
    // (used in amica, but somebody else may have used it)
    infoMap = gbsConfig.getContractionInfoMap(atomID);
    uniqueKey = gbs->getUniqueKey();
    contInfo = 0;

    if (infoMap != 0) {
      infoIt = infoMap->find(uniqueKey);   
      if (infoIt != infoMap->end()){ 
        contInfo = &((*infoIt).second);
      }
    }  

    // Make a new basis set that has the contractions correctly
    // optimized:
    TGaussianBasisSet *tmp = new TGaussianBasisSet(*gbs,atomID,
						   gbsConfig.optimize(),
						   code, contInfo);
    // TEST
    group->insertGBS(new TGaussianBasisSet(*tmp));
    //
    unsigned long numContractedSets = 0;
    numContractedSets = tmp->num_contracted_sets(atomID.c_str());

    // For each contracted basis set:
    for (unsigned long ics=0; ics<numContractedSets; ics++)
    {
      alpha = tmp->exponents(atomID.c_str(),ics);
      int numAlpha = alpha.size();
#if (!defined(INSTALL) && defined(DEBUG))
      cout << "numAlpha is " << numAlpha << endl;
#endif

      // funcTypes: rf. ChemTypes.h enum: s_shell,p_shell, etc.
      funcTypes = tmp->func_types(atomID.c_str(),ics);
      int numFuncTypes = funcTypes.size();
      //vector<double> aoCoeffs = tmp->coefficients(atomID,ics);
      Contraction_ *cont = tmp->getContraction(atomID.c_str(), ics);

      //
      // NORMP step; unnormalization of the primitive functions;
      // if contraction coefficients are given in normalized primitive
      // functions, change them for unnormalized primitives
      // This is generally true and so I do not check, may have
      // to check for generalized basis sets. The scale factor inside
      // the switch's sqrt comes from (2l-1)!!/pow(2.0,l) - TLW
      //
      int icol;
      for (icol=0; icol<numFuncTypes; icol++) {
        for (ialpha=0; ialpha<numAlpha; ialpha++) {

          double ee = 2 * alpha[ialpha];
          double facs = pi32 / (ee * sqrt(ee));

          switch ( funcTypes[icol]) {
            case TGaussianBasisSet::s_shell:
              cont->coefficient(ialpha, icol, 
                        cont->coefficient(ialpha,icol) / sqrt(facs) );
              break;
            case TGaussianBasisSet::p_shell:
              cont->coefficient(ialpha,icol,
                       cont->coefficient(ialpha,icol) /
                       sqrt(0.5*facs/ee) );
              break;
            case TGaussianBasisSet::d_shell:
              cont->coefficient(ialpha,icol, 
                 cont->coefficient(ialpha,icol) / sqrt(0.75*facs/(ee*ee)) );
              break;
            case TGaussianBasisSet::f_shell:
              cont->coefficient(ialpha,icol, 
                 cont->coefficient(ialpha,icol) / sqrt(1.875*facs/pow(ee,3)) );
              break;
            case TGaussianBasisSet::g_shell:
              cont->coefficient(ialpha,icol,
                cont->coefficient(ialpha,icol)/sqrt(6.5625*facs/pow(ee,4)) ) ;
              break;
            case TGaussianBasisSet::h_shell:
             cont->coefficient(ialpha, icol,
              cont->coefficient(ialpha,icol) / sqrt(29.5315*facs/pow(ee,5)) );
              break;
            case TGaussianBasisSet::i_shell:
              cont->coefficient(ialpha,icol,
                    cont->coefficient(ialpha,icol)/
                    sqrt(162.421875*facs/pow(ee,6)) );
              break;
            default:
              EE_RT_ASSERT( 0, EE_FATAL,"Unrecognized funcType type");
              break;
          }  // switch
        }    // for ialpha
      }      // for icol

      double dum, snorm;
      // NORMF step; normalize the contracted basis functions
      for (icol=0; icol<numFuncTypes; icol++) {
        snorm = 0.0;
        for (ialpha=0; ialpha<numAlpha; ialpha++) {
          for (int ialpha2=0; ialpha2<=ialpha; ialpha2++) {
            double ee = alpha[ialpha] + alpha[ialpha2];
            double fac = ee*sqrt(ee);
            switch ( funcTypes[icol]) {
              case TGaussianBasisSet::s_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)/fac;
                break;
              case TGaussianBasisSet::p_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)/(2.0*fac*ee);
                break;
              case TGaussianBasisSet::d_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)*3.0/
                      (4.0*fac*pow(ee,2));
                break;
              case TGaussianBasisSet::f_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)*15.0/
                      (8.0*fac*pow(ee,3));
                break;
              case TGaussianBasisSet::g_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)*105.0/
                      (16.0*fac*pow(ee,4));
                break;
              case TGaussianBasisSet::h_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)*945.0/
                      (32.0*fac*pow(ee,5));
                break;
              case TGaussianBasisSet::i_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)*10395.0/
                      (64.0*fac*pow(ee,6));
                break;
              default:
                EE_RT_ASSERT( 0, EE_FATAL,"Unrecognized funcType type");
                break;
            }  // switch
            if (ialpha != ialpha2) dum *= 2.0;
            snorm += dum;
          }
        }
        if (snorm < 1.0e-10)
          snorm = 0.0;
        else
          snorm = 1.0/sqrt(snorm*pi32);

        for (ialpha=0; ialpha<numAlpha; ialpha++) 
           cont->coefficient(ialpha,icol,
          cont->coefficient(ialpha,icol) * snorm );
      }
#if 0
      double dum, facs;
      // NORMF step; normalize the contracted basis functions
      for (icol=0; icol<numFuncTypes; icol++) {
        for (ialpha=0,facs = 0.0; ialpha<numAlpha; ialpha++) {
          for (int ialpha2=0; ialpha2<=ialpha; ialpha2++) {
            double ee = alpha[ialpha] + alpha[ialpha2];
            double fac = ee*sqrt(ee);
            switch ( funcTypes[icol]) {
              case TGaussianBasisSet::s_shell:
                dum = cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)/fac;
                break;
              case TGaussianBasisSet::p_shell:
                dum = 0.5*cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)/(ee*fac);
                break;
              case TGaussianBasisSet::d_shell:
                dum = 0.75*cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)/(ee*ee*fac);
                break;
              case TGaussianBasisSet::f_shell:
                dum = 1.875*cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)/(pow(ee,3)*fac);
                break;
              case TGaussianBasisSet::g_shell:
                dum = 6.5625*cont->coefficient(ialpha,icol)*
                      cont->coefficient(ialpha2,icol)/(pow(ee,4)*fac);
                break;
              // h and i shells still need to be implemented
              case TGaussianBasisSet::h_shell:
                dum = 0.0;
                break;
              case TGaussianBasisSet::i_shell:
                dum = 0.0;
                break;
              default:
                EE_RT_ASSERT( 0, EE_FATAL,"Unrecognized funcType type");
                break;
            }  // switch
            if (ialpha != ialpha2) dum *= 2;
            facs += dum;
          }    // ialpha2
        }      // ialpha

        if (facs < 1.0e-10)
          facs = 0.0;
        else
          facs = 1.0/sqrt(facs*pi32);

        for (ialpha=0; ialpha<numAlpha; ialpha++) 
           cont->coefficient(ialpha,icol,
          cont->coefficient(ialpha,icol) * facs );
      }      // for icol

      // undo NORMS step
      for (icol=0; icol<numFuncTypes; icol++) {
        for (ialpha=0; ialpha<numAlpha; ialpha++) {

          double ee = 2 * alpha[ialpha];
          double facs = pi32 / (ee * sqrt(ee));

          switch ( funcTypes[icol]) {
            case TGaussianBasisSet::s_shell:
               cont->coefficient(ialpha, icol,
                    cont->coefficient(ialpha,icol) * sqrt(facs) );
              break;
            case TGaussianBasisSet::p_shell:
              cont->coefficient(ialpha,icol, 
                    cont->coefficient(ialpha,icol) * sqrt(0.5*facs/ee));
              break;
            case TGaussianBasisSet::d_shell:
              cont->coefficient(ialpha,icol,
                 cont->coefficient(ialpha,icol) * sqrt(0.75*facs/(ee*ee)));
              break;
            case TGaussianBasisSet::f_shell:
              cont->coefficient(ialpha,icol, 
                 cont->coefficient(ialpha,icol) * sqrt(1.875*facs/pow(ee,3)) );
              break;
            case TGaussianBasisSet::g_shell:
              cont->coefficient(ialpha,icol, 
                 cont->coefficient(ialpha,icol) * sqrt(6.5625*facs/pow(ee,4)));
              break;
            // h and i shells still need to be implemented
            case TGaussianBasisSet::h_shell:
              cont->coefficient(ialpha,icol,0.0);
              break;
            case TGaussianBasisSet::i_shell:
              cont->coefficient(ialpha,icol,0.0);
              break;
            default:
              EE_RT_ASSERT( 0, EE_FATAL,"Unrecognized funcType type");
              break;
          }  // switch
#if (!defined(INSTALL) && defined(DEBUG))
          cout << "normalized coef is " << cont->coefficient(ialpha,icol) <<
                  " for alpha " << alpha[ialpha] << endl;
#endif
        }    // for ialpha
      }      // for icol
#endif

      //tmp->coefficients(atomID,ics,aoCoeffs);
    }        // for ics

    // add the normalized TGaussianBasisSet back to the list
    normalized.push_back(tmp);
  }          // for gbs


  test.insertGBSGroup(atomID, group);
  return normalized;

}

  

/**
 *   Get the correct normalization factor for a given shell.
 */
double ComputeMoCmd::getNormFactor(int shell_type)
{
   double ret = 0.0;
   switch ( shell_type ) {
      case TGaussianBasisSet::s_shell:
         ret = sNormFactor;
         break;
      case TGaussianBasisSet::p_shell:
         ret = pNormFactor;
         break;
      case TGaussianBasisSet::d_shell:
         ret = dNormFactor;
         break;
      case TGaussianBasisSet::f_shell:
         ret = fNormFactor;
         break;
      case TGaussianBasisSet::g_shell:
         ret = gNormFactor;
         break;
      case TGaussianBasisSet::h_shell:
         ret = hNormFactor;
         break;
      case TGaussianBasisSet::i_shell:
         ret = gNormFactor;
         break;
      default:
         EE_RT_ASSERT( 0, EE_FATAL,"Unrecognized funcType type");
         break;
   }  // switch
   return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Get the correct normalization factor for a given shell.
/////////////////////////////////////////////////////////////////////////////
double ComputeMoCmd::getalphapow(int shell_type)
{
  double ret = 0.0;
  switch ( shell_type ) {
    case TGaussianBasisSet::s_shell:
      ret = 0.75;
      break;
    case TGaussianBasisSet::p_shell:
      ret = 1.25;
      break;
    case TGaussianBasisSet::d_shell:
      ret = 1.75;
      break;
    case TGaussianBasisSet::f_shell:
      ret = 2.25;
      break;
    case TGaussianBasisSet::g_shell:
      ret = 2.75;
      break;
    case TGaussianBasisSet::h_shell:
      ret = 3.25;
      break;
    case TGaussianBasisSet::i_shell:
      ret = 3.75;
      break;
    default:
      EE_RT_ASSERT( 0, EE_FATAL,"Unrecognized funcType type");
      break;
  }  // switch
  return ret;
}

/**
 *  Get the odd angular normalization factor for a given shell and 
 *  angular quantum number.
 */
double ComputeMoCmd::getoddNormalize(
      int shell_type, 
      int index,
      TGBSAngFunc *angfunc)
{
   int l, m, n;
   if (angfunc->basisType()==TGBSAngFunc::Cartesian) {
      return 1.0;
   } else {
      // Evaluate numerical prefactor for spherical basis functions
      AngMomFunc func;
      switch ( shell_type ) {
         case TGaussianBasisSet::s_shell:
         case TGaussianBasisSet::p_shell:
            return 1.0;
            break;
         case TGaussianBasisSet::d_shell:
            angfunc->getMaxExponents(shell_type, index, l, m, n);
            if (n == 2) return 1.0;
            return sqrt(3.0);
            break;
         case TGaussianBasisSet::f_shell:
            angfunc->getMaxExponents(shell_type, index, l, m, n);
            func = angfunc->getFunc(shell_type, index);
            if (func.size() == 1) return sqrt(15.0);
            if (func.size() == 2) {
               if (l == 3 || m == 3) return sqrt(2.5);
               return sqrt(15.0);
            }
            if (func.size() == 3) {
               if (l == 3 || m == 3) return sqrt(1.5);
               return 1.0;
            }
            break;
            // This will need to be fixed when h and i are added. - TLW
         case TGaussianBasisSet::g_shell:
         case TGaussianBasisSet::h_shell:
         case TGaussianBasisSet::i_shell:
            return 1.0;
            break;
         default:
            EE_RT_ASSERT( 0, EE_FATAL,
                  "Unrecognized funcType type");
            break;
      }
      // should never get here--makes the compiler happy to return a value
      return 1.0;
   }
}
