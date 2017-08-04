#include <fstream>
  using std::endl;
  using std::ofstream;

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "wxgui/ewxGrid.H"
#include "wxgui/ExportTableDialog.H"

#include "util/JMSPublisher.H"
#include "util/SFile.H"
#include "util/TempStorage.H"


IMPLEMENT_CLASS(ExportTableDialog, ewxGenericFileDialog)


ExportTableDialog::ExportTableDialog()
  : ewxGenericFileDialog()
  , p_grid(NULL)
{
}


ExportTableDialog::ExportTableDialog(wxWindow * parent, ewxGrid * grid)
  : ewxGenericFileDialog()
  , p_grid(NULL)
{
  Create(parent, grid);
  restoreSettings();
}


ExportTableDialog::~ExportTableDialog()
{
  saveSettings();
}


bool ExportTableDialog::Create(wxWindow * parent, ewxGrid * grid)
{
  if (!ewxGenericFileDialog::Create(parent, wxEmptyString,
            wxEmptyString, wxEmptyString, wxEmptyString,
            wxFD_SAVE)) {
    return false;
  }

  p_grid = grid;

  SetTitle("Export Table to File...");
  SetName("ExportTableDialog");

  SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

  SetWildcard("Comma-separated values (*.csv)|*.csv");

  return true;
}


void ExportTableDialog::EndModal(int retCode)
{
  writeCsv();
  return ewxGenericFileDialog::EndModal(retCode);
}


bool ExportTableDialog::writeCsv()
{
  bool ret = true;

  // create tmp file
  SFile *file = TempStorage::getTempFile();

  // write table to file stream
  ofstream ofs(file->path().c_str());
  for (int row=0, rows=p_grid->GetNumberRows(); row<rows; ++row) {
    int col=0, cols=p_grid->GetNumberCols()-1;
    while (col < cols) {
      ofs << p_grid->GetCellValue(row,col) << ",";
      ++col;
    }
    ofs << p_grid->GetCellValue(row,col) << endl;
  }
  ofs.close();

  // put file where it belongs
  Resource *parent = 0;
  Resource *child = 0;
  try {
    string dir = GetDirectory().c_str();
    parent = EDSIFactory::getResource(dir);
    if (!parent) throw InvalidException("Null Resource", WHERE);
    child = parent->createChild(GetFilename().c_str(), file);
    if (!child) throw InvalidException("Null Resource", WHERE);
  } catch (EcceException& ex) {
    ret = false;
    // the child is either a calculation or a session, in either case removing
    // it will take care of all subdirectories and child resources
    if (child) child->remove();
    child = 0;
  }

  // success!
  if (ret) {
    notifyCreate(child->getURL().toString());
  }

  if (file) {
    file->remove();
    delete file;
  }

  return ret;
}


void ExportTableDialog::notifyCreate(const string& url) const
{
  JMSPublisher publisher(GetName().c_str());
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("url", url);
  publisher.publish("ecce_url_created", *msg);
  delete msg;
}

