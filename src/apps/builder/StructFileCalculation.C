#include <fstream>
#include <set>
  using std::set;

#include <wx/link.h>
#include <wx/log.h>

#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"

#include "tdat/DockFragmentIO.H"
#include "tdat/Fragment.H"

#include "dock/DockPrep.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "StructFileCalculation.H"


wxFORCE_LINK_THIS_MODULE(StructFileCalculation)


IMPLEMENT_CALCULATION(SdfCalculation, SDF,
    SdfCalculation::canOpen, SdfCalculation::open, SdfCalculation::inputMasks)
IMPLEMENT_CALCULATION(Mol2Calculation, MOL2,
    Mol2Calculation::canOpen, Mol2Calculation::open, Mol2Calculation::inputMasks)
IMPLEMENT_CALCULATION(PdbqtCalculation, PDBQT,
    PdbqtCalculation::canOpen, PdbqtCalculation::open,
    PdbqtCalculation::inputMasks)
IMPLEMENT_CALCULATION(ObabelCalculation, Other formats (Open Babel),
    ObabelCalculation::canOpen, ObabelCalculation::open,
    ObabelCalculation::inputMasks)


StructFileCalculation::StructFileCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


string StructFileCalculation::extension(const EcceURL& url)
{
  return STLUtil::upper(STLUtil::afterLast(url, "."));
}


bool StructFileCalculation::fragment(Fragment *)
{
  return false;
}


bool StructFileCalculation::getFragment(Fragment &frag)
{
  Resource *resource = EDSIFactory::getResource(p_url);
  if (resource == 0) return false;

  SFile *file = TempStorage::getTempFile();
  if (resource->getDocument(file) == 0) {
    file->remove();
    delete file;
    return false;
  }
  bool ret = DockFragmentIO::read(frag, file->path(), extension(p_url));
  if (!ret) {
    wxLogError("Cannot open %s: %s", p_url.toString().c_str(),
               DockFragmentIO::lastError().c_str());
  }
  file->remove();
  delete file;
  return ret;
}


EcceURL StructFileCalculation::getURL() const
{
  return p_url;
}


bool StructFileCalculation::isReadOnly() const
{
  return false;
}


bool StructFileCalculation::canSaveInPlace() const
{
  return false;
}


static bool maskHas(const string& mask, const string& ext)
{
  vector<string> toks = StringTokenizer(mask).tokenize(";");
  for (size_t i = 0; i < toks.size(); i++) {
    if (STLUtil::upper(toks[i]) == "*." + ext) return true;
  }
  return false;
}


bool SdfCalculation::canOpen(const EcceURL& url)
{
  string e = extension(url);
  return e == "SDF" || e == "MOL" || e == "SD";
}

IPropCalculation *SdfCalculation::open(const EcceURL& url)
{
  return new SdfCalculation(url);
}

string SdfCalculation::inputMasks()
{
  return "*.sdf;*.SDF;*.mol;*.MOL;*.sd;*.SD";
}


bool Mol2Calculation::canOpen(const EcceURL& url)
{
  return extension(url) == "MOL2";
}

IPropCalculation *Mol2Calculation::open(const EcceURL& url)
{
  return new Mol2Calculation(url);
}

string Mol2Calculation::inputMasks()
{
  return "*.mol2;*.MOL2";
}


bool PdbqtCalculation::canOpen(const EcceURL& url)
{
  return extension(url) == "PDBQT";
}

IPropCalculation *PdbqtCalculation::open(const EcceURL& url)
{
  return new PdbqtCalculation(url);
}

string PdbqtCalculation::inputMasks()
{
  return "*.pdbqt;*.PDBQT";
}


// The extensions the other plugins already claim; the Factory asks every
// plugin in name order, so this one must say no to them itself.
static bool claimedElsewhere(const string& ext)
{
  PropCalcInfoMap infoMap = CalculationInfo::GetInfo();
  for (PropCalcInfoMapIter it = infoMap.begin(); it != infoMap.end(); ++it) {
    const string& name = it->second->GetName();
    if (name == "ObabelCalculation" || name == "PropertyTaskFactoryHelper")
      continue;
    if (maskHas(it->second->GetOpenMask(), ext)) return true;
  }
  return false;
}


bool ObabelCalculation::canOpen(const EcceURL& url)
{
  string e = extension(url);
  return !e.empty() && !claimedElsewhere(e) &&
         DockObabel::canRead(STLUtil::lower(e));
}

IPropCalculation *ObabelCalculation::open(const EcceURL& url)
{
  return new ObabelCalculation(url);
}

// Empty without obabel, so the file dialog shows no such entry.
string ObabelCalculation::inputMasks()
{
  const vector<string>& fmts = DockObabel::readFormats();
  string mask;
  for (size_t i = 0; i < fmts.size(); i++) {
    string up = STLUtil::upper(fmts[i]);
    if (claimedElsewhere(up)) continue;
    mask += "*." + fmts[i] + ";*." + up + ";";
  }
  if (!mask.empty()) mask.erase(mask.end() - 1);
  return mask;
}
