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
#include "StressPanel.H"

IMPLEMENT_DYNAMIC_CLASS(StressPanel, VizPropertyPanel)

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
  text += "Tension positive; the stored values are compression positive.\n"
          "QE prints compression positive; MLP stores minus ASE stress.\n"
          "Blue: tension (outward); vermillion: compression (inward).\n"
          "Arrows share a scale proportional to |stress|. Final stress only.";
  p_summary->SetLabel(text);
  p_summary->Wrap(360);
  Layout();
  if (!draw || !lattice) return;

  const double maxStress=std::max(std::fabs(r.principal[0]),std::fabs(r.principal[2]));
  if (maxStress<1e-12 || span<1e-10) return;
  // A common scale preserves the eigenvalue ratios; both halves meet at
  // the cell centre. Compression reverses each half's arrowhead.
  const double scale=0.65*span/maxStress;
  for (int i=0; i<3; ++i) {
    const double length=std::fabs(r.principal[i])*scale;
    if (length<1e-10) continue;
    const bool tension=r.principal[i]>0;
    float red=tension ? 0.f : 0.835f, green=tension ? 0.447f : 0.369f;
    float blue=tension ? 0.698f : 0.f;
    for (int sign : {-1,1}) {
      double end[3], dir[3];
      for (int j=0; j<3; ++j) {
        end[j]=center[j]+sign*length*r.axis[i][j];
        dir[j]=sign*length*r.axis[i][j]*(tension ? 1 : -1)/1.075;
      }
      auto *arrow=new VRVector;
      arrow->setColor(red,green,blue);
      arrow->fixedThickness(0.025f*span,0.055f*span,0.11f*span);
      arrow->position(tension ? center[0] : end[0],
                      tension ? center[1] : end[1],
                      tension ? center[2] : end[2]);
      arrow->direction(dir[0],dir[1],dir[2]);
      arrow->showThroughMolecule();
      root->addChild(arrow);
    }
    // Cell-face atoms can cover an axis endpoint; its value must stay legible.
    auto *label=new SoAnnotation;
    auto *at=new SoTranslation;
    at->translation.setValue(center[0]+1.12*length*r.axis[i][0],
        center[1]+1.12*length*r.axis[i][1],center[2]+1.12*length*r.axis[i][2]);
    label->addChild(at);
    auto *material=new SoMaterial;
    material->diffuseColor.setValue(red,green,blue);
    label->addChild(material);
    auto *font=new SoFont;
    font->size=14;
    label->addChild(font);
    char value[100];
    std::snprintf(value,sizeof value,"%+.3f GPa (%+.3f kbar)",
                  r.principal[i],10*r.principal[i]);
    auto *node=new SoText2;
    node->string=value;
    label->addChild(node);
    root->addChild(label);
  }
  root->whichChild=SO_SWITCH_ALL;
}
