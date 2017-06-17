/**
 * @file
 *
 *
 */

#include "util/ResourceUtils.H"
#include "util/StringConverter.H"

#include "dsm/ResourceDescriptor.H"



/**
 * Returns the member of ResourceDescriptor's RESOURCETYPE enumeration that is
 * represented by the provided string value.
 */
ResourceDescriptor::RESOURCETYPE ResourceUtils::stringToResourceType(const string& str)
{
  ResourceDescriptor::RESOURCETYPE 
    rt = ResourceDescriptor::RT_UNDEFINED;

  string lstr = "";
  if (StringConverter::toLower(str, lstr)) {
    if (lstr == "file") {
      rt = ResourceDescriptor::RT_DOCUMENT;
    }
    else if (lstr == "collection") {
      rt = ResourceDescriptor::RT_COLLECTION;
    }
    else if (lstr == "virtual_document") {
      rt = ResourceDescriptor::RT_VIRTUAL_DOCUMENT;
    }
    else {
      rt = ResourceDescriptor::RT_UNDEFINED;
    }
  }

  return rt;
}


/**
 * Returns the string representation for a member of ResourceDescriptor's
 * RESOURCETYPE enumeration.
 */
string ResourceUtils::resourceTypeToString(ResourceDescriptor::RESOURCETYPE resourceType)
{
  string rtStr = "";
  
  switch (resourceType) {
  case ResourceDescriptor::RT_DOCUMENT:
    rtStr = "file";
    break;
  case ResourceDescriptor::RT_COLLECTION:
    rtStr = "collection";
    break;
  case ResourceDescriptor::RT_VIRTUAL_DOCUMENT:
    rtStr = "virtual_document";
    break;
  case ResourceDescriptor::RT_UNDEFINED:
  case ResourceDescriptor::NUMBER_OF_RT:
    //rtStr = "undefined";
    break;
  }

  return rtStr;
}


/**
 * Returns the member of ResourceDescriptor's CONTENTTYPE enumeration that is
 * represented by the provided string value.
 */
ResourceDescriptor::CONTENTTYPE ResourceUtils::stringToContentType(const string& str)
{
  ResourceDescriptor::CONTENTTYPE 
    ct = ResourceDescriptor::CT_UNDEFINED;

  string lstr = "";
  if (StringConverter::toLower(str, lstr)) {
    if ( (lstr == "ecceproject") ||
         (lstr == "httpd/unix-directory") ) {
      ct = ResourceDescriptor::CT_PROJECT;
    }
    else if (lstr == "eccesession") {
      ct = ResourceDescriptor::CT_SESSION;
    }
    else if (lstr == "eccecalculation") {
      ct = ResourceDescriptor::CT_CALCULATION;
    }
    else if (lstr == "eccemdprepare") {
      ct = ResourceDescriptor::CT_MDPREPARE;
    }
    else if (lstr == "eccemdoptimize") {
      ct = ResourceDescriptor::CT_MDOPTIMIZE;
    }
    else if (lstr == "eccemdenergy") {
      ct = ResourceDescriptor::CT_MDENERGY;
    }
    else if (lstr == "eccemddynamics") {
      ct = ResourceDescriptor::CT_MDDYNAMICS;
    }
    else if (lstr == "chemical/x-ecce-mvm") {
      ct = ResourceDescriptor::CT_CHEMICAL_MVM;
    }
    else if (lstr == "chemical/x-pdb") {
      ct = ResourceDescriptor::CT_CHEMICAL_PDB;
    }
    else {
      ct = ResourceDescriptor::CT_UNDEFINED;
    }
  }

  return ct;
}


/**
 * Returns the string representation for a member of ResourceDescriptor's
 * CONTENTTYPE enumeration.
 */
string ResourceUtils::contentTypeToString(ResourceDescriptor::CONTENTTYPE contentType)
{
  string ctStr = "";

  switch (contentType) {
  case ResourceDescriptor::CT_PROJECT:
    ctStr = "ecceProject";
    break;
  case ResourceDescriptor::CT_SESSION:
    ctStr = "ecceSession";
    break;
  case ResourceDescriptor::CT_CALCULATION:
    ctStr = "ecceCalculation";
    break;
  case ResourceDescriptor::CT_MDPREPARE:
    ctStr = "ecceMdPrepare";
    break;
  case ResourceDescriptor::CT_MDOPTIMIZE:
    ctStr = "ecceMdOptimize";
    break;
  case ResourceDescriptor::CT_MDENERGY:
    ctStr = "ecceMdEnergy";
    break;
  case ResourceDescriptor::CT_MDDYNAMICS:
    ctStr = "ecceMdDynamics";
    break;
  case ResourceDescriptor::CT_CHEMICAL_MVM:
    ctStr = "chemical/x-ecce-mvm";
    break;
  case ResourceDescriptor::CT_CHEMICAL_PDB:
    ctStr = "chemical/x-pdb";
    break;
  case ResourceDescriptor::CT_UNDEFINED:
  case ResourceDescriptor::NUMBER_OF_CT:
    //ctStr = "undefined";
    break;
  }

  return ctStr;
}


/**
 * Returns the member of ResourceDescriptor's APPLICATIONTYPE enumeration that is
 * represented by the provided string value.
 */
ResourceDescriptor::APPLICATIONTYPE ResourceUtils::stringToApplicationType(const string& str)
{
  ResourceDescriptor::APPLICATIONTYPE
    at = ResourceDescriptor::AT_UNDEFINED;

  string lstr = "";
  if (StringConverter::toLower(str, lstr)) {
    if (lstr == "mdstudy") {
      at = ResourceDescriptor::AT_MDSTUDY;
    }
    else if (lstr == "nwchemmd") {
      at = ResourceDescriptor::AT_NWCHEMMD;
    }
    else if (lstr == "reactionstudy") {
      at = ResourceDescriptor::AT_REACTION_STUDY;
    }
    else if (lstr == "condensedreactionstudy") {
      at = ResourceDescriptor::AT_CONDENSED_REACTION_STUDY;
    }
    else if (lstr == "dirdyvtst") {
      at = ResourceDescriptor::AT_DRDVTST;
    }
    else if (lstr == "polyrate") {
      at = ResourceDescriptor::AT_POLYRATE;
    }
    else if (lstr == "solvate") {
      at = ResourceDescriptor::AT_SOLVATE;
    }
    else if (lstr == "metadynamics") {
      at = ResourceDescriptor::AT_METADYNAMICS;
    }
    else {
      at = ResourceDescriptor::AT_UNDEFINED;
    }
  }

  return at;
}

/**
 * Returns the string representation for a member of ResourceDescriptor's
 * APPLICATIONTYPE enumeration.
 */
string ResourceUtils::applicationTypeToString(ResourceDescriptor::APPLICATIONTYPE applicationType)
{
  string atStr = "";

  switch(applicationType) {
  case ResourceDescriptor::AT_MDSTUDY:
    atStr = "MDStudy";
    break;
  case ResourceDescriptor::AT_NWCHEMMD:
    atStr = "NWChemMD";
    break;
  case ResourceDescriptor::AT_REACTION_STUDY:
    atStr = "ReactionStudy";
    break;
  case ResourceDescriptor::AT_CONDENSED_REACTION_STUDY:
    atStr = "CondensedReactionStudy";
    break;
  case ResourceDescriptor::AT_DRDVTST:
    atStr = "DirDyVTST";
    break;
  case ResourceDescriptor::AT_POLYRATE:
    atStr = "Polyrate";
    break;
  case ResourceDescriptor::AT_SOLVATE:
    atStr = "Solvate";
    break;
  case ResourceDescriptor::AT_METADYNAMICS:
    atStr = "Metadynamics";
    break;
  case ResourceDescriptor::AT_UNDEFINED:
  case ResourceDescriptor::NUMBER_OF_AT:
    //atStr = "undefined";
    break;
  }

  return atStr;
}


/**
 * Returns the member of ResourceDescriptor's RUNSTATE enumeration that is
 * represented by the provided string value.
 */
ResourceDescriptor::RUNSTATE ResourceUtils::stringToState(const string& str)
{
  ResourceDescriptor::RUNSTATE 
    state = ResourceDescriptor::STATE_ILLEGAL;

  string lstr = "";
  if (StringConverter::toLower(str, lstr)) {
    if (lstr == "created") {
      state = ResourceDescriptor::STATE_CREATED;
    }
    else if (lstr == "ready") {
      state = ResourceDescriptor::STATE_READY;
    }
    else if (lstr == "submitted") {
      state = ResourceDescriptor::STATE_SUBMITTED;
    }
    else if (lstr == "running") {
      state = ResourceDescriptor::STATE_RUNNING;
    }
    else if (lstr == "complete") {
      state = ResourceDescriptor::STATE_COMPLETED;
    }
    else if (lstr == "loaded") {
      state = ResourceDescriptor::STATE_LOADED;
    }
    else if (lstr == "killed") {
      state = ResourceDescriptor::STATE_KILLED;
    }
    else if (lstr == "unsuccessful") {
      state = ResourceDescriptor::STATE_UNSUCCESSFUL;
    }
    else if (lstr == "failed") {
      state = ResourceDescriptor::STATE_FAILED;
    }
    else if (lstr == "system") {
      state = ResourceDescriptor::STATE_SYSTEM_FAILURE;
    }
    else {
      state = ResourceDescriptor::STATE_ILLEGAL;
    }
  }

  return state;
}


/**
 * Returns the string representation for a member of ResourceDescriptor's
 * RUNSTATE enumeration.
 */
string ResourceUtils::stateToString(ResourceDescriptor::RUNSTATE state)
{
  string stateStr = "";

  switch (state) {
  case ResourceDescriptor::STATE_CREATED:
    stateStr = "Created";
    break;
  case ResourceDescriptor::STATE_READY:
    stateStr = "Ready";
    break;
  case ResourceDescriptor::STATE_SUBMITTED:
    stateStr = "Submitted";
    break;
  case ResourceDescriptor::STATE_RUNNING:
    stateStr = "Running";
    break;
  case ResourceDescriptor::STATE_COMPLETED:
    stateStr = "Complete";
    break;
  case ResourceDescriptor::STATE_LOADED:
    stateStr = "Loaded";
    break;
  case ResourceDescriptor::STATE_KILLED:
    stateStr = "Killed";
    break;
  case ResourceDescriptor::STATE_UNSUCCESSFUL:
    stateStr = "Unsuccessful";
    break;
  case ResourceDescriptor::STATE_FAILED:
    stateStr = "Failed";
    break;
  case ResourceDescriptor::STATE_SYSTEM_FAILURE:
    stateStr = "System";
    break;
  default:
    stateStr = "Illegal";
  }

  return stateStr;
}

ResourceDescriptor::IconType ResourceUtils::stringToIconType(const string& str)
{
  ResourceDescriptor::IconType
    iconType = ResourceDescriptor::IT_UNDEFINED;

  string lstr = "";
  if (StringConverter::toLower(str, lstr)) {
    if (lstr == "normal") {
      iconType = ResourceDescriptor::IT_NORMAL;
    }
    else if (lstr == "selected") {
      iconType = ResourceDescriptor::IT_SELECTED;
    }
    else if (lstr == "expanded") {
      iconType = ResourceDescriptor::IT_EXPANDED;
    }
    else if (lstr == "selectedexpanded") {
      iconType = ResourceDescriptor::IT_SELECTED_EXPANDED;
    }
    else {
      iconType = ResourceDescriptor::IT_UNDEFINED;
    }
  }

  return iconType;
}


string ResourceUtils::iconTypeToString(ResourceDescriptor::IconType iconType)
{
  string itStr = "";

  switch (iconType) {
  case ResourceDescriptor::IT_NORMAL:
    itStr = "normal";
    break;
  case ResourceDescriptor::IT_SELECTED:
    itStr = "selected";
    break;
  case ResourceDescriptor::IT_EXPANDED:
    itStr = "expanded";
    break;
  case ResourceDescriptor::IT_SELECTED_EXPANDED:
    itStr = "selectedexpanded";
    break;
  case ResourceDescriptor::IT_UNDEFINED:
    //itStr = "undefined";
    break;
  }

  return itStr;
}


ResourceDescriptor::ResourceCategory 
ResourceUtils::stringToResourceCategory(const string& str)
{
  ResourceDescriptor::ResourceCategory
    resourceCategory = ResourceDescriptor::RC_UNDEFINED;

  string lstr = "";
  if (StringConverter::toLower(str, lstr)) {
    if (lstr == "document") {
      resourceCategory = ResourceDescriptor::RC_DOCUMENT;
    }
    else if (lstr == "project") {
      resourceCategory = ResourceDescriptor::RC_PROJECT;
    }
    else if (lstr == "escalculation") {
      resourceCategory = ResourceDescriptor::RC_ES_CALCULATION;
    }
    else if (lstr == "mdsession") {
      resourceCategory = ResourceDescriptor::RC_MD_SESSION;
    }
    else if (lstr == "reactionsession") {
      resourceCategory = ResourceDescriptor::RC_REACTION_SESSION;
    }
    else if (lstr == "condensedreactionsession") {
      resourceCategory = ResourceDescriptor::RC_CONDENSED_REACTION_SESSION;
    }
    else if (lstr == "mdtask") {
      resourceCategory = ResourceDescriptor::RC_MD_TASK;
    }
    else if (lstr == "reactiontask") {
      resourceCategory = ResourceDescriptor::RC_REACTION_TASK;
    }
    else if (lstr == "condensedreactiontask") {
      resourceCategory = ResourceDescriptor::RC_CONDENSED_REACTION_TASK;
    }
    else {
      resourceCategory = ResourceDescriptor::RC_UNDEFINED;
    }
  }
  return resourceCategory;
}


string ResourceUtils::resourceCategoryToString(
  ResourceDescriptor::ResourceCategory resourceCategory)
{
  string rcStr = "";

  switch (resourceCategory) {
  case ResourceDescriptor::RC_DOCUMENT:
    rcStr = "document";
    break;
  case ResourceDescriptor::RC_PROJECT:
    rcStr = "project";
    break;
  case ResourceDescriptor::RC_ES_CALCULATION:
    rcStr = "escalculation";
    break;
  case ResourceDescriptor::RC_MD_SESSION:
    rcStr = "mdsession";
    break;
  case ResourceDescriptor::RC_REACTION_SESSION:
    rcStr = "reactionsession";
    break;
  case ResourceDescriptor::RC_CONDENSED_REACTION_SESSION:
    rcStr = "condensedreactionsession";
    break;
  case ResourceDescriptor::RC_MD_TASK:
    rcStr = "mdtask";
    break;
  case ResourceDescriptor::RC_REACTION_TASK:
    rcStr = "reactiontask";
    break;
  case ResourceDescriptor::RC_CONDENSED_REACTION_TASK:
    rcStr = "condensedreactiontask";
    break;
  case ResourceDescriptor::RC_UNDEFINED:
    //rcStr = "undefined";
    break;
  }

  return rcStr;
}






