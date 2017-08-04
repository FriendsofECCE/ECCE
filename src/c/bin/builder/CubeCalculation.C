
#include <wx/link.h>

#include "util/STLUtil.H"

#include "tdat/Fragment.H"
#include "tdat/PropGrids.H"

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "CubeCalculation.H"


wxFORCE_LINK_THIS_MODULE(CubeCalculation)


IMPLEMENT_CALCULATION(
        CubeCalculation,
        Cube,
        CubeCalculation::canOpen,
        CubeCalculation::open,
        CubeCalculation::inputMasks
)


CubeCalculation::CubeCalculation(const EcceURL& url)
  : AbstractPropCalculation(),
    p_url(url)
{
}


CubeCalculation::~CubeCalculation()
{
}


vector<string> CubeCalculation::propertyNames(void)
{
  vector<string> names;
  names.push_back("CUBE");
  return names;
}


bool CubeCalculation::getFragment(Fragment& frag)
{
  bool ret = false;
  
  // Although the Cube panel will also parse the cube file, we must do so
  // now as well so that we can get the chemical system out of it.
  // Get the Resource, then its contents as an istream.
  Resource * resource = EDSIFactory::getResource(getURL());
  if (resource) {
    istream *is = resource->getDocument();
    if (is) {
      PropGrids propGrids;
      if (propGrids.readCubeFile(*is)) {
        ret = true;
        frag = propGrids.getFrag();
      } else {
      }
      delete is;
    }
  }

  return ret;
}


EcceURL CubeCalculation::getURL() const
{
  return p_url;
}


bool CubeCalculation::isReadOnly() const
{
  return true;
}


bool CubeCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "CUBE") {
    ret = true;
  }
  return ret;
}


IPropCalculation *CubeCalculation::open(const EcceURL& url)
{
  return new CubeCalculation(url);
}


string CubeCalculation::inputMasks()
{
  return "*.cube";
}


