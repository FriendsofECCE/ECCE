#include <assert.h>

#include <algorithm>
  using std::copy;
#include <iterator>
  using std::inserter;
#include <set>
  using std::set;
#include <vector>
  using std::vector;

#include <wx/link.h>

#include "util/ResourceUtils.H"
#include "util/StringTokenizer.H"

#include "dsm/EDSIFactory.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"
#include "dsm/ResourceType.H"

#include "CalculationFactory.H"

wxFORCE_LINK_MODULE(CarCalculation)
wxFORCE_LINK_MODULE(CubeCalculation)
wxFORCE_LINK_MODULE(PdbCalculation)
wxFORCE_LINK_MODULE(MvmCalculation)
wxFORCE_LINK_MODULE(XyzCalculation)


PropCalcInfoMap *CalculationInfo::s_info(NULL);


void CalculationInfo::Register()
{
  if (!s_info) {
    PropCalcInfoMap *info = new PropCalcInfoMap;
    // check for reentrance
    if (s_info)
      delete info;
    else
      s_info = info;
  }

  // reentrance guard
  static int entry = 0;
  assert(++entry == 1);

  PropCalcInfoMapIter it = s_info->find(p_name);
  assert(it == s_info->end());
  s_info->insert(make_pair(p_name,this));

  --entry;
}


void CalculationInfo::Unregister()
{
  if (s_info) {
    s_info->erase(p_name);
    if (s_info->empty()) {
      delete s_info;
      s_info = NULL;
    }
  }
}


const CalculationInfo *CalculationInfo::Find(const string& name)
{
  PropCalcInfoMapIter it = s_info->find(name);
  if (it != s_info->end()) {
    return it->second;
  }
  return NULL;
}


bool CalculationFactory::canOpen(const EcceURL& url)
{
  PropCalcInfoMap infoMap = CalculationInfo::GetInfo();
  PropCalcInfoMapIter info, info_end;
  info = infoMap.begin();
  info_end = infoMap.end();
  while (info != info_end) {
    if (info->second->CanOpen(url)) {
      return true;
    }
    ++info;
  }
  return false;
}


IPropCalculation *CalculationFactory::open(const EcceURL& url)
{
  PropCalcInfoMap infoMap = CalculationInfo::GetInfo();
  PropCalcInfoMapIter info, info_end;
  info = infoMap.begin();
  info_end = infoMap.end();
  while (info != info_end) {
    if (info->second->CanOpen(url)) {
      IPropCalculation *calc = info->second->Open(url);
      if (calc) return calc;
    }
    ++info;
  }
  return NULL;
}


string CalculationFactory::openMask()
{
  string ret;

  // append together all input masks
  // e.g. "*.xyz;*.XYZ", "*.pdb;*.PDB"
  PropCalcInfoMap infoMap = CalculationInfo::GetInfo();
  PropCalcInfoMapIter info, info_end;
  info = infoMap.begin();
  info_end = infoMap.end();
  while (info != info_end) {
    if (info->second->GetOpenMask() != "") {
      ret += info->second->GetOpenMask() + ";";
    }
    ++info;
  }
  ret.erase(ret.end()-1); // remove last ';'

  // now make sure we don't have duplicates
  vector<string> tokens = StringTokenizer(ret).tokenize(";");
  set<string> tmp;
  copy(tokens.begin(), tokens.end(), inserter(tmp, tmp.end()));
  ret = "";
  for (set<string>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
    ret += *it + ";";
  }
  ret.erase(ret.end()-1); // remove last ';'

  return ret;
}


IMPLEMENT_CALCULATION(
        PropertyTaskFactoryHelper,
        ECCE Calculations,
        PropertyTaskFactoryHelper::canOpen,
        PropertyTaskFactoryHelper::open,
        PropertyTaskFactoryHelper::inputMasks)


bool PropertyTaskFactoryHelper::canOpen(const EcceURL& url)
{
  return dynamic_cast<IPropCalculation*>(EDSIFactory::getResource(url));
}


IPropCalculation *PropertyTaskFactoryHelper::open(const EcceURL& url)
{
  return dynamic_cast<IPropCalculation*>(EDSIFactory::getResource(url));
}


string PropertyTaskFactoryHelper::inputMasks()
{
  // HACK - (standalone) ebuilder sets this env var
  if (getenv("ECCE_NO_MESSAGING") == NULL) {
    string wildcard;
    ResourceDescriptor &rd = ResourceDescriptor::getResourceDescriptor();
    vector<ResourceType*> typesVec = rd.getResourceTypes();
    vector<ResourceType*>::iterator types;
    for (types = typesVec.begin(); types != typesVec.end(); types++) {
      ResourceType *type = *types;
      ResourceDescriptor::CONTENTTYPE ct =
              ResourceUtils::stringToContentType(type->getContentType());
      if (ct == ResourceDescriptor::CT_CALCULATION
              || ct == ResourceDescriptor::CT_MDPREPARE
              || ct == ResourceDescriptor::CT_MDOPTIMIZE
              || ct == ResourceDescriptor::CT_MDENERGY
              || ct == ResourceDescriptor::CT_MDDYNAMICS) {
        wildcard.append(type->getName() + ";");
      }
    }
    wildcard.erase(wildcard.end() - 1); // remove last ";"
    return wildcard; 
  } else {
    return "";
  }
}

