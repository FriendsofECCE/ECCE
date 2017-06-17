/////////////////////////////////////////////////////////////////////////////
// Name:        MDPrepGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MDPrepGUI.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "MDPrepGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_CIMINDIS = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_SELECT_ATOMS1 = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_SLV_BOX = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_SETMEM = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_RANY = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_TRNS_INIT_AT = wxNewId();
const wxWindowID MDPrepGUI::ID_STATICTEXT_PMF_SELECT_ATOMS4 = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_THERMODYNAMICS = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ORNT_ID = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_SLV_SPHERE = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_MINDIST = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_RAD = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CIDEL = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_PMF_COMBINE = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_TRNS_ATOM = wxNewId();
const wxWindowID MDPrepGUI::ID_NOTEBOOK = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_XMAX = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_FRAG_NAME = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SHOW_ATOMS1 = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SETBC = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE2 = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_XRANGE = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_USEZBOX = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE2_ANGLE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SETX = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SET_EXPND = wxNewId();
const wxWindowID MDPrepGUI::ID_MENU_FEEDBACK = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_XREP = wxNewId();
const wxWindowID MDPrepGUI::ID_FEEDBACK = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_TRNS_ID = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL1 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_SELECT_ATOMS1 = wxNewId();
const wxWindowID MDPrepGUI::ID_CHOICE_MDPREP_PRTLEV = wxNewId();
const wxWindowID MDPrepGUI::ID_STATICTEXT_PMF_SELECT_ATOMS3 = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_SLV_EDGE = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CENTERY = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ALT_LOC = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SET_SLVTOL = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_FORCE_ANGLE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SOL_MOD = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CONFIGURE = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_DEFAULTS_ANGLE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC4 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_DISTANCE2 = wxNewId();
const wxWindowID MDPrepGUI::ID_NOTEBOOK_MDPREP_MAIN = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_GETCHARGE = wxNewId();
const wxWindowID MDPrepGUI::ID_NOTEBOOK_MDPREP_COORDS = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_TRANSLATE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE2_TORSION = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_XUNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_RANZ = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_ZOPTS = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SETDIM = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_HIP = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_CIGRIDSZ = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ORNT_INIT_AT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC8 = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_FORCE_TORSION = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ORNT_FNL_AT = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_DISTANCE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ORNT_Y = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ALGNI = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_SELECT_ATOMS3 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_ZUNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_TRNS_ID = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_DEFAULTS_DISTANCE = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_PMF_CONSTRAINT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_MEMGLBL = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_UP = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_X = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE1_DISTANCE_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_SOLVATE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ALGNK = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ORNT_ID = wxNewId();
const wxWindowID MDPrepGUI::ID_CHOICE_PMF_OPTION = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CHEMSYS = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SELECT_ATOMS3 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SOL_NAM = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_OPTION = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC5 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE1_ANGLE = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_RANX = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_ANGLE2 = wxNewId();
const wxWindowID MDPrepGUI::ID_CHOICE_MDPREP_REPOPTS = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_BXSZ = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_PMF_BIAS = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_TRNS_MOL = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_TRNS_Z = wxNewId();
const wxWindowID MDPrepGUI::ID_CHOICE_MDPREP_BCOPT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC1 = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_TRNS_SOL = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_YRANGE = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_LNK_ADD = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_FORCE_DISTANCE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_MEMSTK = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_ZMIN = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE1_TORSION_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_YMIN = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SETY = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_UP = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_ORIENT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_MEMHEAP = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_ANGLE1_UNIT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SETZ = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_PMF_ALL = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_LNK_SELECT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CIADD = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_BOXZ = wxNewId();
const wxWindowID MDPrepGUI::ID_STATICTEXT_PMF_SELECT_ATOMS1 = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_DOWN = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_RESET = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_REPLIC = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_DISTANCE2_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ORNT_ANGLE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_XMAX = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_LNK_ATOM1 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_MINSOLV = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_YREP = wxNewId();
const wxWindowID MDPrepGUI::ID_STATICTEXT_MDPREP_SLV_SPHERE_UNIT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_XTRNS_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_SELECT_ATOMS3 = wxNewId();
const wxWindowID MDPrepGUI::ID_MENU_MDPREP_HELP = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_YUNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CENTERX = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_BOX_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_ZMAX = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLVTOL = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_HIE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_MINRSEP = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_ROTATE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_CIMINDIS = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE2_DISTANCE_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_DELETE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SKIPY = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_EXPND = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_EDGE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ORNT_X = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SET_MINSLV = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC2 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC10 = wxNewId();
const wxWindowID MDPrepGUI::ID_MDPREP_FRAME = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_SELECT_ATOMS3 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE1_DISTANCE = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_MINRSEP = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_COORDINATES = wxNewId();
const wxWindowID MDPrepGUI::ID_LISTBOX_MDPREP_LNK_LIST = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_CHAIN = wxNewId();
const wxWindowID MDPrepGUI::ID_CHOICE_MDPREP_SLV_GEOM = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_ORNT_ATOM = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ZMAX = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_TRNS_Y = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ZTRNS_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SLV_RMV = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_IZDIS = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE1_TORSION = wxNewId();
const wxWindowID MDPrepGUI::ID_LISTBOX_PMF_DIRECTIVES = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_EDGE = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_INVCPY = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE1_ANGLE_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ORNT_INIT_AT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_ANGLE2_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_BUILDER = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_ANGLE = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_ADD_DIRECTIVE = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_ALIGN = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_TOPOLOGY = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_Y = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SHOW_ATOMS2 = wxNewId();
const wxWindowID MDPrepGUI::ID_STATICTEXT_PMF_SELECT_ATOMS2 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_MINSOLV = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_ORNT_SEG = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_DISTANCE1_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SETZ = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_DELETE = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC9 = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_LOCAL = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_ORNT_ID = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_SELECT_ATOMS2 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_YMIN = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SKIPX = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SETY = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_MINDIST = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SOLVATE = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLCT_MOD = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_HID = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_DETAILS = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SELECT_ATOMS1 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_ANGLE1 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_CISCALE = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_SLVNT = wxNewId();
const wxWindowID MDPrepGUI::ID_COMBOBOX_MDPREP_CINAME = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_TRNS_FNL_AT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ZREP = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE2_ANGLE_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_BOXY = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_ORIENT = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_REMOVESOLVENT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_DOWN = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_SELECT_ATOMS4 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ZREP = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE2_DISTANCE = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_IZDIS = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ORNT_ANGLE_UNIT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_XMIN = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_ORNT_MOL = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE2_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_SKIP = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_FORCE = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_SELECT_ATOMS1 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_SELECT_ATOMS2 = wxNewId();
const wxWindowID MDPrepGUI::ID_CHOICE_MDPREP_SOLVENTS = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_REMOTE = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_Z = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_TRNS_X = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SHOW_ATOMS3 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ORNT_FNL_AT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLVTOL = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_SELECT_ATOMS4 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_YMAX = wxNewId();
const wxWindowID MDPrepGUI::ID_STATICTEXT_MDPREP_SLV_EDGE_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_ORNTSL = wxNewId();
const wxWindowID MDPrepGUI::ID_CHECKBOX_MDPREP_ZRANGE = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_IZDIS = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_FORCE1 = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CENTER = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_TRNS_INIT_AT = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_TRNS_AIDX = wxNewId();
const wxWindowID MDPrepGUI::ID_CHOICE_MDPREP_RMV_OPT = wxNewId();
const wxWindowID MDPrepGUI::ID_RADIOBUTTON_MDPREP_TRNS_SEG = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_LNK_ATOM2 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_BOXDIM = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SKIPZ = wxNewId();
const wxWindowID MDPrepGUI::ID_LISTBOX_MDPREP_CILIST = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_BOXX = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_EXPND = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_PMF_SELECT_ATOMS4 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SETX = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_CINUM = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_YMAX = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_SELECT_ATOMS_2 = wxNewId();
const wxWindowID MDPrepGUI::ID_LISTBOX_MDPREP_CMDS = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ALGNJ = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_TRNS_FNL_AT = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SHOW_ATOMS4 = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SELECT_ATOMS2 = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ALT_LOC_LABEL = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_FINAL_EDIT = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_CNTION = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_ORNT_Z = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_ZMIN = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_LNK_DELETE = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_MEMORY = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_ORNT_AIDX = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_YTRNS_UNIT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_TRNS_ID = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_ADDLNK = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE1_UNIT = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_MDPREP_SLV_RAD = wxNewId();
const wxWindowID MDPrepGUI::wxID_STATIC_PMF_FORCE2_TORSION_UNIT = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_TOPOPT = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC6 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_PMF_DISTANCE1 = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_MDPREP_CENTERZ = wxNewId();
const wxWindowID MDPrepGUI::ID_PANEL_MDPREP_MODIFY_CELL = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC3 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_SLV_XMIN = wxNewId();
const wxWindowID MDPrepGUI::ID_BUTTON_PMF_SELECT_ATOMS4 = wxNewId();
const wxWindowID MDPrepGUI::ID_TEXTCTRL_MDPREP_FRAC7 = wxNewId();

/*!
 * MDPrepGUI type definition
 */

IMPLEMENT_CLASS( MDPrepGUI, ewxFrame )

/*!
 * MDPrepGUI event table definition
 */

BEGIN_EVENT_TABLE( MDPrepGUI, ewxFrame )

////@begin MDPrepGUI event table entries
    EVT_CLOSE( MDPrepGUI::OnCloseWindow )

    EVT_BUTTON( ID_BUTTON_MDPREP_CHEMSYS, MDPrepGUI::OnButtonMdprepChemsysClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_CONFIGURE, MDPrepGUI::OnButtonMdprepConfigureClick )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_HID, MDPrepGUI::OnRadiobuttonMdprepHidSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_HIE, MDPrepGUI::OnRadiobuttonMdprepHieSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_HIP, MDPrepGUI::OnRadiobuttonMdprepHipSelected )

    EVT_CHOICE( ID_CHOICE_MDPREP_SOLVENTS, MDPrepGUI::OnChoiceMdprepSolventsSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SOL_NAM, MDPrepGUI::OnTextctrlMdprepSolNamEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SOL_MOD, MDPrepGUI::OnTextctrlMdprepSolModEnter )

    EVT_COMBOBOX( ID_COMBOBOX_MDPREP_CINAME, MDPrepGUI::OnComboboxMdprepCinameSelected )
    EVT_TEXT_ENTER( ID_COMBOBOX_MDPREP_CINAME, MDPrepGUI::OnComboboxMdprepCinameEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_CINUM, MDPrepGUI::OnTextctrlMdprepCinumEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_GETCHARGE, MDPrepGUI::OnButtonMdprepGetchargeClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_CISCALE, MDPrepGUI::OnTextctrlMdprepCiscaleEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_CIGRIDSZ, MDPrepGUI::OnTextctrlMdprepCigridszEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_CIMINDIS, MDPrepGUI::OnTextctrlMdprepCimindisEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_CIADD, MDPrepGUI::OnButtonMdprepCiaddClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_CIDEL, MDPrepGUI::OnButtonMdprepCidelClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC1, MDPrepGUI::OnTextctrlMdprepFrac1Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC2, MDPrepGUI::OnTextctrlMdprepFrac2Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC3, MDPrepGUI::OnTextctrlMdprepFrac3Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC4, MDPrepGUI::OnTextctrlMdprepFrac4Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC5, MDPrepGUI::OnTextctrlMdprepFrac5Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC6, MDPrepGUI::OnTextctrlMdprepFrac6Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC7, MDPrepGUI::OnTextctrlMdprepFrac7Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC8, MDPrepGUI::OnTextctrlMdprepFrac8Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC9, MDPrepGUI::OnTextctrlMdprepFrac9Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_FRAC10, MDPrepGUI::OnTextctrlMdprepFrac10Enter )

    EVT_BUTTON( ID_BUTTON_MDPREP_LNK_SELECT, MDPrepGUI::OnButtonMdprepLnkSelectClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_LNK_ADD, MDPrepGUI::OnButtonMdprepLnkAddClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_LNK_DELETE, MDPrepGUI::OnButtonMdprepLnkDeleteClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_CENTER, MDPrepGUI::OnButtonMdprepCenterClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_CENTERX, MDPrepGUI::OnButtonMdprepCenterxClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_CENTERY, MDPrepGUI::OnButtonMdprepCenteryClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_CENTERZ, MDPrepGUI::OnButtonMdprepCenterzClick )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_TRNS_MOL, MDPrepGUI::OnRadiobuttonMdprepTrnsMolSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_TRNS_SEG, MDPrepGUI::OnRadiobuttonMdprepTrnsSegSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_TRNS_ATOM, MDPrepGUI::OnRadiobuttonMdprepTrnsAtomSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_TRNS_X, MDPrepGUI::OnTextctrlMdprepTrnsXEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_TRNS_Y, MDPrepGUI::OnTextctrlMdprepTrnsYEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_TRNS_Z, MDPrepGUI::OnTextctrlMdprepTrnsZEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_TRNS_SOL, MDPrepGUI::OnButtonMdprepTrnsSolClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_ORIENT, MDPrepGUI::OnButtonMdprepOrientClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_ALIGN, MDPrepGUI::OnButtonMdprepAlignClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_ORNTSL, MDPrepGUI::OnButtonMdprepOrntslClick )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_ORNT_MOL, MDPrepGUI::OnRadiobuttonMdprepOrntMolSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_ORNT_SEG, MDPrepGUI::OnRadiobuttonMdprepOrntSegSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_MDPREP_ORNT_ATOM, MDPrepGUI::OnRadiobuttonMdprepOrntAtomSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_ORNT_ANGLE, MDPrepGUI::OnTextctrlMdprepOrntAngleEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_ORNT_X, MDPrepGUI::OnTextctrlMdprepOrntXEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_ORNT_Y, MDPrepGUI::OnTextctrlMdprepOrntYEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_ORNT_Z, MDPrepGUI::OnTextctrlMdprepOrntZEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_ROTATE, MDPrepGUI::OnButtonMdprepRotateClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SETX, MDPrepGUI::OnTextctrlMdprepSetxEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SETY, MDPrepGUI::OnTextctrlMdprepSetyEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SETZ, MDPrepGUI::OnTextctrlMdprepSetzEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_SETDIM, MDPrepGUI::OnButtonMdprepSetdimClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_SETBC, MDPrepGUI::OnButtonMdprepSetbcClick )

    EVT_CHOICE( ID_CHOICE_MDPREP_BCOPT, MDPrepGUI::OnChoiceMdprepBcoptSelected )

    EVT_CHOICE( ID_CHOICE_MDPREP_SLV_GEOM, MDPrepGUI::OnChoiceMdprepSlvGeomSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SLV_MINDIST, MDPrepGUI::OnTextctrlMdprepSlvMindistEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_SOLVATE, MDPrepGUI::OnButtonMdprepSolvateClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_SET_MINSLV, MDPrepGUI::OnButtonMdprepSetMinslvClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_MINSOLV, MDPrepGUI::OnTextctrlMdprepMinsolvEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_SET_EXPND, MDPrepGUI::OnButtonMdprepSetExpndClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_EXPND, MDPrepGUI::OnTextctrlMdprepExpndEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_SET_SLVTOL, MDPrepGUI::OnButtonMdprepSetSlvtolClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SLVTOL, MDPrepGUI::OnTextctrlMdprepSlvtolEnter )

    EVT_CHOICE( ID_CHOICE_MDPREP_RMV_OPT, MDPrepGUI::OnChoiceMdprepRmvOptSelected )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_XRANGE, MDPrepGUI::OnCheckboxMdprepXrangeClick )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_YRANGE, MDPrepGUI::OnCheckboxMdprepYrangeClick )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_ZRANGE, MDPrepGUI::OnCheckboxMdprepZrangeClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_SLV_RMV, MDPrepGUI::OnButtonMdprepSlvRmvClick )

    EVT_CHOICE( ID_CHOICE_MDPREP_REPOPTS, MDPrepGUI::OnChoiceMdprepRepoptsSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_XREP, MDPrepGUI::OnTextctrlMdprepXrepEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_YREP, MDPrepGUI::OnTextctrlMdprepYrepEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_ZREP, MDPrepGUI::OnTextctrlMdprepZrepEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_MINRSEP, MDPrepGUI::OnTextctrlMdprepMinrsepEnter )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_RANX, MDPrepGUI::OnCheckboxMdprepRanxClick )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_RANY, MDPrepGUI::OnCheckboxMdprepRanyClick )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_RANZ, MDPrepGUI::OnCheckboxMdprepRanzClick )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_INVCPY, MDPrepGUI::OnCheckboxMdprepInvcpyClick )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_USEZBOX, MDPrepGUI::OnCheckboxMdprepUsezboxClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_REPLIC, MDPrepGUI::OnButtonMdprepReplicClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SKIPX, MDPrepGUI::OnTextctrlMdprepSkipxEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SKIPY, MDPrepGUI::OnTextctrlMdprepSkipyEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_SKIPZ, MDPrepGUI::OnTextctrlMdprepSkipzEnter )

    EVT_BUTTON( ID_BUTTON_MDPREP_SKIP, MDPrepGUI::OnButtonMdprepSkipClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_UP, MDPrepGUI::OnButtonMdprepUpClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_DOWN, MDPrepGUI::OnButtonMdprepDownClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_DELETE, MDPrepGUI::OnButtonMdprepDeleteClick )

    EVT_CHOICE( ID_CHOICE_PMF_OPTION, MDPrepGUI::OnChoicePmfOptionSelected )

    EVT_TEXT( ID_TEXTCTRL_PMF_SELECT_ATOMS1, MDPrepGUI::OnTextctrlPmfSelectAtoms1Updated )

    EVT_BUTTON( ID_BUTTON_PMF_SELECT_ATOMS1, MDPrepGUI::OnButtonPmfSelectAtoms1Click )

    EVT_BUTTON( ID_BUTTON_PMF_SHOW_ATOMS1, MDPrepGUI::OnButtonPmfShowAtoms1Click )

    EVT_TEXT( ID_TEXTCTRL_PMF_SELECT_ATOMS2, MDPrepGUI::OnTextctrlPmfSelectAtoms2Updated )

    EVT_BUTTON( ID_BUTTON_PMF_SELECT_ATOMS2, MDPrepGUI::OnButtonPmfSelectAtoms2Click )

    EVT_BUTTON( ID_BUTTON_PMF_SHOW_ATOMS2, MDPrepGUI::OnButtonPmfShowAtoms2Click )

    EVT_TEXT( ID_TEXTCTRL_PMF_SELECT_ATOMS3, MDPrepGUI::OnTextctrlPmfSelectAtoms3Updated )

    EVT_BUTTON( ID_BUTTON_PMF_SELECT_ATOMS3, MDPrepGUI::OnButtonPmfSelectAtoms3Click )

    EVT_BUTTON( ID_BUTTON_PMF_SHOW_ATOMS3, MDPrepGUI::OnButtonPmfShowAtoms3Click )

    EVT_TEXT( ID_TEXTCTRL_PMF_SELECT_ATOMS4, MDPrepGUI::OnTextctrlPmfSelectAtoms4Updated )

    EVT_BUTTON( ID_BUTTON_PMF_SELECT_ATOMS4, MDPrepGUI::OnButtonPmfSelectAtoms4Click )

    EVT_BUTTON( ID_BUTTON_PMF_SHOW_ATOMS4, MDPrepGUI::OnButtonPmfShowAtoms4Click )

    EVT_BUTTON( ID_BUTTON_PMF_DEFAULTS_DISTANCE, MDPrepGUI::OnButtonPmfDefaultsDistanceClick )

    EVT_BUTTON( ID_BUTTON_PMF_DEFAULTS_ANGLE, MDPrepGUI::OnButtonPmfDefaultsAngleClick )

    EVT_BUTTON( ID_BUTTON_PMF_ADD_DIRECTIVE, MDPrepGUI::OnButtonPmfAddDirectiveClick )

    EVT_LISTBOX( ID_LISTBOX_PMF_DIRECTIVES, MDPrepGUI::OnListboxPmfDirectivesSelected )

    EVT_BUTTON( ID_BUTTON_PMF_UP, MDPrepGUI::OnButtonPmfUpClick )

    EVT_BUTTON( ID_BUTTON_PMF_DOWN, MDPrepGUI::OnButtonPmfDownClick )

    EVT_BUTTON( ID_BUTTON_PMF_DELETE, MDPrepGUI::OnButtonPmfDeleteClick )

    EVT_CHECKBOX( ID_CHECKBOX_MDPREP_SETMEM, MDPrepGUI::OnCheckboxMdprepSetmemClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_MEMHEAP, MDPrepGUI::OnTextctrlMdprepMemheapEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_MEMSTK, MDPrepGUI::OnTextctrlMdprepMemstkEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MDPREP_MEMGLBL, MDPrepGUI::OnTextctrlMdprepMemglblEnter )

    EVT_CHOICE( ID_CHOICE_MDPREP_PRTLEV, MDPrepGUI::OnChoiceMdprepPrtlevSelected )

    EVT_BUTTON( ID_BUTTON_MDPREP_BUILDER, MDPrepGUI::OnButtonMdprepBuilderClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_FINAL_EDIT, MDPrepGUI::OnButtonMdprepFinalEditClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_LOCAL, MDPrepGUI::OnButtonMdprepLocalClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_REMOTE, MDPrepGUI::OnButtonMdprepRemoteClick )

    EVT_BUTTON( ID_BUTTON_MDPREP_RESET, MDPrepGUI::OnButtonMdprepResetClick )

    EVT_MENU( wxID_SAVE, MDPrepGUI::OnSaveClick )

    EVT_MENU( wxID_EXIT, MDPrepGUI::OnExitClick )

    EVT_MENU( ID_MENU_MDPREP_HELP, MDPrepGUI::OnMenuMdprepHelpClick )

    EVT_MENU( ID_MENU_FEEDBACK, MDPrepGUI::OnMenuFeedbackClick )

////@end MDPrepGUI event table entries

END_EVENT_TABLE()

/*!
 * MDPrepGUI constructors
 */

MDPrepGUI::MDPrepGUI( )
{
}

MDPrepGUI::MDPrepGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * MDPrepGUI creator
 */

bool MDPrepGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MDPrepGUI member initialisation
    p_mainTopoSizer = NULL;
    p_solventPanel = NULL;
    p_ionPanel = NULL;
    p_linkPanel = NULL;
    p_translateVSizer = NULL;
    p_translateMolID = NULL;
    p_translateAtomID = NULL;
    p_orientVSizer = NULL;
    p_orientMolID = NULL;
    p_orientAtomID = NULL;
    p_solvateVSizer = NULL;
    p_edgePanel = NULL;
    p_slvEdgeSizer = NULL;
    p_spherePanel = NULL;
    p_slvBoxPanel = NULL;
    p_modifyVSizer = NULL;
    p_invertZPanel = NULL;
    p_invertZDis = NULL;
    p_feedbackSizer = NULL;
////@end MDPrepGUI member initialisation

////@begin MDPrepGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MDPrepGUI creation
    return true;
}

/*!
 * Control creation for MDPrepGUI
 */

void MDPrepGUI::CreateControls()
{    
////@begin MDPrepGUI content construction
    MDPrepGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu533 = new wxMenu;
    itemMenu533->Append(wxID_SAVE, _("&Save\tCtrl-S"), _T(""), wxITEM_NORMAL);
    itemMenu533->AppendSeparator();
    itemMenu533->Append(wxID_EXIT, _("&Quit\tCtrl+q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu533, _("File"));
    wxMenu* itemMenu537 = new wxMenu;
    itemMenu537->Append(ID_MENU_MDPREP_HELP, _("on NWChem MD Prepare..."), _T(""), wxITEM_NORMAL);
    itemMenu537->Append(ID_MENU_FEEDBACK, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu537, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    ewxNotebook* itemNotebook3 = new ewxNotebook( itemFrame1, ID_NOTEBOOK_MDPREP_MAIN, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel4 = new ewxPanel( itemNotebook3, ID_PANEL_TOPOLOGY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    p_mainTopoSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(p_mainTopoSizer);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    p_mainTopoSizer->Add(itemBoxSizer6, 0, wxGROW|wxALL, 0);
    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer6->Add(itemBoxSizer7, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 0);
    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);
    ewxButton* itemButton9 = new ewxButton( itemPanel4, ID_BUTTON_MDPREP_CHEMSYS, _("Chemical System..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemButton9, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer8->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemPanel4, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemPanel4, wxID_STATIC_MDPREP_FRAG_NAME, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer13, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer13->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxNonBoldLabel* itemStaticText15 = new ewxNonBoldLabel( itemPanel4, wxID_STATIC, _("Model:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText16 = new ewxStaticText( itemPanel4, wxID_STATIC_MDPREP_SLCT_MOD, _("Model"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer13->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxNonBoldLabel* itemStaticText18 = new ewxNonBoldLabel( itemPanel4, wxID_STATIC_MDPREP_ALT_LOC_LABEL, _("Alternate Location:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText19 = new ewxStaticText( itemPanel4, wxID_STATIC_MDPREP_ALT_LOC, _("AltLoc"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer13->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxNonBoldLabel* itemStaticText21 = new ewxNonBoldLabel( itemPanel4, wxID_STATIC, _("Chain(s):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText22 = new ewxStaticText( itemPanel4, wxID_STATIC_MDPREP_CHAIN, _("ID"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText22, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel23 = new ewxPanel( itemPanel4, ID_PANEL_MDPREP_TOPOPT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_mainTopoSizer->Add(itemPanel23, 1, wxGROW|wxALL, 0);
    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxVERTICAL);
    itemPanel23->SetSizer(itemBoxSizer24);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer24->Add(itemBoxSizer25, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton26 = new ewxButton( itemPanel23, ID_BUTTON_MDPREP_CONFIGURE, _("Configure Force Field Data..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemButton26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer24->Add(itemBoxSizer27, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText28 = new ewxStaticText( itemPanel23, wxID_STATIC, _("Histidine Default:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(itemStaticText28, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxRadioButton* itemRadioButton29 = new ewxRadioButton( itemPanel23, ID_RADIOBUTTON_MDPREP_HID, _("HID"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton29->SetValue(true);
    itemBoxSizer27->Add(itemRadioButton29, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxRadioButton* itemRadioButton30 = new ewxRadioButton( itemPanel23, ID_RADIOBUTTON_MDPREP_HIE, _("HIE"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton30->SetValue(false);
    itemBoxSizer27->Add(itemRadioButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxRadioButton* itemRadioButton31 = new ewxRadioButton( itemPanel23, ID_RADIOBUTTON_MDPREP_HIP, _("HIP"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton31->SetValue(false);
    itemBoxSizer27->Add(itemRadioButton31, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxNotebook* itemNotebook32 = new ewxNotebook( itemPanel23, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel33 = new ewxPanel( itemNotebook32, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer34 = new wxBoxSizer(wxVERTICAL);
    itemPanel33->SetSizer(itemBoxSizer34);

    p_solventPanel = new ewxPanel( itemPanel33, ID_PANEL_MDPREP_SLVNT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer34->Add(p_solventPanel, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer36 = new wxBoxSizer(wxVERTICAL);
    p_solventPanel->SetSizer(itemBoxSizer36);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer36->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 0);
    ewxStaticText* itemStaticText38 = new ewxStaticText( p_solventPanel, wxID_STATIC, _("Solvent Options:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice39Strings[] = {
        _("Water"),
        _("Methanol"),
        _("Chloroform"),
        _("Tetrahydrofuran"),
        _("Other")
    };
    ewxChoice* itemChoice39 = new ewxChoice( p_solventPanel, ID_CHOICE_MDPREP_SOLVENTS, wxDefaultPosition, wxDefaultSize, 5, itemChoice39Strings, 0 );
    itemBoxSizer37->Add(itemChoice39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer36->Add(itemBoxSizer40, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 0);
    ewxStaticText* itemStaticText41 = new ewxStaticText( p_solventPanel, wxID_STATIC, _("PDB Solvent Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText41, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl42 = new ewxTextCtrl( p_solventPanel, ID_TEXTCTRL_MDPREP_SOL_NAM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemTextCtrl42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText43 = new ewxStaticText( p_solventPanel, wxID_STATIC, _("Solvent Model:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText43, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl44 = new ewxTextCtrl( p_solventPanel, ID_TEXTCTRL_MDPREP_SOL_MOD, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemTextCtrl44, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer45 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer34->Add(itemBoxSizer45, 0, wxGROW|wxALL, 0);
    ewxStaticLine* itemStaticLine46 = new ewxStaticLine( itemPanel33, wxID_STATIC, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL );
    itemBoxSizer45->Add(itemStaticLine46, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText47 = new ewxStaticText( itemPanel33, wxID_STATIC, _("Add Counter Ions"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer45->Add(itemStaticText47, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine48 = new ewxStaticLine( itemPanel33, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer45->Add(itemStaticLine48, 1, wxGROW|wxALL, 3);

    p_ionPanel = new ewxPanel( itemPanel33, ID_PANEL_MDPREP_CNTION, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer34->Add(p_ionPanel, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxVERTICAL);
    p_ionPanel->SetSizer(itemBoxSizer50);

    wxBoxSizer* itemBoxSizer51 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer50->Add(itemBoxSizer51, 0, wxALIGN_LEFT|wxALL, 0);
    wxGridSizer* itemGridSizer52 = new wxGridSizer(2, 2, 3, 3);
    itemBoxSizer51->Add(itemGridSizer52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    ewxStaticText* itemStaticText53 = new ewxStaticText( p_ionPanel, wxID_STATIC, _("Counter Ion Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer52->Add(itemStaticText53, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    wxString itemComboBox54Strings[] = {
        _("Cl"),
        _("Na")
    };
    ewxComboBox* itemComboBox54 = new ewxComboBox( p_ionPanel, ID_COMBOBOX_MDPREP_CINAME, _("Cl"), wxDefaultPosition, wxDefaultSize, 2, itemComboBox54Strings, wxCB_DROPDOWN );
    itemComboBox54->SetStringSelection(_("Cl"));
    itemGridSizer52->Add(itemComboBox54, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText55 = new ewxStaticText( p_ionPanel, wxID_STATIC, _("Number of Counter Ions:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer52->Add(itemStaticText55, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl56 = new ewxTextCtrl( p_ionPanel, ID_TEXTCTRL_MDPREP_CINUM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer52->Add(itemTextCtrl56, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton57 = new ewxButton( p_ionPanel, ID_BUTTON_MDPREP_GETCHARGE, _("Get Charge"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemButton57, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText58 = new ewxStaticText( p_ionPanel, wxID_STATIC, _("Counter Ion Location Parameters:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(itemStaticText58, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer59 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer50->Add(itemBoxSizer59, 0, wxGROW|wxLEFT, 10);
    wxFlexGridSizer* itemFlexGridSizer60 = new wxFlexGridSizer(4, 3, 3, 3);
    itemBoxSizer59->Add(itemFlexGridSizer60, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    ewxStaticText* itemStaticText61 = new ewxStaticText( p_ionPanel, wxID_STATIC, _("Counter Ion Charge Scaling Factor:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText61, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl62 = new ewxTextCtrl( p_ionPanel, ID_TEXTCTRL_MDPREP_CISCALE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemTextCtrl62, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer60->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText64 = new ewxStaticText( p_ionPanel, wxID_STATIC, _("Size of Trial Grid for Counter Ion Positions:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText64, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl65 = new ewxTextCtrl( p_ionPanel, ID_TEXTCTRL_MDPREP_CIGRIDSZ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemTextCtrl65, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer60->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText67 = new ewxStaticText( p_ionPanel, wxID_STATIC, _("Minimum Distance Between Atom and Counter Ion:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText67, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl68 = new ewxTextCtrl( p_ionPanel, ID_TEXTCTRL_MDPREP_CIMINDIS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemTextCtrl68, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText69 = new ewxStaticText( p_ionPanel, wxID_STATIC_MDPREP_CIMINDIS, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText69, 1, wxALIGN_LEFT|wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer70 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer50->Add(itemBoxSizer70, 0, wxGROW|wxALL, 0);
    wxGridSizer* itemGridSizer71 = new wxGridSizer(2, 1, 0, 0);
    itemBoxSizer70->Add(itemGridSizer71, 0, wxALIGN_BOTTOM|wxALL, 5);
    ewxButton* itemButton72 = new ewxButton( p_ionPanel, ID_BUTTON_MDPREP_CIADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton72->SetToolTip(_("Construct counter ion command and add to command list"));
    itemGridSizer71->Add(itemButton72, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxButton* itemButton73 = new ewxButton( p_ionPanel, ID_BUTTON_MDPREP_CIDEL, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton73->SetToolTip(_("Delete counter ion command"));
    itemGridSizer71->Add(itemButton73, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    wxBoxSizer* itemBoxSizer74 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer70->Add(itemBoxSizer74, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    ewxStaticText* itemStaticText75 = new ewxStaticText( p_ionPanel, wxID_STATIC, _("Counter Ion List:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer74->Add(itemStaticText75, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 0);

    wxString* itemListBox76Strings = NULL;
    ewxListBox* itemListBox76 = new ewxListBox( p_ionPanel, ID_LISTBOX_MDPREP_CILIST, wxDefaultPosition, wxSize(125, 75), 0, itemListBox76Strings, wxLB_SINGLE );
    itemBoxSizer74->Add(itemListBox76, 0, wxGROW|wxALL, 0);

    itemNotebook32->AddPage(itemPanel33, _("Solvent/Ions"));

    ewxPanel* itemPanel77 = new ewxPanel( itemNotebook32, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer78 = new wxBoxSizer(wxVERTICAL);
    itemPanel77->SetSizer(itemBoxSizer78);

    wxBoxSizer* itemBoxSizer79 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer79, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText80 = new ewxStaticText( itemPanel77, wxID_STATIC, _("Indices of Last Molecule in Each Fraction:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(itemStaticText80, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxFlexGridSizer* itemFlexGridSizer81 = new wxFlexGridSizer(0, 8, 0, 0);
    itemBoxSizer78->Add(itemFlexGridSizer81, 0, wxALIGN_LEFT|wxALL, 5);
    ewxStaticText* itemStaticText82 = new ewxStaticText( itemPanel77, wxID_STATIC, _("1:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText82, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl83 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl83, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText84 = new ewxStaticText( itemPanel77, wxID_STATIC, _("2:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText84, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl85 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl85, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText86 = new ewxStaticText( itemPanel77, wxID_STATIC, _("3:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText86, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl87 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC3, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl87, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText88 = new ewxStaticText( itemPanel77, wxID_STATIC, _("4:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText88, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl89 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC4, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl89, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText90 = new ewxStaticText( itemPanel77, wxID_STATIC, _("5:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText90, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl91 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC5, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl91, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText92 = new ewxStaticText( itemPanel77, wxID_STATIC, _("6:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText92, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl93 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC6, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl93, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText94 = new ewxStaticText( itemPanel77, wxID_STATIC, _("7:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText94, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl95 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC7, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl95, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText96 = new ewxStaticText( itemPanel77, wxID_STATIC, _("8:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText96, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl97 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC8, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl97, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText98 = new ewxStaticText( itemPanel77, wxID_STATIC, _("9:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText98, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl99 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC9, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl99, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText100 = new ewxStaticText( itemPanel77, wxID_STATIC, _("10:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemStaticText100, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl101 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_MDPREP_FRAC10, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer81->Add(itemTextCtrl101, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer102 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer102, 0, wxGROW|wxALL, 0);
    ewxStaticLine* itemStaticLine103 = new ewxStaticLine( itemPanel77, wxID_STATIC, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL );
    itemBoxSizer102->Add(itemStaticLine103, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText104 = new ewxStaticText( itemPanel77, wxID_STATIC, _("Add Atom Links"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer102->Add(itemStaticText104, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine105 = new ewxStaticLine( itemPanel77, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer102->Add(itemStaticLine105, 1, wxGROW|wxALL, 3);

    p_linkPanel = new ewxPanel( itemPanel77, ID_PANEL_MDPREP_ADDLNK, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer78->Add(p_linkPanel, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer107 = new wxBoxSizer(wxHORIZONTAL);
    p_linkPanel->SetSizer(itemBoxSizer107);

    wxBoxSizer* itemBoxSizer108 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer107->Add(itemBoxSizer108, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    wxFlexGridSizer* itemFlexGridSizer109 = new wxFlexGridSizer(2, 2, 3, 3);
    itemFlexGridSizer109->AddGrowableCol(0);
    itemBoxSizer108->Add(itemFlexGridSizer109, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);
    ewxStaticText* itemStaticText110 = new ewxStaticText( p_linkPanel, wxID_STATIC, _("Atom 1:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer109->Add(itemStaticText110, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl111 = new ewxTextCtrl( p_linkPanel, ID_TEXTCTRL_MDPREP_LNK_ATOM1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer109->Add(itemTextCtrl111, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText112 = new ewxStaticText( p_linkPanel, wxID_STATIC, _("Atom 2:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer109->Add(itemStaticText112, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl113 = new ewxTextCtrl( p_linkPanel, ID_TEXTCTRL_MDPREP_LNK_ATOM2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer109->Add(itemTextCtrl113, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton114 = new ewxButton( p_linkPanel, ID_BUTTON_MDPREP_LNK_SELECT, _("Get Selection"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer108->Add(itemButton114, 0, wxALIGN_RIGHT|wxALL, 3);

    itemBoxSizer107->Add(30, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer116 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer107->Add(itemBoxSizer116, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxBoxSizer* itemBoxSizer117 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer116->Add(itemBoxSizer117, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    ewxButton* itemButton118 = new ewxButton( p_linkPanel, ID_BUTTON_MDPREP_LNK_ADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer117->Add(itemButton118, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    ewxButton* itemButton119 = new ewxButton( p_linkPanel, ID_BUTTON_MDPREP_LNK_DELETE, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer117->Add(itemButton119, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer120 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer107->Add(itemBoxSizer120, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    ewxStaticText* itemStaticText121 = new ewxStaticText( p_linkPanel, wxID_STATIC, _("Atom Link List:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer120->Add(itemStaticText121, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 0);

    wxString* itemListBox122Strings = NULL;
    ewxListBox* itemListBox122 = new ewxListBox( p_linkPanel, ID_LISTBOX_MDPREP_LNK_LIST, wxDefaultPosition, wxSize(-1, 125), 0, itemListBox122Strings, wxLB_SINGLE );
    itemBoxSizer120->Add(itemListBox122, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    itemNotebook32->AddPage(itemPanel77, _("Solute"));

    itemBoxSizer24->Add(itemNotebook32, 1, wxGROW|wxALL, 10);

    itemNotebook3->AddPage(itemPanel4, _("Topology"));

    ewxPanel* itemPanel123 = new ewxPanel( itemNotebook3, ID_PANEL_COORDINATES, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer124 = new wxBoxSizer(wxVERTICAL);
    itemPanel123->SetSizer(itemBoxSizer124);

    ewxNotebook* itemNotebook125 = new ewxNotebook( itemPanel123, ID_NOTEBOOK_MDPREP_COORDS, wxDefaultPosition, wxDefaultSize, 0 );

    ewxPanel* itemPanel126 = new ewxPanel( itemNotebook125, ID_PANEL_MDPREP_TRANSLATE, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    p_translateVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel126->SetSizer(p_translateVSizer);

    wxBoxSizer* itemBoxSizer128 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer128, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton129 = new ewxButton( itemPanel126, ID_BUTTON_MDPREP_CENTER, _("Center System"), wxDefaultPosition, wxSize(100, -1), 0 );
    itemBoxSizer128->Add(itemButton129, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer130 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer130, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton131 = new ewxButton( itemPanel126, ID_BUTTON_MDPREP_CENTERX, _("Center X"), wxDefaultPosition, wxSize(100, -1), 0 );
    itemBoxSizer130->Add(itemButton131, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer132 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer132, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton133 = new ewxButton( itemPanel126, ID_BUTTON_MDPREP_CENTERY, _("Center Y"), wxDefaultPosition, wxSize(100, -1), 0 );
    itemBoxSizer132->Add(itemButton133, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer134 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer134, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton135 = new ewxButton( itemPanel126, ID_BUTTON_MDPREP_CENTERZ, _("Center Z"), wxDefaultPosition, wxSize(100, -1), 0 );
    itemBoxSizer134->Add(itemButton135, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer136 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer136, 0, wxGROW|wxTOP|wxBOTTOM, 0);
    ewxStaticLine* itemStaticLine137 = new ewxStaticLine( itemPanel126, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer136->Add(itemStaticLine137, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText138 = new ewxStaticText( itemPanel126, wxID_STATIC, _("Translate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer136->Add(itemStaticText138, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine139 = new ewxStaticLine( itemPanel126, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer136->Add(itemStaticLine139, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer140 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer140, 0, wxALIGN_LEFT|wxALL, 0);
    ewxRadioButton* itemRadioButton141 = new ewxRadioButton( itemPanel126, ID_RADIOBUTTON_MDPREP_TRNS_MOL, _("Molecule"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton141->SetValue(false);
    itemBoxSizer140->Add(itemRadioButton141, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton142 = new ewxRadioButton( itemPanel126, ID_RADIOBUTTON_MDPREP_TRNS_SEG, _("Segment"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton142->SetValue(false);
    itemBoxSizer140->Add(itemRadioButton142, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton143 = new ewxRadioButton( itemPanel126, ID_RADIOBUTTON_MDPREP_TRNS_ATOM, _("Atoms"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton143->SetValue(false);
    itemBoxSizer140->Add(itemRadioButton143, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_translateMolID = new ewxPanel( itemPanel126, ID_PANEL_MDPREP_TRNS_ID, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_translateVSizer->Add(p_translateMolID, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer145 = new wxBoxSizer(wxHORIZONTAL);
    p_translateMolID->SetSizer(itemBoxSizer145);

    ewxStaticText* itemStaticText146 = new ewxStaticText( p_translateMolID, wxID_STATIC_MDPREP_TRNS_ID, _("Molecule ID:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer145->Add(itemStaticText146, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl147 = new ewxTextCtrl( p_translateMolID, ID_TEXTCTRL_MDPREP_TRNS_ID, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer145->Add(itemTextCtrl147, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_translateAtomID = new ewxPanel( itemPanel126, ID_PANEL_MDPREP_TRNS_AIDX, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_translateVSizer->Add(p_translateAtomID, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer149 = new wxBoxSizer(wxHORIZONTAL);
    p_translateAtomID->SetSizer(itemBoxSizer149);

    ewxStaticText* itemStaticText150 = new ewxStaticText( p_translateAtomID, wxID_STATIC_MDPREP_TRNS_INIT_AT, _("Initial Atom Index:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer149->Add(itemStaticText150, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl151 = new ewxTextCtrl( p_translateAtomID, ID_TEXTCTRL_MDPREP_TRNS_INIT_AT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer149->Add(itemTextCtrl151, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer149->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText153 = new ewxStaticText( p_translateAtomID, wxID_STATIC_MDPREP_TRNS_FNL_AT, _("Final Atom Index:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer149->Add(itemStaticText153, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl154 = new ewxTextCtrl( p_translateAtomID, ID_TEXTCTRL_MDPREP_TRNS_FNL_AT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer149->Add(itemTextCtrl154, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer155 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer155, 0, wxGROW|wxALL, 0);
    wxBoxSizer* itemBoxSizer156 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer155->Add(itemBoxSizer156, 0, wxALIGN_TOP|wxALL, 3);
    ewxStaticText* itemStaticText157 = new ewxStaticText( itemPanel126, wxID_STATIC, _("Translation Vector:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer156->Add(itemStaticText157, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxFlexGridSizer* itemFlexGridSizer158 = new wxFlexGridSizer(3, 3, 0, 0);
    itemBoxSizer155->Add(itemFlexGridSizer158, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    ewxStaticText* itemStaticText159 = new ewxStaticText( itemPanel126, wxID_STATIC, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer158->Add(itemStaticText159, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl160 = new ewxTextCtrl( itemPanel126, ID_TEXTCTRL_MDPREP_TRNS_X, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer158->Add(itemTextCtrl160, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText161 = new ewxStaticText( itemPanel126, wxID_STATIC_MDPREP_XTRNS_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemFlexGridSizer158->Add(itemStaticText161, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText162 = new ewxStaticText( itemPanel126, wxID_STATIC, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer158->Add(itemStaticText162, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl163 = new ewxTextCtrl( itemPanel126, ID_TEXTCTRL_MDPREP_TRNS_Y, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer158->Add(itemTextCtrl163, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText164 = new ewxStaticText( itemPanel126, wxID_STATIC_MDPREP_YTRNS_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemFlexGridSizer158->Add(itemStaticText164, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText165 = new ewxStaticText( itemPanel126, wxID_STATIC, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer158->Add(itemStaticText165, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl166 = new ewxTextCtrl( itemPanel126, ID_TEXTCTRL_MDPREP_TRNS_Z, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer158->Add(itemTextCtrl166, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText167 = new ewxStaticText( itemPanel126, wxID_STATIC_MDPREP_ZTRNS_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemFlexGridSizer158->Add(itemStaticText167, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer168 = new wxBoxSizer(wxHORIZONTAL);
    p_translateVSizer->Add(itemBoxSizer168, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton169 = new ewxButton( itemPanel126, ID_BUTTON_MDPREP_TRNS_SOL, _("Translate Solute"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer168->Add(itemButton169, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook125->AddPage(itemPanel126, _("Translate"));

    ewxPanel* itemPanel170 = new ewxPanel( itemNotebook125, ID_PANEL_MDPREP_ORIENT, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    p_orientVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel170->SetSizer(p_orientVSizer);

    wxBoxSizer* itemBoxSizer172 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer172, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton173 = new ewxButton( itemPanel170, ID_BUTTON_MDPREP_ORIENT, _("Orient"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton173->SetToolTip(_("Align solute with coordinate axis."));
    itemBoxSizer172->Add(itemButton173, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer174 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer174, 0, wxGROW|wxTOP|wxBOTTOM, 0);
    ewxStaticLine* itemStaticLine175 = new ewxStaticLine( itemPanel170, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer174->Add(itemStaticLine175, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText176 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Align Solute"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer174->Add(itemStaticText176, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine177 = new ewxStaticLine( itemPanel170, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer174->Add(itemStaticLine177, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxFlexGridSizer* itemFlexGridSizer178 = new wxFlexGridSizer(3, 2, 3, 3);
    itemFlexGridSizer178->AddGrowableCol(0);
    p_orientVSizer->Add(itemFlexGridSizer178, 0, wxALIGN_LEFT|wxALL, 3);
    ewxStaticText* itemStaticText179 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Atom Translated to Origin:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemStaticText179, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl180 = new ewxTextCtrl( itemPanel170, ID_TEXTCTRL_MDPREP_ALGNI, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemTextCtrl180, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText181 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Atom Aligned with Z-Axis:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemStaticText181, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl182 = new ewxTextCtrl( itemPanel170, ID_TEXTCTRL_MDPREP_ALGNJ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemTextCtrl182, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText183 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Atom Aligned with X=Y Plane:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemStaticText183, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl184 = new ewxTextCtrl( itemPanel170, ID_TEXTCTRL_MDPREP_ALGNK, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemTextCtrl184, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton185 = new ewxButton( itemPanel170, ID_BUTTON_MDPREP_ALIGN, _("Align"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemButton185, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton186 = new ewxButton( itemPanel170, ID_BUTTON_MDPREP_ORNTSL, _("Get Selection"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer178->Add(itemButton186, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer187 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer187, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer187->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer189 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer189, 0, wxGROW|wxTOP|wxBOTTOM, 0);
    ewxStaticLine* itemStaticLine190 = new ewxStaticLine( itemPanel170, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer189->Add(itemStaticLine190, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText191 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Rotate Solute"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer189->Add(itemStaticText191, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine192 = new ewxStaticLine( itemPanel170, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer189->Add(itemStaticLine192, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer193 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer193, 0, wxALIGN_LEFT|wxALL, 0);
    ewxRadioButton* itemRadioButton194 = new ewxRadioButton( itemPanel170, ID_RADIOBUTTON_MDPREP_ORNT_MOL, _("Molecule"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton194->SetValue(false);
    itemBoxSizer193->Add(itemRadioButton194, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton195 = new ewxRadioButton( itemPanel170, ID_RADIOBUTTON_MDPREP_ORNT_SEG, _("Segment"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton195->SetValue(false);
    itemBoxSizer193->Add(itemRadioButton195, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton196 = new ewxRadioButton( itemPanel170, ID_RADIOBUTTON_MDPREP_ORNT_ATOM, _("Atoms"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton196->SetValue(false);
    itemBoxSizer193->Add(itemRadioButton196, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_orientMolID = new ewxPanel( itemPanel170, ID_PANEL_MDPREP_ORNT_ID, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_orientVSizer->Add(p_orientMolID, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer198 = new wxBoxSizer(wxHORIZONTAL);
    p_orientMolID->SetSizer(itemBoxSizer198);

    ewxStaticText* itemStaticText199 = new ewxStaticText( p_orientMolID, wxID_STATIC_MDPREP_ORNT_ID, _("Molecule ID:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer198->Add(itemStaticText199, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl200 = new ewxTextCtrl( p_orientMolID, ID_TEXTCTRL_MDPREP_ORNT_ID, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer198->Add(itemTextCtrl200, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_orientAtomID = new ewxPanel( itemPanel170, ID_PANEL_MDPREP_ORNT_AIDX, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_orientVSizer->Add(p_orientAtomID, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer202 = new wxBoxSizer(wxHORIZONTAL);
    p_orientAtomID->SetSizer(itemBoxSizer202);

    ewxStaticText* itemStaticText203 = new ewxStaticText( p_orientAtomID, wxID_STATIC_MDPREP_ORNT_INIT_AT, _("Initial Atom Index:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer202->Add(itemStaticText203, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl204 = new ewxTextCtrl( p_orientAtomID, ID_TEXTCTRL_MDPREP_ORNT_INIT_AT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer202->Add(itemTextCtrl204, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText205 = new ewxStaticText( p_orientAtomID, wxID_STATIC_MDPREP_ORNT_FNL_AT, _("Final Atom Index:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer202->Add(itemStaticText205, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl206 = new ewxTextCtrl( p_orientAtomID, ID_TEXTCTRL_MDPREP_ORNT_FNL_AT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer202->Add(itemTextCtrl206, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer207 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer207, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText208 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Rotation Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer207->Add(itemStaticText208, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl209 = new ewxTextCtrl( itemPanel170, ID_TEXTCTRL_MDPREP_ORNT_ANGLE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer207->Add(itemTextCtrl209, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText210 = new ewxStaticText( itemPanel170, wxID_STATIC_MDPREP_ORNT_ANGLE_UNIT, _("radians"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer207->Add(itemStaticText210, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer211 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer211, 0, wxALIGN_LEFT|wxRIGHT|wxTOP|wxBOTTOM, 5);
    ewxStaticText* itemStaticText212 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Rotation Vector:  "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer211->Add(itemStaticText212, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText213 = new ewxStaticText( itemPanel170, wxID_STATIC, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer211->Add(itemStaticText213, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl214 = new ewxTextCtrl( itemPanel170, ID_TEXTCTRL_MDPREP_ORNT_X, _T(""), wxDefaultPosition, wxSize(75, -1), 0 );
    itemBoxSizer211->Add(itemTextCtrl214, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText215 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer211->Add(itemStaticText215, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl216 = new ewxTextCtrl( itemPanel170, ID_TEXTCTRL_MDPREP_ORNT_Y, _T(""), wxDefaultPosition, wxSize(75, -1), 0 );
    itemBoxSizer211->Add(itemTextCtrl216, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText217 = new ewxStaticText( itemPanel170, wxID_STATIC, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer211->Add(itemStaticText217, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl218 = new ewxTextCtrl( itemPanel170, ID_TEXTCTRL_MDPREP_ORNT_Z, _T(""), wxDefaultPosition, wxSize(75, -1), 0 );
    itemBoxSizer211->Add(itemTextCtrl218, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer219 = new wxBoxSizer(wxHORIZONTAL);
    p_orientVSizer->Add(itemBoxSizer219, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton220 = new ewxButton( itemPanel170, ID_BUTTON_MDPREP_ROTATE, _("Rotate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer219->Add(itemButton220, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook125->AddPage(itemPanel170, _("Orient"));

    ewxPanel* itemPanel221 = new ewxPanel( itemNotebook125, ID_PANEL_MDPREP_BXSZ, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer222 = new wxBoxSizer(wxVERTICAL);
    itemPanel221->SetSizer(itemBoxSizer222);

    wxBoxSizer* itemBoxSizer223 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer222->Add(itemBoxSizer223, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer224 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer223->Add(itemBoxSizer224, 0, wxALIGN_TOP|wxALL, 5);
    ewxStaticText* itemStaticText225 = new ewxStaticText( itemPanel221, wxID_STATIC, _("Simulation Box Dimensions:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer224->Add(itemStaticText225, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 3);

    wxFlexGridSizer* itemFlexGridSizer226 = new wxFlexGridSizer(3, 3, 0, 0);
    itemBoxSizer223->Add(itemFlexGridSizer226, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    ewxStaticText* itemStaticText227 = new ewxStaticText( itemPanel221, wxID_STATIC, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer226->Add(itemStaticText227, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl228 = new ewxTextCtrl( itemPanel221, ID_TEXTCTRL_MDPREP_SETX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer226->Add(itemTextCtrl228, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText229 = new ewxStaticText( itemPanel221, wxID_STATIC_MDPREP_SETX, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemFlexGridSizer226->Add(itemStaticText229, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText230 = new ewxStaticText( itemPanel221, wxID_STATIC, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer226->Add(itemStaticText230, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl231 = new ewxTextCtrl( itemPanel221, ID_TEXTCTRL_MDPREP_SETY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer226->Add(itemTextCtrl231, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText232 = new ewxStaticText( itemPanel221, wxID_STATIC_MDPREP_SETY, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemFlexGridSizer226->Add(itemStaticText232, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText233 = new ewxStaticText( itemPanel221, wxID_STATIC, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer226->Add(itemStaticText233, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl234 = new ewxTextCtrl( itemPanel221, ID_TEXTCTRL_MDPREP_SETZ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer226->Add(itemTextCtrl234, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText235 = new ewxStaticText( itemPanel221, wxID_STATIC_MDPREP_SETZ, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemFlexGridSizer226->Add(itemStaticText235, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer236 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer222->Add(itemBoxSizer236, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton237 = new ewxButton( itemPanel221, ID_BUTTON_MDPREP_SETDIM, _("Set Dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer236->Add(itemButton237, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook125->AddPage(itemPanel221, _("Box Size"));

    ewxPanel* itemPanel238 = new ewxPanel( itemNotebook125, ID_PANEL_MDPREP_SOLVATE, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    p_solvateVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel238->SetSizer(p_solvateVSizer);

    wxBoxSizer* itemBoxSizer240 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer240, 0, wxALIGN_LEFT|wxRIGHT|wxTOP|wxBOTTOM, 5);
    ewxButton* itemButton241 = new ewxButton( itemPanel238, ID_BUTTON_MDPREP_SETBC, _("Set Boundary Condition"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer240->Add(itemButton241, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer240->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText243 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Options:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer240->Add(itemStaticText243, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice244Strings[] = {
        _("Periodic"),
        _("Vacuum")
    };
    ewxChoice* itemChoice244 = new ewxChoice( itemPanel238, ID_CHOICE_MDPREP_BCOPT, wxDefaultPosition, wxDefaultSize, 2, itemChoice244Strings, 0 );
    itemBoxSizer240->Add(itemChoice244, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer245 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer245, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer246 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer246, 0, wxGROW|wxTOP|wxBOTTOM, 5);
    ewxStaticLine* itemStaticLine247 = new ewxStaticLine( itemPanel238, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer246->Add(itemStaticLine247, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText248 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Solvation Parameters"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer246->Add(itemStaticText248, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine249 = new ewxStaticLine( itemPanel238, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer246->Add(itemStaticLine249, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer250 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer250, 0, wxALIGN_LEFT|wxTOP, 5);
    ewxStaticText* itemStaticText251 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Solvation Geometry:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer250->Add(itemStaticText251, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice252Strings[] = {
        _("Default"),
        _("Cube"),
        _("Rectangular Parallelepiped"),
        _("Sphere"),
        _("Truncated Octahedron")
    };
    ewxChoice* itemChoice252 = new ewxChoice( itemPanel238, ID_CHOICE_MDPREP_SLV_GEOM, wxDefaultPosition, wxDefaultSize, 5, itemChoice252Strings, 0 );
    itemChoice252->SetStringSelection(_("Default"));
    itemBoxSizer250->Add(itemChoice252, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer253 = new wxBoxSizer(wxVERTICAL);
    p_solvateVSizer->Add(itemBoxSizer253, 0, wxALIGN_LEFT|wxRIGHT|wxTOP|wxBOTTOM, 5);
    wxBoxSizer* itemBoxSizer254 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer253->Add(itemBoxSizer254, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
    p_edgePanel = new ewxPanel( itemPanel238, ID_PANEL_MDPREP_SLV_EDGE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer254->Add(p_edgePanel, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);
    p_slvEdgeSizer = new wxBoxSizer(wxHORIZONTAL);
    p_edgePanel->SetSizer(p_slvEdgeSizer);

    ewxStaticText* itemStaticText257 = new ewxStaticText( p_edgePanel, wxID_STATIC_MDPREP_SLV_EDGE, _("Edge Dimension:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_slvEdgeSizer->Add(itemStaticText257, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl258 = new ewxTextCtrl( p_edgePanel, ID_TEXTCTRL_MDPREP_SLV_EDGE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    p_slvEdgeSizer->Add(itemTextCtrl258, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText259 = new ewxStaticText( p_edgePanel, ID_STATICTEXT_MDPREP_SLV_EDGE_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    p_slvEdgeSizer->Add(itemStaticText259, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer254->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_spherePanel = new ewxPanel( itemPanel238, ID_PANEL_MDPREP_SLV_SPHERE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer254->Add(p_spherePanel, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);
    wxBoxSizer* itemBoxSizer262 = new wxBoxSizer(wxHORIZONTAL);
    p_spherePanel->SetSizer(itemBoxSizer262);

    ewxStaticText* itemStaticText263 = new ewxStaticText( p_spherePanel, wxID_STATIC_MDPREP_SLV_RAD, _("Sphere Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer262->Add(itemStaticText263, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl264 = new ewxTextCtrl( p_spherePanel, ID_TEXTCTRL_MDPREP_SLV_RAD, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer262->Add(itemTextCtrl264, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText265 = new ewxStaticText( p_spherePanel, ID_STATICTEXT_MDPREP_SLV_SPHERE_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer262->Add(itemStaticText265, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    p_slvBoxPanel = new ewxPanel( itemPanel238, ID_PANEL_MDPREP_SLV_BOX, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer253->Add(p_slvBoxPanel, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 0);
    wxBoxSizer* itemBoxSizer267 = new wxBoxSizer(wxHORIZONTAL);
    p_slvBoxPanel->SetSizer(itemBoxSizer267);

    ewxStaticText* itemStaticText268 = new ewxStaticText( p_slvBoxPanel, wxID_STATIC_MDPREP_BOXDIM, _("Box:  "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemStaticText268, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText269 = new ewxStaticText( p_slvBoxPanel, wxID_STATIC_MDPREP_SLV_X, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemStaticText269, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl270 = new ewxTextCtrl( p_slvBoxPanel, ID_TEXTCTRL_MDPREP_SLV_BOXX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemTextCtrl270, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText271 = new ewxStaticText( p_slvBoxPanel, wxID_STATIC_MDPREP_SLV_Y, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemStaticText271, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl272 = new ewxTextCtrl( p_slvBoxPanel, ID_TEXTCTRL_MDPREP_SLV_BOXY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemTextCtrl272, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText273 = new ewxStaticText( p_slvBoxPanel, wxID_STATIC_MDPREP_SLV_Z, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemStaticText273, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl274 = new ewxTextCtrl( p_slvBoxPanel, ID_TEXTCTRL_MDPREP_SLV_BOXZ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemTextCtrl274, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText275 = new ewxStaticText( p_slvBoxPanel, wxID_STATIC_MDPREP_SLV_BOX_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer267->Add(itemStaticText275, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer276 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer276, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 0);
    ewxStaticText* itemStaticText277 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Minimum Periodic Solute-Solute Distance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer276->Add(itemStaticText277, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl278 = new ewxTextCtrl( itemPanel238, ID_TEXTCTRL_MDPREP_SLV_MINDIST, _("1.2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer276->Add(itemTextCtrl278, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText279 = new ewxStaticText( itemPanel238, wxID_STATIC_MDPREP_SLV_MINDIST, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer276->Add(itemStaticText279, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer280 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer280, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 0);
    ewxButton* itemButton281 = new ewxButton( itemPanel238, ID_BUTTON_MDPREP_SOLVATE, _("Solvate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer280->Add(itemButton281, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer282 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer282, 0, wxGROW|wxTOP|wxBOTTOM, 5);
    ewxStaticLine* itemStaticLine283 = new ewxStaticLine( itemPanel238, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer282->Add(itemStaticLine283, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText284 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Solvation Controls"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer282->Add(itemStaticText284, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine285 = new ewxStaticLine( itemPanel238, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer282->Add(itemStaticLine285, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer286 = new wxBoxSizer(wxHORIZONTAL);
    p_solvateVSizer->Add(itemBoxSizer286, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 5);
    wxFlexGridSizer* itemFlexGridSizer287 = new wxFlexGridSizer(3, 4, 3, 3);
    itemFlexGridSizer287->AddGrowableCol(1);
    itemBoxSizer286->Add(itemFlexGridSizer287, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    ewxButton* itemButton288 = new ewxButton( itemPanel238, ID_BUTTON_MDPREP_SET_MINSLV, _("Set"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemButton288, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText289 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Minimum solute-solvent distance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemStaticText289, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl290 = new ewxTextCtrl( itemPanel238, ID_TEXTCTRL_MDPREP_MINSOLV, _("0.23"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemTextCtrl290, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 0);

    ewxStaticText* itemStaticText291 = new ewxStaticText( itemPanel238, wxID_STATIC_MDPREP_MINSOLV, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemStaticText291, 1, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 0);

    ewxButton* itemButton292 = new ewxButton( itemPanel238, ID_BUTTON_MDPREP_SET_EXPND, _("Set"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemButton292, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText293 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Expand current simulation box size:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemStaticText293, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl294 = new ewxTextCtrl( itemPanel238, ID_TEXTCTRL_MDPREP_EXPND, _("0.1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemTextCtrl294, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 0);

    ewxStaticText* itemStaticText295 = new ewxStaticText( itemPanel238, wxID_STATIC_MDPREP_EXPND, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemStaticText295, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 0);

    ewxButton* itemButton296 = new ewxButton( itemPanel238, ID_BUTTON_MDPREP_SET_SLVTOL, _("Set"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemButton296, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText297 = new ewxStaticText( itemPanel238, wxID_STATIC, _("Expand simulation box size after solvation:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemStaticText297, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl298 = new ewxTextCtrl( itemPanel238, ID_TEXTCTRL_MDPREP_SLVTOL, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemTextCtrl298, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 0);

    ewxStaticText* itemStaticText299 = new ewxStaticText( itemPanel238, wxID_STATIC_MDPREP_SLVTOL, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer287->Add(itemStaticText299, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 0);

    itemBoxSizer286->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemNotebook125->AddPage(itemPanel238, _("Solvate"));

    ewxPanel* itemPanel301 = new ewxPanel( itemNotebook125, ID_PANEL_MDPREP_REMOVESOLVENT, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer302 = new wxBoxSizer(wxVERTICAL);
    itemPanel301->SetSizer(itemBoxSizer302);

    wxBoxSizer* itemBoxSizer303 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer302->Add(itemBoxSizer303, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);
    ewxStaticText* itemStaticText304 = new ewxStaticText( itemPanel301, wxID_STATIC, _("Remove Solvent:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer303->Add(itemStaticText304, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 3);

    wxString itemChoice305Strings[] = {
        _("Inside Range"),
        _("Outside Range")
    };
    ewxChoice* itemChoice305 = new ewxChoice( itemPanel301, ID_CHOICE_MDPREP_RMV_OPT, wxDefaultPosition, wxDefaultSize, 2, itemChoice305Strings, 0 );
    itemChoice305->SetStringSelection(_("Outside Range"));
    itemBoxSizer303->Add(itemChoice305, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer306 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer302->Add(itemBoxSizer306, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);
    ewxStaticText* itemStaticText307 = new ewxStaticText( itemPanel301, wxID_STATIC, _("Coordinate Ranges:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer306->Add(itemStaticText307, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer308 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer302->Add(itemBoxSizer308, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);
    itemBoxSizer308->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer310 = new wxFlexGridSizer(3, 7, 3, 3);
    itemFlexGridSizer310->AddGrowableRow(5);
    itemFlexGridSizer310->AddGrowableCol(1);
    itemBoxSizer308->Add(itemFlexGridSizer310, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    ewxCheckBox* itemCheckBox311 = new ewxCheckBox( itemPanel301, ID_CHECKBOX_MDPREP_XRANGE, _("X Range"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox311->SetValue(false);
    itemFlexGridSizer310->Add(itemCheckBox311, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText312 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_XMIN, _("Minumum:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText312, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl313 = new ewxTextCtrl( itemPanel301, ID_TEXTCTRL_MDPREP_SLV_XMIN, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemTextCtrl313, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer310->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText315 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_XMAX, _("Maximum:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText315, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl316 = new ewxTextCtrl( itemPanel301, ID_TEXTCTRL_MDPREP_SLV_XMAX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemTextCtrl316, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText317 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_SLV_XUNIT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText317, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    ewxCheckBox* itemCheckBox318 = new ewxCheckBox( itemPanel301, ID_CHECKBOX_MDPREP_YRANGE, _("Y Range"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox318->SetValue(false);
    itemFlexGridSizer310->Add(itemCheckBox318, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText319 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_YMIN, _("Minimum:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText319, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl320 = new ewxTextCtrl( itemPanel301, ID_TEXTCTRL_MDPREP_SLV_YMIN, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemTextCtrl320, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer310->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText322 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_YMAX, _("Maximum:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText322, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl323 = new ewxTextCtrl( itemPanel301, ID_TEXTCTRL_MDPREP_SLV_YMAX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemTextCtrl323, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText324 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_SLV_YUNIT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText324, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    ewxCheckBox* itemCheckBox325 = new ewxCheckBox( itemPanel301, ID_CHECKBOX_MDPREP_ZRANGE, _("Z Range"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox325->SetValue(false);
    itemFlexGridSizer310->Add(itemCheckBox325, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText326 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_ZMIN, _("Minimum:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText326, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl327 = new ewxTextCtrl( itemPanel301, ID_TEXTCTRL_MDPREP_SLV_ZMIN, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemTextCtrl327, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer310->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText329 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_ZMAX, _("Maximum:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText329, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl330 = new ewxTextCtrl( itemPanel301, ID_TEXTCTRL_MDPREP_SLV_ZMAX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemTextCtrl330, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText331 = new ewxStaticText( itemPanel301, wxID_STATIC_MDPREP_SLV_ZUNIT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer310->Add(itemStaticText331, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer332 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer302->Add(itemBoxSizer332, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton333 = new ewxButton( itemPanel301, ID_BUTTON_MDPREP_SLV_RMV, _("Remove Solvent"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer332->Add(itemButton333, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook125->AddPage(itemPanel301, _("Remove Solvent"));

    ewxPanel* itemPanel334 = new ewxPanel( itemNotebook125, ID_PANEL_MDPREP_MODIFY_CELL, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    p_modifyVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel334->SetSizer(p_modifyVSizer);

    wxBoxSizer* itemBoxSizer336 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer336, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText337 = new ewxStaticText( itemPanel334, wxID_STATIC, _("Replication Restrictions:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer336->Add(itemStaticText337, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice338Strings[] = {
        _("None"),
        _("Chains"),
        _("Molecules"),
        _("Fractions")
    };
    ewxChoice* itemChoice338 = new ewxChoice( itemPanel334, ID_CHOICE_MDPREP_REPOPTS, wxDefaultPosition, wxDefaultSize, 4, itemChoice338Strings, 0 );
    itemChoice338->SetStringSelection(_("None"));
    itemBoxSizer336->Add(itemChoice338, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer339 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer339, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText340 = new ewxStaticText( itemPanel334, wxID_STATIC, _("Set Number of Repeat Units:  NX:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer339->Add(itemStaticText340, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl341 = new ewxTextCtrl( itemPanel334, ID_TEXTCTRL_MDPREP_XREP, _("1"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer339->Add(itemTextCtrl341, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer339->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText343 = new ewxStaticText( itemPanel334, wxID_STATIC, _("NY:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer339->Add(itemStaticText343, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl344 = new ewxTextCtrl( itemPanel334, ID_TEXTCTRL_MDPREP_YREP, _("1"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer339->Add(itemTextCtrl344, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer339->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText346 = new ewxStaticText( itemPanel334, wxID_STATIC_MDPREP_ZREP, _("NZ:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer339->Add(itemStaticText346, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl347 = new ewxTextCtrl( itemPanel334, ID_TEXTCTRL_MDPREP_ZREP, _("1"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer339->Add(itemTextCtrl347, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer348 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer348, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText349 = new ewxStaticText( itemPanel334, wxID_STATIC, _("Minimum Atom Separation in Different Copies:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer348->Add(itemStaticText349, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl350 = new ewxTextCtrl( itemPanel334, ID_TEXTCTRL_MDPREP_MINRSEP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer348->Add(itemTextCtrl350, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText351 = new ewxStaticText( itemPanel334, wxID_STATIC_MDPREP_MINRSEP, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer348->Add(itemStaticText351, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer352 = new wxBoxSizer(wxVERTICAL);
    p_modifyVSizer->Add(itemBoxSizer352, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer353 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer352->Add(itemBoxSizer353, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP, 5);
    ewxStaticText* itemStaticText354 = new ewxStaticText( itemPanel334, wxID_STATIC, _("Apply Random Rotation to Repeat Unit About:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer353->Add(itemStaticText354, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer355 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer352->Add(itemBoxSizer355, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    itemBoxSizer355->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox357 = new ewxCheckBox( itemPanel334, ID_CHECKBOX_MDPREP_RANX, _("X-Axis"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox357->SetValue(false);
    itemBoxSizer355->Add(itemCheckBox357, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox358 = new ewxCheckBox( itemPanel334, ID_CHECKBOX_MDPREP_RANY, _("Y-Axis"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox358->SetValue(false);
    itemBoxSizer355->Add(itemCheckBox358, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox359 = new ewxCheckBox( itemPanel334, ID_CHECKBOX_MDPREP_RANZ, _("Z-Axis"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox359->SetValue(false);
    itemBoxSizer355->Add(itemCheckBox359, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer360 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer360, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox361 = new ewxCheckBox( itemPanel334, ID_CHECKBOX_MDPREP_INVCPY, _("Create Inverted Copy Along Z-Axis"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox361->SetValue(false);
    itemBoxSizer360->Add(itemCheckBox361, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_invertZPanel = new ewxPanel( itemPanel334, ID_PANEL_MDPREP_ZOPTS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_modifyVSizer->Add(p_invertZPanel, 0, wxGROW|wxALL, 0);
    wxBoxSizer* itemBoxSizer363 = new wxBoxSizer(wxHORIZONTAL);
    p_invertZPanel->SetSizer(itemBoxSizer363);

    itemBoxSizer363->Add(20, 0, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer365 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer363->Add(itemBoxSizer365, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    wxBoxSizer* itemBoxSizer366 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer365->Add(itemBoxSizer366, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox367 = new ewxCheckBox( p_invertZPanel, ID_CHECKBOX_MDPREP_USEZBOX, _("Use Box Dimension for Z-Separation Distance"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox367->SetValue(false);
    itemBoxSizer366->Add(itemCheckBox367, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_invertZDis = new ewxPanel( p_invertZPanel, ID_PANEL_MDPREP_IZDIS, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer365->Add(p_invertZDis, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer369 = new wxBoxSizer(wxHORIZONTAL);
    p_invertZDis->SetSizer(itemBoxSizer369);

    ewxStaticText* itemStaticText370 = new ewxStaticText( p_invertZDis, wxID_STATIC, _("Z-Axis Separation Distance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer369->Add(itemStaticText370, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl371 = new ewxTextCtrl( p_invertZDis, ID_TEXTCTRL_MDPREP_IZDIS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer369->Add(itemTextCtrl371, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText372 = new ewxStaticText( p_invertZDis, wxID_STATIC_MDPREP_IZDIS, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer369->Add(itemStaticText372, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer373 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer373, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton374 = new ewxButton( itemPanel334, ID_BUTTON_MDPREP_REPLIC, _("Replicate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer373->Add(itemButton374, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer375 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer375, 0, wxGROW|wxTOP|wxBOTTOM, 5);
    ewxStaticLine* itemStaticLine376 = new ewxStaticLine( itemPanel334, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer375->Add(itemStaticLine376, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText377 = new ewxStaticText( itemPanel334, wxID_STATIC, _("Skip Block in Replicate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer375->Add(itemStaticText377, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine378 = new ewxStaticLine( itemPanel334, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer375->Add(itemStaticLine378, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer379 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer379, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText380 = new ewxStaticText( itemPanel334, wxID_STATIC, _("Indices of Block to Skip:  NX:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer379->Add(itemStaticText380, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl381 = new ewxTextCtrl( itemPanel334, ID_TEXTCTRL_MDPREP_SKIPX, _("1"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer379->Add(itemTextCtrl381, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer379->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText383 = new ewxStaticText( itemPanel334, wxID_STATIC, _("NY:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer379->Add(itemStaticText383, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl384 = new ewxTextCtrl( itemPanel334, ID_TEXTCTRL_MDPREP_SKIPY, _("1"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer379->Add(itemTextCtrl384, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer379->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText386 = new ewxStaticText( itemPanel334, wxID_STATIC, _("NZ:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer379->Add(itemStaticText386, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl387 = new ewxTextCtrl( itemPanel334, ID_TEXTCTRL_MDPREP_SKIPZ, _("1"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer379->Add(itemTextCtrl387, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer388 = new wxBoxSizer(wxHORIZONTAL);
    p_modifyVSizer->Add(itemBoxSizer388, 0, wxALIGN_LEFT|wxALL, 0);
    ewxButton* itemButton389 = new ewxButton( itemPanel334, ID_BUTTON_MDPREP_SKIP, _("Skip"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer388->Add(itemButton389, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook125->AddPage(itemPanel334, _("Modify Cell"));

    itemBoxSizer124->Add(itemNotebook125, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    wxBoxSizer* itemBoxSizer390 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer124->Add(itemBoxSizer390, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer391 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer390->Add(itemBoxSizer391, 1, wxGROW|wxALL, 0);
    ewxStaticText* itemStaticText392 = new ewxStaticText( itemPanel123, wxID_STATIC, _("Command List:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer391->Add(itemStaticText392, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 0);

    wxString* itemListBox393Strings = NULL;
    ewxListBox* itemListBox393 = new ewxListBox( itemPanel123, ID_LISTBOX_MDPREP_CMDS, wxDefaultPosition, wxSize(425, 80), 0, itemListBox393Strings, wxLB_SINGLE );
    itemBoxSizer391->Add(itemListBox393, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    wxBoxSizer* itemBoxSizer394 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer390->Add(itemBoxSizer394, 0, wxALIGN_TOP|wxLEFT, 5);
    wxBitmap itemBitmapButton395Bitmap(itemFrame1->GetBitmapResource(wxT("1uparrow.xpm")));
    ewxBitmapButton* itemBitmapButton395 = new ewxBitmapButton( itemPanel123, ID_BUTTON_MDPREP_UP, itemBitmapButton395Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer394->Add(itemBitmapButton395, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 2);

    wxBitmap itemBitmapButton396Bitmap(itemFrame1->GetBitmapResource(wxT("1downarrow.xpm")));
    ewxBitmapButton* itemBitmapButton396 = new ewxBitmapButton( itemPanel123, ID_BUTTON_MDPREP_DOWN, itemBitmapButton396Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer394->Add(itemBitmapButton396, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxBOTTOM, 2);

    wxBitmap itemBitmapButton397Bitmap(itemFrame1->GetBitmapResource(wxT("delete.xpm")));
    ewxBitmapButton* itemBitmapButton397 = new ewxBitmapButton( itemPanel123, ID_BUTTON_MDPREP_DELETE, itemBitmapButton397Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer394->Add(itemBitmapButton397, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 2);

    itemNotebook3->AddPage(itemPanel123, _("Coordinate Commands"));

    ewxPanel* itemPanel398 = new ewxPanel( itemNotebook3, ID_PANEL_THERMODYNAMICS, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer399 = new wxBoxSizer(wxVERTICAL);
    itemPanel398->SetSizer(itemBoxSizer399);

    ewxPanel* itemPanel400 = new ewxPanel( itemPanel398, ID_PANEL_PMF_OPTION, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel400, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer401 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel400->SetSizer(itemBoxSizer401);

    ewxStaticText* itemStaticText402 = new ewxStaticText( itemPanel400, wxID_STATIC, _("Potential of Mean Force Option:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer401->Add(itemStaticText402, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice403Strings[] = {
        _("zalign"),
        _("xyplane"),
        _("distance"),
        _("zdistance"),
        _("angle"),
        _("torsion"),
        _("basepair"),
        _("zaxis"),
        _("zaxis-cog")
    };
    ewxChoice* itemChoice403 = new ewxChoice( itemPanel400, ID_CHOICE_PMF_OPTION, wxDefaultPosition, wxDefaultSize, 9, itemChoice403Strings, 0 );
    itemChoice403->SetStringSelection(_("zalign"));
    itemBoxSizer401->Add(itemChoice403, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox404 = new ewxCheckBox( itemPanel400, ID_CHECKBOX_PMF_BIAS, _("Bias"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox404->SetValue(false);
    if (ShowToolTips())
        itemCheckBox404->SetToolTip(_("Treat this function as a biasing potential"));
    itemBoxSizer401->Add(itemCheckBox404, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox405 = new ewxCheckBox( itemPanel400, ID_CHECKBOX_PMF_ALL, _("All"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox405->SetValue(false);
    if (ShowToolTips())
        itemCheckBox405->SetToolTip(_("Apply this pmf to each equivalent solute molecule in the system"));
    itemBoxSizer401->Add(itemCheckBox405, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox406 = new ewxCheckBox( itemPanel400, ID_CHECKBOX_PMF_COMBINE, _("Combine"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox406->SetValue(false);
    if (ShowToolTips())
        itemCheckBox406->SetToolTip(_("Apply this pmf to the atoms in all of the equivalent solute molecules"));
    itemBoxSizer401->Add(itemCheckBox406, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox407 = new ewxCheckBox( itemPanel400, ID_CHECKBOX_PMF_CONSTRAINT, _("Constraint"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox407->SetValue(false);
    if (ShowToolTips())
        itemCheckBox407->SetToolTip(_("Treat the distance as a contraint"));
    itemBoxSizer401->Add(itemCheckBox407, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel408 = new ewxPanel( itemPanel398, ID_PANEL_PMF_SELECT_ATOMS1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel408, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer409 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel408->SetSizer(itemBoxSizer409);

    ewxStaticText* itemStaticText410 = new ewxStaticText( itemPanel408, wxID_STATIC_PMF_SELECT_ATOMS1, _("Atom Group 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer409->Add(itemStaticText410, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl411 = new ewxTextCtrl( itemPanel408, ID_TEXTCTRL_PMF_SELECT_ATOMS1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer409->Add(itemTextCtrl411, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText412 = new ewxStaticText( itemPanel408, ID_STATICTEXT_PMF_SELECT_ATOMS1, _("(#)"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemStaticText412->SetToolTip(_("Selection string identifier index."));
    itemBoxSizer409->Add(itemStaticText412, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxButton* itemButton413 = new ewxButton( itemPanel408, ID_BUTTON_PMF_SELECT_ATOMS1, _("Get sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer409->Add(itemButton413, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton414 = new ewxButton( itemPanel408, ID_BUTTON_PMF_SHOW_ATOMS1, _("Show sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer409->Add(itemButton414, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel415 = new ewxPanel( itemPanel398, ID_PANEL_PMF_SELECT_ATOMS2, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel415, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer416 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel415->SetSizer(itemBoxSizer416);

    ewxStaticText* itemStaticText417 = new ewxStaticText( itemPanel415, wxID_STATIC_PMF_SELECT_ATOMS_2, _("Atom Group 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer416->Add(itemStaticText417, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl418 = new ewxTextCtrl( itemPanel415, ID_TEXTCTRL_PMF_SELECT_ATOMS2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer416->Add(itemTextCtrl418, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText419 = new ewxStaticText( itemPanel415, ID_STATICTEXT_PMF_SELECT_ATOMS2, _("(#)"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemStaticText419->SetToolTip(_("Selection string identifier index."));
    itemBoxSizer416->Add(itemStaticText419, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxButton* itemButton420 = new ewxButton( itemPanel415, ID_BUTTON_PMF_SELECT_ATOMS2, _("Get sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer416->Add(itemButton420, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton421 = new ewxButton( itemPanel415, ID_BUTTON_PMF_SHOW_ATOMS2, _("Show sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer416->Add(itemButton421, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel422 = new ewxPanel( itemPanel398, ID_PANEL_PMF_SELECT_ATOMS3, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel422, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer423 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel422->SetSizer(itemBoxSizer423);

    ewxStaticText* itemStaticText424 = new ewxStaticText( itemPanel422, wxID_STATIC_PMF_SELECT_ATOMS3, _("Atom Group 3"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer423->Add(itemStaticText424, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl425 = new ewxTextCtrl( itemPanel422, ID_TEXTCTRL_PMF_SELECT_ATOMS3, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer423->Add(itemTextCtrl425, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText426 = new ewxStaticText( itemPanel422, ID_STATICTEXT_PMF_SELECT_ATOMS3, _("(#)"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemStaticText426->SetToolTip(_("Selection string identifier index."));
    itemBoxSizer423->Add(itemStaticText426, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxButton* itemButton427 = new ewxButton( itemPanel422, ID_BUTTON_PMF_SELECT_ATOMS3, _("Get sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer423->Add(itemButton427, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton428 = new ewxButton( itemPanel422, ID_BUTTON_PMF_SHOW_ATOMS3, _("Show sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer423->Add(itemButton428, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel429 = new ewxPanel( itemPanel398, ID_PANEL_PMF_SELECT_ATOMS4, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel429, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer430 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel429->SetSizer(itemBoxSizer430);

    ewxStaticText* itemStaticText431 = new ewxStaticText( itemPanel429, wxID_STATIC_PMF_SELECT_ATOMS4, _("Atom Group 4"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer430->Add(itemStaticText431, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl432 = new ewxTextCtrl( itemPanel429, ID_TEXTCTRL_PMF_SELECT_ATOMS4, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer430->Add(itemTextCtrl432, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText433 = new ewxStaticText( itemPanel429, ID_STATICTEXT_PMF_SELECT_ATOMS4, _("(#)"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemStaticText433->SetToolTip(_("Selection string identifier index."));
    itemBoxSizer430->Add(itemStaticText433, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxButton* itemButton434 = new ewxButton( itemPanel429, ID_BUTTON_PMF_SELECT_ATOMS4, _("Get sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer430->Add(itemButton434, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton435 = new ewxButton( itemPanel429, ID_BUTTON_PMF_SHOW_ATOMS4, _("Show sel."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer430->Add(itemButton435, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel436 = new ewxPanel( itemPanel398, ID_PANEL_PMF_FORCE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel436, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer437 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel436->SetSizer(itemBoxSizer437);

    ewxStaticText* itemStaticText438 = new ewxStaticText( itemPanel436, wxID_STATIC, _("Force 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer437->Add(itemStaticText438, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl439 = new ewxTextCtrl( itemPanel436, ID_TEXTCTRL_PMF_FORCE1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer437->Add(itemTextCtrl439, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText440 = new ewxStaticText( itemPanel436, wxID_STATIC_PMF_FORCE1_UNIT, _("(kJoule/Mole)/(nm**2)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer437->Add(itemStaticText440, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText441 = new ewxStaticText( itemPanel436, wxID_STATIC, _("Force 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer437->Add(itemStaticText441, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl442 = new ewxTextCtrl( itemPanel436, ID_TEXTCTRL_PMF_FORCE2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer437->Add(itemTextCtrl442, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText443 = new ewxStaticText( itemPanel436, wxID_STATIC_PMF_FORCE2_UNIT, _("(kJoule/Mole)/(nm**2)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer437->Add(itemStaticText443, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel444 = new ewxPanel( itemPanel398, ID_PANEL_PMF_FORCE_DISTANCE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel444, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer445 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel444->SetSizer(itemBoxSizer445);

    ewxStaticText* itemStaticText446 = new ewxStaticText( itemPanel444, wxID_STATIC, _("Force 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer445->Add(itemStaticText446, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl447 = new ewxTextCtrl( itemPanel444, ID_TEXTCTRL_PMF_FORCE1_DISTANCE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer445->Add(itemTextCtrl447, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText448 = new ewxStaticText( itemPanel444, wxID_STATIC_PMF_FORCE1_DISTANCE_UNIT, _("(kJoule/Mole)/(nm**2)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer445->Add(itemStaticText448, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText449 = new ewxStaticText( itemPanel444, wxID_STATIC, _("Force 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer445->Add(itemStaticText449, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl450 = new ewxTextCtrl( itemPanel444, ID_TEXTCTRL_PMF_FORCE2_DISTANCE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer445->Add(itemTextCtrl450, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText451 = new ewxStaticText( itemPanel444, wxID_STATIC_PMF_FORCE2_DISTANCE_UNIT, _("(kJoule/Mole)/(nm**2)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer445->Add(itemStaticText451, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel452 = new ewxPanel( itemPanel398, ID_PANEL_PMF_FORCE_ANGLE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel452, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer453 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel452->SetSizer(itemBoxSizer453);

    ewxStaticText* itemStaticText454 = new ewxStaticText( itemPanel452, wxID_STATIC, _("Force 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer453->Add(itemStaticText454, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl455 = new ewxTextCtrl( itemPanel452, ID_TEXTCTRL_PMF_FORCE1_ANGLE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer453->Add(itemTextCtrl455, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText456 = new ewxStaticText( itemPanel452, wxID_STATIC_PMF_FORCE1_ANGLE_UNIT, _("(kJoule/Mole)/(Radian**2)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer453->Add(itemStaticText456, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText457 = new ewxStaticText( itemPanel452, wxID_STATIC, _("Force 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer453->Add(itemStaticText457, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl458 = new ewxTextCtrl( itemPanel452, ID_TEXTCTRL_PMF_FORCE2_ANGLE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer453->Add(itemTextCtrl458, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText459 = new ewxStaticText( itemPanel452, wxID_STATIC_PMF_FORCE2_ANGLE_UNIT, _("(kJoule/Mole)/(Radian**2)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer453->Add(itemStaticText459, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel460 = new ewxPanel( itemPanel398, ID_PANEL_PMF_FORCE_TORSION, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel460, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer461 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel460->SetSizer(itemBoxSizer461);

    ewxStaticText* itemStaticText462 = new ewxStaticText( itemPanel460, wxID_STATIC, _("Force 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer461->Add(itemStaticText462, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl463 = new ewxTextCtrl( itemPanel460, ID_TEXTCTRL_PMF_FORCE1_TORSION, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer461->Add(itemTextCtrl463, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText464 = new ewxStaticText( itemPanel460, wxID_STATIC_PMF_FORCE1_TORSION_UNIT, _("kJoule/Mol"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer461->Add(itemStaticText464, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText465 = new ewxStaticText( itemPanel460, wxID_STATIC, _("Force 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer461->Add(itemStaticText465, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl466 = new ewxTextCtrl( itemPanel460, ID_TEXTCTRL_PMF_FORCE2_TORSION, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer461->Add(itemTextCtrl466, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText467 = new ewxStaticText( itemPanel460, wxID_STATIC_PMF_FORCE2_TORSION_UNIT, _("kJoule/Mol"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer461->Add(itemStaticText467, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel468 = new ewxPanel( itemPanel398, ID_PANEL_PMF_DISTANCE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel468, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer469 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel468->SetSizer(itemBoxSizer469);

    ewxStaticText* itemStaticText470 = new ewxStaticText( itemPanel468, wxID_STATIC, _("Distance 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer469->Add(itemStaticText470, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl471 = new ewxTextCtrl( itemPanel468, ID_TEXTCTRL_PMF_DISTANCE1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer469->Add(itemTextCtrl471, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText472 = new ewxStaticText( itemPanel468, wxID_STATIC_PMF_DISTANCE1_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer469->Add(itemStaticText472, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText473 = new ewxStaticText( itemPanel468, wxID_STATIC, _("Distance 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer469->Add(itemStaticText473, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl474 = new ewxTextCtrl( itemPanel468, ID_TEXTCTRL_PMF_DISTANCE2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer469->Add(itemTextCtrl474, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText475 = new ewxStaticText( itemPanel468, wxID_STATIC_PMF_DISTANCE2_UNIT, _("nm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer469->Add(itemStaticText475, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxButton* itemButton476 = new ewxButton( itemPanel468, ID_BUTTON_PMF_DEFAULTS_DISTANCE, _("Calc. Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer469->Add(itemButton476, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel477 = new ewxPanel( itemPanel398, ID_PANEL_PMF_ANGLE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer399->Add(itemPanel477, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer478 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel477->SetSizer(itemBoxSizer478);

    ewxStaticText* itemStaticText479 = new ewxStaticText( itemPanel477, wxID_STATIC, _("Angle 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer478->Add(itemStaticText479, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl480 = new ewxTextCtrl( itemPanel477, ID_TEXTCTRL_PMF_ANGLE1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer478->Add(itemTextCtrl480, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText481 = new ewxStaticText( itemPanel477, wxID_STATIC_PMF_ANGLE1_UNIT, _("Radian"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer478->Add(itemStaticText481, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText482 = new ewxStaticText( itemPanel477, wxID_STATIC, _("Angle 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer478->Add(itemStaticText482, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl483 = new ewxTextCtrl( itemPanel477, ID_TEXTCTRL_PMF_ANGLE2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer478->Add(itemTextCtrl483, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText484 = new ewxStaticText( itemPanel477, wxID_STATIC_PMF_ANGLE2_UNIT, _("Radian"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer478->Add(itemStaticText484, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxButton* itemButton485 = new ewxButton( itemPanel477, ID_BUTTON_PMF_DEFAULTS_ANGLE, _("Calc. Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer478->Add(itemButton485, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer399->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer487 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer399->Add(itemBoxSizer487, 0, wxGROW|wxALL, 5);
    itemBoxSizer487->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    ewxButton* itemButton489 = new ewxButton( itemPanel398, ID_BUTTON_PMF_ADD_DIRECTIVE, _("Add PMF Directive"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer487->Add(itemButton489, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer487->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer491 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer399->Add(itemBoxSizer491, 0, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer492 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer491->Add(itemBoxSizer492, 1, wxGROW|wxALL, 0);
    ewxStaticText* itemStaticText493 = new ewxStaticText( itemPanel398, wxID_STATIC, _("PMF Directives:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer492->Add(itemStaticText493, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 0);

    wxString* itemListBox494Strings = NULL;
    ewxListBox* itemListBox494 = new ewxListBox( itemPanel398, ID_LISTBOX_PMF_DIRECTIVES, wxDefaultPosition, wxSize(425, 80), 0, itemListBox494Strings, wxLB_SINGLE );
    itemBoxSizer492->Add(itemListBox494, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    wxBoxSizer* itemBoxSizer495 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer491->Add(itemBoxSizer495, 0, wxALIGN_TOP|wxLEFT, 5);
    wxBitmap itemBitmapButton496Bitmap(itemFrame1->GetBitmapResource(wxT("1uparrow.xpm")));
    ewxBitmapButton* itemBitmapButton496 = new ewxBitmapButton( itemPanel398, ID_BUTTON_PMF_UP, itemBitmapButton496Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer495->Add(itemBitmapButton496, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 2);

    wxBitmap itemBitmapButton497Bitmap(itemFrame1->GetBitmapResource(wxT("1downarrow.xpm")));
    ewxBitmapButton* itemBitmapButton497 = new ewxBitmapButton( itemPanel398, ID_BUTTON_PMF_DOWN, itemBitmapButton497Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer495->Add(itemBitmapButton497, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxBOTTOM, 2);

    wxBitmap itemBitmapButton498Bitmap(itemFrame1->GetBitmapResource(wxT("delete.xpm")));
    ewxBitmapButton* itemBitmapButton498 = new ewxBitmapButton( itemPanel398, ID_BUTTON_PMF_DELETE, itemBitmapButton498Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer495->Add(itemBitmapButton498, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 2);

    itemNotebook3->AddPage(itemPanel398, _("Thermodynamics"));

    ewxPanel* itemPanel499 = new ewxPanel( itemNotebook3, ID_PANEL_MDPREP_DETAILS, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer500 = new wxBoxSizer(wxVERTICAL);
    itemPanel499->SetSizer(itemBoxSizer500);

    wxBoxSizer* itemBoxSizer501 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer500->Add(itemBoxSizer501, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox502 = new ewxCheckBox( itemPanel499, ID_CHECKBOX_MDPREP_SETMEM, _("Set Memory Allocation"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox502->SetValue(true);
    itemBoxSizer501->Add(itemCheckBox502, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer503 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer500->Add(itemBoxSizer503, 0, wxGROW|wxALL, 0);
    ewxPanel* itemPanel504 = new ewxPanel( itemPanel499, ID_PANEL_MDPREP_MEMORY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer503->Add(itemPanel504, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    wxBoxSizer* itemBoxSizer505 = new wxBoxSizer(wxVERTICAL);
    itemPanel504->SetSizer(itemBoxSizer505);

    wxFlexGridSizer* itemFlexGridSizer506 = new wxFlexGridSizer(3, 3, 0, 0);
    itemBoxSizer505->Add(itemFlexGridSizer506, 0, wxALIGN_LEFT|wxLEFT|wxBOTTOM, 15);
    ewxStaticText* itemStaticText507 = new ewxStaticText( itemPanel504, wxID_STATIC, _("Heap:  "), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer506->Add(itemStaticText507, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl508 = new ewxTextCtrl( itemPanel504, ID_TEXTCTRL_MDPREP_MEMHEAP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer506->Add(itemTextCtrl508, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText509 = new ewxStaticText( itemPanel504, wxID_STATIC, _("Mb"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText509->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer506->Add(itemStaticText509, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText510 = new ewxStaticText( itemPanel504, wxID_STATIC, _("Stack: "), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer506->Add(itemStaticText510, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl511 = new ewxTextCtrl( itemPanel504, ID_TEXTCTRL_MDPREP_MEMSTK, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer506->Add(itemTextCtrl511, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText512 = new ewxStaticText( itemPanel504, wxID_STATIC, _("Mb"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText512->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer506->Add(itemStaticText512, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText513 = new ewxStaticText( itemPanel504, wxID_STATIC, _("Global:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer506->Add(itemStaticText513, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl514 = new ewxTextCtrl( itemPanel504, ID_TEXTCTRL_MDPREP_MEMGLBL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer506->Add(itemTextCtrl514, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText515 = new ewxStaticText( itemPanel504, wxID_STATIC, _("Mb"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText515->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer506->Add(itemStaticText515, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer516 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer500->Add(itemBoxSizer516, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText517 = new ewxStaticText( itemPanel499, wxID_STATIC, _("Print Level:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer516->Add(itemStaticText517, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice518Strings[] = {
        _("Low"),
        _("Medium"),
        _("High"),
        _("Debug")
    };
    ewxChoice* itemChoice518 = new ewxChoice( itemPanel499, ID_CHOICE_MDPREP_PRTLEV, wxDefaultPosition, wxDefaultSize, 4, itemChoice518Strings, 0 );
    itemBoxSizer516->Add(itemChoice518, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook3->AddPage(itemPanel499, _("System"));

    itemBoxSizer2->Add(itemNotebook3, 1, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer519 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer519, 0, wxALIGN_LEFT|wxALL, 3);

    ewxButton* itemButton520 = new ewxButton( itemFrame1, ID_BUTTON_MDPREP_BUILDER, _("Builder..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer519->Add(itemButton520, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    itemBoxSizer519->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton522 = new ewxButton( itemFrame1, ID_BUTTON_MDPREP_FINAL_EDIT, _("Final Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer519->Add(itemButton522, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer519->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton524 = new ewxButton( itemFrame1, ID_BUTTON_MDPREP_LOCAL, _("Run Local"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer519->Add(itemButton524, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer519->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton526 = new ewxButton( itemFrame1, ID_BUTTON_MDPREP_REMOTE, _("Run Remote..."), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton526->SetToolTip(_("Remote machine must have been set up with all\nnecessary fragment and force field parameter files\nbefore using this option."));
    itemBoxSizer519->Add(itemButton526, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer519->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton528 = new ewxButton( itemFrame1, ID_BUTTON_MDPREP_RESET, _("Reset All"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton528->SetToolTip(_("Reset all parameters to default values"));
    itemBoxSizer519->Add(itemButton528, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticLine* itemStaticLine529 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer2->Add(itemStaticLine529, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    p_feedbackSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(p_feedbackSizer, 0, wxGROW|wxALL, 0);

    WxFeedback* itemWxFeedback531 = new WxFeedback( itemFrame1, ID_FEEDBACK, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
    p_feedbackSizer->Add(itemWxFeedback531, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

////@end MDPrepGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_MDPREP_FRAME
 */

void MDPrepGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_MDPREP_FRAME in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_MDPREP_FRAME in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CHEMSYS
 */

void MDPrepGUI::OnButtonMdprepChemsysClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CHEMSYS in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CHEMSYS in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CONFIGURE
 */

void MDPrepGUI::OnButtonMdprepConfigureClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CONFIGURE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CONFIGURE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HID
 */

void MDPrepGUI::OnRadiobuttonMdprepHidSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HID in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HID in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIE
 */

void MDPrepGUI::OnRadiobuttonMdprepHieSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIP
 */

void MDPrepGUI::OnRadiobuttonMdprepHipSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SOLVENTS
 */

void MDPrepGUI::OnChoiceMdprepSolventsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SOLVENTS in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SOLVENTS in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_NAM
 */

void MDPrepGUI::OnTextctrlMdprepSolNamEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_NAM in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_NAM in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_MOD
 */

void MDPrepGUI::OnTextctrlMdprepSolModEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_MOD in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_MOD in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_MDPREP_CINAME
 */

void MDPrepGUI::OnComboboxMdprepCinameSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_MDPREP_CINAME in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_MDPREP_CINAME in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_MDPREP_CINAME
 */

void MDPrepGUI::OnComboboxMdprepCinameEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_MDPREP_CINAME in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_MDPREP_CINAME in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CINUM
 */

void MDPrepGUI::OnTextctrlMdprepCinumEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CINUM in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CINUM in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_GETCHARGE
 */

void MDPrepGUI::OnButtonMdprepGetchargeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_GETCHARGE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_GETCHARGE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CISCALE
 */

void MDPrepGUI::OnTextctrlMdprepCiscaleEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CISCALE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CISCALE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIGRIDSZ
 */

void MDPrepGUI::OnTextctrlMdprepCigridszEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIGRIDSZ in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIGRIDSZ in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIMINDIS
 */

void MDPrepGUI::OnTextctrlMdprepCimindisEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIMINDIS in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIMINDIS in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIADD
 */

void MDPrepGUI::OnButtonMdprepCiaddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIADD in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIADD in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIDEL
 */

void MDPrepGUI::OnButtonMdprepCidelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIDEL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIDEL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC1
 */

void MDPrepGUI::OnTextctrlMdprepFrac1Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC1 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC1 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC2
 */

void MDPrepGUI::OnTextctrlMdprepFrac2Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC2 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC2 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC3
 */

void MDPrepGUI::OnTextctrlMdprepFrac3Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC3 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC3 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC4
 */

void MDPrepGUI::OnTextctrlMdprepFrac4Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC4 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC4 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC5
 */

void MDPrepGUI::OnTextctrlMdprepFrac5Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC5 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC5 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC6
 */

void MDPrepGUI::OnTextctrlMdprepFrac6Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC6 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC6 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC7
 */

void MDPrepGUI::OnTextctrlMdprepFrac7Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC7 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC7 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC8
 */

void MDPrepGUI::OnTextctrlMdprepFrac8Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC8 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC8 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC9
 */

void MDPrepGUI::OnTextctrlMdprepFrac9Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC9 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC9 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC10
 */

void MDPrepGUI::OnTextctrlMdprepFrac10Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC10 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC10 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_SELECT
 */

void MDPrepGUI::OnButtonMdprepLnkSelectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_SELECT in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_SELECT in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_ADD
 */

void MDPrepGUI::OnButtonMdprepLnkAddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_ADD in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_ADD in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_DELETE
 */

void MDPrepGUI::OnButtonMdprepLnkDeleteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_DELETE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_DELETE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTER
 */

void MDPrepGUI::OnButtonMdprepCenterClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTER in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTER in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERX
 */

void MDPrepGUI::OnButtonMdprepCenterxClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERX in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERX in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERY
 */

void MDPrepGUI::OnButtonMdprepCenteryClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERY in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERY in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERZ
 */

void MDPrepGUI::OnButtonMdprepCenterzClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERZ in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERZ in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_MOL
 */

void MDPrepGUI::OnRadiobuttonMdprepTrnsMolSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_MOL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_MOL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_SEG
 */

void MDPrepGUI::OnRadiobuttonMdprepTrnsSegSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_SEG in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_SEG in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_ATOM
 */

void MDPrepGUI::OnRadiobuttonMdprepTrnsAtomSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_ATOM in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_ATOM in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_X
 */

void MDPrepGUI::OnTextctrlMdprepTrnsXEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_X in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_X in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Y
 */

void MDPrepGUI::OnTextctrlMdprepTrnsYEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Y in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Y in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Z
 */

void MDPrepGUI::OnTextctrlMdprepTrnsZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Z in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Z in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_TRNS_SOL
 */

void MDPrepGUI::OnButtonMdprepTrnsSolClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_TRNS_SOL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_TRNS_SOL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORIENT
 */

void MDPrepGUI::OnButtonMdprepOrientClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORIENT in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORIENT in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ALIGN
 */

void MDPrepGUI::OnButtonMdprepAlignClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ALIGN in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ALIGN in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORNTSL
 */

void MDPrepGUI::OnButtonMdprepOrntslClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORNTSL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORNTSL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_MOL
 */

void MDPrepGUI::OnRadiobuttonMdprepOrntMolSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_MOL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_MOL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_SEG
 */

void MDPrepGUI::OnRadiobuttonMdprepOrntSegSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_SEG in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_SEG in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_ATOM
 */

void MDPrepGUI::OnRadiobuttonMdprepOrntAtomSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_ATOM in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_ATOM in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_ANGLE
 */

void MDPrepGUI::OnTextctrlMdprepOrntAngleEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_ANGLE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_ANGLE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_X
 */

void MDPrepGUI::OnTextctrlMdprepOrntXEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_X in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_X in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Y
 */

void MDPrepGUI::OnTextctrlMdprepOrntYEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Y in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Y in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Z
 */

void MDPrepGUI::OnTextctrlMdprepOrntZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Z in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Z in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ROTATE
 */

void MDPrepGUI::OnButtonMdprepRotateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ROTATE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ROTATE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETX
 */

void MDPrepGUI::OnTextctrlMdprepSetxEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETX in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETX in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETY
 */

void MDPrepGUI::OnTextctrlMdprepSetyEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETY in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETY in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETZ
 */

void MDPrepGUI::OnTextctrlMdprepSetzEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETZ in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETZ in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETDIM
 */

void MDPrepGUI::OnButtonMdprepSetdimClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETDIM in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETDIM in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETBC
 */

void MDPrepGUI::OnButtonMdprepSetbcClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETBC in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETBC in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_BCOPT
 */

void MDPrepGUI::OnChoiceMdprepBcoptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_BCOPT in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_BCOPT in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SLV_GEOM
 */

void MDPrepGUI::OnChoiceMdprepSlvGeomSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SLV_GEOM in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SLV_GEOM in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLV_MINDIST
 */

void MDPrepGUI::OnTextctrlMdprepSlvMindistEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLV_MINDIST in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLV_MINDIST in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SOLVATE
 */

void MDPrepGUI::OnButtonMdprepSolvateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SOLVATE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SOLVATE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_MINSLV
 */

void MDPrepGUI::OnButtonMdprepSetMinslvClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_MINSLV in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_MINSLV in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINSOLV
 */

void MDPrepGUI::OnTextctrlMdprepMinsolvEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINSOLV in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINSOLV in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_EXPND
 */

void MDPrepGUI::OnButtonMdprepSetExpndClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_EXPND in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_EXPND in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_EXPND
 */

void MDPrepGUI::OnTextctrlMdprepExpndEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_EXPND in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_EXPND in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_SLVTOL
 */

void MDPrepGUI::OnButtonMdprepSetSlvtolClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_SLVTOL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_SLVTOL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLVTOL
 */

void MDPrepGUI::OnTextctrlMdprepSlvtolEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLVTOL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLVTOL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_RMV_OPT
 */

void MDPrepGUI::OnChoiceMdprepRmvOptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_RMV_OPT in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_RMV_OPT in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_XRANGE
 */

void MDPrepGUI::OnCheckboxMdprepXrangeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_XRANGE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_XRANGE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_YRANGE
 */

void MDPrepGUI::OnCheckboxMdprepYrangeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_YRANGE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_YRANGE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_ZRANGE
 */

void MDPrepGUI::OnCheckboxMdprepZrangeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_ZRANGE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_ZRANGE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SLV_RMV
 */

void MDPrepGUI::OnButtonMdprepSlvRmvClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SLV_RMV in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SLV_RMV in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_REPOPTS
 */

void MDPrepGUI::OnChoiceMdprepRepoptsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_REPOPTS in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_REPOPTS in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_XREP
 */

void MDPrepGUI::OnTextctrlMdprepXrepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_XREP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_XREP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_YREP
 */

void MDPrepGUI::OnTextctrlMdprepYrepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_YREP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_YREP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ZREP
 */

void MDPrepGUI::OnTextctrlMdprepZrepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ZREP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ZREP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINRSEP
 */

void MDPrepGUI::OnTextctrlMdprepMinrsepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINRSEP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINRSEP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANX
 */

void MDPrepGUI::OnCheckboxMdprepRanxClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANX in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANX in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANY
 */

void MDPrepGUI::OnCheckboxMdprepRanyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANY in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANY in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANZ
 */

void MDPrepGUI::OnCheckboxMdprepRanzClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANZ in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANZ in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_INVCPY
 */

void MDPrepGUI::OnCheckboxMdprepInvcpyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_INVCPY in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_INVCPY in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_USEZBOX
 */

void MDPrepGUI::OnCheckboxMdprepUsezboxClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_USEZBOX in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_USEZBOX in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REPLIC
 */

void MDPrepGUI::OnButtonMdprepReplicClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REPLIC in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REPLIC in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPX
 */

void MDPrepGUI::OnTextctrlMdprepSkipxEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPX in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPX in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPY
 */

void MDPrepGUI::OnTextctrlMdprepSkipyEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPY in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPY in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPZ
 */

void MDPrepGUI::OnTextctrlMdprepSkipzEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPZ in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPZ in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SKIP
 */

void MDPrepGUI::OnButtonMdprepSkipClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SKIP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SKIP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_UP
 */

void MDPrepGUI::OnButtonMdprepUpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_UP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_UP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DOWN
 */

void MDPrepGUI::OnButtonMdprepDownClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DOWN in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DOWN in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DELETE
 */

void MDPrepGUI::OnButtonMdprepDeleteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DELETE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DELETE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PMF_OPTION
 */

void MDPrepGUI::OnChoicePmfOptionSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PMF_OPTION in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PMF_OPTION in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS1
 */

void MDPrepGUI::OnTextctrlPmfSelectAtoms1Updated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS1 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS1 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS1
 */

void MDPrepGUI::OnButtonPmfSelectAtoms1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS1 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS1 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS1
 */

void MDPrepGUI::OnButtonPmfShowAtoms1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS1 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS1 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS2
 */

void MDPrepGUI::OnTextctrlPmfSelectAtoms2Updated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS2 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS2 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS2
 */

void MDPrepGUI::OnButtonPmfSelectAtoms2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS2 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS2 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS2
 */

void MDPrepGUI::OnButtonPmfShowAtoms2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS2 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS2 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS3
 */

void MDPrepGUI::OnTextctrlPmfSelectAtoms3Updated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS3 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS3 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS3
 */

void MDPrepGUI::OnButtonPmfSelectAtoms3Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS3 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS3 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS3
 */

void MDPrepGUI::OnButtonPmfShowAtoms3Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS3 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS3 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS4
 */

void MDPrepGUI::OnTextctrlPmfSelectAtoms4Updated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS4 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS4 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS4
 */

void MDPrepGUI::OnButtonPmfSelectAtoms4Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS4 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS4 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS4
 */

void MDPrepGUI::OnButtonPmfShowAtoms4Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS4 in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS4 in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_DISTANCE
 */

void MDPrepGUI::OnButtonPmfDefaultsDistanceClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_DISTANCE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_DISTANCE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_ANGLE
 */

void MDPrepGUI::OnButtonPmfDefaultsAngleClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_ANGLE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_ANGLE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_ADD_DIRECTIVE
 */

void MDPrepGUI::OnButtonPmfAddDirectiveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_ADD_DIRECTIVE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_ADD_DIRECTIVE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PMF_DIRECTIVES
 */

void MDPrepGUI::OnListboxPmfDirectivesSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PMF_DIRECTIVES in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PMF_DIRECTIVES in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_UP
 */

void MDPrepGUI::OnButtonPmfUpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_UP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_UP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DOWN
 */

void MDPrepGUI::OnButtonPmfDownClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DOWN in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DOWN in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DELETE
 */

void MDPrepGUI::OnButtonPmfDeleteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DELETE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DELETE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_SETMEM
 */

void MDPrepGUI::OnCheckboxMdprepSetmemClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_SETMEM in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_SETMEM in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMHEAP
 */

void MDPrepGUI::OnTextctrlMdprepMemheapEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMHEAP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMHEAP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMSTK
 */

void MDPrepGUI::OnTextctrlMdprepMemstkEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMSTK in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMSTK in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMGLBL
 */

void MDPrepGUI::OnTextctrlMdprepMemglblEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMGLBL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMGLBL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_PRTLEV
 */

void MDPrepGUI::OnChoiceMdprepPrtlevSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_PRTLEV in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_PRTLEV in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_BUILDER
 */

void MDPrepGUI::OnButtonMdprepBuilderClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_BUILDER in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_BUILDER in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_FINAL_EDIT
 */

void MDPrepGUI::OnButtonMdprepFinalEditClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_FINAL_EDIT in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_FINAL_EDIT in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LOCAL
 */

void MDPrepGUI::OnButtonMdprepLocalClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LOCAL in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LOCAL in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REMOTE
 */

void MDPrepGUI::OnButtonMdprepRemoteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REMOTE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REMOTE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_RESET
 */

void MDPrepGUI::OnButtonMdprepResetClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_RESET in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_RESET in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void MDPrepGUI::OnSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void MDPrepGUI::OnExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP
 */

void MDPrepGUI::OnMenuMdprepHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP in MDPrepGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
 */

void MDPrepGUI::OnMenuFeedbackClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in MDPrepGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in MDPrepGUI. 
}

/*!
 * Should we show tooltips?
 */

bool MDPrepGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MDPrepGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MDPrepGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("1uparrow.xpm"))
    {
        ewxBitmap bitmap(_T("1uparrow.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("1downarrow.xpm"))
    {
        ewxBitmap bitmap(_T("1downarrow.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("delete.xpm"))
    {
        ewxBitmap bitmap(_T("delete.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    return wxNullBitmap;
////@end MDPrepGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MDPrepGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MDPrepGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MDPrepGUI icon retrieval
}
