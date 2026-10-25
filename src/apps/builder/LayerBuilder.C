#include "wx/wxprec.h"
#include "wx/wx.h"
#include <wx/sizer.h>
#include <wx/statbox.h>

#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxTextCtrl.H"
#include "viz/SGContainer.H"
#include "viz/PBCLayerCmd.H"

#include "LayerBuilder.H"

const wxWindowID LayerBuilder::ID_SPIN_LAYER_N = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_SPACING = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_SHIFTA = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_SHIFTB = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_ION1SYM = wxNewId();
const wxWindowID LayerBuilder::ID_SPIN_LAYER_ION1N = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_ION1Q = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_ION1MIN = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_ION2SYM = wxNewId();
const wxWindowID LayerBuilder::ID_SPIN_LAYER_ION2N = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_ION2Q = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_ION2MIN = wxNewId();
const wxWindowID LayerBuilder::ID_SPIN_LAYER_WATER = wxNewId();
const wxWindowID LayerBuilder::ID_TEXT_LAYER_WATERMIN = wxNewId();
const wxWindowID LayerBuilder::ID_SPIN_LAYER_SEED = wxNewId();
const wxWindowID LayerBuilder::ID_BUTTON_LAYER_BUILD = wxNewId();


LayerBuilder::LayerBuilder(wxWindow *parent, wxWindowID id)
  : wxPanel(parent, id), WxVizTool()
{
  wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
  SetSizer(top);
  wxBoxSizer* layerSizer = new wxBoxSizer(wxVERTICAL);
  top->Add(layerSizer, 0, wxGROW|wxALL, 4);

  wxWindow* lp = this;
  layerSizer->Add(new ewxStaticText(lp, wxID_STATIC,
      _("Stacks the structure and its cell as one layer.")), 0, wxALL, 3);
  wxFlexGridSizer* lg = new wxFlexGridSizer(0, 4, 2, 4);
  layerSizer->Add(lg, 0, wxGROW|wxALL, 2);
  ewxStaticText* t;
  ewxTextCtrl* tc;
  ewxSpinCtrl* sp;
  t = new ewxStaticText(lp, wxID_STATIC, _("Layers:"));
  lg->Add(t, 0, wxALIGN_CENTER_VERTICAL);
  sp = new ewxSpinCtrl(lp, ID_SPIN_LAYER_N, _("2"), wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 1, 100, 2);
  sp->SetToolTip(_("Number of layers in the new cell"));
  lg->Add(sp, 0, wxALIGN_CENTER_VERTICAL);
  t = new ewxStaticText(lp, wxID_STATIC, wxString::FromUTF8("Spacing (Å):"));
  lg->Add(t, 0, wxALIGN_CENTER_VERTICAL);
  tc = new ewxTextCtrl(lp, ID_TEXT_LAYER_SPACING, _T(""), wxDefaultPosition, wxSize(55, -1), 0);
  tc->SetToolTip(_("Distance from one layer to the next, measured along the normal of the a-b plane; the new c is Layers x Spacing"));
  lg->Add(tc, 0, wxALIGN_CENTER_VERTICAL);
  t = new ewxStaticText(lp, wxID_STATIC, _("Shift a:"));
  lg->Add(t, 0, wxALIGN_CENTER_VERTICAL);
  tc = new ewxTextCtrl(lp, ID_TEXT_LAYER_SHIFTA, _T("0"), wxDefaultPosition, wxSize(55, -1), 0);
  tc->SetToolTip(_("Lateral shift of each layer relative to the one below, as a fraction of a"));
  lg->Add(tc, 0, wxALIGN_CENTER_VERTICAL);
  t = new ewxStaticText(lp, wxID_STATIC, _("Shift b:"));
  lg->Add(t, 0, wxALIGN_CENTER_VERTICAL);
  tc = new ewxTextCtrl(lp, ID_TEXT_LAYER_SHIFTB, _T("0"), wxDefaultPosition, wxSize(55, -1), 0);
  tc->SetToolTip(_("Lateral shift of each layer relative to the one below, as a fraction of b"));
  lg->Add(tc, 0, wxALIGN_CENTER_VERTICAL);

  layerSizer->Add(new ewxStaticText(lp, wxID_STATIC, _("Between the layers, per gap:")), 0, wxLEFT|wxTOP, 3);
  wxFlexGridSizer* ig = new wxFlexGridSizer(0, 8, 2, 3);
  layerSizer->Add(ig, 0, wxGROW|wxALL, 2);
  const wxWindowID ionIds[2][4] = {
      {ID_TEXT_LAYER_ION1SYM, ID_SPIN_LAYER_ION1N, ID_TEXT_LAYER_ION1Q, ID_TEXT_LAYER_ION1MIN},
      {ID_TEXT_LAYER_ION2SYM, ID_SPIN_LAYER_ION2N, ID_TEXT_LAYER_ION2Q, ID_TEXT_LAYER_ION2MIN}};
  for (int r = 0; r < 2; r++) {
    ig->Add(new ewxStaticText(lp, wxID_STATIC, _("Ion:")), 0, wxALIGN_CENTER_VERTICAL);
    tc = new ewxTextCtrl(lp, ionIds[r][0], _T(""), wxDefaultPosition, wxSize(40, -1), 0);
    tc->SetToolTip(_("Element symbol, e.g. Na"));
    ig->Add(tc, 0, wxALIGN_CENTER_VERTICAL);
    ig->Add(new ewxStaticText(lp, wxID_STATIC, _("n:")), 0, wxALIGN_CENTER_VERTICAL);
    sp = new ewxSpinCtrl(lp, ionIds[r][1], _("0"), wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 0, 10000, 0);
    sp->SetToolTip(_("Ions of this element in each gap"));
    ig->Add(sp, 0, wxALIGN_CENTER_VERTICAL);
    ig->Add(new ewxStaticText(lp, wxID_STATIC, _("q:")), 0, wxALIGN_CENTER_VERTICAL);
    tc = new ewxTextCtrl(lp, ionIds[r][2], _T("1"), wxDefaultPosition, wxSize(35, -1), 0);
    tc->SetToolTip(_("Charge of the ion, stored as the atom charge"));
    ig->Add(tc, 0, wxALIGN_CENTER_VERTICAL);
    ig->Add(new ewxStaticText(lp, wxID_STATIC, wxString::FromUTF8("min Å:")), 0, wxALIGN_CENTER_VERTICAL);
    tc = new ewxTextCtrl(lp, ionIds[r][3], _T("3.5"), wxDefaultPosition, wxSize(40, -1), 0);
    tc->SetToolTip(_("Smallest distance to any atom already there"));
    ig->Add(tc, 0, wxALIGN_CENTER_VERTICAL);
  }
  wxFlexGridSizer* wg = new wxFlexGridSizer(0, 6, 2, 3);
  layerSizer->Add(wg, 0, wxGROW|wxALL, 2);
  wg->Add(new ewxStaticText(lp, wxID_STATIC, _("Water:")), 0, wxALIGN_CENTER_VERTICAL);
  sp = new ewxSpinCtrl(lp, ID_SPIN_LAYER_WATER, _("0"), wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 0, 100000, 0);
  sp->SetToolTip(_("Water molecules in each gap"));
  wg->Add(sp, 0, wxALIGN_CENTER_VERTICAL);
  wg->Add(new ewxStaticText(lp, wxID_STATIC, wxString::FromUTF8("min Å:")), 0, wxALIGN_CENTER_VERTICAL);
  tc = new ewxTextCtrl(lp, ID_TEXT_LAYER_WATERMIN, _T("2.0"), wxDefaultPosition, wxSize(40, -1), 0);
  tc->SetToolTip(_("Smallest distance from a water oxygen to any atom already there"));
  wg->Add(tc, 0, wxALIGN_CENTER_VERTICAL);
  wg->Add(new ewxStaticText(lp, wxID_STATIC, _("Seed:")), 0, wxALIGN_CENTER_VERTICAL);
  sp = new ewxSpinCtrl(lp, ID_SPIN_LAYER_SEED, _("1"), wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 0, 1000000, 1);
  sp->SetToolTip(_("The same seed gives the same placement of ions and water"));
  wg->Add(sp, 0, wxALIGN_CENTER_VERTICAL);
  ewxButton* layerBtn = new ewxButton(lp, ID_BUTTON_LAYER_BUILD, _("Build Layers"));
  layerBtn->SetToolTip(_("Stack the current structure and cell as one layer"));
  layerSizer->Add(layerBtn, 0, wxALIGN_LEFT|wxALL, 3);

  Bind(wxEVT_BUTTON, &LayerBuilder::OnButtonLayerBuild, this, ID_BUTTON_LAYER_BUILD);
  top->SetSizeHints(this);
}


void LayerBuilder::OnButtonLayerBuild(wxCommandEvent& event)
{
   auto num = [this](wxWindowID id, double &v) {
      ewxTextCtrl *t = (ewxTextCtrl*)FindWindow(id);
      return t->GetValue().Trim().ToDouble(&v);
   };
   double spacing = 0, sa = 0, sb = 0, wmin = 0, q[2] = {0, 0}, imin[2] = {0, 0};
   if (!num(ID_TEXT_LAYER_SPACING, spacing) || spacing <= 0.0) {
      wxLogWarning(wxString::FromUTF8("Enter the interlayer spacing in Å: the distance from one layer to the next."));
      return;
   }
   if (!num(ID_TEXT_LAYER_SHIFTA, sa) || !num(ID_TEXT_LAYER_SHIFTB, sb) ||
       !num(ID_TEXT_LAYER_WATERMIN, wmin) ||
       !num(ID_TEXT_LAYER_ION1Q, q[0]) || !num(ID_TEXT_LAYER_ION1MIN, imin[0]) ||
       !num(ID_TEXT_LAYER_ION2Q, q[1]) || !num(ID_TEXT_LAYER_ION2MIN, imin[1])) {
      wxLogWarning("A shift, charge or minimum distance is not a number.");
      return;
   }
   wxBusyCursor busy;
   //  The layer command never auto-replicates, so it needs none of
   //  PBC::execute's replication handling.
   PBCCmd *cmd = new PBCLayerCmd("Build layers", &getFW().getSceneGraph(), 0);
   cmd->getParameter("layers")->setInteger(((ewxSpinCtrl*)FindWindow(ID_SPIN_LAYER_N))->GetValue());
   cmd->getParameter("spacing")->setDouble(spacing);
   cmd->getParameter("shiftA")->setDouble(sa);
   cmd->getParameter("shiftB")->setDouble(sb);
   const wxWindowID sym[2] = {ID_TEXT_LAYER_ION1SYM, ID_TEXT_LAYER_ION2SYM};
   const wxWindowID cnt[2] = {ID_SPIN_LAYER_ION1N, ID_SPIN_LAYER_ION2N};
   for (int i = 0; i < 2; i++) {
      std::string p = i == 0 ? "ion1" : "ion2";
      cmd->getParameter(p + "sym")->setString(
         ((ewxTextCtrl*)FindWindow(sym[i]))->GetValue().Trim().ToStdString());
      cmd->getParameter(p + "n")->setInteger(((ewxSpinCtrl*)FindWindow(cnt[i]))->GetValue());
      cmd->getParameter(p + "q")->setDouble(q[i]);
      cmd->getParameter(p + "min")->setDouble(imin[i]);
   }
   cmd->getParameter("water")->setInteger(((ewxSpinCtrl*)FindWindow(ID_SPIN_LAYER_WATER))->GetValue());
   cmd->getParameter("waterMin")->setDouble(wmin);
   cmd->getParameter("seed")->setInteger(((ewxSpinCtrl*)FindWindow(ID_SPIN_LAYER_SEED))->GetValue());
   getFW().execute(cmd);
}
