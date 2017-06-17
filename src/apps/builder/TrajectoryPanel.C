#include <fstream>
  using std::ifstream;

#include <wx/dir.h>
#include <wx/gbsizer.h>
#include <wx/link.h>
#include <wx/tglbtn.h> // for EVT_TOGGLEBUTTON

#include "util/Ecce.H"
#include "util/ImageConverter.H"
#include "util/Event.H"
  using ecce::Event;
#include "util/EventDispatcher.H"
  using ecce::EventDispatcher;
#include "util/Host.H"
#include "util/STLUtil.H"
#include "util/TempStorage.H"

#include "tdat/DisplayStyle.H"
#include "tdat/TBond.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/IPropCalculation.H"
#include "dsm/MdTask.H"
#include "dsm/TaskJob.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxImage.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/NumericTextCtrl.H"
#include "wxgui/NumericValidator.H"
#include "wxgui/PlaybackControl.H"
#include "wxgui/SliderCombo.H"
#include "wxgui/WxUnitsPrompt.H"

#include "viz/CSStyleCmd.H"
#include "viz/MoveAction.H"
#include "viz/NewFragmentCmd.H"

#include "wxviz/PovrayCmd.H"
#include "wxviz/RenderFileCmd.H"
#include "wxviz/TrajectoryReaderNWChem.H"
#include "wxviz/TrajectoryReaderXyz.H"

#include "comm/RunMgmt.H"

#include "TrajectoryPanel.H"


static const char *INIFILE = "wxbuilder.ini";
static const char *CONFIG_DELAY = "/Trajectory/Delay";
static const char *CONFIG_RECOMPUTE = "/Trajectory/Recompute";
static const char *CONFIG_FORMAT = "/Trajectory/Format";
static const char *CONFIG_POVRAY_FINISH = "/Trajectory/PovrayFinish";
static const char *CONFIG_POVRAY_BONDS = "/Trajectory/PovrayBonds";
static const char *CONFIG_POVRAY_SURFACES = "/Trajectory/PovraySurfaces";
static const char *CONFIG_SIZE = "/Trajectory/Size";
static const char *CONFIG_WIDTH = "/Trajectory/Width";
static const char *CONFIG_HEIGHT = "/Trajectory/Height";
static const char *CONFIG_OUTPUT = "/Trajectory/Output";
static const char *CONFIG_PREFIX = "/Trajectory/Prefix";
static const char *CONFIG_VELOCITIES = "/Trajectory/Velocities";
static const char *CONFIG_FORCES = "/Trajectory/Forces";

static const int DEFAULT_WIDTH = 640;
static const int DEFAULT_HEIGHT = 480;
static const wxString FORMAT_ALL_CHOICES[] = {
        "JPEG", "GIF", "TIFF", "PNG", "POV-Ray"
};
static size_t FORMAT_ALL_CHOICES_SIZE = 5;
static const wxString FORMAT_NOIMAGE_CHOICES[] = {
        "POV-Ray"
};
static size_t FORMAT_NOIMAGE_CHOICES_SIZE = 1;
static const wxString FILE_SIZE_CHOICES[] = {
        "Video", "Viewer", "Custom"
};
static size_t FILE_SIZE_CHOICES_SIZE = 3;
static const wxString POVRAY_FINISH_CHOICES[] = {
        "Plastic", "Soft", "Metallic", "Wood"
};
static size_t POVRAY_FINISH_CHOICES_SIZE = 4;
static const wxString POVRAY_BONDS_CHOICES[] = {
        "Bicolor-Proportional", "Bicolor-Symmetric", "Brass"
};
static size_t POVRAY_BONDS_CHOICES_SIZE = 3;
static const wxString POVRAY_SURFACES_CHOICES[] = {
        "Glassy", "Soft"
};
static size_t POVRAY_SURFACES_CHOICES_SIZE = 2;

static void TransferToWindow(wxWindow * win) {
  if (win) {
    wxValidator * val = win->GetValidator();
    if (val) {
      val->TransferToWindow();
    }
  }
}


wxFORCE_LINK_THIS_MODULE(TrajectoryPanel)


const int BORDER = 1;


TrajectoryStreamProvider::TrajectoryStreamProvider(IPropCalculation * calc)
  : p_calc(calc),
    p_job(dynamic_cast<TaskJob*>(calc)),
    p_mdtask(dynamic_cast<MdTask*>(calc))
{
}


/**
 * Generates a vector of trajectory filename strings.
 * 
 * If the calculation was setup to output trajectory files, this function
 * will get a vector of those files produced.
 * @return the vector of trajectory filename strings based on the calculation
 */
vector<string> TrajectoryStreamProvider::getFileSequence()
{
  vector<string> ret;
  size_t idx;

  if (p_calc->getURL().isLocal()) {
    // turn the calc url into a file mask
    // ex) /path/to/trj/dir/file01_0001.trj -> .../file01_????.trj
    SFile file(p_calc->getURL().toString());
    wxString dirPath = file.pathroot();
    wxString name = file.filename();
    wxString ext = file.extension();
    string nameStr = name.c_str();
    size_t index = nameStr.find_last_not_of("0123456789");
    wxString filespec;
    if (index != string::npos) {
      wxString alphaPart = name.Mid(0, index+1);
      wxString numPart = name.Mid(index+1);
      filespec = alphaPart;
      for (idx = 0; idx < numPart.Len(); ++idx) {
        filespec += "?";
      }
    }
    else {
      filespec = name;
    }
    filespec += "." + ext;
    // using the new filespec/mask, get all files based on it
    wxArrayString files;
    wxDir::GetAllFiles(dirPath, &files, filespec, wxDIR_FILES);
    for (idx = 0; idx < files.Count(); ++idx) {
      ret.push_back(files[idx].c_str());
    }
  } else if (p_job) {
    // Determine where the trj files should be:
    // 0 = compute server
    // 1 = nwfs archive
    // 2 = data server
    // This depends on a combination of the run state, the "justFinished"
    // flag, and the user's selected disposition in the MD task editor
    int iLoc = 0;
    bool foundFlag = false;
    if (p_job->getState() > ResourceDescriptor::STATE_RUNNING && p_mdtask) {
      try {
        vector<NWChemMDModel::GUIPanel> panels;
        panels.push_back(NWChemMDModel::FILES);
        NWChemMDModel taskModel(panels);
        p_mdtask->getTaskModel(taskModel);
        FilesModel *fmodel = taskModel.getFilesModel();
        if (fmodel) {
          iLoc = fmodel->getTrajectoryStorage();
        }
      } catch (...) {
      }
    }

    if (iLoc == 2) {
      // Check for trajectories on data server
      // TODO get files based on mimetype
      vector<string> files = p_job->getDataFileNames(JCode::PROPERTY_OUTPUT);

      // files aren't in the correct order so sort them based on seq #
      EcceSortedVector<string, less<string> > sortFiles = files;

      for (idx = 0; idx < sortFiles.size(); ++idx) {
        if (sortFiles[idx].find(".trj") >= 0
                || sortFiles[idx].find(".TRJ") >= 0) {
          // In this case we need to download files into cache on demand
          wxString filepath = wxString::Format("%s/%lu%s",
                  TempStorage::getTrjDirectoryRoot().c_str(),
                  wxGetProcessId(),
                  sortFiles[idx].c_str());
          SFile * file = new SFile(filepath);
          if (file && !file->exists()) {
            file = p_job->getAnyFile(sortFiles[idx], file);
          }
          ret.push_back(filepath.c_str());
          foundFlag = true;
        }
      }
    } else if (iLoc == 1) {
      // Check for files on nwfs archive
      // set foundFlag to indicate whether we need to check the
      // compute server
      string trjDirName = TempStorage::getTrjDirectoryRoot();
      try {
        string errorMessage;
        ret = RunMgmt::updateCacheFromDataArchive(p_job, trjDirName,
                                                  "*.trj", errorMessage);
        foundFlag = errorMessage == "";
      } catch (EcceException& ex) {
        // TODO
      }
    }

    // If the job is running or there are no trajectories on the data
    // server or nwfs archive, try the compute server
    if (!foundFlag) {
      // If its remote, need to use remote commands to get file
      // list and scp/sftp them over.  Or if local just do it.
      if (p_job->launchdata().machine == Host().host_name()) {
        // it's local; get the full path to local run dir
        string path = p_job->jobdata().jobpath;
        wxArrayString files;
        wxDir::GetAllFiles(path, &files, "*.trj", wxDIR_FILES);
        vector<string> unsortFiles;
        for (idx = 0; idx < files.Count(); ++idx) {
          unsortFiles.push_back(files[idx].c_str());
        }
        // Sadly, files are in jumbled order so sort them based on seq #
        // This took a few years to discover.  GDB 1/9/12
        EcceSortedVector<string, less<string> > sortFiles = unsortFiles;
        for (idx = 0; idx < sortFiles.size(); ++idx) {
          ret.push_back(sortFiles[idx]);
        }
      } else {
        string trjDirName = TempStorage::getTrjDirectoryRoot();
        try {
          string errorMessage;
          ret = RunMgmt::updateCacheFromComputeHost(p_job, trjDirName,
                                                    "*.trj", errorMessage);
        } catch (EcceException& ex) {
          // TODO
        }
      }
    }
  }

  return ret;
}


istream * TrajectoryStreamProvider::getStream(const string& url)
{
  ifstream *ret = 0;

  // First, check for trajectories on data server
  if (!p_calc->getURL().isLocal()
          && p_job->getState() != ResourceDescriptor::STATE_RUNNING) {
    // In this case we need to download files into cache on demand
    wxString filepath = wxString::Format("%s/%lu%s",
            TempStorage::getTrjDirectoryRoot().c_str(),
            wxGetProcessId(),
            url.c_str());
    SFile * file = new SFile(filepath);
    if (file && !file->exists()) {
      file = p_job->getAnyFile(url, file);
    }
    if (file) {
      ret = new ifstream(file->path().c_str());
    }
  }

  // If the job is running or there are no trajectories on the data
  // server, they must be either local or on the compute server
  if (p_job->getState() == ResourceDescriptor::STATE_RUNNING
          || (p_job->getState() != ResourceDescriptor::STATE_RUNNING
          && !ret)) {
     ret = new ifstream(url.c_str());
  }

  return ret;
}


TrajectoryPanelModel::TrajectoryPanelModel()
  : fileIndexMin(0),
    fileIndexMax(0),
    fileIndexFrom(0),
    fileIndexTo(0),
    fileIndexCurrent(0),
    stepIndexMin(1),
    stepIndexMax(1),
    stepCurrent(1),
    stepSize(1),
    delay(500),
    recomputeBonds(false),
    showSolvent(false),
    units("Nanometers"),
    outputFormat(""),
    outputFileSize(""),
    povrayFinish(POVRAY_FINISH_CHOICES[0]),
    povrayBonds(POVRAY_BONDS_CHOICES[0]),
    povraySurfaces(POVRAY_SURFACES_CHOICES[0]),
    outputWidth(0),
    outputHeight(0),
    outputDirectory(""),
    outputFilePrefix(""),
    showVelocities(false),
    velocityScale(0.5),
    showForces(false),
    forceScale(-3)
{
}


void TrajectoryPanelModel::restoreSettings()
{
  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  config->Read(CONFIG_DELAY, &delay, 500);
  config->Read(CONFIG_RECOMPUTE, &recomputeBonds, false);
  // no SHOW_SOLVENT - we don't want that persisted (defaults to false)
  config->Read(CONFIG_FORMAT, &outputFormat, "JPEG");
  config->Read(CONFIG_POVRAY_FINISH, &povrayFinish, POVRAY_FINISH_CHOICES[0]);
  config->Read(CONFIG_POVRAY_BONDS, &povrayBonds, POVRAY_BONDS_CHOICES[0]);
  config->Read(CONFIG_POVRAY_SURFACES, &povraySurfaces, POVRAY_SURFACES_CHOICES[0]);
  config->Read(CONFIG_SIZE, &outputFileSize, "Video");
  config->Read(CONFIG_WIDTH, &outputWidth, DEFAULT_WIDTH);
  config->Read(CONFIG_HEIGHT, &outputHeight, DEFAULT_HEIGHT);
  config->Read(CONFIG_OUTPUT, &outputDirectory, Ecce::realUserHome());
  config->Read(CONFIG_PREFIX, &outputFilePrefix, "trajectory");
  config->Read(CONFIG_VELOCITIES, &showVelocities, false);
  config->Read(CONFIG_FORCES, &showForces, false);
}


void TrajectoryPanelModel::saveSettings()
{
  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  config->Write(CONFIG_DELAY, delay);
  config->Write(CONFIG_RECOMPUTE, recomputeBonds);
  // no SHOW_SOLVENT - we don't want that persisted (defaults to false)
  config->Write(CONFIG_FORMAT, outputFormat);
  config->Write(CONFIG_POVRAY_FINISH, povrayFinish);
  config->Write(CONFIG_POVRAY_BONDS, povrayBonds);
  config->Write(CONFIG_POVRAY_SURFACES, povraySurfaces);
  config->Write(CONFIG_SIZE, outputFileSize);
  config->Write(CONFIG_WIDTH, outputWidth);
  config->Write(CONFIG_HEIGHT, outputHeight);
  config->Write(CONFIG_OUTPUT, outputDirectory);
  config->Write(CONFIG_PREFIX, outputFilePrefix);
  config->Write(CONFIG_VELOCITIES, showVelocities);
  config->Write(CONFIG_FORCES, showForces);
}



const wxWindowID TrajectoryOutputPanel::ID_CHOICE_OUTPUT_FORMAT = wxNewId();

const wxWindowID TrajectoryOutputPanel::ID_PANEL_POVRAY = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_CHOICE_POVRAY_FINISH = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_CHOICE_POVRAY_BONDS = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_CHOICE_POVRAY_SURFACES = wxNewId();

const wxWindowID TrajectoryOutputPanel::ID_PANEL_SIZE = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_CHOICE_OUTPUT_SIZE = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_TEXT_OUTPUT_WIDTH = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_LABEL_OUTPUT_WIDTH = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_TEXT_OUTPUT_HEIGHT = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_LABEL_OUTPUT_HEIGHT = wxNewId();

const wxWindowID TrajectoryOutputPanel::ID_TEXT_OUTPUT_DIRECTORY = wxNewId();
const wxWindowID TrajectoryOutputPanel::ID_TEXT_OUTPUT_FILE_PREFIX = wxNewId();


IMPLEMENT_DYNAMIC_CLASS(TrajectoryOutputPanel, ewxPanel)


BEGIN_EVENT_TABLE(TrajectoryOutputPanel, ewxPanel)
  EVT_COMMAND(wxID_ANY, wxEVT_VALIDATION_EVENT,
          TrajectoryOutputPanel::OnValidation)
END_EVENT_TABLE()


TrajectoryOutputPanel::TrajectoryOutputPanel()
  : ewxPanel(),
    p_model(NULL)
{
}


TrajectoryOutputPanel::TrajectoryOutputPanel(TrajectoryPanelModel * model,
        wxWindow * parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : ewxPanel(),
    p_model(NULL)
{
  Create(model, parent, id, pos, size, style, name);
}


TrajectoryOutputPanel::~TrajectoryOutputPanel()
{
}


bool TrajectoryOutputPanel::Create(TrajectoryPanelModel * model,
        wxWindow * parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!ewxPanel::Create(parent,id,pos,size,style,name)) {
    wxFAIL_MSG( wxT("TrajectoryOutputPanel creation failed") );
    return false;
  }

  SetSizer(new wxBoxSizer(wxVERTICAL));

  p_model = model;

  CreateOutputOptions();

  UpdateFormatChoice();
  UpdateSizeChoice();

  TransferDataToWindow();

  return true;
}


void TrajectoryOutputPanel::CreateOutputOptions()
{
  ewxPanel * panel;
  ewxChoice * choice;
  ewxStaticText * stext;
  ewxTextCtrl * text;
  wxBoxSizer * hsizer;
  wxFlexGridSizer * fsizer;

  // format choice
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stext = new ewxStaticText(this, wxID_ANY, "Format");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GenericValidator gval(&p_model->outputFormat);
  if (getenv("ECCE_NO_VIZIMAGES")) {
    choice = new ewxChoice(this, ID_CHOICE_OUTPUT_FORMAT, wxDefaultPosition,
            wxDefaultSize, FORMAT_NOIMAGE_CHOICES_SIZE, FORMAT_NOIMAGE_CHOICES,
            0, gval);
  } else {
    choice = new ewxChoice(this, ID_CHOICE_OUTPUT_FORMAT, wxDefaultPosition,
            wxDefaultSize, FORMAT_ALL_CHOICES_SIZE, FORMAT_ALL_CHOICES, 0,
            gval);
  }
  hsizer->Add(choice, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(hsizer, 0, wxGROW|wxALL, 0);

  // povray options
  panel = new ewxPanel(this, ID_PANEL_POVRAY);
  fsizer = new wxFlexGridSizer(2, BORDER, BORDER);
  panel->SetSizer(fsizer);
  stext = new ewxStaticText(panel, wxID_ANY, "Finish:");
  fsizer->Add(stext, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
  GenericValidator finval(&p_model->povrayFinish);
  choice = new ewxChoice(panel, ID_CHOICE_POVRAY_FINISH, wxDefaultPosition,
          wxDefaultSize, POVRAY_FINISH_CHOICES_SIZE, POVRAY_FINISH_CHOICES, 0,
          finval);
  fsizer->Add(choice, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
  stext = new ewxStaticText(panel, wxID_ANY, "Bonds:");
  fsizer->Add(stext, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
  GenericValidator bondval(&p_model->povrayBonds);
  choice = new ewxChoice(panel, ID_CHOICE_POVRAY_BONDS, wxDefaultPosition,
          wxDefaultSize, POVRAY_BONDS_CHOICES_SIZE, POVRAY_BONDS_CHOICES, 0,
          bondval);
  fsizer->Add(choice, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
  stext = new ewxStaticText(panel, wxID_ANY, "Isosurfaces:");
  fsizer->Add(stext, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
  GenericValidator surfval(&p_model->povraySurfaces);
  choice = new ewxChoice(panel, ID_CHOICE_POVRAY_SURFACES, wxDefaultPosition,
          wxDefaultSize, POVRAY_SURFACES_CHOICES_SIZE, POVRAY_SURFACES_CHOICES,
          0, surfval);
  fsizer->Add(choice, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
  GetSizer()->Add(panel, 0, wxALIGN_CENTER_HORIZONTAL|wxGROW|wxALL, 0);
 
  // file size choice
  panel = new ewxPanel(this, ID_PANEL_SIZE);
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  panel->SetSizer(hsizer);
  stext = new ewxStaticText(panel, wxID_ANY, "File Size");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  choice = new ewxChoice(panel, ID_CHOICE_OUTPUT_SIZE, wxDefaultPosition,
          wxDefaultSize, 3, FILE_SIZE_CHOICES);
  GenericValidator ofsval(&p_model->outputFileSize);
  choice->SetValidator(ofsval);
  hsizer->Add(choice, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  // width text
  stext = new ewxStaticText(panel, ID_LABEL_OUTPUT_WIDTH, "Width");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  text = new ewxTextCtrl(panel, ID_TEXT_OUTPUT_WIDTH);
  NumericValidator<int> owval(&p_model->outputWidth,0,INT_MAX);
  text->SetValidator(owval);
  hsizer->Add(text, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  // height text
  stext = new ewxStaticText(panel, ID_LABEL_OUTPUT_HEIGHT, "Height");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  text = new ewxTextCtrl(panel, ID_TEXT_OUTPUT_HEIGHT);
  NumericValidator<int> ohval(&p_model->outputHeight,0,INT_MAX);
  text->SetValidator(ohval);
  hsizer->Add(text, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(panel, 0, wxGROW|wxALL, 0);

  // output dir
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stext = new ewxStaticText(this, wxID_ANY, "Output Dir:");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  text = new ewxTextCtrl(this, ID_TEXT_OUTPUT_DIRECTORY);
  GenericValidator odval(&p_model->outputDirectory);
  text->SetValidator(odval);
  hsizer->Add(text, 1, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(hsizer, 0, wxGROW|wxALL, 0);

  // file prefix
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stext = new ewxStaticText(this, wxID_ANY, "File Prefix:");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  text = new ewxTextCtrl(this, ID_TEXT_OUTPUT_FILE_PREFIX);
  GenericValidator ofpval(&p_model->outputFilePrefix);
  text->SetValidator(ofpval);
  hsizer->Add(text, 1, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(hsizer, 0, wxGROW|wxALL, 0);
}


void TrajectoryOutputPanel::OnValidation(wxCommandEvent& event)
{
  if (event.GetId() == ID_CHOICE_OUTPUT_FORMAT) {
    UpdateFormatChoice();
  } else if (event.GetId() == ID_CHOICE_OUTPUT_SIZE) {
    UpdateSizeChoice();
  }
  p_model->saveSettings();
  event.Skip();
}


void TrajectoryOutputPanel::UpdateFormatChoice()
{
  bool show;
  if (getenv("ECCE_NO_VIZIMAGES")) {
    show = p_model->outputFormat ==
             FORMAT_NOIMAGE_CHOICES[FORMAT_NOIMAGE_CHOICES_SIZE-1];
  } else {
    show = p_model->outputFormat ==
             FORMAT_ALL_CHOICES[FORMAT_ALL_CHOICES_SIZE-1];
  }
  FindWindow(ID_PANEL_POVRAY)->Show(show);
  FindWindow(ID_PANEL_SIZE)->Show(!show);
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
}


void TrajectoryOutputPanel::UpdateSizeChoice()
{
  bool enable = p_model->outputFileSize
          == FILE_SIZE_CHOICES[FILE_SIZE_CHOICES_SIZE-1];
  FindWindow(ID_TEXT_OUTPUT_WIDTH)->Enable(enable);
  FindWindow(ID_LABEL_OUTPUT_WIDTH)->Enable(enable);
  FindWindow(ID_TEXT_OUTPUT_HEIGHT)->Enable(enable);
  FindWindow(ID_LABEL_OUTPUT_HEIGHT)->Enable(enable);

  if (p_model->outputFileSize == FILE_SIZE_CHOICES[0]) {
    p_model->outputWidth = DEFAULT_WIDTH;
    p_model->outputHeight = DEFAULT_HEIGHT;
    TransferToWindow(FindWindow(ID_TEXT_OUTPUT_WIDTH));
    TransferToWindow(FindWindow(ID_TEXT_OUTPUT_HEIGHT));
  }
}



IMPLEMENT_DYNAMIC_CLASS(TrajectoryOutputDialog, ewxDialog)


BEGIN_EVENT_TABLE(TrajectoryOutputDialog, ewxDialog)
  EVT_COMMAND(wxID_ANY, wxEVT_VALIDATION_EVENT,
          TrajectoryOutputDialog::OnValidation)
END_EVENT_TABLE()


TrajectoryOutputDialog::TrajectoryOutputDialog()
  : ewxDialog()
{
}


TrajectoryOutputDialog::TrajectoryOutputDialog(TrajectoryPanelModel * model,
        wxWindow * parent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  :ewxDialog()
{
  Create(model, parent, id, title, pos, size, style, name);
}


TrajectoryOutputDialog::~TrajectoryOutputDialog()
{
}


bool TrajectoryOutputDialog::Create(TrajectoryPanelModel * model,
        wxWindow * parent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!ewxDialog::Create(parent, id, title, pos, size, style, name)) {
    wxFAIL_MSG( wxT("TrajectoryOutputDialog creation failed") );
    return false;
  }

  SetSizer(new wxBoxSizer(wxVERTICAL));

  TrajectoryOutputPanel  * panel = new TrajectoryOutputPanel(model, this);

  GetSizer()->Add(panel, 1, wxGROW);
  GetSizer()->Add(CreateSeparatedButtonSizer(wxOK|wxCANCEL), 0, wxGROW);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  return true;
}


void TrajectoryOutputDialog::OnValidation(wxCommandEvent& event)
{
  if (event.GetId() == TrajectoryOutputPanel::ID_CHOICE_OUTPUT_FORMAT) {
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Fit();
  }
  event.Skip();
}



const wxWindowID TrajectoryPanel::ID_BUTTON_REFRESH = wxNewId();
const wxWindowID TrajectoryPanel::ID_TEXT_FILE_NAME = wxNewId();

//const wxWindowID TrajectoryPanel::ID_BUTTON_EXPORT_MEASURES = wxNewId();
//const wxWindowID TrajectoryPanel::ID_BUTTON_SELECT_ATOMS = wxNewId();

const wxWindowID TrajectoryPanel::ID_TEXT_STEP_CURRENT = wxNewId();
const wxWindowID TrajectoryPanel::ID_TEXT_STEP_SIZE = wxNewId();
const wxWindowID TrajectoryPanel::ID_TEXT_DELAY = wxNewId();
const wxWindowID TrajectoryPanel::ID_PLAYBACK = wxNewId();

const wxWindowID TrajectoryPanel::ID_CHECK_RECOMPUTE_BONDS = wxNewId();
const wxWindowID TrajectoryPanel::ID_CHECK_SHOW_SOLVENT = wxNewId();

const wxWindowID TrajectoryPanel::ID_CHECK_VELOCITIES = wxNewId();
const wxWindowID TrajectoryPanel::ID_SCALE_VELOCITIES = wxNewId();
const wxWindowID TrajectoryPanel::ID_CHECK_FORCES = wxNewId();
const wxWindowID TrajectoryPanel::ID_SCALE_FORCES = wxNewId();


IMPLEMENT_DYNAMIC_CLASS(TrajectoryPanel, VizPropertyPanel)


BEGIN_EVENT_TABLE(TrajectoryPanel, VizPropertyPanel)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_TICK_EVENT,
          TrajectoryPanel::OnPlaybackTick)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_LOOP_EVENT,
          TrajectoryPanel::OnPlaybackLoop)
  EVT_COMMAND(wxID_ANY, wxEVT_VALIDATION_EVENT, TrajectoryPanel::OnValidation)
  EVT_BUTTON(ID_BUTTON_REFRESH, TrajectoryPanel::OnRefreshClick)
  EVT_TOGGLEBUTTON(PlaybackControl::ID_RECORD, TrajectoryPanel::OnRecordClick)
  EVT_UPDATE_UI(ID_TEXT_STEP_CURRENT, TrajectoryPanel::UpdatePlayback)
  EVT_UPDATE_UI(ID_BUTTON_REFRESH, TrajectoryPanel::UpdateRefresh)
  EVT_UPDATE_UI(ID_CHECK_RECOMPUTE_BONDS, TrajectoryPanel::UpdateRecomputeBonds)
  EVT_UPDATE_UI(ID_CHECK_SHOW_SOLVENT, TrajectoryPanel::UpdateShowSolvent)
  EVT_UPDATE_UI(ID_CHECK_VELOCITIES, TrajectoryPanel::UpdateShowVelocities)
  EVT_UPDATE_UI(ID_SCALE_VELOCITIES, TrajectoryPanel::UpdateShowVelocities)
  EVT_UPDATE_UI(ID_CHECK_FORCES, TrajectoryPanel::UpdateShowForces)
  EVT_UPDATE_UI(ID_SCALE_FORCES, TrajectoryPanel::UpdateShowForces)
END_EVENT_TABLE()


TrajectoryPanel::TrajectoryPanel()
  : VizPropertyPanel(),
    p_model(),
    p_reader(NULL),
    p_streamProvider(NULL),
    p_files(),
    p_job(NULL),
    p_fileName(NULL),
    p_playback(NULL)
{
}


TrajectoryPanel::TrajectoryPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(),
    p_model(),
    p_reader(NULL),
    p_streamProvider(NULL),
    p_files(),
    p_job(NULL),
    p_fileName(NULL),
    p_playback(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


TrajectoryPanel::~TrajectoryPanel()
{
}


bool TrajectoryPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style,
                                name)) {
    wxFAIL_MSG( wxT("TrajectoryPanel creation failed") );
    return false;
  }

  // do this so that it doesn't appear too small by itself in AUI
  SetMinSize(wxSize(400,-1));

  p_model.restoreSettings();

  return true;
}


void TrajectoryPanel::refresh()
{
  doRefreshClick();
}


void TrajectoryPanel::receiveFocus()
{
}


void TrajectoryPanel::loseFocus()
{
  p_playback->Pause();
}


void TrajectoryPanel::initialize()
{
  p_streamProvider = new TrajectoryStreamProvider(getCalculation());
  p_job = dynamic_cast<TaskJob*>(getCalculation());

  if (!GetSizer()) SetSizer(new wxBoxSizer(wxVERTICAL));

  wxSizer * hsizer;
  wxGridBagSizer * gridsizer;
  ewxStaticText * stext;
  NumericTextCtrl * ntext;
  ewxCheckBox * check;
  SliderCombo * slider;

  // refresh, file name label, file name, and file index
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stext = new ewxStaticText(this, wxID_ANY, "File Name:");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  p_fileName = new ewxTextCtrl(this, ID_TEXT_FILE_NAME, "", wxDefaultPosition,
          wxDefaultSize, wxTE_READONLY);
  hsizer->Add(p_fileName, 1, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  wxCustomButton * button;
  button = new wxCustomButton(this, ID_BUTTON_REFRESH,
          wxBitmap(ewxImage::get(ewxImage::REFRESH)),
          wxDefaultPosition, wxDefaultSize, wxCUSTBUT_BUTTON);
  hsizer->Add(button, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(hsizer, 0, wxGROW|wxALL, 0);

  // Current step and step size
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stext = new ewxStaticText(this, wxID_ANY, "Step:");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  ntext = new NumericTextCtrl(this, ID_TEXT_STEP_CURRENT);
  NumericValidator<long> currval(&p_model.stepCurrent, &p_model.stepIndexMin,
                                 &p_model.stepIndexMax);
  ntext->SetValidator(currval);
  hsizer->Add(ntext, 1, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  stext = new ewxStaticText(this, wxID_ANY, "Step Size:");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  ntext = new NumericTextCtrl(this, ID_TEXT_STEP_SIZE);
  NumericValidator<long> stepval(&p_model.stepSize, &p_model.stepIndexMin,
                                 &p_model.stepIndexMax);
  ntext->SetValidator(stepval);
  hsizer->Add(ntext, 1, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(hsizer, 0, wxGROW|wxALL, 0);

  // THE NEXT TWO "LINES" ARE ALIGNED IN A SINGLE GRID BAG SIZER
  // per-step recompute bonds and show solvent
  gridsizer = new wxGridBagSizer(BORDER,BORDER);
  check = new ewxCheckBox(this, ID_CHECK_RECOMPUTE_BONDS,
                          "Recompute Bonds Per Step");
  GenericValidator rbval(&p_model.recomputeBonds);
  check->SetValidator(rbval);
  gridsizer->Add(check, wxGBPosition(0,0), wxGBSpan(1,2));
  check = new ewxCheckBox(this, ID_CHECK_SHOW_SOLVENT,
                          "Show Solvent");
  GenericValidator ssval(&p_model.showSolvent);
  check->SetValidator(ssval);
  gridsizer->Add(check, wxGBPosition(0,2), wxGBSpan(1,2));

  // vector options
  // velocities check
  check = new ewxCheckBox(this, ID_CHECK_VELOCITIES, "Velocities");
  GenericValidator svval(&p_model.showVelocities);
  check->SetValidator(svval);
  gridsizer->Add(check, wxGBPosition(1,0));
  // velocities slider
  slider = new SliderCombo(this, ID_SCALE_VELOCITIES);
  NumericValidator<double> velval(&p_model.velocityScale,-2,2);
  slider->SetValidator(velval);
  slider->SetRange(-2.0,2.0);
  gridsizer->Add(slider, wxGBPosition(1,1));
  // forces check
  check = new ewxCheckBox(this, ID_CHECK_FORCES, "Forces");
  GenericValidator sfval(&p_model.showForces);
  check->SetValidator(sfval);
  gridsizer->Add(check, wxGBPosition(1,2));
  // forces slider
  slider = new SliderCombo(this, ID_SCALE_FORCES);
  NumericValidator<double> forceval(&p_model.forceScale,-5,-1);
  slider->SetValidator(forceval);
  slider->SetRange(-5.0,-1.0);
  gridsizer->Add(slider, wxGBPosition(1,3));
  GetSizer()->Add(gridsizer, 0, wxGROW|wxALL, 0);

  // delay and playback
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stext = new ewxStaticText(this, wxID_ANY, "Delay:");
  hsizer->Add(stext, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  ntext = new NumericTextCtrl(this, ID_TEXT_DELAY);
  NumericValidator<int> delval(&p_model.delay, 1, 10000);
  ntext->SetValidator(delval.Units("ms"));
  hsizer->Add(ntext, 1, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  p_playback = new PlaybackControl(this, ID_PLAYBACK),
  p_playback->SetDelay(&p_model.delay);
  p_playback->SetStartIndex(&p_model.stepIndexMin);
  p_playback->SetStopIndex(&p_model.stepIndexMax);
  p_playback->SetStepSize(&p_model.stepSize);
  p_playback->SetCurrentStep(&p_model.stepCurrent);
  hsizer->Add(p_playback, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(hsizer, 0, wxGROW|wxALL, 0);

  // We load files here since this is not a traditional property panel
  // that receives update messages and calls refresh()
  // Rely on property panel call to refresh to force the load
  refresh();

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
}


void TrajectoryPanel::ReadStep(const long& step)
{
  if (!p_reader) return;

  p_model.stepCurrent = step;

  SGContainer &sg = getFW().getSceneGraph();
  SGFragment * frag = sg.getFragment();

  double velocityScale = pow(10.0,p_model.velocityScale);
  double forceScale = pow(10.0,p_model.forceScale);
  p_reader->setVelocityScale(velocityScale);
  p_reader->setForceScale(forceScale);

  if (p_reader->readStep(p_model.stepCurrent-1,
                         p_model.showVelocities, p_model.showForces)) {
    frag->touchNumbers();
    sg.touchChemDisplay();
  }

  if (p_reader && p_reader->hasGenBonds() && p_model.recomputeBonds) {
    frag->deleteAllBonds();
    frag->addCovalentBonds(false);
    frag->touchNumbers();
    sg.touchChemDisplay();
  } else {
    frag->touch();
    if (sg.proteinStyleExists()) {
      frag->touchProteins();
    }
  }

  // Update measures here
  MoveAction act;
  act.apply(sg.getTopLabel());

  CaptureMeasures();

  string name = "GeomChange";
  if (frag->numResidues() > 0) name = "ResidueChange";
  EventDispatcher::getDispatcher().publish(Event(name));

  if (p_playback->IsRecording()) {
    SaveIt();
  }

  p_model.fileIndexCurrent =
            ExtractSequenceNumber(p_reader->getCurrentFileName());

  SFile sfile(p_reader->getCurrentFileName());
  p_fileName->ChangeValue(sfile.filename());
  p_fileName->SetToolTip(sfile.path());

  TransferToWindow(FindWindow(ID_TEXT_STEP_CURRENT));

  // This allows the rendering to occur in a timely manner even if the atom
  // table is displayed.  Tested at 30 ms on two-hemes
  ::wxYieldIfNeeded();
}


void TrajectoryPanel::OnPlaybackTick(wxCommandEvent& event)
{
  ReadStep(event.GetExtraLong());
}

  
void TrajectoryPanel::OnPlaybackLoop(wxCommandEvent& event)
{
  OnPlaybackTick(event);
}


void TrajectoryPanel::OnValidation(wxCommandEvent& event)
{
  wxWindowID id = event.GetId();
  if (id == ID_TEXT_STEP_CURRENT
          || id == ID_CHECK_FORCES
          || id == ID_CHECK_VELOCITIES
          || id == ID_SCALE_FORCES
          || id == ID_SCALE_VELOCITIES
     ) {
    if (!p_playback->IsPlaying()) {
      ReadStep(p_model.stepCurrent);
    }
  } else if (id == ID_CHECK_SHOW_SOLVENT) {
    doRefreshClick();
  }
  p_model.saveSettings();
}


void TrajectoryPanel::OnRefreshClick(wxCommandEvent& event)
{
  doRefreshClick();
}


void TrajectoryPanel::doRefreshClick()
{
  wxBusyCursor c;
  // stop animating, if animating
  p_playback->Pause();
  // save the current step so we can restore it later
  long saved_step = p_model.stepCurrent;
  LoadFiles();
  ReadStep(saved_step);
}


void TrajectoryPanel::OnRecordClick(wxCommandEvent& event)
{
  if (event.IsChecked()) {
    // stop animating, if animating
    p_playback->Pause();
    TrajectoryOutputDialog dialog(&p_model, this);
    if (dialog.ShowModal() == wxID_OK) {
      event.Skip();
    } else {
      // unpress the record toggle
      ((wxCustomButton*) event.GetEventObject())->SetValue(false);
    }
  }
}


/**
 * Enable/disable a widget whether we are animating.
 */
void TrajectoryPanel::UpdatePlayback(wxUpdateUIEvent& event)
{
  event.Enable(!p_playback->IsPlaying());
}


void TrajectoryPanel::UpdateRefresh(wxUpdateUIEvent& event)
{
  TaskJob * taskJob = dynamic_cast<TaskJob*>(getCalculation());
  event.Show(taskJob && taskJob->getState()==ResourceDescriptor::STATE_RUNNING);
}


void TrajectoryPanel::UpdateRecomputeBonds(wxUpdateUIEvent& event)
{
  bool bondFlag = p_reader && p_reader->hasGenBonds();
  event.Enable(bondFlag);

  // override user selection when it conflicts with the Generate Bonds setting
  if (!bondFlag)
    ((ewxCheckBox*)FindWindow(ID_CHECK_RECOMPUTE_BONDS))->SetValue(false);
}


void TrajectoryPanel::UpdateShowSolvent(wxUpdateUIEvent& event)
{
  event.Enable(p_reader && p_reader->hasSolvent());
}


void TrajectoryPanel::UpdateShowVelocities(wxUpdateUIEvent& event)
{
  event.Enable(p_reader && p_reader->hasVelocities());
}


void TrajectoryPanel::UpdateShowForces(wxUpdateUIEvent& event)
{
  event.Enable(p_reader && p_reader->hasForces());
}


/**
 * Extracts the file number from the file name.
 *
 * We need to ignore the path and also use p_regexStart so that we
 * can be as sure as possible that we extract the right number.
 * Consider the example of /msrc/home/d39974/ClO4_00001.trj
 * There are two possible incorrect places to extract the sequence number.
 */
int TrajectoryPanel::ExtractSequenceNumber(const string& filenameString)
{
  long ret = -1;
  SFile file(filenameString);
  string pathAndName = file.pathroot() + "/" + file.pathtail();
  // ex) pathAndName = /msrc/home/d39974/C104_00001
  size_t lastAlpha = pathAndName.find_last_not_of("0123456789");
  wxString num = pathAndName.substr(lastAlpha+1);
  num.ToLong(&ret);
  return static_cast<int>(ret);
}


void TrajectoryPanel::LoadFiles()
{
  SGContainer &sg = getFW().getSceneGraph();
  SGFragment * frag = sg.getFragment();

  p_files = p_streamProvider->getFileSequence();

  if (p_files.empty()) {
    // TODO indicate no trajectory files i.e. disable GUI
    return;
  }

  // Compute file name range
  vector<string>::const_iterator file = p_files.begin();
  long min,max,val;
  min = max = ExtractSequenceNumber(*file);
  for (file += 1; file != p_files.end(); ++file) {
    val = ExtractSequenceNumber(*file);
    if (val < min) min = val;
    if (val > max) max = val;
  }
  p_model.fileIndexMin = min;
  p_model.fileIndexMax = max;
  p_model.fileIndexCurrent = min;
  p_model.fileIndexFrom = min;
  p_model.fileIndexTo = max;

  Command *cmd = new NewFragmentCmd("Clear Fragment", &sg);
  getFW().execute(cmd);

  if (!p_reader) {
    if (STLUtil::endsWith( STLUtil::upper(*p_files.begin()),"XYZ") ) {
      p_reader = new TrajectoryReaderXyz(&sg);
      WxUnitsPrompt prompt(NULL, true);
      if (prompt.ShowModal() == wxID_OK) {
        p_reader->setUnits(prompt.getUnits());
        p_reader->setGenBonds(prompt.getGenBonds());
      }
    } else {
      p_reader = new TrajectoryReaderNWChem(&sg);
    }
  }

  p_reader->setFileNames(p_files, p_model.showSolvent);
  p_model.stepCurrent = 1;
  p_model.stepIndexMax = p_reader->getStepCount();

  double velocityScale = pow(10.0,p_model.velocityScale);
  double forceScale = pow(10.0,p_model.forceScale);
  p_reader->setVelocityScale(velocityScale);
  p_reader->setForceScale(forceScale);

  (void)p_reader->readStep(p_model.stepCurrent-1,
                           p_model.showVelocities, p_model.showForces);

  int numSoluteAtoms = p_reader->getSoluteAtomsCount();
  DisplayStyle style = QueryDisplayStyle(numSoluteAtoms);
  frag->touchNumbers();
  // Don't really need this that I know of so tossing it
  //sg.touchChemDisplay();

  // Inside this method, the second part of the if statement is where
  // performanc bogs down when touchChemdisplay is called.  The command
  // executes quickly but for whatever reason, the moiv code gets slow
  SetDisplayStyle(style, numSoluteAtoms);

  // This is here because the SetDisplayStyle method uses commands in 
  // batch mode which means that things like centering after each 
  // command get skipped.  This resulted a huge performance benefit.
  getFW().center();

  SFile sfile(p_reader->getCurrentFileName());
  p_fileName->ChangeValue(sfile.filename());
  p_fileName->SetToolTip(sfile.path());

  TransferDataToWindow();

  string name = "GeomChange";
  if (frag->numResidues() > 0) name = "ResidueChange";
  EventDispatcher::getDispatcher().publish(Event(name));
}


void TrajectoryPanel::CaptureMeasures()
{
}


void TrajectoryPanel::SaveIt()
{
  wxString directory = p_model.outputDirectory;
  wxString prefix = p_model.outputFilePrefix;
  wxString sep = "/";
  wxString path;
  if (prefix == "") prefix = "trajectory";
  if (!directory.EndsWith(sep)) {
    path = directory + sep + prefix;
  } else {
    path = directory + prefix;
  }

  path = wxString::Format("%s%05ld", path.c_str(), p_model.stepCurrent);

  wxString rgbpath = path + ".rgb";
  wxString format = p_model.outputFormat; 
  wxString outpath;
  if (format == "JPEG") {
    outpath = path + ".jpeg";
  } else if (format == "TIFF") {
    outpath = path + ".tiff";
  } else if (format == "GIF") {
    outpath = path + ".gif";
  } else if (format == "POV-Ray") {
    outpath = path + ".pov";
  } else {
    outpath = rgbpath;
  }

  wxString msg = "Saving " + outpath;
  wxLogMessage(msg, 0);

  SGViewer &viewer = getFW().getViewer();
  wxSize size = viewer.GetSize();
  SbColor color = viewer.getBackgroundColor();
  int width = size.GetWidth();
  int height = size.GetHeight();

  // If the user picked the "Viewer" option, don't override values
  if (p_model.outputFileSize != "Viewer") {
    width = p_model.outputWidth;
    height = p_model.outputHeight;
  }
  Command * cmd;
  if (format == "POV-Ray") {
    cmd = new PovrayCmd("Render POV-Ray", &viewer);
    cmd->getParameter("filename")->setString(outpath.c_str());
    cmd->getParameter("finishStyle")->setString(p_model.povrayFinish.c_str());
    cmd->getParameter("bondStyle")->setString(p_model.povrayBonds.c_str());
    cmd->getParameter("isosurfaceStyle")->setString(p_model.povraySurfaces.c_str());
  } else {
    cmd = new RenderFileCmd("Render File", &viewer);
    cmd->getParameter("width")->setInteger(width);
    cmd->getParameter("height")->setInteger(height);
    cmd->getParameter("red")->setDouble(color[0]);
    cmd->getParameter("green")->setDouble(color[1]);
    cmd->getParameter("blue")->setDouble(color[2]);
    cmd->getParameter("type")->setString("RGB");
    cmd->getParameter("filename")->setString(rgbpath.c_str());
  }

  try {
    getFW().execute(cmd);
    // check whether site has disabled creation of image files
    if (!getenv("ECCE_NO_VIZIMAGES") && format!="POV-Ray") {
      try {
        ImageConverter imconv;
        imconv.convert(rgbpath.c_str(), outpath.c_str(), width, height,
                       8 /*image depth*/, true /*remove inFile*/);
      } catch (EcceException& ex) {
        wxString msg = "Unable to convert to image format " + format + ":\n";
        msg += ex.what();
        wxLogError(msg, 0);
      }
    }
  } catch (EcceException& ex) {
    wxString msg = "PovrayCmd or RenderFileCmd failed:\n";
    msg += ex.what();
    wxLogError(msg, 0);
    return;
  }
}


/**
 * Checks to see if the user wants to load the atoms in in wire frame or CPK.
 * Erich says a good default is to make solute atoms cpk and solvent wire.
 * This assumes solutes added first, then solvent...
 */
DisplayStyle TrajectoryPanel::QueryDisplayStyle(const int& numSoluteAtoms)
{
  DisplayStyle ds(DisplayStyle::CPK);

  wxString limit = getenv("ECCE_WIREFRAME_LIMIT");
  long theLimit = 5000;
  (void)limit.ToLong(&theLimit);

  if (numSoluteAtoms >= theLimit) {
    wxString msg;
    msg << "This system contains " << numSoluteAtoms << " solute atoms. "
        << " Depending on your hardware configuration, rendering in a style"
        << " other than wireframe may be slow.  Do you want to switch to"
        << " wireframe?";
    ewxMessageDialog dialog(this, msg, "Change Display Style?", wxYES_NO);
    if (dialog.ShowModal() == wxID_YES) {
      ds.setStyle(DisplayStyle::WIRE);
    }
  }

  return ds;
}


/**
 * This method sets picks a display style for a newly loaded system.
 * We have to handle:
 *   . cases where the current style does not apply to the new system,
 *   . systems that are large and we prompt the user to go to wireframe
 *   . reseting styles if there are currently mixed styles
 *
 *TODO need to figure out the logic and then implement - kinda complicated
 * psudo code
 * if (multiple styles are being used)
 *    what??
 *    if there are two styles name Solvent and Solute
 *       if we have solute
 *          apply CPK/WIRE
 *       else
 *          what to do
 *    else
 * else 
 *   if (curstyle >= CAWIRE)
 *     if frag has bio residues
 *          use current style
 *     else 
 *          ??call the query method in case the system is large?
 *          if force to wireframe
 *             set all to wireframe
 *          else
 *             if have solvent
 *                 switch to ballwire?
 *             else
 *                 switch to combined CPK/wireframe
 *    else //curstyle < CAWIRE
 *          ??call the query method in case the system is large?
 *          if force to wireframe
 *             set all to wireframe
 *          else
 *             if have solvent
 *                 switch to ballwire?
 *             else
 *                 switch to combined CPK/wireframe
 *
 */
void TrajectoryPanel::SetDisplayStyle(const DisplayStyle& style,
        const int& lastSoluteAtom)
{
  SGContainer &sg = getFW().getSceneGraph();
  SGFragment &frag = *sg.getFragment();

  int numAtoms = frag.numAtoms();

  Preferences styleprefs("StyleChooser");

  if (style.getStyle() == DisplayStyle::WIRE) {
    // In this case, change all the atoms to wireframe.
    // This special case is here because its much faster to execute
    // the command this way.
    DisplayDescriptor wire("Wireframe", "Wireframe", "Element");
    wire.load(styleprefs, "Wireframe");
    Command *cmd = new CSStyleCmd("CSStyleCmd", &sg);
    cmd->getParameter("descriptor")->setString(wire.toString());
    cmd->getParameter("all")->setBoolean(true);
    getFW().execute(cmd,true);
  } else if (lastSoluteAtom != numAtoms) {
    // Don't override user prefernce if there is no solvent
    // Solute atoms will be made CPK
    for (int i=0; i<lastSoluteAtom; ++i) {
      frag.m_atomHighLight.push_back(i);
      const vector<TBond*>& abonds = frag.atomRef(i)->bondList();
      for (size_t bdx=0; bdx<abonds.size(); bdx++) {
        frag.m_bondHighLight.push_back(abonds[bdx]->index());
      }
    }
    // Restore from styles preferences
    ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
    string style = config->Read("/DefaultStyle", "CPK").c_str();
    //string style = "CPK";
    string styledd = config->Read(style).c_str();
    string scheme = "Element";
    DisplayDescriptor *dd = 0;
    if (styledd != "")
       dd = new DisplayDescriptor(styledd);
    else
       dd = new DisplayDescriptor("default", style, scheme);
    if (!dd->isValid(dd->getStyle(),dd->getColorScheme(),frag.numResidues()>0))
    {
      dd->setColorScheme(dd->getDefaultColorScheme(dd->getStyle()));
    }

    Command *cmd = new CSStyleCmd("CSStyleCmd", &sg);
    cmd->getParameter("descriptor")->setString(dd->toString());
    cmd->getParameter("all")->setBoolean(false);
    getFW().execute(cmd,true);
    delete dd;

    frag.m_atomHighLight.clear();
    frag.m_bondHighLight.clear();

    //
    // Solvent atoms will be made wireframe
    //
    for (int i=lastSoluteAtom; i<numAtoms; ++i) {
       frag.m_atomHighLight.push_back(i);
       const vector<TBond*>& bonds = frag.atomRef(i)->bondList();
       for (size_t bdx=0; bdx<bonds.size(); bdx++) {
          frag.m_bondHighLight.push_back(bonds[bdx]->index());
       }
    }
    DisplayDescriptor wire("Solvent", "Wireframe", "Element");
    wire.load(styleprefs, "Wireframe");
    cmd = new CSStyleCmd("CSStyleCmd", &sg);
    cmd->getParameter("descriptor")->setString(wire.toString());
    cmd->getParameter("all")->setBoolean(false);
    getFW().execute(cmd,true);

    frag.m_atomHighLight.clear();
    frag.m_bondHighLight.clear();
  } else {
    // Restore from styles preferences
    ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
    string style = config->Read("/DefaultStyle", "CPK").c_str();
    string styledd = config->Read(style).c_str();
    string scheme = "Element";
    DisplayDescriptor *dd = 0;
    if (styledd != "")
       dd = new DisplayDescriptor(styledd);
    else
       dd = new DisplayDescriptor("default", style, scheme);
    if (!dd->isValid(dd->getStyle(),dd->getColorScheme(),frag.numResidues()>0))
    {
      dd->setColorScheme(dd->getDefaultColorScheme(dd->getStyle()));
    }

    Command *cmd = new CSStyleCmd("Style", (Receiver*)&sg);
    cmd->getParameter("descriptor")->setString(dd->toString());
    cmd->getParameter("all")->setBoolean(true);
    getFW().execute(cmd,true);
    
    sg.fixAllNubs();

    delete dd;
  }

  // Clear these so we have a consistent state
  frag.m_atomHighLight.clear();
  frag.m_bondHighLight.clear();
}



