#include <algorithm>
#include <cmath>
#include <cstdio>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include "tdat/LatticeDef.H"
#include "tdat/TAtm.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "ClipPlanePanel.H"

ClipPlanePanel::ClipPlanePanel(wxWindow *parent,
                               std::function<SGContainer*()> getSG)
  : wxPanel(parent, wxID_ANY), p_getSG(getSG)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  p_onBox = new wxCheckBox(this, wxID_ANY, _("Clip"));
  p_onBox->SetToolTip(_("Remove everything on one side of a plane"));
  row->Add(p_onBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  p_dirChoice = new wxChoice(this, wxID_ANY);
  p_dirChoice->Append(_("Plane normal to x"));
  p_dirChoice->Append(_("Plane normal to y"));
  p_dirChoice->Append(_("Plane normal to z"));
  p_dirChoice->Append(_("Miller plane (h k l)"));
  p_dirChoice->Append(_("Plane of three selected atoms"));
  p_dirChoice->SetSelection(0);
  row->Add(p_dirChoice, 1, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  top->Add(row, 0, wxEXPAND);

  wxBoxSizer *hkl = new wxBoxSizer(wxHORIZONTAL);
  p_hs = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxSize(60, -1),
                        wxSP_ARROW_KEYS, -10, 10, 1);
  p_ks = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(60, -1),
                        wxSP_ARROW_KEYS, -10, 10, 0);
  p_ls = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(60, -1),
                        wxSP_ARROW_KEYS, -10, 10, 0);
  hkl->Add(new wxStaticText(this, wxID_ANY, "h"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  hkl->Add(p_hs, 0, wxALL, 3);
  hkl->Add(new wxStaticText(this, wxID_ANY, "k"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  hkl->Add(p_ks, 0, wxALL, 3);
  hkl->Add(new wxStaticText(this, wxID_ANY, "l"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  hkl->Add(p_ls, 0, wxALL, 3);
  top->Add(hkl, 0);

  wxBoxSizer *off = new wxBoxSizer(wxHORIZONTAL);
  off->Add(new wxStaticText(this, wxID_ANY, _("Offset")), 0,
           wxALIGN_CENTER_VERTICAL | wxALL, 3);
  p_slider = new wxSlider(this, wxID_ANY, 500, 0, 1000);
  off->Add(p_slider, 1, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  p_offsetText = new wxStaticText(this, wxID_ANY, "0.00 A", wxDefaultPosition,
                                  wxSize(70, -1));
  off->Add(p_offsetText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  p_flipButton = new wxButton(this, wxID_ANY, _("Flip side"));
  off->Add(p_flipButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
  top->Add(off, 0, wxEXPAND);

  p_note = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_note, 0, wxEXPAND | wxALL, 3);
  SetSizer(top);

  p_onBox->Bind(wxEVT_CHECKBOX, &ClipPlanePanel::OnOn, this);
  p_dirChoice->Bind(wxEVT_CHOICE, &ClipPlanePanel::OnDirection, this);
  p_hs->Bind(wxEVT_SPINCTRL, &ClipPlanePanel::OnHKL, this);
  p_ks->Bind(wxEVT_SPINCTRL, &ClipPlanePanel::OnHKL, this);
  p_ls->Bind(wxEVT_SPINCTRL, &ClipPlanePanel::OnHKL, this);
  p_slider->Bind(wxEVT_SLIDER, &ClipPlanePanel::OnSlider, this);
  p_flipButton->Bind(wxEVT_BUTTON, &ClipPlanePanel::OnFlip, this);
  syncControls();
}

//  The unit normal, the reference point's projection on it, and the range of
//  the atoms' projections about it.  False (with p_problem set) if there is
//  no plane to be had.
bool ClipPlanePanel::plane(double n[3], double& ref, double& lo, double& hi)
{
  p_problem.clear();
  SGContainer *sg = p_getSG ? p_getSG() : 0;
  SGFragment *frag = sg ? sg->getFragment() : 0;
  if (!frag || frag->numAtoms() == 0) {
    p_problem = "no structure";
    return false;
  }
  const int count = (int)frag->numAtoms();
  double centre[3] = {0, 0, 0};
  n[0] = n[1] = n[2] = 0;
  switch (p_dir) {
    case DIR_X: n[0] = 1; break;
    case DIR_Y: n[1] = 1; break;
    case DIR_Z: n[2] = 1; break;
    case DIR_HKL: {
      LatticeDef *lat = frag->getLattice();
      if (!lat) { p_problem = "no unit cell: a Miller plane needs one"; return false; }
      if (p_h == 0 && p_k == 0 && p_l == 0) { p_problem = "h k l are all zero"; return false; }
      MPoint v = lat->getPlaneNormal(p_h, p_k, p_l);
      n[0] = v.x(); n[1] = v.y(); n[2] = v.z();
      break;
    }
    case DIR_ATOMS: {
      double p[3][3];
      for (int i = 0; i < 3; i++) {
        if (p_atoms[i] < 1 || p_atoms[i] > count) {
          p_problem = "select three atoms first";
          return false;
        }
        const double *c = frag->atomRef(p_atoms[i] - 1)->coordinates();
        for (int j = 0; j < 3; j++) { p[i][j] = c[j]; centre[j] += c[j] / 3.0; }
      }
      double u[3], w[3];
      for (int j = 0; j < 3; j++) { u[j] = p[1][j] - p[0][j]; w[j] = p[2][j] - p[0][j]; }
      n[0] = u[1] * w[2] - u[2] * w[1];
      n[1] = u[2] * w[0] - u[0] * w[2];
      n[2] = u[0] * w[1] - u[1] * w[0];
      break;
    }
  }
  const double len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
  if (len < 1e-8) { p_problem = "the three atoms are on a line"; return false; }
  for (int j = 0; j < 3; j++) n[j] /= len;

  lo = 1e30; hi = -1e30;
  for (int i = 0; i < count; i++) {
    const double *c = frag->atomRef(i)->coordinates();
    const double d = c[0] * n[0] + c[1] * n[1] + c[2] * n[2];
    lo = std::min(lo, d);
    hi = std::max(hi, d);
  }
  ref = p_dir == DIR_ATOMS
      ? centre[0] * n[0] + centre[1] * n[1] + centre[2] * n[2]
      : 0.5 * (lo + hi);
  return true;
}

void ClipPlanePanel::apply()
{
  SGContainer *sg = p_getSG ? p_getSG() : 0;
  if (!sg) return;
  double n[3], ref = 0, lo = 0, hi = 0;
  if (!plane(n, ref, lo, hi)) {
    if (sg->clipPlaneOn()) sg->setClipPlane(false, SbVec3f(1, 0, 0), 0);
    syncControls();
    return;
  }
  const double d = ref + p_offset;
  const double s = p_flip ? -1.0 : 1.0;
  sg->setClipPlane(p_on, SbVec3f(s * n[0], s * n[1], s * n[2]), s * d);
  syncControls();
}

void ClipPlanePanel::syncControls()
{
  double n[3], ref = 0, lo = 0, hi = 0;
  const bool ok = plane(n, ref, lo, hi);
  p_onBox->SetValue(p_on);
  p_dirChoice->SetSelection((int)p_dir);
  p_hs->Enable(p_dir == DIR_HKL);
  p_ks->Enable(p_dir == DIR_HKL);
  p_ls->Enable(p_dir == DIR_HKL);
  p_hs->SetValue(p_h);
  p_ks->SetValue(p_k);
  p_ls->SetValue(p_l);
  p_slider->Enable(ok);
  if (ok) {
    //  The slider spans the structure, a little beyond each end.
    const double a = lo - ref - 2.0, b = hi - ref + 2.0;
    p_offset = std::max(a, std::min(b, p_offset));
    p_slider->SetValue((int)std::lround(1000.0 * (p_offset - a) / (b - a)));
  }
  p_offsetText->SetLabel(wxString::Format("%.2f A", p_offset));
  p_note->SetLabel(wxString::FromUTF8(p_problem.c_str()));
}

void ClipPlanePanel::setOn(bool on) { p_on = on; apply(); }
void ClipPlanePanel::setDirection(Direction dir) { p_dir = dir; apply(); }
void ClipPlanePanel::setHKL(int h, int k, int l)
{
  p_h = h; p_k = k; p_l = l;
  p_dir = DIR_HKL;
  apply();
}
bool ClipPlanePanel::setAtoms(int a, int b, int c)
{
  p_atoms[0] = a; p_atoms[1] = b; p_atoms[2] = c;
  p_dir = DIR_ATOMS;
  apply();
  return p_problem.empty();
}
void ClipPlanePanel::setOffset(double angstrom) { p_offset = angstrom; apply(); }
void ClipPlanePanel::flip() { p_flip = !p_flip; apply(); }
void ClipPlanePanel::refresh() { apply(); }

std::string ClipPlanePanel::describe() const
{
  char buf[200];
  snprintf(buf, sizeof buf, "on %d dir %d flip %d offset %.3f problem [%s]",
           p_on ? 1 : 0, (int)p_dir, p_flip ? 1 : 0, p_offset, p_problem.c_str());
  return buf;
}

void ClipPlanePanel::OnOn(wxCommandEvent&) { setOn(p_onBox->GetValue()); }

void ClipPlanePanel::OnDirection(wxCommandEvent&)
{
  const Direction dir = (Direction)p_dirChoice->GetSelection();
  if (dir == DIR_ATOMS) {
    SGContainer *sg = p_getSG ? p_getSG() : 0;
    SGFragment *frag = sg ? sg->getFragment() : 0;
    if (frag && frag->m_atomHighLight.size() >= 3)
      for (int i = 0; i < 3; i++) p_atoms[i] = frag->m_atomHighLight[i] + 1;
    else
      p_atoms[0] = p_atoms[1] = p_atoms[2] = 0;
  }
  p_offset = 0.0;
  setDirection(dir);
}

void ClipPlanePanel::OnHKL(wxSpinEvent&)
{
  setHKL(p_hs->GetValue(), p_ks->GetValue(), p_ls->GetValue());
}

void ClipPlanePanel::OnSlider(wxCommandEvent&)
{
  double n[3], ref = 0, lo = 0, hi = 0;
  if (!plane(n, ref, lo, hi)) return;
  const double a = lo - ref - 2.0, b = hi - ref + 2.0;
  setOffset(a + (b - a) * p_slider->GetValue() / 1000.0);
}

void ClipPlanePanel::OnFlip(wxCommandEvent&) { flip(); }
