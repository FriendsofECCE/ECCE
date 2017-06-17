///////////////////////////////////////////////////////////////////////////////
// FILENAME: ICalcUtils.C
//
//
// CLASS SYNOPSIS:
// Utility functions on tdat objects that require access to the data server.
// This is here so we don't have weird dependencies between the tdat and
// dsm libraries.
//
// SEE ALSO:
//    
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

#include <iostream>
using std::cout;
using std::endl;

#include "dsm/ICalcUtils.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/JCode.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"

#include "util/ErrMsg.H"
#include "util/STLUtil.H"

#include "tdat/TTheory.H"
#include "tdat/Fragment.H"

#include "dsm/TGBSConfig.H"
#include "dsm/GBSRules.H"

// Static initialization;
   EDSIServerCentral central;
   EDSIGaussianBasisSetLibrary* ICalcUtils::p_gbsFactory = 
   new EDSIGaussianBasisSetLibrary(central.getDefaultBasisSetLibrary());


/*******************************************************************
 Method : makeConsistent
 Summary: Make consistent does the following:

          1) Changes the config to match the fragment, if it has changed;
	     tries to map the old basis sets onto the new frag elements,
             if possible.
	  2) Removes DFT fittings if the code & theory does not support them.
          3) Removes ECPs if the code does not support them.
          4) Changes coordsys to cartesian if the code does not support
	     spherical.

	  Returns false if still incomplete or inconsistent after the
	  above changes.  If makeConsistent returns false, this
	  means that the config was cleared because it could not be
	  made consistent with the existing code/fragment.  Make
	  consistent will put error messages on the stack.  These
	  messages must be retrieved separately.

	  Note that makeConsistent takes in a reference pointer.
	  This means that the pointer will get changed if a new
	  config is created as a result of this function.  User
	  needs to reset the TGBSConfig after the call to this function.
*******************************************************************/
bool ICalcUtils::makeConsistent
(
  Fragment* frag,
  TGBSConfig*& config,
  const JCode* code,
  TTheory* theory
)
{
  // Verify preconditions:
  EE_ASSERT(frag, EE_FATAL, "Fragment is null!");
  EE_ASSERT(config, EE_FATAL, "Config is null!");
  EE_ASSERT(code, EE_FATAL, "Code is null!");
  EE_ASSERT(theory, EE_FATAL, "Theory is null!");
  bool ret = true;

  // Since we have no way of checking imported basis sets (they only have
  // one group with all unknown basis sets) leave the config as-is if imported
  if (config->name() == "Imported") {
    return true; 
  }

  // First see if fragment has changed:
  string fragTag = frag->uniqueTagStr();
  string orderedFragTag; 
  string configTag = config->uniqueTagStr();

  // We need a re-sort hack, because these guys aren't always
  // coming out in alphabetical order like they are supposed to
  TGBSConfigTags fragSort(fragTag.c_str(), true, true);
  TGBSConfigTags configSort(configTag.c_str(), true, true);
  orderedFragTag = "";
  configTag = "";

  int i;
  for (i = 0; i < fragSort.size(); i++) {
    if (i != 0) {
      orderedFragTag += " ";
    }
    orderedFragTag += fragSort[i];
  }
  for (i = 0; i < configSort.size(); i++) {
    if (i != 0) {
      configTag += " ";
    }
    configTag += configSort[i];
  }

  // If fragment changed, make config consistent with new frag:
  if (orderedFragTag != configTag)
  {
    // Make a new config with same meta data as old:
    TGBSConfig* newConfig = new TGBSConfig();
    newConfig->type(config->type());
    newConfig->coordsys(config->coordsys());
    newConfig->name(config->name());
    newConfig->optimize(config->optimize());

    newConfig->resetAllGroups(config->type(), fragTag, (TGBSGroup*) 0); 

    if (newConfig->type() == TGBSConfig::simple) {

      TGBSGroup* newGroup, *oldGroup;
      if (config->isEmpty())
	oldGroup = (TGBSGroup*) 0;
      else
	oldGroup = (*(config->begin())).second;

      newGroup = completeGroup(oldGroup, fragTag);
      newConfig->resetGroup(fragTag, newGroup);
      delete newGroup;
    }
    else
    {
      TGBSConfig::iterator itOld, itNew;
      for (itNew = newConfig->begin(); itNew != newConfig->end(); itNew++)
      {
	itOld = config->find((*itNew).first);
	if (itOld != config->end())
	  newConfig->resetGroup((*itNew).first, (*itOld).second);
	else
	{
	  // Try resetting to the first group in the config
	  TGBSGroup* newGroup, *oldGroup;

	  if (config->isEmpty())
	    oldGroup = (TGBSGroup*) 0;
	  else
	    oldGroup = (*(config->begin())).second;

	  newGroup = completeGroup(oldGroup, (*itNew).first);
	  newConfig->resetGroup((*itNew).first, newGroup);
	  delete newGroup;
	}
      }
    }
    // change config to the new config:

    delete config;
    config = newConfig;
  }

  // Then Check ecps, dft, and spherical consistency against code:
  // Change what you can to make consistent
  ErrMsg errs;
  errs.flush();

  if (!GBSRules::isComplete(frag,config,code,theory))
      ret = false;

  else if ( !GBSRules::isConsistent(frag,config,code,theory))
  {
    // Check if ECPs supported:
    if (errs.check("EE_GBS_ECP")) {
      config->clearType(TGaussianBasisSet::ecp);
    }
      
    // Check if DFTs supported:
    if (errs.check("EE_GBS_DFTFITTING") ||
	errs.check("EE_GBS_DFTFITTING_THEORY") ){
      config->clearType(TGaussianBasisSet::exchange);
      config->clearType(TGaussianBasisSet::charge);
    }
  
    // Check if spherical supported:
    if (errs.check("EE_GBS_SPHERICAL")) 
      config->coordsys(TGaussianBasisSet::Cartesian);

    // Check to see if the config passes after the above fixes:
    if ( ! GBSRules::isConsistent(frag,config,code,theory) ) {
      ret = false;
    }   
  }

  // Remove ECPs if they don't support any elements in the group:
  GBSRules::checkECPCoverage(config);

  // If config didn't pass, then clear out all data:
  if (ret == false)
    config->resetAllGroups(config->type(), fragTag, (TGBSGroup*) 0);
  
  return ret;
}


TGBSGroup* ICalcUtils::completeGroup(TGBSGroup* oldGroup, string tag)
{
  TGBSGroup* group = 0;
 
  if (oldGroup != (TGBSGroup*) 0)
  {
    // Create empty new group:
    group = new TGBSGroup;
    const TGaussianBasisSet* oldGBS;
 
    // first add the orbital basis set
    const OrbitalMap* orbitalMap = oldGroup->getOrbitalGBSMappings();
    string orbitalName = (*(orbitalMap->begin())).first;
    
    vector<TGaussianBasisSet*> factoryList = 
      p_gbsFactory->lookup(orbitalName.c_str(), 
			   TGaussianBasisSet::UnknownGBSType, tag.c_str());
 
   
    group->insertOrbitalGBS(orbitalName, factoryList);
    
    // Next add any other basis sets besides those in the orbital:
    const vector<TGaussianBasisSet*>* gbsList = oldGroup->getOrderedList();
    for (int i = 0; i < gbsList->size(); i++)
    {
      oldGBS = (*gbsList)[i];
      if (!group->contains(oldGBS->p_name, oldGBS->p_type) &&
	  (*gbsList)[i]->p_type != TGaussianBasisSet::appended_tight &&
	  (*gbsList)[i]->p_type != TGaussianBasisSet::appended_diffuse)
      {
	factoryList = p_gbsFactory->lookup(oldGBS->p_name.c_str(), 
					   oldGBS->p_type, tag.c_str());
	group->insertGBS(factoryList[0]);
      }
    }
    if (oldGroup->ecp())
    {
      oldGBS = oldGroup->ecp();
      if (! group->contains(oldGBS->p_name, oldGBS->p_type))
      {
	factoryList = p_gbsFactory->lookup(oldGBS->p_name.c_str(), 
					   oldGBS->p_type, tag.c_str());
	group->insertGBS(factoryList[0]);
      }
    }
  }
  return group;
}

/*******************************************************************
 Method : Import method for TGBSConfig
          (Put here because it has edsi dependencies)

 Summary: Creates a TGBSConfig object out of an imported basis set.
          The imported data is passed in as a file stream. 
          File format is shown below:

          If using NameBasis, basis set names are provided, one per
          element per line.  No fancy wildcard characters allowed at this
          point.

NameBasis
basis "ao basis" cartesian
    H   library "DZVP2 (DFT Orbital)"
    Si  library "DZVP (DFT Orbital)"
    O   library "DZVP (DFT Orbital)"
END
EndNameBasis

NumericalBasis
basis "ao basis" cartesian print
H    S
        18.73113700         0.03349500
         2.82539400         0.23472700
         0.64012200         0.81375700
H    S
         0.16127800         1.00000000
O    S
      5484.67170000         0.00183100
       825.23495000         0.01395000
       188.04696000         0.06844500
        52.96450000         0.23271400
        16.89757000         0.47019300
         5.79963500         0.35852100
END
EndNumericalBasis

*******************************************************************/
TGBSConfig* ICalcUtils::importConfig(istream& infile)
{
  TGBSConfig* config = new TGBSConfig();
  char line[501];

  // Set config's meta data:
  config->type(TGBSConfig::element);
  config->optimize(true); // will generally be the case

  // Parse input file
  while(!infile.eof()) 
  {
    line[0] = '\0';

    if (infile.getline(line,500) <= 0) continue; 

    if (strncmp(line,"NameBasis", 9) == 0) { // basis set names follow
      
      importNameBasis(infile, config);

    } else if (strncmp(line, "NumericalBasis", 14) == 0 ) { // exp & coef

      importNumericalBasis(infile, config);
    } 
  }

  return config;
}

void ICalcUtils::importNumericalBasis(istream& infile, TGBSConfig* config) {

  char line[501];
  TGaussianBasisSet* gbsTemplate = 0;

  while(!infile.eof()) {

    line[0] = '\0';
    if (infile.getline(line,500) <= 0) continue;     

    if(strncmp(line, "EndNumericalBasis", 17) == 0) {  // Done with section
      break;

    } else if (strncmp(line, "END", 3) == 0) {
      delete gbsTemplate;
      gbsTemplate = 0;

    } else if(strncmp(line, "basis", 5) == 0) { // a basis set follows

      gbsTemplate = getGbsTemplate(line, config);
      
      if (gbsTemplate == 0) { // parsing error - skip rest of this basis set
        while(strncmp(line, "END", 3) != 0) {
          infile.getline(line, 500);
        }
      } 
        
      // Parse the basis set and add it to group:
      importGBSContractions(infile, gbsTemplate, config);

    } else if(strncmp(line, "ECP", 3) == 0) {
      // maybe call getGbsTemplate here as well
      importECPContractions(infile, config);

    }
  }
}

void ICalcUtils::importNameBasis(istream& infile, TGBSConfig* config) {

  char line[501];
  char element[10];
  char firstWord[25];
  string errMsg;
  string basisName;
  int firstQuote;
  int lastQuote;
  TGaussianBasisSet* gbsTemplate = 0;
  EDSIServerCentral central;
  EDSIGaussianBasisSetLibrary lib(central.getDefaultBasisSetLibrary());

  // Parse NameBasis section of imported *.gbs file
  while(!infile.eof()) {

    line[0] = '\0';
    if (infile.getline(line,500) <= 0) continue; 

    sscanf(line, "%s", firstWord);

    if(strcmp(firstWord, "EndNameBasis") == 0) { // Done with NameBasis section
      break;

    } else if (strcmp(firstWord, "END") == 0) { // end of basis set section
      delete gbsTemplate;
      gbsTemplate = 0;

    } else if(strcmp(firstWord, "basis") == 0 ||
              strcmp(firstWord, "ECP") == 0 ) { // named basis sets follow
      
      gbsTemplate = getGbsTemplate(line, config);

      if (gbsTemplate == 0) {
        // there was a parsing error - skip the rest of this basis set:
        while(strncmp(line, "END", 3) != 0) {
          infile.getline(line, 500);
        }
      } 
    
    } else { // Parse a basis set for a particular element

      // Get the element and basis set name
      string lineStr(line);
      firstQuote = lineStr.find_first_of("\"");
      lastQuote = lineStr.find_last_of("\"");
      
      if (sscanf(line, "%s", element) == 0 || 
          firstQuote == string::npos || lastQuote == string::npos) {
        errMsg += "Invalid NameBasis format:\n" + lineStr + "\n";
        continue;
      } 
      basisName = lineStr.substr(firstQuote+1, lastQuote-firstQuote-1);

      // Now lookup the basis set for that element:
      vector<TGaussianBasisSet*> gbsList 
        = lib.lookup(basisName.c_str(), gbsTemplate->p_type, element);

      if (gbsList.size() == 0) {
        errMsg += "Basis set: " + basisName + " not found in library!\n";
        continue;
      }

      // Now see if a group exists for that element in the config.
      // If not, create a group and add it to the config:
      TGBSGroup* group = config->findGroup(element);
      if (group == 0) {
        group = new TGBSGroup();
        config->insertGBSGroup(element, group);
      }

      // Add the library basis set(s) to the group:
      if (gbsList[0]->p_category == TGaussianBasisSet::ORBITAL) {
        // Make sure the aggregate name is formatted properly
        group->insertOrbitalGBS(lib.getNiceName(basisName), gbsList);

      } else {
        group->insertGBS(gbsList[0]);
      }
    }
  }
  if (errMsg != "") {
    EE_ASSERT(0, EE_WARNING, errMsg);
  }

}

TGaussianBasisSet* ICalcUtils::getGbsTemplate(const string& line,
                                              TGBSConfig* config) {

  TGaussianBasisSet* ret = new TGaussianBasisSet();
  bool error = false;
  string errMsg;

  // Find out what type of basis set it is
  // Assign default values which may be overwritten if it is a name
  // basis and it is looked up from the library
  
  if(line.find("ao basis") != string::npos) {
    // In this category, we don't yet know the type or category of 
    // these basis sets (could be a mixture of orbital, aux diffuse,
    // aux polarization, or aux rydberg)
    ret->p_type = TGaussianBasisSet::UnknownGBSType;
    ret->p_category = TGaussianBasisSet::ORBITAL; 
  }
  else if (line.find("cd basis") != string::npos) {
    ret->p_type = TGaussianBasisSet::charge;
    ret->p_category = TGaussianBasisSet::AUXILIARY;
  }
  else if (line.find("xc basis") != string::npos) {
    ret->p_type = TGaussianBasisSet::exchange;
    ret->p_category = TGaussianBasisSet::AUXILIARY;
  }
  else if (line.find("ECP") != string::npos) {
    ret->p_type = TGaussianBasisSet::ecp;
    ret->p_category = TGaussianBasisSet::AUXILIARY;
  }
  else {
    error = true;
    if (line.find("ri-mp2 basis") != string::npos) {
      errMsg += "ri-mp2 not supported at this time."
        " Omitting this basis set from imported config. ";
    }
    else {
      errMsg += "Unrecognized basis set.  Omiiting this component"
        " from the config.  ";
    }
  }
  
  // Determine polarization type:
  if(line.find("cartesian") != string::npos) {
    ret->p_coordSys = TGaussianBasisSet::Cartesian;
  } else if (line.find("spherical") != string::npos) {
    ret->p_coordSys = TGaussianBasisSet::Spherical;
  } 
  
  // If this is an orbital group, set config polarization as well
  // (we might need to change this logic)
  if (ret->p_type == TGaussianBasisSet::UnknownGBSType) {
    config->coordsys(ret->p_coordSys);
  }
  
  ret->p_name = "Imported";
  ret->p_contType = TGaussianBasisSet::UnknownContType; 

  if (error == true) {
    delete ret;
    ret = 0;
    EE_RT_ASSERT(0, EE_WARNING, errMsg);
  }
  return ret;
}


/*******************************************************************
 Method : importGBSCpmtractions
 Summary: Reads in contractions from standard format .gbs file. 

          gbsTemplate provides the common info that all basis sets in 
          this section will share

*******************************************************************/
void ICalcUtils::importGBSContractions(istream& infile, 
                                       TGaussianBasisSet* gbsTemplate,
                                       TGBSConfig* config)
{
  TGaussianBasisSet* newGBS = 0;
  TGBSGroup* group = 0;
  string curElement = "";
  string nextElement;
  Contraction_* cont = 0;
  ContractionSet* contSet = 0;
  TGaussianBasisSet::AngularMomentum shell;
  vector<double> exponents;
  vector<double> coefficients;
  char line[501];
  char* tok;
  int i;
  double value = 0;
  int numExtraShells;

  string tmpline;
  while(!infile.eof()) 
  {
    line[0] = '\0';

    if (infile.getline(line, 500) <= 0) continue;

    // Just to skip empty lines
    tmpline = line;
    STLUtil::stripLeadingAndTrailingWhiteSpace(tmpline);

    if (tmpline.size() == 0) continue;

    if (isalpha(tmpline[0])) { // line is the start of a new contraction or END
      tok = strtok(line, " "); // read the element
      nextElement = tok;

      // For some reason, strtok is returning all uppercase letters for
      // the element, even though it's not this way in the ecce.out file.
      // This hack converts all letters but the first to lower case:
      for (int s = 1; s < nextElement.length(); s++) {
        nextElement[s] = tolower(nextElement[s]);
      }

      if (cont != 0) { // save vectors of coef and exponents

        cont->num_exponents = exponents.size();
        cont->num_coefficients = coefficients.size()/cont->num_exponents;
        cont->exponents = new double[cont->num_exponents];
        cont->coefficients = new double[coefficients.size()];

        for (i = 0; i < cont->num_exponents; i++)
          cont->exponents[i] = exponents[i];
        for (i = 0; i < coefficients.size(); i++)
          cont->coefficients[i] = coefficients[i];
    
        exponents.clear();
        coefficients.clear();
      
        // Fill in additional shells so they match the # of coefficients
        if (cont->shells.size() < cont->num_coefficients) {
          numExtraShells = cont->num_coefficients - cont->shells.size();
          shell = cont->shells[cont->shells.size()-1];
          for (i = 0; i < numExtraShells; i++)
            cont->shells.push_back(shell);
        }
          
        contSet->push_back(cont); // add the contraction to it's set
      }
      if (nextElement != curElement) { // new element or END

        if (newGBS != 0) { // Insert the current basis set into the config
          
          // Insert the contractons into the basis set
          newGBS->p_contractions[curElement] = contSet; 

          // Find the group for curElement (or make one if it doesnt exist)
          group = config->findGroup(curElement);
          if (group == 0) {
            group = new TGBSGroup();
            config->insertGBSGroup(curElement, group);
          }

          // Insert the basis set into the group
          if (newGBS->p_category == TGaussianBasisSet::ORBITAL) {
            vector<TGaussianBasisSet*> gbsList;
            gbsList.push_back(newGBS);
            group->insertOrbitalGBS(newGBS->p_name, gbsList);
            
          } else {
            group->insertGBS(newGBS);
          }                 
        }

        if (STLUtil::compareString(tmpline, "END") == 0) { // end of basis sets
          break;

        } else { // Prepare a new basis set and contraction set for the
                 // next element
          curElement = nextElement;
          contSet = new ContractionSet;
          newGBS = new TGaussianBasisSet(*gbsTemplate);
   
        }
        
      }
      cont = new Contraction_;
      tok = strtok((char*)0, " ");
      EE_RT_ASSERT(tok, EE_FATAL, "Invalid basis set specification--no shell.");

      for (i = 0; i < strlen(tok); i++) {
        shell = TGaussianBasisSet::charToShell(tok[i]);
        cont->shells.push_back(shell);
      }
      
    } else { // parse exp and coef
      tok = strtok(line, " ");

      value = strtod(tok, NULL);
      exponents.push_back(value);
   
      while ( (tok = strtok((char*)0, " ")) != (char*)0) {
        value = strtod(tok, NULL);
        coefficients.push_back(value);
      }
    }
  }
   
}

void ICalcUtils::importECPContractions(istream& infile,
                                       TGBSConfig* config) 
{
  EE_RT_ASSERT(0, EE_WARNING, 
               "ECPs not yet supported for import.  "
               "Omitting this component from the config.");

  char line[501];
  line[0] = '\0';

  // Skip the rest of this section from the file
  while(strncmp(line, "END", 3) != 0) {
    infile.getline(line, 500);
  }

}


