#include <fstream>
  using std::ifstream;
  using std::ofstream;
  using std::ios;
#include <string>
  using std::string;

#include <wx/link.h>

#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/TempStorage.H"

#include "tdat/Fragment.H"

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "wxgui/WxPDBPrompt.H"
#include "wxgui/ewxLogTextCtrl.H"

#include "PdbCalculation.H"


wxFORCE_LINK_THIS_MODULE(PdbCalculation)


IMPLEMENT_CALCULATION(
        PdbCalculation,
        PDB,
        PdbCalculation::canOpen,
        PdbCalculation::open,
        PdbCalculation::inputMasks)



PdbCalculation::PdbCalculation()
  : AbstractPropCalculation()
  , p_url("")
  , p_internalFormat(true)
{
}


PdbCalculation::PdbCalculation(const EcceURL& url)
  : AbstractPropCalculation()
  , p_url(url)
  , p_internalFormat(true)
{
}


bool PdbCalculation::fragment(Fragment *frag)
{
  bool ret = false;

  SFile *pdb_file = TempStorage::getTempFile();
  ofstream pdb_stream(pdb_file->path().c_str());
  if (pdb_stream) {
    if (frag->dumpPDB(pdb_stream)) {
      EDSI *edsi = EDSIFactory::getEDSI(p_url);
      if (edsi) {
        ifstream ifs(pdb_file->path().c_str());
        if (ifs) {
          if (edsi->putDataSet(ifs)) {
            ret = true;
          }
          ifs.close();
        }
        delete edsi;
      }
    }
    pdb_stream.close();
  }
  pdb_file->remove();
  delete pdb_file;

  return ret;
}


bool PdbCalculation::getFragment(Fragment &frag)
{
  bool ret = false;

  Resource *resource = EDSIFactory::getResource(p_url);

  // create temporary file for fragment reading
  SFile *file = TempStorage::getTempFile();
  file = resource->getDocument(file);
  ifstream is(file->path().c_str());
  if (!is) {
    return false;
  }

  // check for parts of the file we don't understand
  p_internalFormat = true;
  string line;
  getline(is, line);
  while (is) {
    if (!STLUtil::startsWith(line, "ATOM")
            && !STLUtil::startsWith(line, "HETATM")
            && !STLUtil::startsWith(line, "MODEL")
            && !STLUtil::startsWith(line, "CONECT")) {
      p_internalFormat = false;

      wxLogWarning("PDB file is read-only because it contains information that ECCE cannot regenerate.  Save file to a new name if you wish to edit it.");
      break;
    }
    getline(is, line);
  }
  // reset file stream after the above check
  is.clear();
  is.seekg(0, ios::beg);

  // check for multiple models, etc
  int numModels = 1;
  vector<string> altLocVec;
  vector<bool> groupFlgVec;
  vector<string> chainVec;
  int totalAtoms;
  string altLoc;
  int model;
  string chain;
  bool hasStuff = frag.prescanPDB(is, numModels, altLocVec, groupFlgVec,
                  chainVec, totalAtoms) && (numModels>1 || altLocVec[0]!="");
  // prescanPDB can invalidate our file stream
  is.clear();
  is.seekg(0, ios::beg);
  if (hasStuff) {
    WxPDBPrompt prompt(NULL, wxID_ANY, "ECCE PDB Reader");
    prompt.setModelRange(numModels);
    prompt.setAltLocOptions(altLocVec);
    prompt.setChainOptions(chainVec);
    if (prompt.ShowModal() == wxID_OK) {
      model = prompt.getPDBModel();
      altLoc = prompt.getAltLocation();
      chain = prompt.getChainID();
      if (chain == "All") chain = " ";
      ret = frag.restorePDB(is,1.0,true,model,altLoc,true,false,chain);
    }
  } else {
    ret = frag.restorePDB(is, 1.0, true);
  }

  is.close();
  file->remove();
  delete file;

  return ret;
}


EcceURL PdbCalculation::getURL() const
{
  return p_url;
}


bool PdbCalculation::isReadOnly() const
{
  return !p_internalFormat;
}


// IPropCalculation framework
//
// We allow the user to open PDB files but...
// If the PDB format used does not match that which we "dump" ourself, then
// we open the PDB in a limited and read-only manner.
// Otherwise, we're fine to open and edit our limited* PDB files.
// * Meaning, we only understand a few of the PDB keywords such as ATOM/HEATOM.

bool PdbCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "PDB" || ext == "ENT") {
    ret = true;
  }

  return ret;
}


IPropCalculation *PdbCalculation::open(const EcceURL& url)
{
  return new PdbCalculation(url);
}


string PdbCalculation::inputMasks()
{
  return "*.pdb;*.PDB;*.ent;*.ENT";
}

