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

#include "wxgui/WxUnitsPrompt.H"

#include "XyzCalculation.H"


wxFORCE_LINK_THIS_MODULE(XyzCalculation)


IMPLEMENT_CALCULATION(
        XyzCalculation,
        XYZ,
        XyzCalculation::canOpen,
        XyzCalculation::open,
        XyzCalculation::inputMasks)



XyzCalculation::XyzCalculation()
  : AbstractPropCalculation()
  , p_url("")
{
}


XyzCalculation::XyzCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
{
}


bool XyzCalculation::fragment(Fragment *frag)
{
  bool ret = false;

  SFile *xyz_file = TempStorage::getTempFile();
  ofstream xyz_stream(xyz_file->path().c_str());
  if (xyz_stream) {
    if (frag->dumpXYZ(xyz_stream)) {
      EDSI *edsi = EDSIFactory::getEDSI(p_url);
      if (edsi) {
        ifstream ifs(xyz_file->path().c_str());
        if (ifs) {
          if (edsi->putDataSet(ifs)) {
            ret = true;
          }
          ifs.close();
        }
        delete edsi;
      }
    }
    xyz_stream.close();
  }
  xyz_file->remove();
  delete xyz_file;

  return ret;
}


bool XyzCalculation::getFragment(Fragment &frag)
{
  bool ret = false;
  string units = "angstroms";
  WxUnitsPrompt unitsPrompt(NULL);
  if (unitsPrompt.ShowModal() != wxID_OK) {
     return false;
  } else {
     units = unitsPrompt.getUnits();
  }

  double mult = 1.0;
  if (units == "Bohr") mult = 0.529177;
  if (units == "Picometers") mult = 0.01;
  if (units == "Nanometers") mult = 10.0;


  Resource *resource = EDSIFactory::getResource(p_url);

  // create temporary file for fragment reading
  SFile *file = TempStorage::getTempFile();
  file = resource->getDocument(file);
  ifstream is(file->path().c_str());

  ret = frag.restoreXYZ(is, mult, true);
  
  // cleanup
  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL XyzCalculation::getURL() const
{
  return p_url;
}


bool XyzCalculation::isReadOnly() const
{
  return false;
}


// IPropCalculation framework
bool XyzCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "XYZ") {
    Fragment frag;
    Resource *resource = EDSIFactory::getResource(url);
    istream * is = resource->getDocument();
    if (frag.restoreXYZ(*is, 1.0, false)) {
      string tmp;
      getline(*is, tmp);
      if (!is->good() || is->eof()) {
        ret = true;
      } else {
        getline(*is, tmp);
        if (!is->good() || is->eof()) {
          ret = true;
        }
      }
    }
    delete is;
  }

  return ret;
}


IPropCalculation *XyzCalculation::open(const EcceURL& url)
{
  return new XyzCalculation(url);
}


string XyzCalculation::inputMasks()
{
  return "*.xyz;*.XYZ";
}

