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

#include "CarCalculation.H"


wxFORCE_LINK_THIS_MODULE(CarCalculation)


IMPLEMENT_CALCULATION(
        CarCalculation,
        CAR,
        CarCalculation::canOpen,
        CarCalculation::open,
        CarCalculation::inputMasks)



CarCalculation::CarCalculation()
  : AbstractPropCalculation()
  , p_url("")
{
}


CarCalculation::CarCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


bool CarCalculation::fragment(Fragment *frag)
{
  bool ret = false;

  SFile *car_file = TempStorage::getTempFile();
  ofstream car_stream(car_file->path().c_str());
  if (car_stream) {
    if (frag->dumpCAR(car_stream)) {
      EDSI *edsi = EDSIFactory::getEDSI(p_url);
      if (edsi) {
        ifstream ifs(car_file->path().c_str());
        if (ifs) {
          if (edsi->putDataSet(ifs)) {
            ret = true;
          }
          ifs.close();
        }
        delete edsi;
      }
    }
    car_stream.close();
  }
  car_file->remove();
  delete car_file;

  return ret;
}


bool CarCalculation::getFragment(Fragment &frag)
{
  bool ret = false;

  Resource *resource = EDSIFactory::getResource(p_url);

  // create temporary file for fragment reading
  SFile *file = TempStorage::getTempFile();
  file = resource->getDocument(file);
  ifstream is(file->path().c_str());

  ret = frag.restoreCAR(is, 1.0, true);
  
  // cleanup
  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL CarCalculation::getURL() const
{
  return p_url;
}


bool CarCalculation::isReadOnly() const
{
  return false;
}


// IPropCalculation framework
bool CarCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "CAR") {
    ret = true;
  }

  return ret;
}


IPropCalculation *CarCalculation::open(const EcceURL& url)
{
  return new CarCalculation(url);
}


string CarCalculation::inputMasks()
{
  return "*.car;*.CAR";
}

