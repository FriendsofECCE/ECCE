
#include "util/InvalidException.H"

#include "tdat/PropGrids.H"
#include "tdat/PropVecGrid.H"
#include "tdat/SingleGrid.H"

#include "dsm/EDSIFactory.H"
#include "dsm/PropertyTask.H"

#include "viz/IsoSurfaceCmd.H"
#include "viz/IsoValueCmd.H"
#include "viz/SurfDisplayTypeCmd.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/NumericValidator.H"
#include "wxgui/SliderCombo.H"
#include "wxgui/StringListCtrl.H"

#include "Cube.H"
#include "CubeCalculation.H"


#define EWX_FL __FILE__,__LINE__


IMPLEMENT_DYNAMIC_CLASS(Cube, VizPropertyPanel)


const wxWindowID Cube::ID_LIST = wxNewId();
const wxWindowID Cube::ID_TEXT_A = wxNewId();
const wxWindowID Cube::ID_TEXT_GRID_A = wxNewId();
const wxWindowID Cube::ID_TEXT_B = wxNewId();
const wxWindowID Cube::ID_TEXT_GRID_B = wxNewId();
const wxWindowID Cube::ID_BUTTON = wxNewId();
const wxWindowID Cube::ID_SLIDER = wxNewId();

const wxWindowID Cube::ID_MENU_SOLID = wxNewId();
const wxWindowID Cube::ID_MENU_MESH = wxNewId();
const wxWindowID Cube::ID_MENU_CONTOUR = wxNewId();
const wxWindowID Cube::ID_MENU_REDGREEN = wxNewId();
const wxWindowID Cube::ID_MENU_REDBLUE = wxNewId();
const wxWindowID Cube::ID_MENU_BLUEYELLOW = wxNewId();
const wxWindowID Cube::ID_MENU_LIGHTDARK = wxNewId();
const wxWindowID Cube::ID_MENU_OPAQUE = wxNewId();
const wxWindowID Cube::ID_MENU_SEMITRANSPARENT = wxNewId();

map<wxString,wxWindowID> Cube::s_id;
map<wxWindowID,wxString> Cube::s_str;


BEGIN_EVENT_TABLE(Cube, VizPropertyPanel)
  EVT_MENU_RANGE(ID_MENU_SOLID, ID_MENU_SEMITRANSPARENT, Cube::OnMenuClick)
  EVT_LIST_ITEM_SELECTED(ID_LIST, Cube::OnListLeftClick)
  EVT_LIST_ITEM_RIGHT_CLICK(ID_LIST, Cube::OnListRightClick)
  EVT_COMMAND_SCROLL(ID_SLIDER, Cube::OnSliderEndMotion)
  EVT_TEXT_ENTER(ID_SLIDER, Cube::OnSliderTextEnter)
  EVT_BUTTON(ID_BUTTON, Cube::OnCompute)
END_EVENT_TABLE()


static int BORDER=2;
static const char * CONFIG_INIFILE         = "wxbuilder.ini";
static const char * CONFIG_SURFACE         = "/CUBE/Surface";
static const char * CONFIG_COLORS          = "/CUBE/Colors";
static const char * CONFIG_TRANSPARENCY    = "/CUBE/Transparency";
static const char * CONFIG_SOLID           = "Solid";
static const char * CONFIG_MESH            = "Mesh";
static const char * CONFIG_CONTOUR         = "Contour";
static const char * CONFIG_REDGREEN        = "Red and Green";
static const char * CONFIG_REDBLUE         = "Red and Blue";
static const char * CONFIG_BLUEYELLOW      = "Blue and Yellow";
static const char * CONFIG_LIGHTDARK       = "Light and Dark";
static const char * CONFIG_OPAQUE          = "Opaque";
static const char * CONFIG_SEMITRANSPARENT = "Semi Transparent";


Cube::Cube()
  : VizPropertyPanel()
  , p_list(NULL)
  , p_labelChemSys(NULL)
  , p_textA(NULL)
  , p_textAGrid(NULL)
  , p_textB(NULL)
  , p_textBGrid(NULL)
  , p_button(NULL)
  , p_slider(NULL)
  , p_multA(1.0)
  , p_multB(1.0)
  , p_propGrids(NULL)
{
}


Cube::Cube(IPropCalculation *calculation, wxWindow* parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : VizPropertyPanel()
  , p_list(NULL)
  , p_labelChemSys(NULL)
  , p_textA(NULL)
  , p_textAGrid(NULL)
  , p_textB(NULL)
  , p_textBGrid(NULL)
  , p_button(NULL)
  , p_slider(NULL)
  , p_multA(1.0)
  , p_multB(1.0)
  , p_propGrids(NULL)
{
}


bool Cube::Create(IPropCalculation *calculation, wxWindow* parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style,
                                name)) {
    wxFAIL_MSG( wxT("Cube creation failed") );
    return false;
  }

  SetMinSize(wxSize(400,-1)); // so it doesn't get too small in AUI

  if (s_id.empty()) {
    s_id[CONFIG_SOLID]           = ID_MENU_SOLID;
    s_id[CONFIG_MESH]            = ID_MENU_MESH;
    s_id[CONFIG_CONTOUR]         = ID_MENU_CONTOUR;
    s_id[CONFIG_REDGREEN]        = ID_MENU_REDGREEN;
    s_id[CONFIG_REDBLUE]         = ID_MENU_REDBLUE;
    s_id[CONFIG_BLUEYELLOW]      = ID_MENU_BLUEYELLOW;
    s_id[CONFIG_LIGHTDARK]       = ID_MENU_LIGHTDARK;
    s_id[CONFIG_OPAQUE]          = ID_MENU_OPAQUE;
    s_id[CONFIG_SEMITRANSPARENT] = ID_MENU_SEMITRANSPARENT;
    // reverse mapping
    for (map<wxString,wxWindowID>::iterator it = s_id.begin();
            it != s_id.end();
            ++it) {
      s_str[it->second] = it->first;
    }
  }

  return true;
}


Cube::~Cube()
{
}


void Cube::refresh()
{
  if (p_propGrids) {
    delete p_propGrids;
    p_propGrids = NULL;
  }
  p_list->DeleteAllItems();

  // Find the PropGrids instance(s).  Different lookups whether we are a
  // PropertyTask or a CubeCalculation.
  IPropCalculation *calc = getCalculation();
  PropertyTask *task = NULL;
  CubeCalculation *cube = NULL;
  istream *is;
  try {
    if ((task = dynamic_cast<PropertyTask*>(calc))) {
      // Get the PropVecGrid and then attemp to consolidate its PropGrids.
      PropVecGrid * propVecGrid =
              dynamic_cast<PropVecGrid*>(calc->getProperty("CUBE"));
      if (!propVecGrid) throw InvalidException("Missing CUBE property", EWX_FL);
      int numPropGrids = propVecGrid->getNumPropGrids();
      if (numPropGrids <= 0) throw InvalidException("Missing grids", EWX_FL);
      p_propGrids = propVecGrid->getPropGrids(0);
      for (int idx=1; idx<numPropGrids; ++idx) {
        PropGrids * propGrids = propVecGrid->getPropGrids(idx);
        if (!p_propGrids->absorb(propGrids)) {
          wxLogWarning("Could not consolidate multiple grids");
        }
        delete propGrids;
      }
    } else if ((cube = dynamic_cast<CubeCalculation*>(calc))) {
      // Get the Resource, then its contents as an istream.
      Resource * resource = EDSIFactory::getResource(calc->getURL());
      if (!resource) throw InvalidException("Missing Resource", EWX_FL);
      is = resource->getDocument();
      if (!is) throw InvalidException("Missing document", EWX_FL);
      p_propGrids = new PropGrids();
      if (!p_propGrids->readCubeFile(*is)) {
        throw InvalidException("Could not read grid", EWX_FL);
      }
    } else {
    }
  } catch (EcceException &ex) {
    wxLogError("Cube: %s", ex.what());
    if (p_propGrids) {
      delete p_propGrids;
      p_propGrids = NULL;
    }
    if (is) delete is;
  }

  if (p_propGrids) {
    for (int idx=0, numGrids=p_propGrids->numGrids(); idx<numGrids; ++idx) {
      p_list->Append(p_propGrids->returnGrid(idx)->name());
    }
    p_labelChemSys->SetLabel(p_propGrids->getFrag().generateEmpiricalFormula());
  }
}


void Cube::receiveFocus()
{
  WxVizToolFW& fw = getFW();
  if (p_propGrids
          && getGridIndex() >= 0
          && getGridIndex() < p_propGrids->numGrids()) {
    SGContainer& sg = fw.getSceneGraph();
    sg.setCurrentGrid(p_propGrids->returnGrid(getGridIndex()));
  }
  fw.getViewer().setTransparencyType(SoGLRenderAction::SCREEN_DOOR);
  //fw.getViewer().setTransparencyType(SoGLRenderAction::DELAYED_ADD);
  fw.getSceneGraph().getMORoot()->whichChild.setValue(SO_SWITCH_ALL);
}


void Cube::lostFocus()
{
  WxVizToolFW& fw = getFW();
  //fw.getViewer().setTransparencyType(SoGLRenderAction::SCREEN_DOOR);
  fw.getViewer().setTransparencyType(SoGLRenderAction::DELAYED_ADD);
  fw.getSceneGraph().getMORoot()->whichChild.setValue(SO_SWITCH_NONE);
}


void Cube::initialize()
{
  // let's create everything we hold pointers to first
  p_list = new StringListCtrl(this, ID_LIST);
  p_labelChemSys = new ewxStaticText(this, wxID_ANY, "<Formula>");
  p_slider = new SliderCombo(this, ID_SLIDER);
  p_slider->SetRange(-5.0,5.0);
  p_slider->SetValue(-1.0);
  p_textA = new ewxTextCtrl(this, ID_TEXT_A);
  p_textAGrid = new ewxStaticText(this, ID_TEXT_GRID_A, "<Left Click>");
  p_textB = new ewxTextCtrl(this, ID_TEXT_B);
  p_textBGrid = new ewxStaticText(this, ID_TEXT_GRID_B, "<Right Click>");
  p_button = new ewxButton(this, ID_BUTTON, "A * GridA + B * GridB");
  
  NumericValidator<float> aval(&p_multA, -3.4E38, 3.4E38);
  NumericValidator<float> bval(&p_multB, -3.4E38, 3.4E38);
  p_textA->SetValidator(aval);
  p_textB->SetValidator(bval);

  // next, let's create everything we don't hold pointers to
  ewxStaticText * labelA = new ewxStaticText(this, wxID_STATIC, "A:");
  ewxStaticText * labelB = new ewxStaticText(this, wxID_STATIC, "B:");
  ewxStaticText * labelTimes1 = new ewxStaticText(this, wxID_STATIC, "*");
  ewxStaticText * labelTimes2 = new ewxStaticText(this, wxID_STATIC, "*");

  wxSizer * hmainsizer = new wxBoxSizer(wxHORIZONTAL);
  wxSizer * vsizerRight = new wxBoxSizer(wxVERTICAL);
  wxSizer * hsizerChemSys = new wxBoxSizer(wxHORIZONTAL);
  wxFlexGridSizer * flexsizer = new wxFlexGridSizer(4, BORDER, BORDER);

  wxSizerFlags flexLabelFlags;
  flexLabelFlags.Align(wxALIGN_CENTER_VERTICAL);

  SetSizer(hmainsizer);

  // let's layout the right side of the panel first
  hsizerChemSys->Add(p_labelChemSys, 1, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  hsizerChemSys->Add(p_slider, 0, wxALL, BORDER);
  vsizerRight->Add(hsizerChemSys, 0, wxGROW);

  flexsizer->Add(labelA, flexLabelFlags);
  flexsizer->Add(p_textA, flexLabelFlags);
  flexsizer->Add(labelTimes1, flexLabelFlags);
  flexsizer->Add(p_textAGrid, flexLabelFlags);
  flexsizer->Add(labelB, flexLabelFlags);
  flexsizer->Add(p_textB, flexLabelFlags);
  flexsizer->Add(labelTimes2, flexLabelFlags);
  flexsizer->Add(p_textBGrid, flexLabelFlags);
  vsizerRight->Add(flexsizer, 0, wxGROW|wxALL, BORDER);

  vsizerRight->Add(p_button, 0, wxGROW|wxALL, BORDER);

  // now the left side, and while we're at it, the whole thing
  hmainsizer->Add(p_list, 1, wxGROW|wxALL, BORDER);
  hmainsizer->Add(vsizerRight);

  hmainsizer->Fit(this);
  hmainsizer->SetSizeHints(this);

  // actually load the CUBE data
  refresh();

  TransferDataToWindow();
}


wxWindow* Cube::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
          "Cube Menu");
 
#define EWX_NB wxNullBitmap, wxNullBitmap
  tb->AddRadioTool(ID_MENU_SOLID, CONFIG_SOLID, EWX_NB);
  tb->AddRadioTool(ID_MENU_MESH, CONFIG_MESH, EWX_NB);
  tb->AddRadioTool(ID_MENU_CONTOUR, CONFIG_CONTOUR, EWX_NB);

  tb->AddSeparator();

  tb->AddRadioTool(ID_MENU_REDGREEN, CONFIG_REDGREEN,  EWX_NB);
  tb->AddRadioTool(ID_MENU_REDBLUE, CONFIG_REDBLUE, EWX_NB);
  tb->AddRadioTool(ID_MENU_BLUEYELLOW, CONFIG_BLUEYELLOW, EWX_NB);
  tb->AddRadioTool(ID_MENU_LIGHTDARK, CONFIG_LIGHTDARK, EWX_NB);

  tb->AddSeparator();

  tb->AddRadioTool(ID_MENU_OPAQUE, CONFIG_OPAQUE, EWX_NB);
  tb->AddRadioTool(ID_MENU_SEMITRANSPARENT, CONFIG_SEMITRANSPARENT, EWX_NB);
#undef EWX_NB

  tb->Realize();

  ewxConfig *config = ewxConfig::getConfig(CONFIG_INIFILE);
  tb->ToggleTool(s_id[config->Read(CONFIG_COLORS,CONFIG_REDGREEN)],true);
  tb->ToggleTool(s_id[config->Read(CONFIG_SURFACE,CONFIG_SOLID)],true);
  tb->ToggleTool(s_id[config->Read(CONFIG_TRANSPARENCY,CONFIG_SEMITRANSPARENT)],
                 true);

  return tb;
}


void Cube::updateSurface()
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   IPropCalculation *expt = getCalculation();

   ewxConfig *config = ewxConfig::getConfig(CONFIG_INIFILE);
   wxString stype = config->Read(CONFIG_SURFACE, CONFIG_SOLID);
   Command *cmd = new SurfDisplayTypeCmd("Surface Type", &sg, expt);
   cmd->getParameter("IsosurfStyle")->setString(stype.c_str());
   fw.execute(cmd);
}


void Cube::updateIsoValue()
{
   double isovalue = static_cast<double>(p_slider->GetFloatValue());
   updateIsoValue(isovalue);
}


void Cube::updateIsoValue(double isovalue)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  IPropCalculation *expt = getCalculation();

  double posR, posG, posB, negR, negG, negB;
  getSurfaceColors(posR, posG, posB, negR, negG, negB);

  Command *cmd = new IsoValueCmd("Iso Value", &sg, expt);
  cmd->getParameter("Value")->setDouble(isovalue);
  cmd->getParameter("transparency")->setDouble(getTransparency());
  cmd->getParameter("positiveRed")->setDouble(posR);
  cmd->getParameter("positiveGreen")->setDouble(posG);
  cmd->getParameter("positiveBlue")->setDouble(posB);
  cmd->getParameter("negativeRed")->setDouble(negR);
  cmd->getParameter("negativeGreen")->setDouble(negG);
  cmd->getParameter("negativeBlue")->setDouble(negB);
  fw.execute(cmd);
}


void Cube::getSurfaceColors(double& posR, double& posG, double& posB,
                            double& negR, double& negG, double& negB)
{
   ewxConfig *config = ewxConfig::getConfig(CONFIG_INIFILE);
   wxString colorType = config->Read(CONFIG_COLORS, CONFIG_REDGREEN);
   if (colorType == CONFIG_REDGREEN) {
      posR = 1.0; posG = 0.0; posB = 0.0;
      negR = 0.0; negG = 1.0; negB = 0.0;
   } else if (colorType == CONFIG_REDBLUE) {
      posR = 1.0; posG = 0.0; posB = 0.0;
      negR = 0.0; negG = 0.2; negB = 1.0;
   } else if (colorType == CONFIG_BLUEYELLOW) {
      posR = 0.0; posG = 0.2; posB = 1.0;
      negR = 1.0; negG = 1.0; negB = 0.0;
   } else if (colorType == CONFIG_LIGHTDARK) {
      posR = 0.95; posG = 0.95; posB = 0.95;
      negR = 0.2; negG = 0.2; negB = 0.2;

      char* clrptr;
      // first check for 0.0 to 1.0 normalized values
      char* clrstr = getenv("ECCE_SURFACE_COLOR1");
      if (clrstr) {
         clrptr = strtok(clrstr, " ,:/");
         posR = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         posG = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         posB = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         negR = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         negG = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         negB = strtod(clrptr, NULL);
      }

      // then check for 0 to 255 8-bit/1-byte values
      clrstr = getenv("ECCE_SURFACE_COLOR255");
      if (clrstr) {
         clrptr = strtok(clrstr, " ,:/");
         int clrval = (int)strtol(clrptr, NULL, 10);
         posR = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         posG = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         posB = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         negR = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         negG = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         negB = ((double)clrval)/255.0;
      }
   }
}


double Cube::getTransparency()
{
   double ret = 0.0;
   ewxConfig *config = ewxConfig::getConfig(CONFIG_INIFILE);
   if (config->Read(CONFIG_TRANSPARENCY, CONFIG_OPAQUE)
            == CONFIG_SEMITRANSPARENT) {
      ret = 0.5;
   }
   return ret;
}


int Cube::getGridIndex()
{
  return p_list->GetFirstSelected();
}


void Cube::OnMenuClick(wxCommandEvent& event)
{
  ewxConfig *config = ewxConfig::getConfig(CONFIG_INIFILE);
  wxString key;
  bool wasSurface = false;
  wxWindowID id = event.GetId();

  if (id == ID_MENU_SOLID
          || id == ID_MENU_MESH
          || id == ID_MENU_CONTOUR) {
    key = CONFIG_SURFACE;
    wasSurface = true;
  } else if (id == ID_MENU_REDGREEN
          || id == ID_MENU_REDBLUE
          || id == ID_MENU_BLUEYELLOW
          || id == ID_MENU_LIGHTDARK) {
    key = CONFIG_COLORS;
  } else if (id == ID_MENU_OPAQUE
          || id == ID_MENU_SEMITRANSPARENT) {
    key = CONFIG_TRANSPARENCY;
  }

  config->Write(key, s_str[id]);

  if (wasSurface)
    updateSurface();
  else
    updateIsoValue();
}


void Cube::OnListLeftClick(wxListEvent& event)
{
  if (p_propGrids
          && getGridIndex() >= 0
          && getGridIndex() < p_propGrids->numGrids()) {
    WxVizToolFW& fw = getFW();
    SGContainer& sg = fw.getSceneGraph();
    IPropCalculation *expt = getCalculation();
    sg.setCurrentGrid(p_propGrids->returnGrid(getGridIndex()));

    double fieldMin = 0.;
    double fieldMax = 0.;
    SingleGrid *grid = sg.getCurrentGrid();
    if (grid) {
      fieldMin = grid->fieldMin();
      fieldMax = grid->fieldMax();
    }
    bool validField = fieldMax > fieldMin;

    if (validField) {
      // Copied, like most of this other code, from the MoPanel.
      double posR, posG, posB, negR, negG, negB;
      getSurfaceColors(posR, posG, posB, negR, negG, negB);

      Command * cmd = new IsoSurfaceCmd("Iso Surface", &sg, expt);
      cmd->getParameter("transparency")->setDouble(getTransparency());
      cmd->getParameter("positiveRed")->setDouble(posR);
      cmd->getParameter("positiveGreen")->setDouble(posG);
      cmd->getParameter("positiveBlue")->setDouble(posB);
      cmd->getParameter("negativeRed")->setDouble(negR);
      cmd->getParameter("negativeGreen")->setDouble(negG);
      cmd->getParameter("negativeBlue")->setDouble(negB);
      fw.execute(cmd);

      updateSurface();

      // only change label if this truly is a left click (could be a right one)
      if (::wxGetMouseState().LeftDown()) {
        p_textAGrid->SetLabel(grid->name());
      }

      // TODO Determine max abs value of field for the (abs value) slider.
    } else {
      wxLogError("Bad field data");
    }

    p_slider->Enable(validField);
  }

  event.Skip();
}


void Cube::OnListRightClick(wxListEvent& event)
{
  p_textBGrid->SetLabel(event.GetText());
  event.Skip();
}


void Cube::OnSliderEndMotion(wxScrollEvent& event)
{
  updateIsoValue();
  event.Skip();
}


void Cube::OnSliderTextEnter(wxCommandEvent& event)
{
  updateIsoValue();
  event.Skip();
}


void Cube::OnCompute(wxCommandEvent& event)
{
  // Use the grid names to get the grid indices and the SingleGrids.
  // Then let the magic happen.
  wxString labelA = p_textAGrid->GetLabel();
  wxString labelB = p_textBGrid->GetLabel();
  int idxA = p_list->Find(labelA);
  int idxB = p_list->Find(labelB);
  if (p_propGrids
          && idxA != idxB
          && idxA >= 0
          && idxA < p_propGrids->numGrids()
          && idxB >= 0
          && idxB < p_propGrids->numGrids()) {
    // our new grid
    SingleGrid *grid = new SingleGrid;
    // generate the name
    wxString name;
    name << p_multA << "*" << labelA << " + " << p_multB << "*" << labelB;
    grid->name(name.c_str());
    // generate the grid, add to our cache
    SingleGrid *gridA = p_propGrids->returnGrid(idxA);
    SingleGrid *gridB = p_propGrids->returnGrid(idxB);
    grid->addFields(p_multA, gridA, p_multB, gridB);
    p_propGrids->addGrid(grid);
    // select and show in list
    p_list->Append(name);
    p_list->SetSelection(name);
  }
}

