#include <fstream>
  using std::ifstream;
  using std::ofstream;

#include <wx/link.h>

#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/TempStorage.H"

#include "tdat/Fragment.H"

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "SdfCalculation.H"


wxFORCE_LINK_THIS_MODULE(SdfCalculation)


IMPLEMENT_CALCULATION(
        SdfCalculation,
        SDF,
        SdfCalculation::canOpen,
        SdfCalculation::open,
        SdfCalculation::inputMasks)



SdfCalculation::SdfCalculation()
  : AbstractPropCalculation()
  , p_url("")
{
}


SdfCalculation::SdfCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


bool SdfCalculation::fragment(Fragment *frag)
{
  //  Read-only: ECCE has no SDF writer.
  return false;
}


bool SdfCalculation::getFragment(Fragment &frag)
{
  bool ret = false;

  Resource *resource = EDSIFactory::getResource(p_url);
  if (resource == 0) return false;

  // create temporary file for fragment reading
  SFile *file = TempStorage::getTempFile();
  if (resource->getDocument(file) == 0) {
    file->remove();
    delete file;
    return false;
  }
  ifstream is(file->path().c_str());

  ret = frag.restoreSDF(is, true);
  
  // cleanup
  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL SdfCalculation::getURL() const
{
  return p_url;
}


bool SdfCalculation::isReadOnly() const
{
  return false;
}


// IPropCalculation framework
bool SdfCalculation::canOpen(const EcceURL& url)
{
  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  return ext == "SDF" || ext == "MOL";
}


IPropCalculation *SdfCalculation::open(const EcceURL& url)
{
  return new SdfCalculation(url);
}


string SdfCalculation::inputMasks()
{
  return "*.sdf;*.SDF;*.mol;*.MOL";
}
