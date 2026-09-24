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

#include <exception>
#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"
#include "util/InternalException.H"
#include "util/CancelException.H"
#include "util/Receiver.H"
#include "util/ProgressMonitor.H"

#include "tdat/TGBSAngFunc.H"
#include "tdat/EspField.H"
#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/SingleGrid.H"

#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSConfig.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"
#include "dsm/ICalcUtils.H"
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
bool ComputeMoCmd::execute()
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

    //  Either ESP type is an ordinary density surface that is then
    //  coloured, so both take the whole density path and add a step at
    //  the end; they differ only in how the potential is obtained.
    bool wantEspCharges = (fieldType==ESP_CHARGES_FIELD_TYPE);
    bool wantEsp = (fieldType==ESP_FIELD_TYPE) || wantEspCharges;
    bool doingDensity = (fieldType=="Density" || wantEsp);
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
      //  Tested through the flags, not the field type string: an ESP
      //  surface IS a density surface, and comparing the literal names
      //  here left it with no MO range, no density array, and a null
      //  dereference on the first grid point.
      if (doingDensity || doingSpinDensity) {
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

      //  A semiempirical code never writes a basis set, so there is
      //  nothing for gbsConfig() to return; rebuild one from the Slater
      //  exponents the code did report.  Null stays null, and the guard
      //  below then declines to render rather than rendering wrongly.
      if (gbsConfig == (TGBSConfig *)0 || gbsConfig->empty()) {
        TGBSConfig *slater = ICalcUtils::slaterBasisConfig(calc);
        if (slater != (TGBSConfig *)0) {
          delete gbsConfig;
          gbsConfig = slater;
        }
      }

      //  Checked here rather than at the guard further down: coordsys()
      //  is read before it, so a missing basis used to be a null
      //  dereference and not the clean refusal it was written to be.
      if (gbsConfig == (TGBSConfig *)0) return false;

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
        //  atoms is NOT deleted here either, for the same reason as
        //  angfunc below: Fragment::atoms() hands back a NEWLY
        //  ALLOCATED vector that the caller owns, and the ESP block
        //  further down still reads it.  Deleting it here left both ESP
        //  paths holding a dangling pointer -- which showed up as an
        //  atom count of 18446744071472107968, a negative number read
        //  as unsigned.
        //
        //  This is the second thing in this one cleanup block that the
        //  ESP work outlived.  Finding the first should have prompted
        //  checking the rest of it.
        //
        //  NOT deleted here.  The exact ESP walks the basis again after
        //  the field is built and needs the angular functions to do it,
        //  so deleting at this point handed buildEspBasis() a dangling
        //  pointer and took the viewer down inside it -- before it could
        //  print anything, which is what made the crash look like it
        //  was in the colour rendering.  Deleted at the end of the
        //  block instead, once nothing else can want it.

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

          //  Colour the density surface by the electrostatic potential.
          //  Computed here, on the same grid, because the surface the
          //  colour belongs to is the one just built, and -- for the
          //  exact potential -- because that density is what the
          //  flattened basis is checked against.
          if (wantEsp) {
            bool haveEsp = false;
            cerr << "ESP: field type '" << fieldType << "', "
                 << (wantEspCharges ? "point-charge" : "canonical")
                 << " path, grid " << gridRes << " points" << endl;

            if (!wantEspCharges) {
              vector<EspBasisFunction> espBasis;
              const bool built = buildEspBasis(sgfrag, gbsConfig, code,
                                               angfunc, maxShell,
                                               length_shell, espBasis);
              cerr << "ESP: basis walk " << (built ? "ok" : "FAILED")
                   << ", " << espBasis.size() << " functions" << endl;
              if (built) {
                interrupted = !computeEspExact(grid, atoms, espBasis,
                                               moCoefs, occ,
                                               uhfCalc ? moCoefsBeta : 0,
                                               occBeta, field, gridRes,
                                               xDelta, yDelta, zDelta);
                haveEsp = !interrupted &&
                          (grid->colorFieldData() != (float*)0);
              }
              if (!haveEsp && !interrupted) {
                //  The exact potential declined -- a basis it cannot
                //  represent, or the two basis walks disagreeing.  The
                //  point-charge map is still worth showing, and the
                //  field type recorded on the grid says which it is.
                grid->type(ESP_CHARGES_FIELD_TYPE);
              }
            }

            if (!haveEsp && !interrupted) {
              interrupted = !computeEsp(grid, atoms, gridRes,
                                        xDelta, yDelta, zDelta);
            }
          }

        } else {
          cvsg->removeMOGrid(grid->name());
        }

        // Do some cleanup
        delete angfunc;
        angfunc = 0;

        delete atoms;
        atoms = 0;

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


/////////////////////////////////////////////////////////////////////////////
// Description
//   The electrostatic potential on the grid, from the atomic partial
//   charges, for colouring a density surface.
//
//   This is the cheap approximation -- a sum of point charges rather
//   than an integral over the electron density -- which is what
//   Avogadro and the other viewers use for the same picture, and it is
//   fast: a few multiply-adds per grid point per atom, against the
//   density evaluation that has already run and costs far more.  It is
//   still reported through the progress monitor and is still
//   interruptible, so it cannot turn into a silent hang on a large grid.
//
//   The charge source is ESPCHARGE where the calculation has it, since
//   those charges are fitted to reproduce this very potential, and
//   Mulliken charges otherwise.  Mulliken charges are a much cruder
//   basis for a potential map, so which was used is named in the
//   progress message rather than left for the user to guess.
/////////////////////////////////////////////////////////////////////////////
bool ComputeMoCmd::computeEsp(SingleGrid *grid, vector<TAtm*> *atoms,
                              unsigned long gridRes,
                              float xDelta, float yDelta, float zDelta)
{
  if (grid == 0 || atoms == 0 || atoms->empty()) {
    cerr << "ESP: no grid or no atoms; nothing to colour." << endl;
    return true;
  }

  IPropCalculation *calc = getCalculation();
  if (calc == 0) {
    cerr << "ESP: no calculation; nothing to colour." << endl;
    return true;
  }

  //  Per-atom charges, preferring the fitted ones.
  const unsigned long numAtoms = atoms->size();
  vector<double> charges(numAtoms, 0.0);
  const char *source = 0;

  PropTable *espTable = (PropTable*)calc->getProperty("ESPCHARGE");
  if (espTable != 0 && (unsigned long)espTable->rows() == numAtoms &&
      espTable->columns() >= 1) {
    for (unsigned long a = 0; a < numAtoms; a++) {
      charges[a] = espTable->value((int)a, 0);
    }
    source = "ESP charges";
  } else {
    PropVector *mulliken = (PropVector*)calc->getProperty("MULLIKEN");
    if (mulliken == 0 || (unsigned long)mulliken->rows() != numAtoms) {
      //  Said out loud.  Returning quietly here leaves a surface that
      //  renders perfectly and is all one colour, which looks like a
      //  rendering bug and is not one.
      cerr << "ESP: no usable atomic charges, so the surface is left "
              "uncoloured." << endl;
      cerr << "ESP:   ESPCHARGE "
           << (espTable ? "present" : "absent");
      if (espTable) {
        cerr << " with " << espTable->rows() << " rows and "
             << espTable->columns() << " columns";
      }
      cerr << ", MULLIKEN " << (mulliken ? "present" : "absent");
      if (mulliken) cerr << " with " << mulliken->rows() << " rows";
      cerr << "; the fragment has " << numAtoms << " atoms." << endl;
      return true;
    }
    for (unsigned long a = 0; a < numAtoms; a++) {
      charges[a] = mulliken->value((int)a);
    }
    source = "Mulliken charges";
  }

  const int resX = grid->dimensions()[0];
  const int resY = grid->dimensions()[1];
  const int resZ = grid->dimensions()[2];
  const double xStart = grid->origin()[0];
  const double yStart = grid->origin()[1];
  const double zStart = grid->origin()[2];

  //  Coordinates are Angstrom here and the potential is wanted in
  //  atomic units, as elsewhere in this file.
  const double atob = 1/0.52917724924;

  vector<double> ax(numAtoms), ay(numAtoms), az(numAtoms);
  for (unsigned long a = 0; a < numAtoms; a++) {
    const double *c = (*atoms)[a]->coordinates();
    ax[a] = c[0]*atob;
    ay[a] = c[1]*atob;
    az[a] = c[2]*atob;
  }

  float *esp = new float[gridRes];
  char msg[120];

  unsigned long idx = 0;
  for (int k = 0; k < resZ; k++) {

    if (p_monitor != 0) {
      sprintf(msg, "Electrostatic potential from %s: plane %d of %d",
              source, k+1, resZ);
      if (p_monitor->isInterrupted(msg, (int)((k+1)*100.0/resZ))) {
        delete [] esp;
        return false;
      }
    }

    const double z = (zStart + k*zDelta)*atob;
    for (int j = 0; j < resY; j++) {
      const double y = (yStart + j*yDelta)*atob;
      for (int i = 0; i < resX; i++) {
        const double x = (xStart + i*xDelta)*atob;

        double v = 0.0;
        for (unsigned long a = 0; a < numAtoms; a++) {
          const double dx = x - ax[a];
          const double dy = y - ay[a];
          const double dz = z - az[a];
          double r = sqrt(dx*dx + dy*dy + dz*dz);
          //  A grid point can land on a nucleus, where the point-charge
          //  potential is infinite.  Clamping keeps one such point from
          //  taking the whole colour range with it -- the ramp is scaled
          //  to the extreme value, so a single infinity would flatten
          //  every real feature to the midpoint.
          if (r < 1.0e-3) r = 1.0e-3;
          v += charges[a]/r;
        }
        esp[idx++] = (float)v;
      }
    }
  }

  grid->setColorFieldData(esp);
  grid->findColorMinMax();
  if (grid->colorFieldData() == (float*)0) {
    cerr << "ESP: every potential value was NaN or infinite, so the "
            "surface is left uncoloured.  That normally means the "
            "geometry or the grid is bad rather than the charges."
         << endl;
    return true;
  }
  cerr << "ESP: coloured from " << source << "; range "
       << grid->colorFieldMin() << " to " << grid->colorFieldMax()
       << " Hartree/e" << endl;
  return true;
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   Flatten the basis set into one entry per atomic orbital.
//
//   This walks the same nesting as the field evaluation above -- atoms,
//   basis sets, contracted sets, shell columns, degeneracies -- and it
//   MUST stay in step with it, because the index it counts is the
//   column index into the MO coefficient matrix.  Two walks of one
//   structure is the shape this codebase has been bitten by before, so
//   the ESP that uses this cross-checks itself against the field
//   evaluation before trusting it.
//
//   Shells the angular function table cannot describe are skipped here
//   exactly as they are skipped there, leaving an empty entry, so that
//   everything after them still lines up.
/////////////////////////////////////////////////////////////////////////////
bool ComputeMoCmd::buildEspBasis(const SGFragment *sgfrag,
                                 TGBSConfig *gbsConfig,
                                 const JCode *code,
                                 TGBSAngFunc *angfunc,
                                 int maxShell, const int *length_shell,
                                 vector<EspBasisFunction>& basis)
{
  basis.clear();
  if (sgfrag == 0 || gbsConfig == 0 || angfunc == 0) return false;

  const double atob = 1/0.52917724924;

  //  ONE call, and this function owns the result: Fragment::atoms()
  //  allocates a fresh vector every time.  Calling it again just to
  //  test for null, as an earlier version of this guard did, leaked one
  //  per invocation.
  vector<TAtm*> *atoms = sgfrag->atoms();
  double *atomCoords = sgfrag->coordinates();
  const unsigned long numAtoms = sgfrag->numAtoms();

  if (atoms == 0 || atomCoords == 0 || numAtoms == 0 ||
      atoms->size() != numAtoms) {
    delete atoms;
    return false;
  }

  for (unsigned long idxAtom = 0; idxAtom < numAtoms; idxAtom++) {

    const string atomID = (*atoms)[idxAtom]->atomicSymbol();
    const unsigned long idxAtomCoord = idxAtom*3;

    double center[3];
    for (int k = 0; k < 3; k++) center[k] = atomCoords[idxAtomCoord+k]*atob;

    vector<const TGaussianBasisSet*> gbslist = gbsConfig->getGBSList(atomID);
    vector<TGaussianBasisSet*> normalized =
      normalize(atomID, gbslist, *gbsConfig, code);

    const int gbsSize = normalized.size();

    for (int gbs_index = 0; gbs_index < gbsSize; gbs_index++) {
      const TGaussianBasisSet *gbs = normalized[gbs_index];
      if (gbs == 0) continue;

      const int numContractedSets = gbs->num_contracted_sets(atomID.c_str());

      for (int ics = 0; ics < numContractedSets; ics++) {

        vector<double> alpha = gbs->exponents(atomID.c_str(), ics);
        vector<TGaussianBasisSet::AngularMomentum> funcTypes =
          gbs->func_types(atomID.c_str(), ics);
        Contraction_ *cont = gbs->getContraction(atomID.c_str(), ics);
        if (cont == 0) { delete atoms; return false; }

        const int numAlpha = alpha.size();
        const int numFuncTypes = funcTypes.size();

        for (int icol = 0; icol < numFuncTypes; icol++) {

          const int shell_type = funcTypes[icol];

          //  Beyond what the angular table describes.  The field
          //  evaluation leaves these orbitals at zero and steps over
          //  their columns; do the same, so the indices still match.
          if (shell_type > maxShell-1) {
            for (int deg = 0; deg < length_shell[shell_type]; deg++) {
              basis.push_back(EspBasisFunction());
            }
            continue;
          }

          for (int deg = 0; deg < length_shell[shell_type]; deg++) {

            EspBasisFunction fn;
            for (int k = 0; k < 3; k++) fn.center[k] = center[k];

            const double oddNormalize =
              getoddNormalize(shell_type, deg, angfunc);

            AngMomFunc terms = angfunc->getFunc(shell_type, deg);
            for (size_t t = 0; t < terms.size(); t++) {
              //  An r^k factor is not a Cartesian Gaussian and cannot go
              //  through the Coulomb integrals as one.  No shipped
              //  MOOrdering uses one; refuse rather than silently drop
              //  the term if that ever changes.
              if (terms[t].m_k != 0) { delete atoms; return false; }

              fn.powerX.push_back(terms[t].m_l);
              fn.powerY.push_back(terms[t].m_m);
              fn.powerZ.push_back(terms[t].m_n);
              fn.angularCoef.push_back(terms[t].m_coefficient*oddNormalize);
            }

            for (int ia = 0; ia < numAlpha; ia++) {
              fn.exponent.push_back(alpha[ia]);
              fn.contraction.push_back(cont->coefficient(ia, icol));
            }

            basis.push_back(fn);
          }
        }
      }
    }
  }

  delete atoms;
  return !basis.empty();
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   The canonical electrostatic potential: what a unit positive test
//   charge feels.
//
//       V(r) = sum_A Z_A/|r - R_A|  -  integral rho(r')/|r - r'| dr'
//
//   The second term is an integral over the electron density, not a sum
//   over point charges.  The physics lives in EspField, where it is
//   tested against cases whose answer is known; what is here is the
//   density matrix, the grid loop, and the check that the basis this
//   was handed actually describes the calculation.
//
//   This is the expensive one -- order (grid points x basis functions
//   squared) rather than (grid points x atoms) -- which is why it
//   reports progress per plane and is interruptible.
/////////////////////////////////////////////////////////////////////////////
bool ComputeMoCmd::computeEspExact(SingleGrid *grid, vector<TAtm*> *atoms,
                                   const vector<EspBasisFunction>& basis,
                                   PropTable *moCoefs,
                                   const vector<double>& occ,
                                   PropTable *moCoefsBeta,
                                   const vector<double>& occBeta,
                                   const float *densityField,
                                   unsigned long gridRes,
                                   float xDelta, float yDelta, float zDelta)
{
  p_espBasisMismatch = 0.0;
  if (grid == 0 || atoms == 0 || moCoefs == 0 || basis.empty()) {
    cerr << "ESP: cannot compute the exact potential (grid "
         << (grid ? "ok" : "null") << ", atoms "
         << (atoms ? "ok" : "null") << ", MO coefficients "
         << (moCoefs ? "ok" : "null") << ", basis "
         << basis.size() << " functions)" << endl;
    return true;
  }

  const size_t nbas = basis.size();

  //  The MO matrix must describe the basis this was built from.  If it
  //  does not, the two basis walks have diverged and nothing here can
  //  be trusted.
  if ((size_t)moCoefs->columns() != nbas) {
    cerr << "ESP: the MO matrix has " << moCoefs->columns()
         << " columns but the basis was flattened to " << nbas
         << " functions; declining rather than guessing." << endl;
    return true;
  }

  //  Density matrix, P = sum_i n_i C_i C_i over both spins.
  vector<double> P(nbas*nbas, 0.0);

  for (int pass = 0; pass < 2; pass++) {
    PropTable *C = (pass == 0) ? moCoefs : moCoefsBeta;
    const vector<double>& n = (pass == 0) ? occ : occBeta;
    if (C == 0 || n.empty()) continue;
    if ((size_t)C->columns() != nbas) continue;

    const int numMO = C->rows();
    for (int m = 0; m < numMO && m < (int)n.size(); m++) {
      if (n[m] == 0.0) continue;

      for (size_t mu = 0; mu < nbas; mu++) {
        const double cmu = C->value(m, (int)mu);
        if (cmu == 0.0) continue;
        for (size_t nu = 0; nu < nbas; nu++) {
          P[mu*nbas + nu] += n[m]*cmu*C->value(m, (int)nu);
        }
      }
    }
  }

  EspField::Pairs pairs;
  EspField::selectPairs(basis, P, 1.0e-8, pairs);
  cerr << "ESP: " << nbas << " basis functions, " << pairs.size()
       << " significant pairs" << endl;
  if (pairs.size() == 0) {
    cerr << "ESP: no significant density matrix pairs; leaving it "
            "uncoloured." << endl;
    return true;
  }

  const double atob = 1/0.52917724924;

  const int resX = grid->dimensions()[0];
  const int resY = grid->dimensions()[1];
  const int resZ = grid->dimensions()[2];
  const double xStart = grid->origin()[0];
  const double yStart = grid->origin()[1];
  const double zStart = grid->origin()[2];

  //=========================================================
  //  Do the two basis walks agree?
  //
  //  buildEspBasis() walks the basis separately from the field
  //  evaluation above, and two walks of one structure is exactly the
  //  shape this codebase has been bitten by before.  Here a divergence
  //  would not fail -- it would produce a smooth, wrong potential that
  //  still looks like an ESP map.
  //
  //  So it is checked rather than trusted: the density rebuilt from the
  //  flattened basis must reproduce the density the loop above already
  //  computed, at points spread across the grid.  If it does not,
  //  decline, and the caller falls back to the point-charge map.
  //=========================================================
  if (densityField != 0) {
    int agreed = 0, tested = 0;
    double worst = 0.0;

    for (int sample = 0; sample < 40; sample++) {
      const unsigned long at =
        (unsigned long)((double)sample/40.0*(gridRes-1));

      const double reference = densityField[at];
      //  Only where there is something to compare; the far corners of
      //  the box are zero and would agree trivially.
      if (fabs(reference) < 1.0e-6) continue;

      const int i = at % resX;
      const int j = (at/resX) % resY;
      const int k = (at/((unsigned long)resX*resY)) % resZ;

      const double rho = EspField::density(basis, pairs,
                                           (xStart + i*xDelta)*atob,
                                           (yStart + j*yDelta)*atob,
                                           (zStart + k*zDelta)*atob);
      tested++;
      const double error = fabs(rho - reference)/fabs(reference);
      if (error > worst) worst = error;
      if (error < 1.0e-3) agreed++;
    }

    cerr << "ESP: rebuilt density agrees with the computed one at "
         << agreed << " of " << tested << " sampled points" << endl;
    if (tested > 0 && agreed < tested) {
      //  Not an assertion: a basis this cannot represent is a reason to
      //  fall back, not to take the application down.
      p_espBasisMismatch = worst;
      cerr << "ESP: the two basis walks disagree (worst " << worst
           << "); falling back to atomic charges." << endl;
      return true;
    }
  }

  //  Nuclei.  Sanity-checked rather than trusted: an implausible count
  //  here would be allocated before anything noticed, and a failed
  //  allocation leaves the application with an exception it does not
  //  handle rather than a surface it cannot colour.
  const unsigned long numAtoms = atoms->size();
  if (numAtoms == 0 || numAtoms > 1000000UL) {
    cerr << "ESP: implausible atom count " << numAtoms
         << "; declining." << endl;
    return true;
  }

  vector<EspNucleus> nuclei(numAtoms);
  for (unsigned long a = 0; a < numAtoms; a++) {
    const TAtm *atom = (*atoms)[a];
    if (atom == 0) {
      cerr << "ESP: atom " << a << " is null; declining." << endl;
      return true;
    }
    const double *c = atom->coordinates();
    if (c == 0) {
      cerr << "ESP: atom " << a << " has no coordinates; declining."
           << endl;
      return true;
    }
    for (int k = 0; k < 3; k++) nuclei[a].center[k] = c[k]*atob;
    nuclei[a].charge = atom->atomicNumber();
  }

  cerr << "ESP: " << nuclei.size() << " nuclei, allocating "
       << gridRes << " points" << endl;

  float *esp = new float[gridRes];
  char msg[160];

  //  Wrapped, because the first attempt at this reported only
  //  "Unhandled unknown exception" and died -- which says nothing about
  //  what threw or where.  Anything escaping here is reported with its
  //  type and the plane it happened on, and the surface is left
  //  uncoloured rather than the application being taken down.
  unsigned long idx = 0;
  int plane = 0;
  try {
    for (plane = 0; plane < resZ; plane++) {

      if (p_monitor != 0) {
        sprintf(msg,
                "Electrostatic potential: plane %d of %d, %d orbital pairs",
                plane+1, resZ, (int)pairs.size());
        if (p_monitor->isInterrupted(msg, (int)((plane+1)*100.0/resZ))) {
          delete [] esp;
          cerr << "ESP: interrupted on plane " << plane+1 << endl;
          return false;
        }
      }
      if (plane == 0) {
        cerr << "ESP: first plane starting" << endl;
      }

      const double z = (zStart + plane*zDelta)*atob;
      for (int j = 0; j < resY; j++) {
        const double y = (yStart + j*yDelta)*atob;
        for (int i = 0; i < resX; i++) {
          double point[3];
          point[0] = (xStart + i*xDelta)*atob;
          point[1] = y;
          point[2] = z;

          esp[idx++] = (float)EspField::potential(basis, pairs, nuclei,
                                                  point);
        }
      }
      if (plane == 0) {
        cerr << "ESP: first plane done" << endl;
      }
    }
  }
  catch (const std::exception& e) {
    cerr << "ESP: threw on plane " << plane+1 << " of " << resZ
         << ": " << e.what() << endl;
    delete [] esp;
    return true;
  }
  catch (...) {
    cerr << "ESP: threw a non-standard exception on plane " << plane+1
         << " of " << resZ << endl;
    delete [] esp;
    return true;
  }

  grid->setColorFieldData(esp);
  grid->findColorMinMax();
  cerr << "ESP: potential computed, range " << grid->colorFieldMin()
       << " to " << grid->colorFieldMax() << " Hartree/e" << endl;
  return true;
}
