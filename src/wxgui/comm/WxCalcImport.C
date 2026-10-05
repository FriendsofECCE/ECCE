/**
 * @file
 *
 *
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>
using namespace std;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/SFile.H"
#include "util/Preferences.H"

#include "dsm/TaskJob.H"

#include "comm/RCommand.H"
#include "comm/JobParser.H"

#include "wxgui/ewxFileDialog.H"
#include "wxgui/WxCalcImportClient.H"
#include "wxgui/WxCalcImport.H"


/////////////////////////////////////////////////////////////////////////////
// Description
//   Constructor.
/////////////////////////////////////////////////////////////////////////////
WxCalcImport::WxCalcImport(wxWindow* parent) :
              ewxFileDialog(parent, "ECCE Import Calculation from Output File",
                        "", "", "All file extensions (*.*)|*.*|All files (*)|*",
                        wxOK|wxCANCEL|wxCENTRE, wxDefaultPosition)
{
  p_listener = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Destructor.
/////////////////////////////////////////////////////////////////////////////
WxCalcImport::~WxCalcImport()
{
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void WxCalcImport::registerListener(WxCalcImportClient *l)
{
  p_listener = l;
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   Execute the import.  First verify that the file is a readable,
//   regular file.  A user supplied calc name is optional.
/////////////////////////////////////////////////////////////////////////////
void WxCalcImport::importCalc()
{
  // Set preferences to the directory/filter used last invocation.
  string dir;
  int filter;

  Preferences prefs("CalcImport");
  if (prefs.getString("CalcImport.Directory", dir))
    SetDirectory(dir.c_str());
  else
    SetDirectory(Ecce::realUserHome());

  if (prefs.getInt("CalcImport.FilterIndex", filter))
    SetFilterIndex(filter);

  // Display the import dialog and process selection
  if (ShowModal() == wxID_OK) {
    importFile(GetPath().ToStdString());

    // Update preferences to this directory/filter for the next invocation
    // using the existing prefs instance created to read preferences above
    string dir = GetDirectory().ToStdString();
    prefs.setString("CalcImport.Directory", dir);
    prefs.setInt("CalcImport.FilterIndex", GetFilterIndex());
    prefs.saveFile();
  }
}


/**
 * Import the output file at fileSpec into the listener's container.
 */
void WxCalcImport::importFile(const string& fileSpec)
{
  {
    TaskJob *taskJob = 0;

    EE_ASSERT(p_listener, EE_FATAL, "No listener registered");
    string message = "";
    bool status = false;

    // Verify the file is a readable regular file
    if (fileSpec == "")
      message = "No file specified for import.";
    else {
      SFile file(fileSpec);
      if (!file.exists())
        message = "File specified for import does not exist.";
      else if (file.is_dir())
        message = "Directory rather than file specified for import.";
      else if (!file.is_regular_file())
        message = "File specified for import is not a regular text file.";
      else if (!file.is_readable())
        message = "File specified for import cannot be read.";
      else {
        string name = file.pathtail();
        if (name == "ecce")
          name = "NWChem";
        else if (name == "g16")
          name = "Gaussian16";
        else if (name == "g09")
          name = "Gaussian09";
         else if (name == "g03")
          name = "Gaussian03";
        else if (name == "g98")
          name = "Gaussian98";
        else if (name == "g94")
          name = "Gaussian94";

        // Try extracting the "Start" directive name from NWChem output
        // files because that contains a calculation name that is used
        // for naming associated files (.q partial charges) so knowing
        // that name is critical to be able to parse that data
        RCommand localconn("system");
        if (localconn.isOpen()) {
          string cmd = "grep -i \"^start \" ";
          cmd.append(fileSpec);
          string oput;
          if (localconn.execout(cmd, oput)) {
            // Strip up to second token
            oput = oput.substr(oput.find_first_not_of(" \t",6));
            // Strip trailing whitespace and newlines
            oput = oput.substr(0, oput.find_last_not_of(" \t\n\r")+1 );
            if (oput != "")
              name = oput;
          } else {
            cmd = "grep -i \"^restart \" ";
            cmd.append(fileSpec);
            if (localconn.execout(cmd, oput)) {
              // Strip up to second token
              oput = oput.substr(oput.find_first_not_of(" \t",8));
              // Strip trailing whitespace and newlines
              oput = oput.substr(0, oput.find_last_not_of(" \t\n\r")+1 );
              if (oput != "")
                name = oput;
            }
          }
        }

        // Gaussian: name the calculation after the job's own title, the
        // line Gaussian prints between dashes just before "Symbolic
        // Z-matrix:" (it does so for Cartesian input too), rather than
        // after the file -- every G16 import used to be "Gaussian16".
        // Calculation names allow only letters, digits, '.' and '_'
        // (CalcMgr::isValidName), so anything else becomes '_'.
        if (name.compare(0, 8, "Gaussian") == 0) {
          ifstream in(fileSpec.c_str());
          vector<string> recent;
          string line;
          while (getline(in, line)) {
            if (line.find("Symbolic Z-matrix:") != string::npos) {
              const size_t n = recent.size();
              if (n >= 3 &&
                  recent[n-1].find_first_not_of(" -") == string::npos &&
                  recent[n-3].find_first_not_of(" -") == string::npos) {
                string title = recent[n-2];
                const size_t b = title.find_first_not_of(" \t");
                const size_t e = title.find_last_not_of(" \t\r");
                title = (b == string::npos) ? "" : title.substr(b, e-b+1);
                string clean;
                for (size_t i = 0; i < title.size() && clean.size() < 64; i++) {
                  const char ch = title[i];
                  clean += (isalnum((unsigned char)ch) || ch == '.' || ch == '_')
                           ? ch : '_';
                }
                if (!clean.empty()) name = clean;
              }
              break;
            }
            recent.push_back(line);
            if (recent.size() > 3) recent.erase(recent.begin());
          }
        }

        taskJob = p_listener->getContainer(name);
        if (taskJob == 0) {
          message = "Can't access or write to the specified parent project for the import.";
        } else {
          status = JobParser::importCalculation(file.path().c_str(),
                                                taskJob, name, message);
        }
      }
    }

    p_listener->importValidationComplete(taskJob, status, message);
  }
}


