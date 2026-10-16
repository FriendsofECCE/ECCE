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

#include "GroCalculation.H"


wxFORCE_LINK_THIS_MODULE(GroCalculation)


IMPLEMENT_CALCULATION(
        GroCalculation,
        GRO,
        GroCalculation::canOpen,
        GroCalculation::open,
        GroCalculation::inputMasks)



GroCalculation::GroCalculation()
  : AbstractPropCalculation()
  , p_url("")
{
}


GroCalculation::GroCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


bool GroCalculation::fragment(Fragment *frag)
{
  bool ret = false;

  SFile *gro_file = TempStorage::getTempFile();
  ofstream gro_stream(gro_file->path().c_str());
  if (gro_stream) {
    if (frag->dumpGRO(gro_stream)) {
      EDSI *edsi = EDSIFactory::getEDSI(p_url);
      if (edsi) {
        ifstream ifs(gro_file->path().c_str());
        if (ifs) {
          if (edsi->putDataSet(ifs)) {
            ret = true;
          }
          ifs.close();
        }
        delete edsi;
      }
    }
    gro_stream.close();
  }
  gro_file->remove();
  delete gro_file;

  return ret;
}


bool GroCalculation::getFragment(Fragment &frag)
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

  ret = frag.restoreGRO(is, true);
  
  // cleanup
  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL GroCalculation::getURL() const
{
  return p_url;
}


bool GroCalculation::isReadOnly() const
{
  return false;
}


// IPropCalculation framework
bool GroCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "GRO") {
    ret = true;
  }

  return ret;
}


IPropCalculation *GroCalculation::open(const EcceURL& url)
{
  return new GroCalculation(url);
}


string GroCalculation::inputMasks()
{
  return "*.gro;*.GRO";
}

