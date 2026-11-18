/**
 * @file
 *
 * See GBSAssign.H.
 */
#include <vector>

#include "util/ErrMsg.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/GBSAssign.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGBSGroup.H"

using std::string;
using std::vector;

GBSAssign::Result GBSAssign::assign(EDSIGaussianBasisSetLibrary& library,
    TGBSConfig *config, const string& tag, const string& bsName,
    TGaussianBasisSet::GBSType bstype, bool multAllowed)
{
  Result res;
  string notCovered = "Basis set " + bsName + " does not cover " + tag;

  vector<TGaussianBasisSet*> gbsList =
      library.lookup(bsName.c_str(), bstype, tag.c_str());
  if (gbsList.empty()) {
    res.severity = ERROR;
    res.message = notCovered;
    return res;
  }

  // An aggregate (ORCA's def2 sets) returns its ECP part for every element,
  // empty where the element has no core; kept, it would show Def2-ECP on H.
  // A group may hold several elements ("H I"): keep it if any has a core.
  TGBSConfigTags els(tag.c_str());
  for (size_t i = 0; i < gbsList.size();) {
    bool core = false;
    for (const string& el : els)
      core = core || gbsList[i]->p_contractions.count(el);
    if (gbsList[i]->p_type == TGaussianBasisSet::ecp && els.size() && !core) {
      delete gbsList[i];
      gbsList.erase(gbsList.begin() + i);
    } else {
      i++;
    }
  }
  if (gbsList.empty()) {
    res.severity = ERROR;
    res.message = notCovered;
    return res;
  }

  TGBSConfig::iterator groupIt = config->find(tag);
  EE_ASSERT((groupIt != config->end()), EE_FATAL,
            ("Trying to insert basis set into nonexistent or improperly "
             "labeled group: " + tag).c_str());
  TGBSGroup *group = groupIt->second;
  res.group = group;

  if ((group == NULL || group->getOrbitalGBSMappings()->size() == 0) &&
      gbsList[0]->p_category == TGaussianBasisSet::AUXILIARY) {
    res.severity = ERROR;
    res.message = "You first must supply an orbital basis set for group " + tag;
    return res;
  }

  if (gbsList[0]->p_category == TGaussianBasisSet::ORBITAL) {
    if (!library.isSupported(bsName, bstype, tag)) {
      res.group = 0;
      res.severity = ERROR;
      res.message = notCovered;
      return res;
    }
    bool replaceGroup = false;
    if (!multAllowed || group == NULL) {
      config->removeGBSGroup(tag);
      group = new TGBSGroup();
      replaceGroup = true;
    }
    // recompute: appended tight/diffuse functions follow the new exponents
    group->insertOrbitalGBS(bsName, gbsList, true);
    if (replaceGroup) config->insertGBSGroup(tag, group);
    res.group = group;
    return res;
  }

  if (!library.isSupported(bsName, bstype, tag)) {
    res.severity = WARNING;
    res.message = notCovered;
  }
  group = groupIt->second;
  group->insertGBS(new TGaussianBasisSet(*(gbsList[0])), true);
  res.group = group;
  return res;
}
