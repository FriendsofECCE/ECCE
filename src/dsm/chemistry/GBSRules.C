///////////////////////////////////////////////////////////////////////////////
// FILENAME: GBSRules.C
//
//
// CLASS SYNOPSIS:
// 
//
// SEE ALSO:
//    
///////////////////////////////////////////////////////////////////////////////

#include <vector>
  using std::vector;
#include <string>
  using std::string;
#include <map>
  using std::map;

#include <string.h>

#include "util/ErrMsg.H"

#include "tdat/TTheory.H"
#include "tdat/Fragment.H"

#include "dsm/JCode.H"
#include "dsm/TGBSConfig.H"
#include "dsm/GBSRules.H"

/*******************************************************************
 Method : checkOrbitalCoverage
 Summary: Returns FALSE if, for any defined group, its orbital GBS
          does not cover all the elements in that group.

 Precondition:  config is not NULL
*******************************************************************/
bool GBSRules::checkOrbitalCoverage(const TGBSConfig* config)
{
  TGBSConfig::const_iterator config_it;
  bool ret = true;
  const TGaussianBasisSet* orbitalGBS;
  const TGBSGroup* group;
  vector<const TGaussianBasisSet*>* orbitalGBSList;
  string orbitalNames;
  string tag;
  bool notCovered;
  size_t i,j;

  for(config_it = config->begin(); config_it != config->end();
      config_it++)
  { 
    if ((*config_it).second == NULL)
      continue; // skip empty groups

    notCovered = false;
    group = (*config_it).second;
    tag = (*config_it).first;
    
    orbitalGBSList = group->getOrbitalGBSList();
    TGBSConfigTags tags(tag.c_str());
    
    if (orbitalGBSList != 0) {
      for (j = 0; j < orbitalGBSList->size(); j++) {
	orbitalGBS = (*orbitalGBSList)[j];

	for (i = 0 ; i < tags.size(); i++) {

	  if (orbitalGBS->p_contractions.find(tags[i]) == 
	      orbitalGBS->p_contractions.end()) {
	    notCovered = true;
	  }
	  if (!group->hasContractions(tags[i], orbitalGBS)) {
	    notCovered = true;
	  }
 	}
      }
      delete orbitalGBSList;
    }
    // Use group (aggregate) name to report error messages
    if (notCovered)
      orbitalNames += (*config_it).second->name() + " ";    
  }

  if (orbitalNames.size() > 0) 
  {
    ErrMsg().message("EE_ORBITAL_COVERAGE",
		     EE_FATAL, WHERE, orbitalNames.c_str());
    ret = false;
  }
  return ret;
}

/*******************************************************************
 Method : checkECPCoverage
 Summary: Returns FALSE if there are ECP related ERRORS.
          
          At this time, the only anomalies are WARNINGS.  Specifically, the
	  message EE_GBS_COVERAGE is put on the queue if an ecp is specified
	  butdoesn't cover any elements.  This means ecps, NOT the orbitals.

Precondition:  config is not NULL
*******************************************************************/
bool GBSRules::checkECPCoverage(TGBSConfig* config)

{
  TGBSConfig::iterator config_it;
  bool ret = true;
  bool found;
  string errMsg;

  for(config_it = config->begin(); config_it != config->end(); 
      config_it++) {
    if ((*config_it).second == NULL)
      continue;

    if((*config_it).second->ecp()) {
      found = false;
      TGBSConfigTags tags((*config_it).first.c_str());
      TGBSConfigTags::iterator atm_it;
      const TGaussianBasisSet* ecp;

      for(atm_it = tags.begin(); atm_it != tags.end(); atm_it++) 
      {
        ecp = (*config_it).second->ecp();
        if(ecp->p_contractions.find(*atm_it) != ecp->p_contractions.end()) {
          found = true; // if the ecp covers 1 element in the group, then ok
	  break;
        } 	  
      }
      if (found == false) {
	errMsg += ecp->p_name + " does not support element(s) " + 
	          (*config_it).first + ".\n";
      }
    }
  }
  if (errMsg.size() > 0) {
    ErrMsg().message("EE_ECP_COVERAGE",EE_WARNING,WHERE, errMsg.c_str());
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//   Returns TRUE if the TGBSConfig completely covers all elements of
//   the chemical system.   This means that the orbital(s) cover the
//   elements and that if any dft sets are used, they exist for all.
//   ?? What about ECPs?
//
//  Implementation
//
//    I thought of the following potential rules to check:
//      1 is the orbital covered
//      2 are there auxiliary sets that aren't being used
//      3 are ECPs properly covered
//      4 are DFTs properly covered
//    In talking to Dave, only #1 seems to be necessary.  This leaves
//    the possibility of unused sets being included but that isn't
//    really a problem.
///////////////////////////////////////////////////////////////////////////////

bool GBSRules::isComplete
(
  const Fragment* frag,
  const TGBSConfig* config,
  const JCode* code,
  const TTheory* theory,
  bool verifyAllGroups    // some groups may not be defined yet, so
                          // you only want to validate the defined ones
) 
{
  bool ret = true;
  ErrMsg errs;

  // Verify that all componets are complete
  if (!frag || !config || !code || !theory)
    return false;
  
  // Verify that every group contains data:
  if(verifyAllGroups && config->hasEmptyGroups())
  {
    ret = false;
    errs.message("EE_GBS_GROUPS_EMPTY", EE_FATAL, WHERE);
  }

  // Verify that orbital basis sets cover all elements in the group:
  ret = ret && checkOrbitalCoverage(config);

  // Verify that there are exchange and charge fitting sets on every element
  // (This is only checked if in final state before a save)
  
  if (verifyAllGroups)
  {
    bool dft = (theory->category() == "DFT");
    bool recommended=true;
    bool fullDFTRequired=true;
    bool enterFitting=false;
 
    code->get_bool("RecommendDFTCharge",recommended);
    code->get_bool("RequireFullDFT",fullDFTRequired);
    code->get_bool("DFTFitting",enterFitting);

    bool some,all;
    bool esome,eall;
    config->hasExchangeFitting(esome,eall);
    config->hasChargeFitting(some,all);

    //////////////
    // "recommended" means that with this code, fitting sets should be
    //   supplied or else lots of cpu will be wasted.  We don't force this
    //   on the user though so it would only be a warning condition.
    // "fullrequired" means that if fitting sets are supplied for any
    //   elements, then they must be supplied for all of them.
    //
    // Given these definitions, this is the logic I arrived at for msgs:
    //   fullrequired && recommended && some && !all - error and warning
    //   fullrequired && recommended && !some && !all - warning about fitting
    //   fullrequired && !recommended && some && !all - error
    //   fullrequired &&  all - noop
    //   recommended && all - noop
    //   recommended && some && !all - warning about fitting
    //   recommended && !some && !all - noop
    //
    // And finally, though I tried to get out of it, we also have
    // to make sure exchange sets aren't only partially covered.  Since
    // there are so many possibilities, I tried to simplify by checking
    // the exchange only if all other tests passed.
    //////////////
    if (dft && enterFitting)
    {
      if (fullDFTRequired && some && !all) {
        ret = false;
        errs.message("EE_GBS_DFTFIT_ALL", EE_FATAL, WHERE);
      }
      // We've been told that this isn't a hard and fast rule because with
      // the right DFT functional, it isn't necessar to have CD fitting sets
      //if (recommended && !some) {
      //  errs.message("EE_GBS_DFTFIT_MISS", EE_WARNING, WHERE);
      //} 
      if (fullDFTRequired && esome && !eall) {
        ret = false;
        errs.message("EE_GBS_DFTFIT_ALL", EE_FATAL, WHERE);
      }
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns TRUE if the object is completely consitent with the
//    rest of the ESCalc setup.  The following are checked:
//      . maxLValue for the code
//      . maxFunctions for the code
//      . maxPrimitives for the code
//      . maxShellPrimitives for the code
//      . if ecps are used, does the code support ecps
//      . if dfts are set, does the code support dft
//      . if dfts are set, is the calc a dft calc
//
//   For the convenience of the caller, a CodeCap object is optional.
//   If it is not supplied, it will be retrieved.
///////////////////////////////////////////////////////////////////////////////
bool GBSRules::isConsistent
(
  const Fragment* frag,
  TGBSConfig* config,
  const JCode* code,
  const TTheory* theory
) 
{
  bool ret = true;

  if(!code || !config || !frag || !theory) return false;

  string catName = theory->category();

  static const int dontCheckThreshold = 64000;

  ErrMsg errs;

  ///////////
  // Check max L value
  ///////////
  int ivalue=0;
  code->get_int("MaxLValue",ivalue);
  if (ivalue < TGaussianBasisSet::i_shell) {
    int max_l_value = config->max_l_value();
    if (max_l_value > ivalue) {
      ret = false;
     errs.message("EE_GBS_EXCEED_MAX_LVAL",EE_FATAL,
                    WHERE,ivalue,max_l_value);
    }
  }

  TagCountMap* ttcMap = frag->tagCountsSTL();

  ///////////
  // Check Max Functions
  ///////////
  if (ret) {
    code->get_int("MaxTotalFunctions",ivalue);
    if (ivalue < dontCheckThreshold) {
       int numFun = config->num_functions(*ttcMap);
      if (numFun > ivalue) {
        ret = false;
        errs.message("EE_GBS_EXCEED_MAX_TOTAL_FUNCS", EE_FATAL, WHERE,
                     ivalue,numFun);
      }
    }
  }

  ///////////
  // Check Max Primitives
  ///////////
  if (ret) {
    code->get_int("MaxTotalPrimitives",ivalue);
    if (ivalue < dontCheckThreshold) {
     int numPrim = config->num_primitives(*ttcMap);
      if (numPrim > ivalue) {
        ret = false;
        errs.message("EE_GBS_EXCEED_MAX_TOTAL_PRIMS", EE_FATAL, WHERE,
                     ivalue,numPrim);
      }
    }
  }

  delete ttcMap;

  ///////////
  // Check Max Primitives on a per Shell basis
  // A possible bug/error is if the data in the codeCap file is not
  // well formed.  The code currently assumes that there are never too
  // many values in the vector (not > ::i_shell).  If there are less,
  // its not an error.
  ///////////
  vector<int> *counts = config->max_primitives();

  if (ret && counts) {
    vector<int> codeVec;
    code->get_int_list("MaxPrimitives",codeVec);
    for (size_t shell=TGaussianBasisSet::s_shell; shell<codeVec.size(); shell++) {
      if ((*counts)[shell] > codeVec[shell]) {
        ret = false;
        errs.message("EE_GBS_EXCEED_MAX_PRIMS", EE_FATAL, WHERE,
                       shell,codeVec[shell]);
      }
    }
  }

  ///////////
  // Check ECPS
  ///////////
  bool supportsECP=false;
  if (ret) {
    code->get_bool("ECPS",supportsECP);
    if (!supportsECP && config->hasECPs()) {
      errs.message("EE_GBS_ECP", EE_FATAL, WHERE);
      ret = false;
    }
  }

  ///////////
  // Check DFT Fitting Sets
  ///////////
  bool supportsDFTFitting = false;
  if (ret) {
    code->get_bool("DFTFitting",supportsDFTFitting);
    bool hasDFT = config->hasDFTs();
    if (hasDFT && !supportsDFTFitting) {
      errs.message("EE_GBS_DFTFITTING", EE_FATAL, WHERE);
      ret = false;
    } else if (hasDFT && catName != "DFT") {
      errs.message("EE_GBS_DFTFITTING_THEORY", EE_FATAL, WHERE);
      ret = false;
    }

  }


  if (ret) {
    vector<string> nospherical(5);
    string oneEntry;
    if (code->get_string("TheoryNoSpherical",oneEntry)) {
      nospherical.push_back(oneEntry);
    } else {
      code->get_string_list("TheoryNoSpherical",nospherical);
    }
    for(size_t idx = 0; idx < nospherical.size(); idx++)
    {
      // category is empty for some codes (is this right?)
      if(catName != "" && nospherical[idx] == catName &&
	 config->coordsys() == TGaussianBasisSet::Spherical)
      {
        ret = false;
        errs.message("EE_GBS_SPHERICAL",EE_FATAL, WHERE,
                     code->name().c_str(),catName.c_str());
      }
    }
  }
  return ret;
}


/*******************************************************************
 Method : autoOptimize
 Summary: autoOptimizes the config based on the code settings

 Precondition:  config, code, are not NULL
*******************************************************************/
void GBSRules::autoOptimize(TGBSConfig* config, const JCode* code,
			    const TTheory* theory) {

  EE_ASSERT(code, EE_FATAL, "No code defined!");
  EE_ASSERT(config, EE_FATAL, "No config defined!");

  // 1) SET POLARIZATION
  bool hasSpherical = config->hasSpherical();
  bool sphericalsSupportedCode = false;
  bool sphericalsSupportedTheory = true;

  code->get_bool("SphericalCoordinates",sphericalsSupportedCode);

  // Check to see that spherical is supported by the theory as well:
      // (theory could be null if in browse mode of the basis tool)
  if (sphericalsSupportedCode && theory != 0) {

    // get theories for which sphericals are not supported
    vector<string> nospherical;
    code->get_string_list("TheoryNoSpherical",nospherical);

    // check all non-spherical theories to see if our theory is one of them
    string cat = theory->category();
    vector<string>::iterator it;
    for(it=nospherical.begin();it!=nospherical.end();it++) {
      if((*it) == cat) {
	sphericalsSupportedTheory = false;
      }
    }
  }
  if (sphericalsSupportedCode && sphericalsSupportedTheory &&
      hasSpherical ) {

    config->coordsys(TGaussianBasisSet::Spherical);

  } else {
    config->coordsys(TGaussianBasisSet::Cartesian);
  }

  // 2) SET OPTIMIZE
  bool preferOpt;
  bool hasGC = config->hasGeneralContractions();
  code->get_bool("PreferOptimization", preferOpt);

  // Don't select optimize if basis sets don't have general contractions
  if (!hasGC) {
    config->optimize(false);

  } else if (preferOpt) {
    config->optimize(true);

  } else {
    config->optimize(false);
  }
}



