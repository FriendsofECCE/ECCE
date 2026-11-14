#include <cmath>
#include <cstdio>
#include <memory>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "dsm/IPropCalculation.H"
#include "tdat/PropVector.H"
#include "tdat/LatticeDef.H"
#include "util/StressTensor.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/VRVector.H"
#include "wxviz/WxVizToolFW.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoAnnotation.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoFont.H"
#include "inv/nodes/SoText2.H"
#include "inv/nodes/SoCallback.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/elements/SoViewVolumeElement.H"
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/misc/SoState.H"
#include "StressPanel.H"

IMPLEMENT_DYNAMIC_CLASS(StressPanel, VizPropertyPanel)

namespace {
// An axis pointing at the viewer projects its tip onto the middle of the
// cell, so the label would sit on the atoms. Move such a label clear of the cell, down and to
// the right in view space at render time, since the camera keeps moving.
struct LabelNudge { SoCallback *node; SbVec3f axis; float span; };

void nudgeLabel(void *data, SoAction *action)
{
  if (!action->isOfType(SoGLRenderAction::getClassTypeId())) return;
  auto *n=static_cast<LabelNudge*>(data);
  SoState *state=action->getState();
  const SbViewVolume &vv=SoViewVolumeElement::get(state);
  SbVec3f up=vv.getViewUp(), right=vv.getProjectionDirection().cross(up);
  up.normalize(); right.normalize();
  const float sx=n->axis.dot(right), sy=n->axis.dot(up);
  if (std::sqrt(sx*sx+sy*sy)>0.5f) return;
  SoModelMatrixElement::translateBy(state,n->node,
      0.8f*n->span*right-0.15f*n->span*up);
}
}

bool StressPanel::Create(IPropCalculation *calculation, wxWindow *parent,
    wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
    const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation,parent,id,pos,size,style,name))
    return false;
  SetSizer(new wxBoxSizer(wxVERTICAL));
  auto *heading = new wxStaticText(this, wxID_ANY,
      "Final stress: Cartesian tensor (GPa)");
  GetSizer()->Add(heading, 0, wxALL, 3);
  p_grid = new wxGrid(this, wxID_ANY);
  p_grid->CreateGrid(3,3);
  p_grid->EnableEditing(false);
  p_grid->SetRowLabelSize(25);
  for (int i=0; i<3; ++i) {
    wxString label = wxString::Format("%c", 'X'+i);
    p_grid->SetRowLabelValue(i,label);
    p_grid->SetColLabelValue(i,label);
  }
  p_grid->SetMinSize(wxSize(240,120));
  GetSizer()->Add(p_grid, 0, wxEXPAND|wxALL, 3);
  p_summary = new wxStaticText(this, wxID_ANY, "");
  GetSizer()->Add(p_summary, 0, wxALL, 3);
  return true;
}

void StressPanel::refresh()
{
  if (p_grid) updateStress(hasFocus());
}

string StressPanel::labelAnchorText() const
{
  string out;
  char line[120];
  for (size_t i=0; i<p_labelAnchors.size(); ++i) {
    std::snprintf(line,sizeof line,"label %zu anchor=%.4f,%.4f,%.4f\n",i,
                  p_labelAnchors[i][0],p_labelAnchors[i][1],p_labelAnchors[i][2]);
    out+=line;
  }
  return out;
}

string StressPanel::summaryText() const
{
  return p_summary ? p_summary->GetLabel().ToStdString() : string();
}

void StressPanel::receiveFocus()
{
  // The final tensor belongs to the final geometry, never a trace frame.
  selectFragStep(-1);
  updateStress(true);
}

void StressPanel::loseFocus()
{
  getFW().getSceneGraph().getPropertySwitch("STRESS")->whichChild = SO_SWITCH_NONE;
}

void StressPanel::updateStress(bool draw)
{
  p_grid->ClearGrid();
  SGContainer& sg = getFW().getSceneGraph();
  SoSwitch *root = sg.getPropertySwitch("STRESS");
  root->removeAllChildren();
  p_labelAnchors.clear();
  auto *vec = dynamic_cast<PropVector*>(getCalculation()->getProperty("STRESS"));
  StressTensor::Result r;
  double stored[9];
  if (!vec || vec->rows()!=9 || vec->units()!="kbar") {
    p_summary->SetLabel("No supported final stress (expected nine values in kbar).");
    return;
  }
  for (int i=0; i<9; ++i) stored[i]=vec->value(i);
  if (!StressTensor::analyse(stored,r)) {
    p_summary->SetLabel("Final stress contains non-finite values.");
    return;
  }
  for (int i=0; i<3; ++i)
    for (int j=0; j<3; ++j)
      p_grid->SetCellValue(i,j,wxString::Format("%+.4f",r.sigma[i][j]));
  p_grid->AutoSizeColumns();
  wxString text = wxString::Format(
      "Pressure p = -tr(sigma)/3: %.4f GPa (%.4f kbar)\n"
      "Von Mises: %.4f GPa; max shear: %.4f GPa\n",
      r.pressure,10*r.pressure,r.vonMises,r.maxShear);
  for (int i=0; i<3; ++i)
    text += wxString::Format("Principal %d: %+.4f GPa (%+.4f kbar)\n",
                            i+1,r.principal[i],10*r.principal[i]);
  LatticeDef *lattice = sg.getFragment()->getLattice();
  double cell[3][3], center[3]={0,0,0}, span=0;
  if (lattice) {
    std::unique_ptr<vector<MPoint>> vectors(lattice->toVectors());
    MPoint corner = lattice->getLatticeCorner();
    center[0]=corner.x(); center[1]=corner.y(); center[2]=corner.z();
    for (int i=0; i<3; ++i) {
      MPoint v=(*vectors)[i];
      cell[i][0]=v.x(); cell[i][1]=v.y(); cell[i][2]=v.z();
      span=std::max(span,v.length());
      for (int j=0; j<3; ++j) center[j]+=0.5*cell[i][j];
    }
    text += wxString::FromUTF8(StressTensor::cellWords(r,cell)) + "\n";
  } else text += "No unit cell: principal arrows unavailable.\n";
  text += "Tension positive; Quantum ESPRESSO's own output has the opposite sign.\n"
          "Blue: tension (outward); vermillion: compression (inward); arrows lie outside the cell.\n"
          "Arrows share a scale proportional to |stress|. Final stress only.";
  p_summary->SetLabel(text);
  p_summary->Wrap(360);
  Layout();
  if (!draw || !lattice) return;

  const double maxStress=std::max(std::fabs(r.principal[0]),std::fabs(r.principal[2]));
  if (maxStress<1e-12 || span<1e-10) return;
  // Arrows sit outside the cell, on the face the principal axis crosses, so
  // atoms never hide them; compression ends at the face, tension starts there.
  const double scale=0.65*span/maxStress;
  const double det=cell[0][0]*(cell[1][1]*cell[2][2]-cell[1][2]*cell[2][1])
                  -cell[0][1]*(cell[1][0]*cell[2][2]-cell[1][2]*cell[2][0])
                  +cell[0][2]*(cell[1][0]*cell[2][1]-cell[1][1]*cell[2][0]);
  if (std::fabs(det)<1e-12) return;
  for (int i=0; i<3; ++i) {
    const double length=std::fabs(r.principal[i])*scale;
    if (length<1e-10) continue;
    const bool tension=r.principal[i]>0;
    float red=tension ? 0.f : 0.835f, green=tension ? 0.447f : 0.369f;
    float blue=tension ? 0.698f : 0.f;
    // Distance from the centre to the cell surface along the axis: the
    // largest fractional coordinate of the axis reaches +-1/2 first.
    double fmax=0;
    for (int k=0; k<3; ++k) {
      const double *b=cell[(k+1)%3], *c=cell[(k+2)%3];
      const double cx=b[1]*c[2]-b[2]*c[1], cy=b[2]*c[0]-b[0]*c[2], cz=b[0]*c[1]-b[1]*c[0];
      fmax=std::max(fmax,std::fabs((cx*r.axis[i][0]+cy*r.axis[i][1]+cz*r.axis[i][2])/det));
    }
    const double face=0.5/fmax;
    for (int sign : {-1,1}) {
      double inner[3], outer[3], dir[3];
      for (int j=0; j<3; ++j) {
        inner[j]=center[j]+sign*face*r.axis[i][j];
        outer[j]=inner[j]+sign*length*r.axis[i][j];
        dir[j]=sign*length*r.axis[i][j]*(tension ? 1 : -1)/1.075;
      }
      auto *arrow=new VRVector;
      arrow->setColor(red,green,blue);
      arrow->fixedThickness(0.025f*span,0.055f*span,0.11f*span);
      arrow->position(tension ? inner[0] : outer[0],
                      tension ? inner[1] : outer[1],
                      tension ? inner[2] : outer[2]);
      arrow->direction(dir[0],dir[1],dir[2]);
      arrow->showThroughMolecule();
      root->addChild(arrow);
    }
    // One label per axis, past the outer tip of the + half. Left-justified
    // text starts at the anchor, so for the axis pointing at the viewer it
    // sits beside the tip rather than over it.
    auto *label=new SoAnnotation;
    auto *at=new SoTranslation;
    double anchor[3];
    for (int j=0; j<3; ++j)
      anchor[j]=center[j]+(face+length+0.06*span)*r.axis[i][j];
    at->translation.setValue(anchor[0],anchor[1],anchor[2]);
    p_labelAnchors.push_back({anchor[0],anchor[1],anchor[2]});
    label->addChild(at);
    auto *nudge=new SoCallback;
    // Lives as long as the scene node; a few bytes per label per redraw.
    nudge->setCallback(nudgeLabel,new LabelNudge{nudge,
        SbVec3f(r.axis[i][0],r.axis[i][1],r.axis[i][2]),(float)span});
    label->addChild(nudge);
    auto *material=new SoMaterial;
    material->diffuseColor.setValue(red,green,blue);
    label->addChild(material);
    auto *font=new SoFont;
    font->size=14;
    label->addChild(font);
    char value[100];
    std::snprintf(value,sizeof value,"  %+.3f GPa (%+.3f kbar)",
                  r.principal[i],10*r.principal[i]);
    auto *node=new SoText2;
    node->string=value;
    label->addChild(node);
    root->addChild(label);
  }
  root->whichChild=SO_SWITCH_ALL;
}
