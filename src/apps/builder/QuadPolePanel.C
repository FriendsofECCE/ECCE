#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
//  using std::copy;
  using std::fill;
#include <limits>
#include <cctype>
#include <vector>

#include "wx/grid.h" 
#include <wx/link.h>

#include "util/InternalException.H"
#include "util/PreferenceLabels.H"
#include "util/Multipole.H"

#include "tdat/PropVector.H"

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"

#include "wxgui/ewxButton.H"
#include "wxgui/SliderCombo.H"

#include "wxgui/ewxStaticText.H"

#include "viz/AddPolarizabilityCmd.H"
#include "viz/AddMultipoleCmd.H"

#include "dsm/JCode.H"
#include "dsm/PropertyTask.H"

#include "wxviz/WxVizToolFW.H"

#include "QuadPolePanel.H"


// TODO
// . Find a better way to set the scale range
// . Handle disparate geometry indices (see old CVDerivites code (searchfor Gaussian)


// ----------------------------------------------------------------------------




IMPLEMENT_DYNAMIC_CLASS(QuadPolePanel, MomentsPanel)


QuadPolePanel::QuadPolePanel()
  : MomentsPanel(), p_axesText(NULL)
{
   p_useOriginalGeomHack = false;
}


QuadPolePanel::QuadPolePanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : MomentsPanel(), p_axesText(NULL)
{
  p_useOriginalGeomHack = false;
  Create(calculation, parent, id, pos, size, style, name);
}



bool QuadPolePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!MomentsPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("QuadPolePanel creation failed") );
    return false;
  }
  p_grid->SetColLabelSize(0);  // No labels for us

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  // Designed HACK to decide which geometry we need
  // See cvderivs.C for the original implemenation which was not based on
  // property names but on panel name.  This should be equivalent.
  set<string> keys = getPropertyNames();
  string pname = *keys.begin();
  PropertyTask *task = dynamic_cast<PropertyTask*>(calculation);
  if (calculation && task) {
     if (pname == "POLARIZ" || pname == "HYPERPOL1") {
        const JCode *code = task->application();
        p_useOriginalGeomHack =  code->name().find("Gaussian") != string::npos;
     }
  }

  if (isMultipole()) {
     if (calculation && task && task->application())
        p_code = task->application()->name();
     string up = p_code;
     for (size_t i = 0; i < up.size(); i++) up[i] = (char)toupper((unsigned char)up[i]);
     const bool orca = up.find("ORCA") != string::npos;
     const bool gaussian = up.find("GAUSSIAN") != string::npos;
     p_origin = orca ? "mass" : "charge";
     wxString origin = orca ?
        wxString::FromUTF8("Origin: centre of mass (the ORCA output states "
                           "\"Choice of electric origin: Center of mass\").") :
        gaussian ?
        wxString::FromUTF8("Origin: centre of nuclear charge (Gaussian's "
                           "standard orientation; the output does not "
                           "state it).") :
        wxString::FromUTF8("Origin: as in the output (not stated); drawn at "
                           "the centre of nuclear charge.");
     int rank = AddMultipoleCmd::rankOf(*keys.begin());
     wxString note = wxString::Format(wxString::FromUTF8(
        "Surface r = s|f(n)|, f(n) = \u03a3 T n n%s; blue where f > 0, "
        "vermillion where f < 0. Table values are as the code printed them "
        "(including traces); the surface shows the traceless part, "
        "in D\u00b7\u00c5^%d.\n%s"),
        rank == 2 ? "" : rank == 3 ? " n" : " n n", rank - 1, origin);
     ewxStaticText *text = new ewxStaticText(this, wxID_STATIC, note);
     text->Wrap(300);
     GetSizer()->Add(text, 0, wxALIGN_LEFT|wxALL, 3);
     p_axesText = new ewxStaticText(this, wxID_STATIC, wxEmptyString);
     GetSizer()->Add(p_axesText, 0, wxALIGN_LEFT|wxALL, 3);
  }

  if (isPolarizability()) {
     PropVector *vec = (PropVector*) calculation->getProperty(*keys.begin());
     double ew[3], ev[3][3];
     if (AddPolarizabilityCmd::principalAxes(vec, ew, ev)) {
        string units = vec->units();
        // Gaussian and ORCA report bohr^3 and the property carries no unit.
        if (units.empty() || units == "au") units = "a.u.";
        wxString note = wxString::Format(wxString::FromUTF8(
           "Principal values (%s): %.3f, %.3f, %.3f\n"
           "Isotropic mean \u03b1 = (\u03b11+\u03b12+\u03b13)/3 = %.3f"),
           (units == "a.u." ? "a.u. = bohr\u00b3" : units).c_str(),
           ew[0], ew[1], ew[2], (ew[0]+ew[1]+ew[2])/3.0);
        ewxStaticText *text = new ewxStaticText(this, wxID_STATIC, note);
        text->SetToolTip(wxString::FromUTF8(
           "The ellipsoid is centred on the centre of "
           "nuclear charge; its semi-axes lie along the principal axes of "
           "the symmetrised tensor, in proportion to the principal values."));
        GetSizer()->Add(text, 0, wxALIGN_LEFT|wxALL, 3);
        p_grid->SetMinSize(wxSize(200, 80));
        p_axesText = new ewxStaticText(this, wxID_STATIC, wxEmptyString);
        GetSizer()->Add(p_axesText, 0, wxALIGN_LEFT|wxALL, 3);
     }
  }

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  return true;
}


bool QuadPolePanel::isPolarizability() const
{
  set<string> keys = getPropertyNames();
  return keys.size() > 0 && *keys.begin() == "POLARIZ";
}


QuadPolePanel::~QuadPolePanel()
{
}

bool QuadPolePanel::isMultipole() const
{
  set<string> keys = getPropertyNames();
  return keys.size() > 0 && AddMultipoleCmd::rankOf(*keys.begin()) > 0;
}

bool QuadPolePanel::isRelevant(IPropCalculation *propCalc) const
{
  set<string> names;
  names.insert("POLARIZ");
  names.insert("HYPERPOL1");
  return isRelevent(propCalc, "Gaussian", "Geo", names);
}


/**
 * Fill the table and initialize the UI.
 */
void QuadPolePanel::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     string key = *keys.begin();
     PropVector *vec = (PropVector*) expt->getProperty(key);
     if (vec) {
        const vector<string> *rowLabels = vec->rowLabels();

        // TODO the label and scalarize value

        int nRows = vec->rows();
        p_grid->InsertRows(0,nRows/3);
        p_grid->InsertCols(0,3);
        wxString cellv;
        for (int idx=0; idx<nRows; idx++) {
           if (rowLabels) {
              cellv = wxString::Format (wxString("%s=")+PrefLabels::DOUBLEFORMAT, (*rowLabels)[idx].c_str(),vec->value(idx));
           } else {
              // No labels
              cellv = wxString::Format (PrefLabels::DOUBLEFORMAT, vec->value(idx));
           }
           // Remove too much white space but make sure we have a space 
           // after the = or its too hard to read.
           cellv.Replace(" ","",true);
           cellv.Replace("=","= ",true);
           p_grid->SetCellValue((idx/3),idx%3,cellv);
        }
     }

  }


   p_grid->AutoSize();
}


void QuadPolePanel::showVectors(bool guessScale)
{
   if (isMultipole() && p_slider) {
      showMultipole(guessScale);
      return;
   }
   if (!isPolarizability() || !p_slider) return;

   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
   SGContainer& sg = fw.getSceneGraph();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_MOMENTS_VECCOLOR);
   wxColour color = btn->GetBackgroundColour();

   // The slider is in Angstrom per 10 a.u. so that its two decimals suit
   // both water and benzene; the command takes Angstrom per a.u.
   Command *cmd = new AddPolarizabilityCmd("Polarizability", &sg, expt);
   cmd->getParameter("property")->setString(*(getPropertyNames().begin()));
   cmd->getParameter("neutral")->setString(
         color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString());
   cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue()/10.0);
   if (guessScale) {
      cmd->getParameter("newData")->setBoolean(true);
   }
   fw.execute(cmd);

   double scale = cmd->getParameter("Amplitude")->getDouble();
   if (guessScale) {
      p_slider->SetValue(scale*10.0);
   }

   double ew[3], ev[3][3];
   if (p_axesText && AddPolarizabilityCmd::principalAxes(
         (PropVector*) expt->getProperty(*(getPropertyNames().begin())),
         ew, ev)) {
      p_axesText->SetLabel(wxString::Format(wxString::FromUTF8(
         "Semi-axes: %.2f, %.2f, %.2f \u00c5  (%.4f \u00c5 per a.u.)"),
         ew[0]*scale, ew[1]*scale, ew[2]*scale, scale));
   }
}


/**
 * The slider is the length of the largest lobe in Angstrom; the command
 * takes Angstrom per D.Angstrom^(k-1).
 */
void QuadPolePanel::showMultipole(bool guessScale)
{
   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
   SGContainer& sg = fw.getSceneGraph();
   const string prop = *(getPropertyNames().begin());
   const int rank = AddMultipoleCmd::rankOf(prop);

   vector<double> full;
   if (!AddMultipoleCmd::tensor((PropVector*) expt->getProperty(prop), rank,
                                p_code, full))
      return;
   const double fmax = Multipole::maxAbs(rank, full);
   if (fmax <= 0.0) return;

   Command *cmd = new AddMultipoleCmd("Multipole", &sg, expt);
   cmd->getParameter("property")->setString(prop);
   cmd->getParameter("Code")->setString(p_code);
   cmd->getParameter("Center")->setString(p_origin);
   cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue()/fmax);
   if (guessScale) cmd->getParameter("newData")->setBoolean(true);
   fw.execute(cmd);

   const double scale = cmd->getParameter("Amplitude")->getDouble();
   if (guessScale) p_slider->SetValue(scale*fmax);
   if (p_axesText)
      p_axesText->SetLabel(wxString::Format(wxString::FromUTF8(
         "Largest |f| = %.3f D\u00b7\u00c5^%d; scale %.4f \u00c5 per unit"),
         fmax, rank - 1, scale));
}


void QuadPolePanel::receiveFocus()
{
   //cout << "Using geometry " << (p_useOriginalGeomHack ? -2 : -1) << endl;
   selectFragStep(p_useOriginalGeomHack ? -2 : -1);
   if (isPolarizability() || isMultipole()) {
      // The molecule has to show through the ellipsoid.
      getFW().getViewer().setTransparencyType(
         SoGLRenderAction::SORTED_OBJECT_BLEND);
   }
   showVectors(p_guessScale);
   p_guessScale = false;

   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_ALL);
}

void QuadPolePanel::loseFocus()
{
   selectFragStep(-1);  // restore to default
   if (isPolarizability() || isMultipole()) {
      getFW().getViewer().setTransparencyType(SoGLRenderAction::DELAYED_ADD);
   }
   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}
