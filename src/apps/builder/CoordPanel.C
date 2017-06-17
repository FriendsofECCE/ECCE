/**
 * @file CoordPanel.C
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CoordPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;

#include "viz/AlignCmd.H"
#include "viz/CenterCmd.H"
#include "viz/LatticeBuildCmd.H"
#include "viz/RotateAboutLineCmd.H"
#include "viz/TranslateCmd.H"

#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxTextCtrl.H"

#include "CoordPanel.H"

#include <iostream>

IMPLEMENT_DYNAMIC_CLASS( CoordPanel, CoordPanelGUI )


/*!
 * CoordPanel constructors
 */
CoordPanel::CoordPanel( )
  : CoordPanelGUI(),
    WxVizTool()
{
  initializeGUI();
}


CoordPanel::CoordPanel( wxWindow* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size, long style )
  : CoordPanelGUI(),
    WxVizTool()
{
  Create(parent, id, pos, size, style);
  initializeGUI();
}


bool CoordPanel::Create( wxWindow* parent, wxWindowID id,
                         const wxPoint& pos, const wxSize& size, long style )
{
  if (!CoordPanelGUI::Create(parent, id, pos, size, style)) {
    wxFAIL_MSG( wxT("CoordPanel creation failed") );
    return false;
  }

  return true;
}


/**
 * Set validators on input fields
 */
void CoordPanel::initializeGUI()
{
  ewxTextCtrl *text;
  ewxNumericValidator validator(0.,1.,0.);

  // Translate panel
  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_BY_X));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_BY_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_BY_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_FROM_X));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_TO_X));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_TO_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TRANS_TO_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  // Lattice panel
  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM1_X));
  validator.setHardRange("(,)");
  validator.setValue(1.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM1_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM1_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM2_X));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM2_Y));
  validator.setHardRange("(,)");
  validator.setValue(1.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM2_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM3_X));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM3_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM3_Z));
  validator.setHardRange("(,)");
  validator.setValue(1.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ROTATE_START_X));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ROTATE_START_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ROTATE_START_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ROTATE_END_X));
  validator.setHardRange("(,)");
  validator.setValue(1.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ROTATE_END_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ROTATE_END_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ROTATE_DEGREE));
  validator.setHardRange("(,)");
  validator.setValue(90.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ALIGN_NX));
  validator.setHardRange("(,)");
  validator.setValue(1.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ALIGN_NY));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_ALIGN_NZ));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIO_CENTER_ORIGIN
 */
void CoordPanel::OnCenterSelected( wxCommandEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALIGN_TO_AXIS
 */
void CoordPanel::OnAlignToAxisSelected( wxCommandEvent& event )
{
  ((ewxStaticText*)FindWindow(ID_STATIC_PLAX))->SetLabel("Axis   ");

  ((ewxButton*)FindWindow(ID_BUTTON_ALIGN_X_YZ))->SetLabel("  X Axis   ");
  ((ewxButton*)FindWindow(ID_BUTTON_ALIGN_Y_ZX))->SetLabel("  Y Axis   ");
  ((ewxButton*)FindWindow(ID_BUTTON_ALIGN_Z_XY))->SetLabel("  Z Axis   ");

  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALIGN_TO_PLANE
 */
void CoordPanel::OnAlignToPlaneSelected( wxCommandEvent& event )
{
  ((ewxStaticText*)FindWindow(ID_STATIC_PLAX))->SetLabel("Normal");

  ((ewxButton*)FindWindow(ID_BUTTON_ALIGN_X_YZ))->SetLabel("YZ Plane");
  ((ewxButton*)FindWindow(ID_BUTTON_ALIGN_Y_ZX))->SetLabel("ZX Plane");
  ((ewxButton*)FindWindow(ID_BUTTON_ALIGN_Z_XY))->SetLabel("XY Plane");

  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_X_YZ
 */
void CoordPanel::OnButtonAlignX_YZClick( wxCommandEvent& event )
{
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NX))->setValueAsDouble(1.0);
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NY))->setValueAsDouble(0.0);
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NZ))->setValueAsDouble(0.0);

  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Y_ZX
 */
void CoordPanel::OnButtonAlignY_ZXClick( wxCommandEvent& event )
{
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NX))->setValueAsDouble(0.0);
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NY))->setValueAsDouble(1.0);
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NZ))->setValueAsDouble(0.0);

  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Z_XY
 */
void CoordPanel::OnButtonAlignZ_XYClick( wxCommandEvent& event )
{
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NX))->setValueAsDouble(0.0);
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NY))->setValueAsDouble(0.0);
  ((ewxTextCtrl*)FindWindow(ID_ALIGN_NZ))->setValueAsDouble(1.0);

  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CENTER
 */
void CoordPanel::OnCenterClick( wxCommandEvent& event )
{
  ewxRadioButton *radio0 = (ewxRadioButton*)FindWindow(ID_RADIO_CENTER_ORIGIN);
  ewxRadioButton *radio1 = (ewxRadioButton*)FindWindow(ID_RADIO_CENTER_X);
  ewxRadioButton *radio2 = (ewxRadioButton*)FindWindow(ID_RADIO_CENTER_Y);
  ewxRadioButton *radio3 = (ewxRadioButton*)FindWindow(ID_RADIO_CENTER_Z);

  WxVizToolFW &fw = getFW();

  Command *cmd = new CenterCmd("Center", &fw.getSceneGraph());
  if (radio0->GetValue()) {
    cmd->getParameter("axis")->setString("");
  } else if (radio1->GetValue()) {
    cmd->getParameter("axis")->setString("x");
  } else if (radio2->GetValue()) {
    cmd->getParameter("axis")->setString("y");
  } else if (radio3->GetValue()) {
    cmd->getParameter("axis")->setString("z");
  }

  ewxChoice *choice = (ewxChoice*)FindWindow(ID_CENTER_APPLY_TO);
  if (choice->GetStringSelection() == "All") {
    cmd->getParameter("apply")->setString("all");
  } else {
    cmd->getParameter("apply")->setString("selection");
  }

  fw.execute(cmd);

  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_BY_XYZ
 */
void CoordPanel::OnTransByXEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_X);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_X);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_X);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rto = rfrom + rby;
  text1->setValueAsDouble(rto);
  event.Skip();
}

void CoordPanel::OnTransByYEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Y);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Y);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Y);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rto = rfrom + rby;
  text1->setValueAsDouble(rto);
  event.Skip();
}

void CoordPanel::OnTransByZEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Z);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Z);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Z);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rto = rfrom + rby;
  text1->setValueAsDouble(rto);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_TRANS_EXPAND
 */
void CoordPanel::OnTransExpandClick( wxCommandEvent& event )
{
  for (size_t i=10; i<20; ++i)
    p_translateSizer->Show(i, event.IsChecked());
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_FROM_XYZ
 */
void CoordPanel::OnTransFromXEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_X);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_X);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_X);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rby = rto - rfrom;
  text3->setValueAsDouble(rby);
  event.Skip();
}

void CoordPanel::OnTransFromYEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Y);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Y);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Y);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rby = rto - rfrom;
  text3->setValueAsDouble(rby);
  event.Skip();
}

void CoordPanel::OnTransFromZEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Z);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Z);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Z);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rby = rto - rfrom;
  text3->setValueAsDouble(rby);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_FROM_PICK
 */
void CoordPanel::OnTransFromPickClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  SGFragment *frag = fw.getSceneGraph().getFragment();

  double from_x, from_y, from_z;
  double to_x, to_y, to_z, x, y, z;

  if (frag->m_atomHighLight.size() > 0) {
    int idx = frag->m_atomHighLight[0];
    TAtm *atm = frag->atomRef(idx);
    const double *coords = atm->coordinates();
    ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_X);
    text->setValueAsDouble(coords[0]);
    from_x = coords[0];
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Y);
    text->setValueAsDouble(coords[1]);
    from_y = coords[1];
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Z);
    text->setValueAsDouble(coords[2]);
    from_z = coords[2];

    text = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_X);
    to_x = text->getValueAsDouble();
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Y);
    to_y = text->getValueAsDouble();
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Z);
    to_z = text->getValueAsDouble();

    x = to_x - from_x;
    y = to_y - from_y;
    z = to_z - from_z;
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_X);
    text->setValueAsDouble(x);
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Y);
    text->setValueAsDouble(y);
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Z);
    text->setValueAsDouble(z);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_TO_XYZ
 */
void CoordPanel::OnTransToXEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_X);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_X);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_X);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rby = rto - rfrom;
  text3->setValueAsDouble(rby);
  event.Skip();
}

void CoordPanel::OnTransToYEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Y);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Y);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Y);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rby = rto - rfrom;
  text3->setValueAsDouble(rby);
  event.Skip();
}

void CoordPanel::OnTransToZEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Z);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Z);
  ewxTextCtrl *text3 = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Z);
  double rto = text1->getValueAsDouble();
  double rfrom = text2->getValueAsDouble();
  double rby = text3->getValueAsDouble();
  rby = rto - rfrom;
  text3->setValueAsDouble(rby);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_TO_PICK
 */
void CoordPanel::OnTransToPickClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  SGFragment *frag = fw.getSceneGraph().getFragment();

  double from_x, from_y, from_z;
  double to_x, to_y, to_z, x, y, z;

  if (frag->m_atomHighLight.size() > 0) {
    int idx = frag->m_atomHighLight[0];
    TAtm *atm = frag->atomRef(idx);
    const double *coords = atm->coordinates();
    ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_X);
    text->setValueAsDouble(coords[0]);
    to_x = coords[0];
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Y);
    text->setValueAsDouble(coords[1]);
    to_y = coords[1];
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_TO_Z);
    text->setValueAsDouble(coords[2]);
    to_z = coords[2];

    text = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_X);
    from_x = text->getValueAsDouble();
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Y);
    from_y = text->getValueAsDouble();
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_FROM_Z);
    from_z = text->getValueAsDouble();

    x = to_x - from_x;
    y = to_y - from_y;
    z = to_z - from_z;
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_X);
    text->setValueAsDouble(x);
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Y);
    text->setValueAsDouble(y);
    text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Z);
    text->setValueAsDouble(z);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANSLATE
 */
void CoordPanel::OnTranslateClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();

  Command *cmd = new TranslateCmd("Translate", &fw.getSceneGraph());
  double x, y, z;

  ewxTextCtrl *text;
  text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_X);
  x = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Y);
  y = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_TRANS_BY_Z);
  z = text->getValueAsDouble();

  ewxChoice *choice = (ewxChoice*)FindWindow(ID_TRANS_APPLY_TO);
  if (choice->GetStringSelection() == "All") {
    cmd->getParameter("apply")->setString("all");
  } else {
    cmd->getParameter("apply")->setString("selection");
  }

  cmd->getParameter("x")->setDouble(x);
  cmd->getParameter("y")->setDouble(y);
  cmd->getParameter("z")->setDouble(z);

  fw.execute(cmd);
  event.Skip();
}

void CoordPanel::OnDuplicateDim1XEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim1YEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim1ZEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim2XEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim2YEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim2ZEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim3XEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim3YEnter( wxCommandEvent& event )
{
  event.Skip();
}

void CoordPanel::OnDuplicateDim3ZEnter( wxCommandEvent& event )
{
  event.Skip();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_START_X
 */
void CoordPanel::OnRotateStartUpdated( wxCommandEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_START_PICK
 */
void CoordPanel::OnRotateStartPickClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  SGFragment *frag = fw.getSceneGraph().getFragment();

  if (frag->m_atomHighLight.size() > 0) {
    int idx = frag->m_atomHighLight[0];
    TAtm *atm = frag->atomRef(idx);
    const double *coords = atm->coordinates();
    ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_ROTATE_START_X);
    text->setValueAsDouble(coords[0]);
    text = (ewxTextCtrl*)FindWindow(ID_ROTATE_START_Y);
    text->setValueAsDouble(coords[1]);
    text = (ewxTextCtrl*)FindWindow(ID_ROTATE_START_Z);
    text->setValueAsDouble(coords[2]);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_END_X
 */
void CoordPanel::OnRotateEndUpdated( wxCommandEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_END_PICK
 */
void CoordPanel::OnRotateEndPickClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  SGFragment *frag = fw.getSceneGraph().getFragment();

  if (frag->m_atomHighLight.size() > 0) {
    int idx = frag->m_atomHighLight[0];
    TAtm *atm = frag->atomRef(idx);
    const double *coords = atm->coordinates();
    ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_ROTATE_END_X);
    text->setValueAsDouble(coords[0]);
    text = (ewxTextCtrl*)FindWindow(ID_ROTATE_END_Y);
    text->setValueAsDouble(coords[1]);
    text = (ewxTextCtrl*)FindWindow(ID_ROTATE_END_Z);
    text->setValueAsDouble(coords[2]);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE
 */
void CoordPanel::OnRotateClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();

  Command *cmd = new RotateAboutLineCmd("Rotate", &fw.getSceneGraph());
  double x1, y1, z1, x2, y2, z2, angle;

  ewxTextCtrl *text;
  text = (ewxTextCtrl*)FindWindow(ID_ROTATE_START_X);
  x1 = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_ROTATE_START_Y);
  y1 = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_ROTATE_START_Z);
  z1 = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_ROTATE_END_X);
  x2 = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_ROTATE_END_Y);
  y2 = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_ROTATE_END_Z);
  z2 = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_ROTATE_DEGREE);
  angle = text->getValueAsDouble();

  cmd->getParameter("x1")->setDouble(x1);
  cmd->getParameter("y1")->setDouble(y1);
  cmd->getParameter("z1")->setDouble(z1);
  cmd->getParameter("x2")->setDouble(x2);
  cmd->getParameter("y2")->setDouble(y2);
  cmd->getParameter("z2")->setDouble(z2);
  cmd->getParameter("angle")->setDouble(angle);
  ewxChoice *choice = (ewxChoice*)FindWindow(ID_ROT_APPLY_TO);
  if (choice->GetStringSelection() == "All") {
    cmd->getParameter("apply")->setString("all");
  } else {
    cmd->getParameter("apply")->setString("selection");
  }

  fw.execute(cmd);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_X
 */

void CoordPanel::OnAlignToSelected( wxCommandEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ALIGN
 */
void CoordPanel::OnAlignClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();

  Command *cmd = new AlignCmd("Align", &fw.getSceneGraph());
  double x, y, z;
  bool toPlane = true;

  if (((ewxRadioButton*)FindWindow(ID_ALIGN_TO_AXIS))->GetValue()) {
    toPlane = false;
  }

  x = ((ewxTextCtrl*)FindWindow(ID_ALIGN_NX))->getValueAsDouble();
  y = ((ewxTextCtrl*)FindWindow(ID_ALIGN_NY))->getValueAsDouble();
  z = ((ewxTextCtrl*)FindWindow(ID_ALIGN_NZ))->getValueAsDouble();

  cmd->getParameter("toPlane")->setBoolean(toPlane);
  cmd->getParameter("vect_x")->setDouble(x);
  cmd->getParameter("vect_y")->setDouble(y);
  cmd->getParameter("vect_z")->setDouble(z);

  ewxChoice *choice = (ewxChoice*)FindWindow(ID_ALIGN_APPLY_TO);
  if (choice->GetStringSelection() == "All") {
    cmd->getParameter("apply")->setString("all");
  } else {
    cmd->getParameter("apply")->setString("selection");
  }

  fw.execute(cmd);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_DUPLICATE_DIM2_COUNT
 */
void CoordPanel::OnDuplicateDim2CountUpdated( wxSpinEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DUPLICATE
 */
void CoordPanel::OnDuplicateClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  Command *cmd = new LatticeBuildCmd("Build Lattice", &fw.getSceneGraph());

  double x, y, z;
  int ncnt;
  bool randomize;
  ewxTextCtrl *text;
  ewxSpinCtrl *spin;
  ewxCheckBox *check;

  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM1_X);
  x = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM1_Y);
  y = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM1_Z);
  z = text->getValueAsDouble();
  spin = (ewxSpinCtrl*)FindWindow(ID_DUPLICATE_DIM1_COUNT);
  ncnt = spin->GetValue();
  check = (ewxCheckBox*)FindWindow(ID_RANDOM_ORIENT);
  randomize = check->GetValue();

  cmd->getParameter("x1")->setDouble(x);
  cmd->getParameter("y1")->setDouble(y);
  cmd->getParameter("z1")->setDouble(z);
  cmd->getParameter("axis1")->setInteger(ncnt);
  cmd->getParameter("random")->setBoolean(randomize);

  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM2_X);
  x = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM2_Y);
  y = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM2_Z);
  z = text->getValueAsDouble();
  spin = (ewxSpinCtrl*)FindWindow(ID_DUPLICATE_DIM2_COUNT);
  ncnt = spin->GetValue();

  cmd->getParameter("x2")->setDouble(x);
  cmd->getParameter("y2")->setDouble(y);
  cmd->getParameter("z2")->setDouble(z);
  cmd->getParameter("axis2")->setInteger(ncnt);

  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM3_X);
  x = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM3_Y);
  y = text->getValueAsDouble();
  text = (ewxTextCtrl*)FindWindow(ID_DUPLICATE_DIM3_Z);
  z = text->getValueAsDouble();
  spin = (ewxSpinCtrl*)FindWindow(ID_DUPLICATE_DIM3_COUNT);
  ncnt = spin->GetValue();

  cmd->getParameter("x3")->setDouble(x);
  cmd->getParameter("y3")->setDouble(y);
  cmd->getParameter("z3")->setDouble(z);
  cmd->getParameter("axis3")->setInteger(ncnt);

  fw.execute(cmd);
  event.Skip();
}
