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

#include "MvmCalculation.H"


wxFORCE_LINK_THIS_MODULE(MvmCalculation)


IMPLEMENT_CALCULATION(
        MvmCalculation,
        ECCE Internal MVM,
        MvmCalculation::canOpen,
        MvmCalculation::open,
        MvmCalculation::inputMasks)



MvmCalculation::MvmCalculation()
  : AbstractPropCalculation()
  , p_url("")
{
}


MvmCalculation::MvmCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


bool MvmCalculation::fragment(Fragment *frag)
{
  bool ret = false;

  SFile *mvm_file = TempStorage::getTempFile();
  ofstream mvm_stream(mvm_file->path().c_str());
  if (mvm_stream) {
    if (frag->dumpMVM(mvm_stream)) {
      EDSI *edsi = EDSIFactory::getEDSI(p_url);
      if (edsi) {
        ifstream ifs(mvm_file->path().c_str());
        if (ifs) {
          if (edsi->putDataSet(ifs)) {
            ret = true;
          }
          ifs.close();
        }
        delete edsi;
      }
    }
    mvm_stream.close();
  }
  mvm_file->remove();
  delete mvm_file;

  return ret;
}


bool MvmCalculation::getFragment(Fragment &frag)
{
  bool ret = false;

  Resource *resource = EDSIFactory::getResource(p_url);

  // create temporary file for fragment reading
  SFile *file = TempStorage::getTempFile();
  file = resource->getDocument(file);
  ifstream is(file->path().c_str());

  ret = frag.restoreMVM(is);
  
  // cleanup
  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL MvmCalculation::getURL() const
{
  return p_url;
}


bool MvmCalculation::isReadOnly() const
{
  return false;
}


// IPropCalculation framework
bool MvmCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "MVM") {
    ret = true;
  }

  return ret;
}


IPropCalculation *MvmCalculation::open(const EcceURL& url)
{
  return new MvmCalculation(url);
}


string MvmCalculation::inputMasks()
{
  return "*.mvm";
}

