#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/gbsizer.h"
#include "wx/tglbtn.h"
#include "wx/fileconf.h"

#include "util/Event.H"
#include "util/EventDispatcher.H"
#include "util/StringConverter.H"

#include "tdat/TBond.H"

#include "viz/AtomMoveCmd.H"
#include "viz/CleanCmd.H"
#include "viz/DeleteCmd.H"
#include "viz/HydrogensCmd.H"
#include "viz/RmHydrogensCmd.H"

#include "wxgui/CleanParameters.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ImageDropDown.H"
#include "wxgui/PerTabPanel.H"
#include "wxgui/TearableContent.H"

#include "wxviz/ViewerEvtHandler.H"

#include "MiniPerTab.H"

IMPLEMENT_DYNAMIC_CLASS( MiniPerTab, ewxPanel )

/*!
 * MiniPerTab event table definition
 */
BEGIN_EVENT_TABLE( MiniPerTab, ewxPanel )
  EVT_TOGGLEBUTTON(ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(1+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(2+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(3+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(4+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(5+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(6+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(7+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(8+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(9+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(10+ID_ELT_BASE, MiniPerTab::OnElementClick)
  EVT_TOGGLEBUTTON(11+ID_ELT_BASE, MiniPerTab::OnElementClick)
  // EVT_BUTTON(ViewerEvtHandler::ID_ADD_H, MiniPerTab::OnAddhClick)
  // EVT_BUTTON(ViewerEvtHandler::ID_DEL_H, MiniPerTab::OnRemovehClick)
  // EVT_BUTTON(ViewerEvtHandler::ID_CLEAN_COORD, MiniPerTab::OnCleanClick)
  EVT_BUTTON(wxID_REMOVE, MiniPerTab::OnRemoveClick)
  EVT_BUTTON(ID_SHOW_TABLE, MiniPerTab::OnMoreClick)
  EVT_COMBOBOX(ID_BOND_SEL, MiniPerTab::OnBondSel)
  EVT_COMBOBOX(ID_SHAPE_SEL, MiniPerTab::OnShapeSel)
  //  EVT_CHILD_FOCUS(MiniPerTab::OnChildFocus)
END_EVENT_TABLE()

MiniPerTab::MiniPerTab( ) : Listener("MiniPerTab")
{
  p_sizer = 0;
  p_gridSizer = 0;
}


MiniPerTab::MiniPerTab( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                        const wxSize& size, long style )
  : Listener("MiniPerTab")
{
  subscribe();

  p_elements[0] = "H";
  p_elements[1] = "C";
  p_elements[2] = "N";
  p_elements[3] = "O";

  p_elements[4] = "F";
  p_elements[5] = "Si";
  p_elements[6] = "P";
  p_elements[7] = "S";

  p_elements[8] = "Cl";
  p_elements[9] = "K";
  p_elements[10] = "Ca";
  p_elements[11] = "Mg";

  p_replaceIndex = 8;

  Create(parent, id, pos, size, style);
}


MiniPerTab::~MiniPerTab()
{
  unsubscribe();
}


/**
 * MiniPerTab creator
 */
bool MiniPerTab::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                         const wxSize& size, long style )
{

  ewxPanel::Create(parent, id, pos, size, style);

  CreateControls();
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
  Centre();
  return true;
}


/**
 * Control creation for MiniPerTab
 */
void MiniPerTab::CreateControls()
{
  p_sizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(p_sizer);

  p_gridSizer = new wxGridBagSizer(3, 3);
  p_sizer->Add(p_gridSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 2);

  wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

  ewxButton * btn = new ewxButton(this, ViewerEvtHandler::ID_ADD_H,
                                  "Add H", wxDefaultPosition, wxSize(48, 24));
  hSizer->Add(btn, 0, wxCENTER|wxALL, 2);
  Connect(ViewerEvtHandler::ID_ADD_H, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(MiniPerTab::OnAddhClick));

  btn = new ewxButton(this, ViewerEvtHandler::ID_DEL_H,
                      "Del H", wxDefaultPosition, wxSize(48, 24));
  hSizer->Add(btn, 0, wxCENTER|wxALL, 2);
  Connect(ViewerEvtHandler::ID_DEL_H, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(MiniPerTab::OnRemovehClick));

  ewxBitmapButton *clean = new ewxBitmapButton(this,
          ViewerEvtHandler::ID_CLEAN_COORD,
          ewxBitmap("clean.png", wxBITMAP_TYPE_PNG));
  hSizer->Add(clean, 0, wxCENTER|wxALL, 2);
  Connect(ViewerEvtHandler::ID_CLEAN_COORD, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(MiniPerTab::OnCleanClick));

  wxArrayString bondLabels;
  bondLabels.Add( wxT(TBond::orderToString(TBond::Single) ));
  bondLabels.Add( wxT(TBond::orderToString(TBond::Double) ));
  bondLabels.Add( wxT(TBond::orderToString(TBond::Triple) ));
  bondLabels.Add( wxT(TBond::orderToString(TBond::Aromatic) ));
  bondLabels.Add( wxT(TBond::orderToString(TBond::Metal) ));

  wxArrayString bondImages;
  bondImages.Add( wxT(TBond::orderToImage(TBond::Single)) );
  bondImages.Add( wxT(TBond::orderToImage(TBond::Double)) );
  bondImages.Add( wxT(TBond::orderToImage(TBond::Triple)) );
  bondImages.Add( wxT(TBond::orderToImage(TBond::Aromatic)) );
  bondImages.Add( wxT(TBond::orderToImage(TBond::Metal)) );

  p_bonds = new ImageDropDown(this, ID_BOND_SEL, wxEmptyString,
                              wxDefaultPosition, wxSize(100, 28),
                              bondLabels, bondImages,
                              wxSize(60, 16),
                              wxCB_READONLY);//|wxODCB_DCLICK_CYCLES); //|wxNO_BORDER);
  p_bonds->SetSelection(0);

  hSizer->Add(p_bonds, 0, wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxBOTTOM, 2);

  wxArrayString shapeLabels;
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::lone)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::terminal)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::linear)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::bent)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::trigonalPlanar)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::pyramidal)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::tetrahedral)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::squarePlanar)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::trigonalBipyramidal)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::squarePyramidal)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::octahedral)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::cubic)) );
  shapeLabels.Add( wxT(ShapeData::shapeToString(ShapeData::decahedral)) );

  wxArrayString shapeImages;
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::lone)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::terminal)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::linear)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::bent)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::trigonalPlanar)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::pyramidal)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::tetrahedral)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::squarePlanar)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::trigonalBipyramidal)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::squarePyramidal)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::octahedral)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::cubic)) );
  shapeImages.Add( wxT(ShapeData::shapeToImage(ShapeData::decahedral)) );

  p_shapes = new ImageDropDown(this, ID_SHAPE_SEL, wxEmptyString,
                               wxDefaultPosition, wxSize(60, 32),
                               shapeLabels, shapeImages,
                               wxSize(24, 24),
                               wxCB_READONLY);//|wxODCB_DCLICK_CYCLES);
  p_shapes->SetSelection(6);

  hSizer->Add(p_shapes, 0, wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxBOTTOM, 2);

  p_sizer->Add(hSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 2);
  
  setElements();
  //  p_sizer->AddSpacer(10);
}


void MiniPerTab::restoreSettings(wxConfig * config)
{
  for (int i=8; i<12; ++i) {
    wxString elt;
    config->Read("/MiniPerTab/"+StringConverter::toString(i), &elt,
                 p_elements[i].c_str());
    p_elements[i] = elt.c_str();
  }
  setElements();
}


void MiniPerTab::saveSettings(wxConfig * config)
{
  for (int i=8; i<12; ++i) {
    config->Write("/MiniPerTab/"+StringConverter::toString(i), p_elements[i]);
  }
}


void MiniPerTab::setElements()
{
  p_gridSizer->Clear(true);

  TPerTab pertab;
  Preferences prefs("PerTable");
  string color;
  
  wxWindow *btn;
  int i, j, index=0;
  for (i=0; i<9; ++i) {
    for (j=0; j<2; ++j) {
      if (i == 4 || i == 7) {
        p_gridSizer->Add(5, 5, wxGBPosition(j,i));
      }
      else if (i<7) {
        p_eltBtns[index] = new wxToggleButton(this, index+ID_ELT_BASE,
                                              p_elements[index],
                                              wxDefaultPosition,
                                              wxSize(28, 24));
        ewxColor col;
        if (prefs.getString(p_elements[index]+".Color", color))
          col = ewxColor(color);
        else
          col = ewxColor(pertab.color(pertab.atomicNumber(p_elements[index])));
        p_eltBtns[index]->SetBackgroundColour(col);
        p_eltBtns[index]->SetFont(ewxStyledWindow::getBoldFont());
        p_gridSizer->Add(p_eltBtns[index], wxGBPosition(j,i));
        ++index;
      }
      else if (i == 8) {
        if (j == 0)
          btn = new ewxButton(this, wxID_REMOVE, "Delete", wxDefaultPosition,
                              wxDefaultSize, wxBU_EXACTFIT);
        else
          btn = new ewxButton(this, ID_SHOW_TABLE, "More...",wxDefaultPosition,
                              wxDefaultSize, wxBU_EXACTFIT);
        p_gridSizer->Add(btn, wxGBPosition(j,i));
      }
    }
  }
  p_gridSizer->AddGrowableCol(7);

  // Check the default Carbon button
  p_eltBtns[1]->SetValue(true);
  Layout();
}


void MiniPerTab::eventMCB(const Event& event)
{
   string name = event.getName();
   string value = event.getValue();

   if (name == "ElementChanged") {
     onElementChanged(value);
   }
   else if (name == "BondChanged") {
     p_bonds->SetStringSelection(value);
     p_bonds->SetFocus();
   }
   else if (name == "ShapeChanged") {
     p_shapes->SetStringSelection(value);
     p_shapes->SetFocus();
   }
}


void MiniPerTab::OnAddhClick(wxCommandEvent &event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new HydrogensCmd("Add Hydrogens", &sg);
  fw.execute(cmd);
}


void MiniPerTab::OnRemovehClick(wxCommandEvent &event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new RmHydrogensCmd("Remove Hydrogens", &sg);
  fw.execute(cmd);
}


void MiniPerTab::OnCleanClick(wxCommandEvent &event)
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   SGFragment *frag = sg.getFragment();

   string msg = "To restrict the clean,\nselect atoms before invoking clean";
   // The above message makes the dialog soooo wide so forget it
   msg = "";
   ewxProgressDialog *dlg = 0;
   
   // Try to avoid flashing a dialog
   if (frag->numAtoms() > 10) {
      dlg = new ewxProgressDialog ("ECCE Force Field Clean",msg.c_str(),1000,0, wxPD_AUTO_HIDE|wxPD_CAN_ABORT|wxPD_ELAPSED_TIME|wxPD_SMOOTH);
      dlg->Show();
   }

   Command *cmd = new CleanCmd("Force Field Clean", &sg, dlg);
   CleanParameters *params = new CleanParameters(this);
   cmd->getParameter("iterations")->setInteger(params->getMaxIterations());
   cmd->getParameter("nonBondCutoff")->setDouble(params->getForceCutoff());
   cmd->getParameter("convergence")->setDouble(params->getConvergence());

   // This could result in a CancelException
   fw.execute(cmd);

   if (dlg) dlg->Destroy();
}


void MiniPerTab::OnRemoveClick(wxCommandEvent &event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new DeleteCmd("Delete Selection", &sg);
  fw.execute(cmd);
}


void MiniPerTab::OnMoreClick(wxCommandEvent &event)
{
  PerTabPanel *pertab = new PerTabPanel(this, false,
          ID_ITEM_DEFAULT, false, true, true);
  TearableContent *tc = new TearableContent(pertab);
  pertab->SetName("Elements");
  int w, h;
  pertab->GetSize(&w, &h);
  tc->Position(wxGetMousePosition(), wxSize(-w, 1));
  tc->Popup();
  event.Skip();
}


void MiniPerTab::OnBondSel(wxCommandEvent &event)
{
  string bond = p_bonds->GetStringSelection().c_str();
  EventDispatcher::getDispatcher().publish(Event("BondChanged", bond));
  event.Skip();
}


void MiniPerTab::OnShapeSel(wxCommandEvent &event)
{
  string shape = p_shapes->GetStringSelection().c_str();
  EventDispatcher::getDispatcher().publish(Event("ShapeChanged", shape));
  event.Skip();
}


/*
void MiniPerTab::OnChildFocus(wxChildFocusEvent &event)
{
  if (event.GetWindow()->GetId() == ID_BOND_SEL) {
    string bond = p_bonds->GetStringSelection().c_str();
    EventDispatcher::getDispatcher().publish(Event("BondChanged", bond));
  }
  else if (event.GetWindow()->GetId() == ID_SHAPE_SEL) {
    string shape = p_shapes->GetStringSelection().c_str();
    EventDispatcher::getDispatcher().publish(Event("ShapeChanged", shape));
  }
  event.Skip();
}
*/


void MiniPerTab::OnElementClick(wxCommandEvent &event)
{
  // Let other listeners know selection changed
  Event evt("ElementChanged", p_elements[event.GetId()-ID_ELT_BASE]);
  EventDispatcher::getDispatcher().publish(evt);
  
  event.Skip();
}


void MiniPerTab::onElementChanged(string elt)
{
  bool found = false;
  int i=0;
  for (; i<NUM_ELT_BTN; i++) {
    if (p_elements[i] == elt) {
      found = true;
      p_eltBtns[i]->SetValue(true);
      p_eltBtns[i]->SetFocus();
    }
    else
      p_eltBtns[i]->SetValue(false);
  }

  if (!found && elt != "") {
    p_elements[p_replaceIndex] = elt;
    p_eltBtns[p_replaceIndex]->SetLabel(elt);
    TPerTab pertab;
    Color col = pertab.color(pertab.atomicNumber(elt));
    p_eltBtns[p_replaceIndex]->SetBackgroundColour(ewxColor(col));
    p_eltBtns[p_replaceIndex]->SetValue(true);
    p_eltBtns[p_replaceIndex]->SetFocus();
    ++p_replaceIndex;
    if (p_replaceIndex == NUM_ELT_BTN)
      p_replaceIndex = 8;
  }
}
