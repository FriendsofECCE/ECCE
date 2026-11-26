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

#include "Mol2Calculation.H"


wxFORCE_LINK_THIS_MODULE(Mol2Calculation)


IMPLEMENT_CALCULATION(
        Mol2Calculation,
        MOL2,
        Mol2Calculation::canOpen,
        Mol2Calculation::open,
        Mol2Calculation::inputMasks)



Mol2Calculation::Mol2Calculation()
  : AbstractPropCalculation()
  , p_url("")
{
}


Mol2Calculation::Mol2Calculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


bool Mol2Calculation::fragment(Fragment *frag)
{
  //  Read-only: ECCE has no MOL2 writer.
  return false;
}


bool Mol2Calculation::getFragment(Fragment &frag)
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

  ret = frag.restoreMOL2(is, true);
  
  // cleanup
  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL Mol2Calculation::getURL() const
{
  return p_url;
}


bool Mol2Calculation::isReadOnly() const
{
  return false;
}


// IPropCalculation framework
bool Mol2Calculation::canOpen(const EcceURL& url)
{
  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  return ext == "MOL2";
}


IPropCalculation *Mol2Calculation::open(const EcceURL& url)
{
  return new Mol2Calculation(url);
}


string Mol2Calculation::inputMasks()
{
  return "*.mol2;*.MOL2";
}
