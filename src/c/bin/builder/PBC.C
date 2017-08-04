
#include <iostream>
  using namespace std;

#include <wx/sizer.h>

#include "util/Transformations.H"
#include "util/Event.H"
#include "util/EventDispatcher.H"
using namespace ecce;

#include "tdat/LatticeDef.H"
#include "tdat/PointGroup.H"
#include "tdat/SymmetryOps.H"

#include "dsm/ICalculation.H"


#include "wxgui/ewxChoice.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxListViewComboPopup.H"

#include "viz/SGLattice.H"
#include "viz/PBCReplicateCmd.H"
#include "viz/PBCEditLatticeCmd.H"
#include "viz/PBCFoldCmd.H"
#include "viz/PBCEquivRectCmd.H"
#include "viz/PBCSuperCmd.H"
#include "viz/PBCCreateCmd.H"
#include "viz/PBCDeleteCmd.H"
#include "viz/PBCEditVectorsCmd.H"
#include "viz/PBCGenerateFragCmd.H"

#include "PBC.H"

//#define DEBUG

IMPLEMENT_DYNAMIC_CLASS( PBC, PBCGUI )
const wxWindowID PBC::ID_COMBO_SPACE_GROUP = wxNewId();

/**
 This panel introduces all kinds of interaction problems because it has
 both a real fragment and a replicated fragment.  The replicated fragment
 is essentially a view of the real fragment.  Unfortunately we can't easily
 distinguish between the two.

 On possibility is to have a separate SG node for the replicated view.  The
 problem with that design is that there is no way to know when to swap between
 the real and replicated view.  There is no notion of panel having focus as
 with the property panels.  We could add that but...

 A second option is to send a "readonly message" that forces some sort of
 readonly mode for any dialogs that let you edit the fragment.  This is 
 exactly the type of confusing behavior we tried to move away from with 
 residue editing.

 A third option is to take advantage of the fact that all commands 
 execute through the command manager and forward them on to the lattice panel
 to review if the fragment should first revert to the base fragment.

 A similar option is to have the command manager refuse to execute commands
 if the lattice is not sized by 1,1,1.  This is a bit tricky though because 
 we'd have to have a list of commands that can pass through this filter.
 Basically this is similar to the previous option except for where the 
 decision is managed.

 We will follow the third option for now with the addition of having the
 user be prompted that they are viewing a replicated systems and the command
 can't be issued against it.  They would then have the capability to
 automatically revert to the unit cell. 

 We will hardwire the commands that can be let through because there are few:
  . superlattice

 */

PBC::PBC( )
{
   p_baseFrag = 0;
   p_noStashFlag = false;
}

PBC::PBC( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
   Create(parent, id, pos, size, style);
   subscribe("PBC");
   p_baseFrag = 0;
   p_noStashFlag = false;
}


PBC::~PBC()
{
   unsubscribe();
   if (p_baseFrag != 0) delete p_baseFrag;
}


bool PBC::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
  if (!wxPanel::Create(parent, id, pos, size, style)) {
    wxFAIL_MSG( wxT("PBC creation failed") );
    return false;
  }

  CreateControls();

   wxComboCtrl * cc = new wxComboCtrl(p_latticePanel, ID_COMBO_SPACE_GROUP,
         wxEmptyString,
         wxDefaultPosition, wxSize(80, 22),
         wxCC_STD_BUTTON|wxTE_PROCESS_ENTER);
   //NOTE the event here must match the event we send in ewxListViewComboPopup
   //The event of interest has not been settled.
   Connect(ID_COMBO_SPACE_GROUP,
                wxEVT_COMMAND_TEXT_ENTER,
               (wxObjectEventFunction)
               (wxEventFunction)&PBC::OnListChange,0,this);

   // Make sure we use popup that allows focusing the listview.
   cc->UseAltPopupWindow();
   cc->SetPopupMinWidth(525);
   cc->SetPopupMaxHeight(540);

   cc->GetTextCtrl()->SetBackgroundColour(ewxStyledWindow::getInputColor());
   cc->GetTextCtrl()->SetFont(ewxStyledWindow::getBoldFont());

   ewxListViewComboPopup* iface =
     new ewxListViewComboPopup(ID_COMBO_SPACE_GROUP);

   cc->SetPopupControl(iface);

   //vector<string> selections = PointGroup::getGroups();
   vector<string> groups = PointGroup::getSpaceGroups();

   for (size_t i=0; i<groups.size(); i++ )
      iface->AddSelection(groups[i]);


   p_symSizer->Insert( 1, cc, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
   p_symSizer->Layout();
   cc->SetValue("P1");

  ewxNumericValidator validator;
  validator.setHardRange("(0.,360.)");
  validator.setValue(0.0);

  ewxTextCtrl *txt;
  validator.setHardRange("[0.,)");
  validator.setValue(5.0);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_B);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_C);
  txt->SetValidator(validator);

  validator.setHardRange("(0.,360.)");
  validator.setValue(90.0);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_ALPHA);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_BETA);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_GAMMA);
  txt->SetValidator(validator);

  validator.setHardRange("(,)");
  validator.setValue(0.0);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1X);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Y);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Z);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2X);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Y);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Z);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3X);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Y);
  txt->SetValidator(validator);
  txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Z);
  txt->SetValidator(validator);

  /*
  ewxSpinCtrl *spin;
  spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_XREP);
  spin->SetRange(1,100);
  spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_YREP);
  spin->SetRange(1,100);
  spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_ZREP);
  spin->SetRange(1,100);
  */

  // Want to interject this so we get a decent default size
  // Showing the lattice sizer; lattice vectors not as easy to edit
  ewxChoice *choice = (ewxChoice*)FindWindow(ID_CHOICE_PBC_TYPE);
  choice->SetStringSelection("Lattice");
  p_latticePanel->GetSizer()->Show((size_t)2,true);
  p_latticePanel->GetSizer()->Show((size_t)1,false);

  indicateHaveLattice(false);
  // Hack - can't properly set up the code for checking the fragment
  // to decide if create button should be enabled because we haven't been
  // connected to the toolkit yet.  Sooo, here we forcibly disable it after
  // running through normal logic
  ewxButton *btn = (ewxButton*)FindWindow(ID_BUTTON_PBC_CREATE);
  btn->Enable(false);

  GetSizer()->Layout();
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
  Centre();

  return true;
}

void PBC::OnListChange(wxListEvent& event)
{
   // For now assign symmetry only if they hit Generate
   //assignSymmetry();
}

bool PBC::Enable(bool enable)
{
   bool ret = true;
   ewxButton *btn = (ewxButton*)FindWindow(ID_BUTTON_PBC_CREATE);
   btn->Enable(!enable);
   p_latticePanel->Enable(enable);

   return ret;
}



/**
 * Execute a PBCCmd.
 * PBCCommands are designed to support restoring to the base fragment and
 * re-replicating as needed.
 * The basefrag is the unit cell where as the scenegraph frag has the 
 * replicated atoms.
 */
void PBC::execute(PBCCmd *cmd)
{
   WxVizToolFW &fw = getFW();
   if (cmd->shouldAutoReplicate() && p_baseFrag) {
      //cout << "command is " << cmd->getName() << endl;
      // In this case, get the current dimensions so we can re-apply them
      int na, nb, nc;
      LatticeDef *lattice = p_baseFrag->getLattice();
      lattice->getReplicationFactors(na,nb,nc);
      //cout << "replication factors are " << na << " " << nb << " " << nc << endl;
      restoreUnitCell();

      fw.execute(cmd);
      //cout << "now rereplicating " << endl;
      replicate(na,nb,nc);

   } else {
     //cout << "NOT auto replicating" << endl;
     fw.execute(cmd);
   }
}

void PBC::eventMCB(const Event& event)
{
   string name = event.getName();

   if (name == "StyleChange") {
      // This is necessary or atoms will disappear if style changes
      if (IsShownOnScreen()) refresh();
   } else if (name == "ResidueChange" || name == "GeomChange") {
      string value = event.getValue();
      SGFragment *frag = getFW().getSceneGraph().getFragment();
      if (frag->numAtoms() == 0) {
         if (p_baseFrag) {
            delete p_baseFrag;
            p_baseFrag = 0;
         }
         ewxSpinCtrl *spin;
         spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_XREP);
         spin->SetValue(1);
         spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_YREP);
         spin->SetValue(1);
         spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_ZREP);
         spin->SetValue(1);
      }

      getFW().getSceneGraph().touchLattice();
      if (IsShownOnScreen()) refresh();

      // Must be after refresh since replicate takes data from UI
      if (value == "replicate") {
         // This indicates and undo that should replicate after the
         // base fragment is restored.
         SGFragment *frag = getFW().getSceneGraph().getFragment();
         LatticeDef *lattice = frag->getLattice();
         int na, nb, nc;
         lattice->getReplicationFactors(na, nb, nc);
         replicate(na, nb, nc);
      } else if (value == "rereplicate") {
         // Enables other tools to perform an action and then replicate
         // without using commands that inherit from PBCCmd
         replicate();
      }

   } else if (name == "LatticeChange") {
      if (IsShownOnScreen()) refresh();
   } else if (name == "StepChange") {
      // This event is associated with viewing.  No editing functions
      // so no frag stashing
      if (IsShownOnScreen()) refresh();
   }
}

void PBC::stashFragment()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   LatticeDef *lattice = frag->getLattice();
   if (lattice) {
      int na, nb, nc;
      lattice->getReplicationFactors(na, nb, nc);
      if (true) { //na == 1 && nb == 1 && nc == 1) {
         // Only stash if its not replicated.  Stashed frag should never
         // be replicated.
//#define DEBUG
#ifdef DEBUG
      cout << "stashing a new frag" << frag->numAtoms() << endl;
#endif
         if (p_baseFrag != 0) delete p_baseFrag;
         p_baseFrag = new SGFragment(*frag);
      }
   } else {
#ifdef DEBUG
      cout << "stashing a new frag" << endl;
#endif
      if (p_baseFrag != 0) delete p_baseFrag;
      p_baseFrag = new SGFragment(*frag);
   }
}

void PBC::updateReadOnly()
{
   bool changeable = !getFW().isReadOnly();
   if (!changeable) {
      wxWindow *win = FindWindow(ID_CHOICE_PBC_TYPE);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_B);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_C);
      win->Enable(changeable);

      win = FindWindow(ID_TEXTCTRL_PBC_A1X);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A1Y);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A1Z);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A2X);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A2Y);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A2Z);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A3X);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A3Y);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_A3Z);
      win->Enable(changeable);

      win = FindWindow(ID_TEXTCTRL_PBC_ALPHA);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_BETA);
      win->Enable(changeable);
      win = FindWindow(ID_TEXTCTRL_PBC_GAMMA);
      win->Enable(changeable);

      win = FindWindow(ID_CHECKBOX_PBC_LOCK);
      win->Enable(changeable);
      win = FindWindow(ID_BUTTON_PBC_EQUIVRECT);
      win->Enable(changeable);
      win = FindWindow(ID_BUTTON_PBC_FOLD);
      win->Enable(changeable);
      win = FindWindow(ID_PBC_GENERATE);
      win->Enable(changeable);
      win = FindWindow(ID_BUTTON_PBC_DELETE);
      win->Enable(changeable);
      win = FindWindow(ID_COMBO_SPACE_GROUP);
      win->Enable(changeable);
   }
}


void PBC::updateSpaceGroup()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   LatticeDef *lattice = frag->getLattice();
   if (frag && lattice) {
     string sym = frag->pointGroup();
     wxComboCtrl *cc = (wxComboCtrl*)FindWindow(ID_COMBO_SPACE_GROUP);
     if (PointGroup::isSpaceGroup(sym)) {
       cc->SetValue(sym);
     } else {
       cc->SetValue("P1");
     }
   }
}

void PBC::indicateHaveLattice()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();

   SGLattice *sgl = (SGLattice*)getFW().getSceneGraph().findNode("lattice");
   LatticeDef *lattice = frag->getLattice();
   bool enable = frag && lattice && sgl;
   indicateHaveLattice(enable);


}


/**
 * The idea is to only show the create button when there is no lattice
 * and one can be created and hide the rest and visa versa.  However,
 * there seems to be no way to get the sizing to work correctly within aui.
 * The panel will grow from an original create only sizer or in fact grow
 * at any time but getting it to shrink up is another problem.
 * Hiding the full panel contents is really lame since it doesn't shrink.
 * Gary's suggestion was to hide the one sizer with the delete button
 * so the size doesn't change but I'm sure this violates every good standard...
 * Hardwired calls to SetSize don't work either.
 */
void PBC::indicateHaveLattice(bool haveLattice)
{
   GetSizer()->Show((size_t)0,!haveLattice);
   //p_latticePanel->Show(haveLattice);
   p_latticePanel->GetSizer()->Show((size_t)4,haveLattice);
   Layout();
   Enable(haveLattice);
}

Command *PBC::restoreUnitCell()
{
   // Need to restore to the base fragment first
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   frag->clear();
   *frag = *p_baseFrag;
   frag->touchNumbers();
   getFW().getSceneGraph().touchChemDisplay();

   PBCCmd *cmd = new PBCReplicateCmd("Restore Unit Cell", 
                                              &getFW().getSceneGraph(), p_baseFrag);
   cmd->getParameter("axis1")->setInteger(1);
   cmd->getParameter("axis2")->setInteger(1);
   cmd->getParameter("axis3")->setInteger(1);
   cmd->execute();
   return cmd;
}

void PBC::showReplication()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();

   SGLattice *sgl = (SGLattice*)getFW().getSceneGraph().findNode("lattice");
   LatticeDef *lattice = frag->getLattice();
   if (frag && lattice && sgl) {
      int na, nb, nc;
      lattice->getReplicationFactors(na, nb, nc);
      replicate(na,nb,nc);
   }
}

void PBC::refresh()
{
   // Not totally sure about this... Relies on refresh being called when 
   // dialog opened
   // At that point, it is NOT shown on screen so this check must be done 
   // internally by methods that want to call refresh.
   // What we are trying to avoid is stashing fragments (think large md)
   // when the PBC toolkit is irrelevent
   //if (!IsShownOnScreen()) return;

   updateReadOnly();
   if (!p_noStashFlag) stashFragment();
   //showReplication();

   SGFragment *frag = getFW().getSceneGraph().getFragment();

   SGLattice *sgl = (SGLattice*)getFW().getSceneGraph().findNode("lattice");
   LatticeDef *lattice = frag->getLattice();

   if (frag) {
      if (lattice && sgl) {

         double a,b,c,alpha,beta,gamma;
         lattice->getLattice(a,b,c,alpha,beta,gamma);
         ewxTextCtrl *txt;
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A);
         txt->SetValue(wxString::Format (_T("%lf"), a));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_B);
         txt->SetValue(wxString::Format (_T("%lf"), b));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_C);
         txt->SetValue(wxString::Format (_T("%lf"), c));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_ALPHA);
         txt->SetValue(wxString::Format (_T("%lf"), alpha));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_BETA);
         txt->SetValue(wxString::Format (_T("%lf"), beta));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_GAMMA);
         txt->SetValue(wxString::Format (_T("%lf"), gamma));

         vector<MPoint> *vecs = lattice->toVectors();
         MPoint a1 = (*vecs)[0];
         MPoint a2 = (*vecs)[1];
         MPoint a3 = (*vecs)[2];

         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1X);
         txt->SetValue(wxString::Format (_T("%lf"), a1.x()));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Y);
         txt->SetValue(wxString::Format (_T("%lf"), a1.y()));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Z);
         txt->SetValue(wxString::Format (_T("%lf"), a1.z()));

         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2X);
         txt->SetValue(wxString::Format (_T("%lf"), a2.x()));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Y);
         txt->SetValue(wxString::Format (_T("%lf"), a2.y()));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Z);
         txt->SetValue(wxString::Format (_T("%lf"), a2.z()));

         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3X);
         txt->SetValue(wxString::Format (_T("%lf"), a3.x()));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Y);
         txt->SetValue(wxString::Format (_T("%lf"), a3.y()));
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Z);
         txt->SetValue(wxString::Format (_T("%lf"), a3.z()));

         delete vecs;

         bool replicated = false;
         int na1, na2, na3;
         lattice->getReplicationFactors(na1, na2, na3);
         if (na1 > 1 || na2 > 1 || na3 > 1) replicated = true;
         ewxButton *btn;
         btn = (ewxButton*)FindWindow(ID_BUTTON_PBC_SUPER);
         btn->Enable(replicated);
         btn = (ewxButton*)FindWindow(ID_BUTTON_PBC_RESTORE);
         btn->Enable(replicated);

         updateReplicateBtn(-1,-1,-1);

      } else {
         // No lattice
      }

      // This call necessary to update the validators so that 
      // we don't get change notifications due to the edits we just made.
      // Uggggg
      TransferDataFromWindow();
   }

   indicateHaveLattice();
   wxCheckBox *show = (wxCheckBox*)FindWindow(ID_CHECKBOX_PBC_SHOWLATTICE);
   showLattice(lattice && show->IsChecked());
}


void PBC::enforceUniform(int x)
{
   wxCheckBox *cbox = (wxCheckBox*)FindWindow(ID_CHECKBOX_PBC_UNIFORM);
   if (cbox->IsChecked()) {
      ewxSpinCtrl *spin;
      spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_XREP);
      spin->SetValue(x);
      spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_YREP);
      spin->SetValue(x);
      spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_ZREP);
      spin->SetValue(x);
   }
}

/**
 * Update the enable/disable state of the replicate button.
 * spin->GetValue should not be called from a spin ctrl callback.
 * It causes bad decrementing for some reason.  Sooo, hack by passing in
 * -1 args for spins that should dynamically aqcuire the value.  Else
 * passed value (ie from the event object) can be used.
 * 
 * Unable to get the correct value from typing values in so I just am
 * tossing this capability for now.
 */
void PBC::updateReplicateBtn(int x, int y, int z)
{

   /*
   int na = x, nb = y, nc = z;
   ewxSpinCtrl *spin;
   ewxButton *btn;
   if (x == -1) {
      spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_XREP);
      na = spin->GetValue();
   }
   if (y == -1) {
      spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_YREP);
      nb = spin->GetValue();
   }
   if (z == -1) {
      spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_ZREP);
      nc = spin->GetValue();
   }
   bool oktoreplicate = na > 1 || nb > 1 || nc > 1;
   btn = (ewxButton*)FindWindow(ID_BUTTON_PBC_REPLICATE);
   btn->Enable(oktoreplicate);
   */

   return;
}

void PBC::OnChoicePbcTypeSelected( wxCommandEvent& event )
{
   ewxChoice *choice = (ewxChoice*)FindWindow(ID_CHOICE_PBC_TYPE);
   if (choice) {
      wxString value = choice->GetStringSelection();
      if (value == "Lattice Vectors") {
         p_latticePanel->GetSizer()->Show((size_t)2,false);
         p_latticePanel->GetSizer()->Show((size_t)1,true);
      } else if (value == "Lattice") {
         p_latticePanel->GetSizer()->Show((size_t)1,false);
         p_latticePanel->GetSizer()->Show((size_t)2,true);
      } else {
         cerr << "Choice Not implemented: " << value << endl;
      }
   }
   Layout();
   Fit();
   event.Skip();
}

/**
 * Turns on (or off) traversal of the lattice scene graph object.
 */
void PBC::showLattice(bool showit)
{
    SoSwitch *s = (SoSwitch*)getFW().getSceneGraph().findNode("latticeswitch");
    if (s) {
      s->whichChild.setValue(showit ? SO_SWITCH_ALL : SO_SWITCH_NONE);
    }
}

/**
 * User changed one of the a,b,c,alpha,beta,gamma...
 */
void PBC::unitCellChanged()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   if (frag) {
      LatticeDef *lattice = frag->getLattice();
      if (lattice) {
         ewxTextCtrl *txt;
         ewxCheckBox *lock;
         double a,b,c,alpha,beta,gamma;
         //TODO turn this into a command so it can be undone
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A);
         sscanf(txt->GetValue().c_str(), "%lf",&a);
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_B);
         sscanf(txt->GetValue().c_str(), "%lf",&b);
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_C);
         sscanf(txt->GetValue().c_str(), "%lf",&c);
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_ALPHA);
         sscanf(txt->GetValue().c_str(), "%lf",&alpha);
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_BETA);
         sscanf(txt->GetValue().c_str(), "%lf",&beta);
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_GAMMA);
         sscanf(txt->GetValue().c_str(), "%lf",&gamma);
         lock = (ewxCheckBox*)FindWindow(ID_CHECKBOX_PBC_LOCK);

         WxVizToolFW &fw = getFW();
         PBCCmd *cmd = new PBCEditLatticeCmd("Edit Lattice", 
                                              &fw.getSceneGraph(), p_baseFrag);
         cmd->getParameter("a")->setDouble(a);
         cmd->getParameter("b")->setDouble(b);
         cmd->getParameter("c")->setDouble(c);
         cmd->getParameter("alpha")->setDouble(alpha);
         cmd->getParameter("beta")->setDouble(beta);
         cmd->getParameter("gamma")->setDouble(gamma);
         cmd->getParameter("lock")->setBoolean(lock->IsChecked());
         execute(cmd);

      }
   }

}

void PBC::vectorChanged()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   if (frag) {
      LatticeDef *lattice = frag->getLattice();
      if (lattice) {
         MPoint a1, a2, a3;

         ewxTextCtrl *txt;
         ewxCheckBox *check;
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1X);
         a1.x(txt->getValueAsDouble());
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Y);
         a1.y(txt->getValueAsDouble());
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Z);
         a1.z(txt->getValueAsDouble());

         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2X);
         a2.x(txt->getValueAsDouble());
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Y);
         a2.y(txt->getValueAsDouble());
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Z);
         a2.z(txt->getValueAsDouble());

         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3X);
         a3.x(txt->getValueAsDouble());
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Y);
         a3.y(txt->getValueAsDouble());
         txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Z);
         a3.z(txt->getValueAsDouble());

         check = (ewxCheckBox*)FindWindow(ID_CHECKBOX_PBC_LOCK);
         bool lock = check->GetValue();

         WxVizToolFW &fw = getFW();
         PBCCmd *cmd = new PBCEditVectorsCmd("Edit Lattice Vectors", 
                                              &fw.getSceneGraph(), p_baseFrag);
         cmd->getParameter("a1x")->setDouble(a1.x());
         cmd->getParameter("a1y")->setDouble(a1.y());
         cmd->getParameter("a1z")->setDouble(a1.z());
         cmd->getParameter("a2x")->setDouble(a2.x());
         cmd->getParameter("a2y")->setDouble(a2.y());
         cmd->getParameter("a2z")->setDouble(a2.z());
         cmd->getParameter("a3x")->setDouble(a3.x());
         cmd->getParameter("a3y")->setDouble(a3.y());
         cmd->getParameter("a3z")->setDouble(a3.z());
         cmd->getParameter("lock")->setBoolean(lock);
         execute(cmd);

      }
   }
}

void PBC::replicate()
{
   int a,b,c;
   ewxSpinCtrl *spin;
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_XREP);
   a = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_YREP);
   b = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PBC_ZREP);
   c = spin->GetValue();
   replicate(a,b,c);
}

void PBC::replicate(int a, int b, int c)
{
   if (p_baseFrag == 0) return;

   // Want to stick with the same base frag
   p_noStashFlag = true;


   // Clear existing system and use basefrag?
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   frag->resizeToUnitCell();
   /* This is too dependent upon having a good base frag
   frag->clear();
   *frag = *p_baseFrag;
   frag->touchNumbers();
   */

   WxVizToolFW &fw = getFW();
   PBCCmd *cmd = new PBCReplicateCmd("Replicate", &fw.getSceneGraph(), p_baseFrag);

   ewxTextCtrl *txt;

   double x,y,z;
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1X);
   x = txt->getValueAsDouble();
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Y);
   y = txt->getValueAsDouble();
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A1Z);
   z = txt->getValueAsDouble();
   cmd->getParameter("x1")->setDouble(x);
   cmd->getParameter("y1")->setDouble(y);
   cmd->getParameter("z1")->setDouble(z);

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2X);
   x = txt->getValueAsDouble();
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Y);
   y = txt->getValueAsDouble();
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A2Z);
   z = txt->getValueAsDouble();
   cmd->getParameter("x2")->setDouble(x);
   cmd->getParameter("y2")->setDouble(y);
   cmd->getParameter("z2")->setDouble(z);

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3X);
   x = txt->getValueAsDouble();
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Y);
   y = txt->getValueAsDouble();
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PBC_A3Z);
   z = txt->getValueAsDouble();
   cmd->getParameter("x3")->setDouble(x);
   cmd->getParameter("y3")->setDouble(y);
   cmd->getParameter("z3")->setDouble(z);

   cmd->getParameter("axis1")->setInteger(a);
   cmd->getParameter("axis2")->setInteger(b);
   cmd->getParameter("axis3")->setInteger(c);

   fw.execute(cmd);

   // Don't want to stash a new frag but must update the replication
   frag = getFW().getSceneGraph().getFragment();
   if (frag && p_baseFrag) {
      int na, nb, nc;
      LatticeDef *lattice = frag->getLattice();
      lattice->getReplicationFactors(na, nb, nc);

      lattice = p_baseFrag->getLattice();
      lattice->setReplicationFactors(na, nb, nc);
   }

   p_noStashFlag = false;

}

void PBC::OnTextctrlPbcA1xEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcA1yEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcA1zEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}


void PBC::OnTextctrlPbcA2xEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}


void PBC::OnTextctrlPbcA2yEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcA2zEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcA3xEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcA3yEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcA3zEnter( wxCommandEvent& event )
{
   vectorChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcAEnter( wxCommandEvent& event )
{
   //setA??
   unitCellChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcAlphaEnter( wxCommandEvent& event )
{
   unitCellChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcBEnter( wxCommandEvent& event )
{
   unitCellChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcBetaEnter( wxCommandEvent& event )
{
   unitCellChanged();
   event.Skip();
}

void PBC::OnTextctrlPbcCEnter( wxCommandEvent& event )
{
   unitCellChanged();
   event.Skip();
}


void PBC::OnTextctrlPbcGammaEnter( wxCommandEvent& event )
{
   unitCellChanged();
   event.Skip();
}

void PBC::OnSpinctrlPbcXrepUpdated( wxSpinEvent& event )
{
   enforceUniform(event.GetPosition());
   updateReplicateBtn(event.GetPosition(),-1,-1);
   event.Skip();
}

void PBC::OnSpinctrlPbcXrepTextUpdated( wxCommandEvent& event )
{
   //updateReplicateBtn(event.GetInt(),-1,-1);
   //updateReplicateBtn(-1,-1,-1);
   event.Skip();
}

void PBC::OnSpinctrlPbcYrepUpdated( wxSpinEvent& event )
{
   enforceUniform(event.GetPosition());
   updateReplicateBtn(-1,event.GetPosition(),-1);
   event.Skip();
}

void PBC::OnSpinctrlPbcYrepTextUpdated( wxCommandEvent& event )
{
   //updateReplicateBtn(-1,-1,-1);
   event.Skip();
}

void PBC::OnSpinctrlPbcZrepUpdated( wxSpinEvent& event )
{
   enforceUniform(event.GetPosition());
   updateReplicateBtn(-1,-1,event.GetPosition());
   event.Skip();
}

void PBC::OnSpinctrlPbcZrepTextUpdated( wxCommandEvent& event )
{
   //updateReplicateBtn(-1,-1,-1);
   event.Skip();
}

void PBC::OnPbcGenerateClick( wxCommandEvent& event )
{
   wxComboCtrl *cc = (wxComboCtrl*)FindWindow(ID_COMBO_SPACE_GROUP);
   string sym = cc->GetValue().c_str();
   if (!PointGroup::isSpaceGroup(sym)) {
      ::wxLogWarning("You must specify a valid space group.");
   } else {

      SGFragment *frag = getFW().getSceneGraph().getFragment();
      string origsym = frag->pointGroup();
      frag->pointGroup(sym);
      try {
         //TODO good threshold
         //SymmetryOps::generateLatticeFragment(*frag, 0.1);
         //frag->touch();
         //getFW().getSceneGraph().touchChemDisplay();
         WxVizToolFW &fw = getFW();
         Command *cmd = new PBCGenerateFragCmd("Generate from space group", &fw.getSceneGraph(), p_baseFrag);
         //TODO good threshold
         cmd->getParameter("threshold")->setDouble(0.1);
         fw.execute(cmd);
      } catch (EcceException& ex) {
         frag->pointGroup(origsym); // restore
         ::wxLogWarning("Unable to process space group.");
      }
   }

}

void PBC::OnButtonPbcReplicateClick( wxCommandEvent& event )
{
   wxBusyCursor busy;
   replicate();

   event.Skip();
}

void PBC::OnCheckboxPbcShowlatticeClick( wxCommandEvent& event )
{
   showLattice(event.IsChecked());
   event.Skip();
}

void PBC::OnCheckboxPbcCenterClick( wxCommandEvent& event )
{
   center(event.IsChecked());

   event.Skip();
}

void PBC::center(bool docenter)
{
   // TODO this should be a command...
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   LatticeDef *lattice = frag->getLattice();
   lattice->setCentering(docenter);
   getFW().getSceneGraph().touchLattice();
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
}

void PBC::OnButtonPbcFoldClick( wxCommandEvent& event )
{
   WxVizToolFW &fw = getFW();
   PBCCmd *cmd = new PBCFoldCmd("Fold atoms into lattice", 
         &fw.getSceneGraph(), p_baseFrag);
   execute(cmd);

   event.Skip();
}

void PBC::OnButtonPbcSuperClick( wxCommandEvent& event )
{
   makeSuperLattice();
   event.Skip();
}

void PBC::makeSuperLattice()
{
   WxVizToolFW &fw = getFW();
   PBCCmd *cmd = new PBCSuperCmd("Make super lattice", 
         &fw.getSceneGraph(), p_baseFrag);
   execute(cmd);
}

void PBC::OnButtonPbcCreateClick( wxCommandEvent& event )
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   if (frag->numAtoms() == 0) {
      ::wxLogWarning("Import or build a fragment first.");
   } else {
      WxVizToolFW &fw = getFW();
      Command *cmd = new PBCCreateCmd("Create default lattice", 
            &getFW().getSceneGraph());
      fw.execute(cmd);
   }

   event.Skip();
}

void PBC::OnButtonPbcDeleteClick( wxCommandEvent& event )
{
   WxVizToolFW &fw = getFW();
   Command *cmd = new PBCDeleteCmd("Delete lattice", 
         &fw.getSceneGraph(), p_baseFrag);
   fw.execute(cmd);

   event.Skip();
}

void PBC::OnButtonPbcRestoreClick( wxCommandEvent& event )
{

   Command *cmd = restoreUnitCell();
   delete cmd;

   event.Skip();
}

void PBC::OnCheckboxPbcUniformClick( wxCommandEvent& event )
{
   // No action - handled as spin controls change
   event.Skip();
}

void PBC::OnButtonPbcEquivrectClick( wxCommandEvent& event )
{
   WxVizToolFW &fw = getFW();
   PBCCmd *cmd = new PBCEquivRectCmd("Create equivalent rectangle", 
         &fw.getSceneGraph(), p_baseFrag);
   execute(cmd);

   event.Skip();
}
