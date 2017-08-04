#include <iostream>
using namespace std;


#include "Slice.H"
#include "Plane.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/Fragment.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxColorDialog.H"

#include "viz/SGLattice.H"
#include "viz/StyledPlane.H"
#include "viz/NewPlaneCmd.H"
#include "viz/TransformCellCmd.H"
#include "viz/SliceCmd.H"

static const char *INIFILE = "wxbuilder.ini";
static const int COL_COLOR = 0;
static const int COL_NAME = 1;

IMPLEMENT_DYNAMIC_CLASS( Slice, SliceGUI )


// These are used as plane names in the scene graph and should not
// have spaces or any other funky chars in them or the planes will not
// be found during a search.
static const string PLANE1 = "Plane1";
static const string PLANE2 = "Plane2";


/**
 * The slicer is currently hardwired to two planes. These planes,
 * once added to the scenegraph stay there but may be turned off.
 * The planes move independent of each other but are both hkl planes
 * offset from each other.
 */
Slice::Slice( )
{
}

Slice::Slice( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
  : SliceGUI(parent, id, pos, size, style)
{
   subscribe("Slice");

   // Initialize user prefered colors
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   wxString color1, color2;
   color1 = config->Read("Slice/Color1","#FFFF80");
   color2 = config->Read("Slice/Color2","#80C0FF");

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_SLICE_COLOR_1);
   btn->SetBackgroundColour(color1);
   btn = (wxButton*)FindWindow(ID_BUTTON_SLICE_COLOR_2);
   btn->SetBackgroundColour(color2);

   bool trans1, trans2;
   config->Read("Slice/Transparent1", &trans1, false);
   config->Read("Slice/Transparent2", &trans2, false);
   wxCheckBox *box = (wxCheckBox*)FindWindow(ID_CHECKBOX_SLICE_TRANSPARENT_1);
   box->SetValue(trans1);
   box = (wxCheckBox*)FindWindow(ID_CHECKBOX_SLICE_TRANSPARENT_2);
   box->SetValue(trans2);


   this->Enable(false);

}

Slice::~Slice()
{
   unsubscribe();
}

/**
 * Process builder events.  In particular, enable/disable the
 * slicer.  Currently, we disable the entire thing if there is no 
 * lattice.  Long term, we want to support drawing non-lattice planes
 * so we may opt for a different strategy.
 */
void Slice::eventMCB(const Event& event)
{
   string name = event.getName();

   if (name == "ResidueChange" || name == "GeomChange") {
      SGFragment *frag = getFW().getSceneGraph().getFragment();
      LatticeDef *lattice = frag->getLattice();
      if (frag->numAtoms() == 0 || lattice == 0) resetUI();
      this->Enable(lattice);
      // A build operation can cause these to be out of wack
      wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SHOWPLANE);
      if (cbox->IsChecked()) {
         generateHKLPlane();
      }
   } else if (name == "LatticeChange") {
      SGFragment *frag = getFW().getSceneGraph().getFragment();
      LatticeDef *lattice = frag->getLattice();
      if (frag->numAtoms() == 0 || lattice == 0) resetUI();
      this->Enable(lattice);
      wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SHOWPLANE);
      if (lattice != 0) {
         generateHKLPlane();
         showPlanes(cbox->IsChecked());
      } else {
         showPlanes(false);
      }
   } else if (name == "LatticeReplicate") {
      wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SHOWPLANE);
      if (cbox->IsChecked()) {
         generateHKLPlane();
      }
   } else if (name == "StepChange") {
      updateReadOnly();
   }
}

void Slice::resetUI()
{
   setHKL(1,1,1);
   wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SHOWPLANE);
   cbox->SetValue(false);
   updateReadOnly();
}

void Slice::updateReadOnly()
{
   bool changeable = !getFW().isReadOnly();
   if (!changeable) {
      wxWindow *win = FindWindow( ID_BUTTON_SLICER_SLICE );
      win->Enable(changeable);
      win = FindWindow( ID_BUTTON_SLICER_XFORM );
      win->Enable(changeable);
   }
}


void Slice::OnSpinctrlSliceHUpdated( wxSpinEvent& event )
{
   // Turn it on when user changes values
   wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SHOWPLANE);
   if (! cbox->IsChecked()) {
     cbox->SetValue(true);
     showPlanes(true);
   }

   generateHKLPlane();
   event.Skip();
}

void Slice::OnSpinctrlSliceKUpdated( wxSpinEvent& event )
{
   // Turn it on when user changes values
   wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SHOWPLANE);
   if (! cbox->IsChecked()) {
     cbox->SetValue(true);
     showPlanes(true);
   }

   generateHKLPlane();
   event.Skip();
}

void Slice::OnSpinctrlSliceLUpdated( wxSpinEvent& event )
{
   // Turn it on when user changes values
   wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SHOWPLANE);
   if (! cbox->IsChecked()) {
     cbox->SetValue(true);
     showPlanes(true);
   }

   generateHKLPlane();
   event.Skip();
}

void Slice::getHKL(int& h, int& k, int& l)
{
   ewxSpinCtrl *spin;
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_H);
   h = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_K);
   k = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_L);
   l = spin->GetValue();
}

void Slice::setHKL(int h, int k, int l)
{
   ewxSpinCtrl *spin;
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_H);
   spin->SetValue(h);
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_K);
   spin->SetValue(k);
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_L);
   spin->SetValue(l);
   // Update them 
   generateHKLPlane();
}

void Slice::generatePlane1()
{
   string name = PLANE1;

   int h, k, l;
   getHKL(h,k,l);

   ewxSpinCtrl *spin;

   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_OFFSET_1);
   int d = spin->GetValue();

   wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SLICE_TRANSPARENT_1);
   bool transparent = cbox->IsChecked();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_SLICE_COLOR_1);
   wxColour color = btn->GetBackgroundColour();

   generateHKLPlane(name, color,transparent,h,k,l,d/100.);
}

void Slice::generatePlane2()
{
   string name = PLANE2;

   int h, k, l;
   getHKL(h,k,l);

   ewxSpinCtrl *spin;
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_OFFSET_2);
   int d = spin->GetValue();

   wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_SLICE_TRANSPARENT_2);
   bool transparent = cbox->IsChecked();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_SLICE_COLOR_2);
   wxColour color = btn->GetBackgroundColour();

   generateHKLPlane(name, color,transparent,h,k,l,d/100.);
}


void Slice::generateHKLPlane()
{
   generatePlane1();
   generatePlane2();
}

void Slice::generateHKLPlane(const string& name, const wxColour& color, bool transparent, int h,int k, int l, float offset)
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   LatticeDef *lattice = frag->getLattice();
   if (lattice != 0) {

      NewPlaneCmd *cmd = new NewPlaneCmd("New Plane", &getFW().getSceneGraph());
      cmd->getParameter("displacement")->setDouble(offset);
      cmd->getParameter("h")->setInteger(h);
      cmd->getParameter("k")->setInteger(k);
      cmd->getParameter("l")->setInteger(l);
      cmd->getParameter("r")->setInteger(color.Red());
      cmd->getParameter("g")->setInteger(color.Green());
      cmd->getParameter("b")->setInteger(color.Blue());
      cmd->getParameter("transparent")->setBoolean(transparent);
      cmd->getParameter("name")->setString(name);
      //cout << "plane cmd " << *cmd << endl;
      // --NOTE--
      // We Bypass getFW().execute for now since it enforces rules on 
      // commands for replicated systems and this command doesn't care
      // about the two frag problem
      //getFW().execute(cmd);
      cmd->execute();
   }
}


void Slice::OnCheckboxShowplaneClick( wxCommandEvent& event )
{

   if (event.IsChecked()) {
      generateHKLPlane();
   }
   showPlanes(event.IsChecked());


   event.Skip();
}

void Slice::showPlanes(bool flag)
{
   StyledPlane *p1 = getFW().getSceneGraph().getPlane(PLANE1);
   if (p1) p1->show(flag);

   StyledPlane *p2 = getFW().getSceneGraph().getPlane(PLANE2);
   if (p2) p2->show(flag);
}



void Slice::OnButtonSliceColor1Click( wxCommandEvent& event )
{
   wxPoint pos = wxGetMousePosition();
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_SLICE_COLOR_1);
   wxColour color = btn->GetBackgroundColour();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Plane Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);

      wxColour color = btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("Slice/Color1", bgcolor);

      generatePlane1();
   }
   event.Skip();
}

void Slice::OnCheckboxSliceTransparent1Click( wxCommandEvent& event )
{
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("Slice/Transparent1", event.IsChecked());
   generatePlane1();
   event.Skip();
}

void Slice::OnSpinctrlSliceOffset1Updated( wxSpinEvent& event )
{
   generatePlane1();
   event.Skip();
}


void Slice::OnButtonSliceColor2Click( wxCommandEvent& event )
{
   wxPoint pos = wxGetMousePosition();
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_SLICE_COLOR_2);
   wxColour color = btn->GetBackgroundColour();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Plane Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);

      wxColour color = btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("Slice/Color2", bgcolor);

      generatePlane2();
   }
   event.Skip();
}

void Slice::OnCheckboxSliceTransparent2Click( wxCommandEvent& event )
{
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("Slice/Transparent2", event.IsChecked());

   generatePlane2();
   event.Skip();
}

void Slice::OnSpinctrlSliceOffset2Updated( wxSpinEvent& event )
{
   generatePlane2();
   event.Skip();
}


void Slice::OnButtonSlicerXformClick( wxCommandEvent& event )
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   LatticeDef *lattice = frag->getLattice();
   int na, nb, nc;
   lattice->getReplicationFactors(na, nb, nc);
   bool rereplicate = (na != 1 || nb != 1 || nc != 1);

   TransformCellCmd *cmd = new TransformCellCmd("Transform Cell",
         &getFW().getSceneGraph());

   int h, k, l;
   getHKL(h,k,l);

   cmd->getParameter("h")->setInteger(h);
   cmd->getParameter("k")->setInteger(k);
   cmd->getParameter("l")->setInteger(l);


   // NOTE that the user may have the option to cancel mid-stream so we
   // don't want to reset fields if they do that.
   if (getFW().execute(cmd)) {
      setHKL(0,0,1);
      if (rereplicate) {
          EventDispatcher::getDispatcher().publish(Event("GeomChange","rereplicate"));
      }

   }



   event.Skip();
}

void Slice::OnButtonSlicerSliceClick( wxCommandEvent& event )
{
  ewxSpinCtrl *spin;
  spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_OFFSET_1);
  int d1 = spin->GetValue();
  double disp1 = d1/100.0;

  spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_SLICE_OFFSET_2);
  int d2 = spin->GetValue();
  double disp2 = d2/100.0;

  int h, k, l;
  getHKL(h,k,l);

  SliceCmd *cmd = 
     new SliceCmd("Slice", &getFW().getSceneGraph());
  cmd->getParameter("displacement1")->setDouble(disp1);
  cmd->getParameter("displacement2")->setDouble(disp2);
  cmd->getParameter("h")->setInteger(h);
  cmd->getParameter("k")->setInteger(k);
  cmd->getParameter("l")->setInteger(l);
  getFW().execute(cmd);

  event.Skip();
}
