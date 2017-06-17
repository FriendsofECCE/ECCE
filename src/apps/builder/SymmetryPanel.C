
#include <iostream>
  using std::cout;
  using std::endl;

/**
 * Panel that implements translations of the system
 */
#include "wx/combo.h"
#include "wx/listbase.h"

#include "tdat/PointGroup.H"

#include "viz/AssignSymmetryCmd.H"
#include "viz/CleanSymmetryCmd.H"
#include "viz/FindIrreduceFragCmd.H"
#include "viz/FindSymmetryCmd.H"
#include "viz/GenerateFragCmd.H"
#include "viz/GenerateGhostsCmd.H"
#include "viz/RemoveGhostsCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxListViewComboPopup.H"
#include "wxgui/ewxNumericValidator.H"

#include "SymmetryPanel.H"


/**
 * SymmetryPanel default constructor
 */
SymmetryPanel::SymmetryPanel( )
{
  initializeGUI();
}



/**
 * SymmetryPanel constructor
 */
SymmetryPanel::SymmetryPanel( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style )
{
  Create(parent, id, pos, size, style);
  subscribe();
  initializeGUI();
}



/**
 * SymmetryPanel destructor
 */
SymmetryPanel::~SymmetryPanel()
{
  unsubscribe();
}


/**
 * SymmetryPanel eventMCB for Listener interface
 */
void SymmetryPanel::eventMCB(const Event& event)
{
  string name = event.getName();

  if (name == "GeomChange") {
    SGFragment *frag = getFW().getSceneGraph().getFragment();
    wxComboCtrl *cc = (wxComboCtrl*)FindWindow(ID_COMBO_SYMMETRY_GROUP);
    cc->SetValue(frag->pointGroup());
  }
}


/**
 * If C1, many of the buttons are disabled.
 */
void SymmetryPanel::updateUI(bool isC1)
{
   ewxButton *btn = 0;

   btn = (ewxButton*)FindWindow(ID_BUTTON_SYMM_CLEAN);
   btn->Enable(!isC1);
   btn = (ewxButton*)FindWindow(ID_BUTTON_SYMM_FIND_IRREDUCIBLE);
   btn->Enable(!isC1);
   btn = (ewxButton*)FindWindow(ID_BUTTON_SYMM_GENERATE_MOLECULE);
   btn->Enable(!isC1);

}



/**
 *
 */
void SymmetryPanel::initializeGUI()
{
   // Create a validator for the threshold field
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SYMM_THRESHOLD);
   txt->SetValidator(ewxNumericValidator(0.0, 1.0, .01));

   wxComboCtrl * cc = new wxComboCtrl(this, ID_COMBO_SYMMETRY_GROUP,
         wxEmptyString,
         wxDefaultPosition, wxSize(60, 22),
         wxCC_STD_BUTTON|wxTE_PROCESS_ENTER);
   //NOTE the event here must match the event we send in ewxListViewComboPopup
   //The event of interest has not been settled.
   Connect(ID_COMBO_SYMMETRY_GROUP,
               //wxEVT_COMMAND_LIST_END_LABEL_EDIT,
                wxEVT_COMMAND_TEXT_ENTER,
               //wxEVT_COMMAND_LIST_ITEM_SELECTED,
               (wxObjectEventFunction)
               (wxEventFunction)&SymmetryPanel::OnListChange,0,this);

   // Make sure we use popup that allows focusing the listview.
   cc->UseAltPopupWindow();
   cc->SetPopupMinWidth(330);
   cc->SetPopupMaxHeight(140);

   cc->GetTextCtrl()->SetBackgroundColour(ewxStyledWindow::getInputColor());
   cc->GetTextCtrl()->SetFont(ewxStyledWindow::getBoldFont());

   ewxListViewComboPopup* iface =
     new ewxListViewComboPopup(ID_COMBO_SYMMETRY_GROUP);

   cc->SetPopupControl(iface);

   vector<string> selections = PointGroup::getGroups();

   for (size_t i=0; i<selections.size(); i++ )
      iface->AddSelection(selections[i]);


   p_topSizer->Insert( 1, cc, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );

   // @todo have to have framework set to do this
   //cc->SetValue(getFW().getSceneGraph().getFragment()->pointGroup());
   cc->SetValue("C1");
   updateUI(cc->GetValue() == "C1");

}



////////////////////////////////////////////////////////////////////////////
// EVENT HANDLERS //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

void SymmetryPanel::OnButtonSymmFindClick( wxCommandEvent& event )
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SYMM_THRESHOLD);
   double tbd = txt->getValueAsDouble();

   WxVizToolFW &fw = getFW();

   Command *cmd = new FindSymmetryCmd("Find Symmetry", &fw.getSceneGraph());
   cmd->getParameter("threshold")->setDouble(tbd);
   fw.execute(cmd);

   wxComboCtrl *cc = (wxComboCtrl*)FindWindow(ID_COMBO_SYMMETRY_GROUP);

   SGFragment *frag = fw.getSceneGraph().getFragment();
   cc->SetValue(frag->pointGroup());

   updateUI(cc->GetValue() == "C1");
   event.Skip();

}


/**
 * @todo connect this method to something
 */
void SymmetryPanel::OnTextctrlSymmAssignEnter( wxCommandEvent& event )
{
   assignSymmetry();

   event.Skip();

}

void SymmetryPanel::OnButtonSymmGenerateGhostsClick( wxCommandEvent& event )
{
   WxVizToolFW &fw = getFW();

   Command *cmd = new GenerateGhostsCmd("Generate Ghosts", &fw.getSceneGraph());
   fw.execute(cmd);

   event.Skip();

}



void SymmetryPanel::OnButtonDelGhostsClick( wxCommandEvent& event )
{
   WxVizToolFW &fw = getFW();

   Command *cmd = new RemoveGhostsCmd("Delete Ghosts", &fw.getSceneGraph());
   fw.execute(cmd);

   event.Skip();

}

void SymmetryPanel::OnButtonSymmFindIrreducibleClick( wxCommandEvent& event )
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SYMM_THRESHOLD);
   double tbd = txt->getValueAsDouble();

   WxVizToolFW &fw = getFW();

   Command *cmd = new FindIrreduceFragCmd("Find Irreducible Fragment", &fw.getSceneGraph());
   cmd->getParameter("threshold")->setDouble(tbd);
   fw.execute(cmd);

   event.Skip();

}

void SymmetryPanel::OnButtonSymmGenerateMoleculeClick( wxCommandEvent& event )
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SYMM_THRESHOLD);
   double tbd = txt->getValueAsDouble();

   WxVizToolFW &fw = getFW();

   Command *cmd = new GenerateFragCmd("Generate Fragment", &fw.getSceneGraph());
   cmd->getParameter("threshold")->setDouble(tbd);
   fw.execute(cmd);

   event.Skip();

}

void SymmetryPanel::OnButtonSymmCleanClick( wxCommandEvent& event )
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SYMM_THRESHOLD);
   double tbd = txt->getValueAsDouble();

   WxVizToolFW &fw = getFW();

   Command *cmd = new CleanSymmetryCmd("Symmetry Clean", &fw.getSceneGraph());
   cmd->getParameter("threshold")->setDouble(tbd);
   fw.execute(cmd);

   event.Skip();
}



/**
 * This method is intended to be activated if the value of the combo box
 * changes from drop down or type-in.
 */
void SymmetryPanel::OnListChange(wxListEvent& event)
{
   //wxComboCtrl *cc = (wxComboCtrl*)FindWindow(ID_COMBO_SYMMETRY_GROUP);
   assignSymmetry();
}

void SymmetryPanel::assignSymmetry()
{
   wxComboCtrl *cc = (wxComboCtrl*)FindWindow(ID_COMBO_SYMMETRY_GROUP);
   string group = cc->GetValue().c_str();

   WxVizToolFW &fw = getFW();

   Command *cmd = new AssignSymmetryCmd("Assign Symmetry", &fw.getSceneGraph());
   cmd->getParameter("group")->setString(group);
   fw.execute(cmd);
   updateUI(cc->GetValue() == "C1");
}

////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

wxControl * SymmetryPanel::getControl(int id)
{
  return dynamic_cast<wxControl*>(FindWindow(id));
}


wxTextCtrl * SymmetryPanel::getTextCtrl(int id)
{
  return dynamic_cast<wxTextCtrl*>(FindWindow(id));
}

