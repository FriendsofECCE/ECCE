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

#include "CifCalculation.H"


wxFORCE_LINK_THIS_MODULE(CifCalculation)


IMPLEMENT_CALCULATION(
        CifCalculation,
        CIF,
        CifCalculation::canOpen,
        CifCalculation::open,
        CifCalculation::inputMasks)



CifCalculation::CifCalculation()
  : AbstractPropCalculation()
  , p_url("")
{
}


CifCalculation::CifCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


bool CifCalculation::fragment(Fragment *frag)
{
  bool ret = false;

  SFile *cif_file = TempStorage::getTempFile();
  ofstream cif_stream(cif_file->path().c_str());
  if (cif_stream) {
    if (frag->dumpCIF(cif_stream)) {
      EDSI *edsi = EDSIFactory::getEDSI(p_url);
      if (edsi) {
        ifstream ifs(cif_file->path().c_str());
        if (ifs) {
          if (edsi->putDataSet(ifs)) {
            ret = true;
          }
          ifs.close();
        }
        delete edsi;
      }
    }
    cif_stream.close();
  }
  cif_file->remove();
  delete cif_file;

  return ret;
}


bool CifCalculation::getFragment(Fragment &frag)
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

  ret = frag.restoreCIF(is, true);
  
  // cleanup
  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL CifCalculation::getURL() const
{
  return p_url;
}


bool CifCalculation::isReadOnly() const
{
  return false;
}


// IPropCalculation framework
bool CifCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "CIF") {
    ret = true;
  }

  return ret;
}


IPropCalculation *CifCalculation::open(const EcceURL& url)
{
  return new CifCalculation(url);
}


string CifCalculation::inputMasks()
{
  return "*.cif;*.CIF";
}

