#include <algorithm>
  using std::copy;
  using std::fill;

#include <wx/link.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "tdat/PropTable.H"
#include "tdat/PropTSVector.H"
#include "tdat/Fragment.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"
#include "dsm/ChemistryTask.H"

#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"

#include "PlotMeta3D.H"


wxFORCE_LINK_THIS_MODULE(PlotMeta3D)

const wxWindowID PlotMeta3D::ID_XMIN = wxNewId();
const wxWindowID PlotMeta3D::ID_XMAX = wxNewId();
const wxWindowID PlotMeta3D::ID_YMIN = wxNewId();
const wxWindowID PlotMeta3D::ID_YMAX = wxNewId();
const wxWindowID PlotMeta3D::ID_AMIN = wxNewId();
const wxWindowID PlotMeta3D::ID_AMAX = wxNewId();
const wxWindowID PlotMeta3D::ID_SIMTMP = wxNewId();
const wxWindowID PlotMeta3D::ID_TMPRTR = wxNewId();
const wxWindowID PlotMeta3D::ID_RATE = wxNewId();

static double k_B = 3.166815e-6;  // Boltzmann constant
static double R_gas = 1.9858775e-3; // Ideal gas constant
static double k_B_ov_h = 2.083665e10; // Boltzmann constant over Planck's constant
static double HtoKcal = 627.510; // Hartree to Kcal/mol conversion

IMPLEMENT_DYNAMIC_CLASS(PlotMeta3D, PropertyPanel)

BEGIN_EVENT_TABLE(PlotMeta3D, PropertyPanel)
  EVT_TEXT_ENTER(ID_XMIN, PlotMeta3D::refreshPlot)
  EVT_TEXT_ENTER(ID_XMAX, PlotMeta3D::refreshPlot)
  EVT_TEXT_ENTER(ID_YMIN, PlotMeta3D::refreshPlot)
  EVT_TEXT_ENTER(ID_YMAX, PlotMeta3D::refreshPlot)
  EVT_TEXT_ENTER(ID_TMPRTR, PlotMeta3D::refreshTemp)
END_EVENT_TABLE()

PlotMeta3D::PlotMeta3D()
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  p_xvalues = NULL;
  p_yvalues = NULL;
  p_values = NULL;
  p_kvalues = NULL;
}


PlotMeta3D::PlotMeta3D(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
  p_xvalues = NULL;
  p_yvalues = NULL;
  p_values = NULL;
  p_kvalues = NULL;
}


bool PlotMeta3D::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PlotMeta3D creation failed") );
    return false;
  }

  Fragment *frag = calculation->fragment();
  bool useShake = false;
  if (frag) {
    QMMMModel *terms = new QMMMModel(*frag);
    frag->setQMMMModel(terms);
    if (getCalculation()) {
      ChemistryTask *calc = dynamic_cast<ChemistryTask*>(getCalculation());
      if (calc) {
        calc->getQMMMModel(*terms);
        vector<QMMMParams*> *shakeTerms = terms->getQMMMTerms("shake");
        if (shakeTerms->size() > 0) {
          useShake = true;
        }
        delete shakeTerms;
      }
    }
    delete terms;
  }
  if (useShake) {
    // Check to see if molecule is linear
    int solventNumAtoms = frag->getSolventNumAtoms();
    int firstAtom = frag->getFirstSolventAtomIndex();
    int numAtoms = frag->numAtoms();
    if (solventNumAtoms == 3) {
      double r1x,r1y,r1z,r2x,r2y,r2z,r3x,r3y,r3z;
      TAtm *atm1 = frag->atomRef(firstAtom);
      TAtm *atm2 = frag->atomRef(firstAtom+1);
      TAtm *atm3 = frag->atomRef(firstAtom+2);
      const double *crd1 = atm1->coordinates();
      const double *crd2 = atm2->coordinates();
      const double *crd3 = atm3->coordinates();
      r1x = crd1[0];
      r1y = crd1[1];
      r1z = crd1[2];
      r2x = crd2[0];
      r2y = crd2[1];
      r2z = crd2[2];
      r3x = crd3[0];
      r3y = crd3[1];
      r3z = crd3[2];
      double r12x = r1x - r2x;
      double r12y = r1y - r2y;
      double r12z = r1z - r2z;
      double r13x = r1x - r3x;
      double r13y = r1y - r3y;
      double r13z = r1z - r3z;
      double r12 = sqrt(r12x*r12x+r12y*r12y+ r12z*r12z);
      double r13 = sqrt(r13x*r13x+r13y*r13y+ r13z*r13z);
      double rdot = r12x*r13x+r12y*r13y+r12z*r13z;
      rdot = rdot/(r12*r13);
      if (fabs(fabs(rdot)-1.0) <= 0.0001) {
          p_DOF = 5*((numAtoms-firstAtom)/solventNumAtoms)
                + 3*firstAtom - 3;
      } else {
        p_DOF = 6*((numAtoms-firstAtom)/solventNumAtoms)
              + 3*firstAtom - 3;
      }
    } else {
      p_DOF = 6*((numAtoms-firstAtom)/solventNumAtoms)
            + 3*firstAtom - 3;
    }
  } else {
    p_DOF = frag->numAtoms();
    p_DOF = 3*p_DOF - 3;
  }
  p_Temperature = 298.0;

  return true;
}


PlotMeta3D::~PlotMeta3D()
{
  if (p_xvalues) delete p_xvalues;
  if (p_yvalues) delete p_yvalues;
  if (p_values) delete p_values;
  if (p_kvalues) delete p_kvalues;
}


void PlotMeta3D::refresh()
{
  doPlot();
}


void PlotMeta3D::initialize()
{
  set<string> names;
  set<string>::iterator name;
  PropTable *prop = NULL;
  PropVector *propv = NULL;

  p_xvalues=NULL;
  p_yvalues=NULL;
  p_xsize = 0;
  p_ysize = 0;
  p_nsize = 0;

  if (!GetSizer()) {
    SetSizer(new wxBoxSizer(wxVERTICAL));
  }

  // Catch the case where we goofed the registry.
  names = getPropertyNames();
  if (names.empty()) {
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
                    "No properties registered for VectorPlotPropertyPanel"));
    return;
  }
  for (name = names.begin(); name != names.end(); ++name) {
    prop = dynamic_cast<PropTable*>(getCalculation()->getProperty(*name));
    if (prop) {
      const Property_Ref *propRef = PropFactory::getPropRef(*name);
      if (propRef) {
        p_dataNames.push_back(*name);
      }
    } else {
      propv = dynamic_cast<PropTSVector*>(getCalculation()->getProperty(*name));
      if (propv) {
        const Property_Ref *propRef = PropFactory::getPropRef(*name);
        if (propRef) {
          p_dataNames.push_back(*name);
        }
      }
    }
  }
               
  // create our wxPlotCtrl and add to sizer
//  p_plotCtrl = new ewxPlotCtrl(this);
//  p_plotCtrl->SetMinSize(wxSize(300,200));
//  p_plotCtrl->SetShowXAxisLabel(true);
//  p_plotCtrl->SetShowYAxisLabel(true);
//  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  //Create boxes for values of minimum and maximum locations
  //and corresponding free energies
  wxBoxSizer *xminSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(xminSizer, 0, wxEXPAND);
  xminSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "X Coordinate of Free Energy Minimum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_xminCtrl = new ewxTextCtrl(this,ID_XMIN);
  xminSizer->Add(p_xminCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxBoxSizer *yminSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(yminSizer, 0, wxEXPAND);
  yminSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Y Coordinate of Free Energy Minimum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_yminCtrl = new ewxTextCtrl(this,ID_YMIN);
  yminSizer->Add(p_yminCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxBoxSizer *aminSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(aminSizer, 0, wxEXPAND);
  aminSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Value of Free Energy Minimum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_aminText = new ewxStaticText(this,ID_AMIN);
  aminSizer->Add(p_aminText, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxBoxSizer *xmaxSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(xmaxSizer, 0, wxEXPAND);
  xmaxSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "X Coordinate of Free Energy Maximum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_xmaxCtrl = new ewxTextCtrl(this,ID_XMAX);
  xmaxSizer->Add(p_xmaxCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxBoxSizer *ymaxSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(ymaxSizer, 0, wxEXPAND);
  ymaxSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Y Coordinate of Free Energy Maximum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_ymaxCtrl = new ewxTextCtrl(this,ID_YMAX);
  ymaxSizer->Add(p_ymaxCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxBoxSizer *amaxSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(amaxSizer, 0, wxEXPAND);
  amaxSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Value of Free Energy Maximum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_amaxText = new ewxStaticText(this,ID_AMAX);
  amaxSizer->Add(p_amaxText, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);


  //Create labels for simulation temperature
  wxBoxSizer *tmprtrSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(tmprtrSizer, 0, wxEXPAND);
  tmprtrSizer->Add(new wxStaticText(this,wxID_STATIC,
              "Average Simulation Temperature: "),
              0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  char buf[13];
  sprintf(buf,"%12.3f",p_Temperature);
  p_tmprtrText = new ewxStaticText(this,ID_SIMTMP,buf);
  tmprtrSizer->Add(p_tmprtrText, 0,
                   wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  //Create input field for user specified temperature
  wxBoxSizer *tmpInpSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(tmpInpSizer, 0, wxEXPAND);
  tmpInpSizer->Add(new wxStaticText(this,wxID_STATIC, "User Temperature: "),
                  0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_tmprtrCtrl = new ewxTextCtrl(this,ID_TMPRTR);
  tmpInpSizer->Add(p_tmprtrCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  //Create text field for calculated rate constant
  wxBoxSizer *rateSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(rateSizer, 0, wxEXPAND);
  rateSizer->Add(new wxStaticText(this,wxID_STATIC,
              "Calculated Rate Constant: "),
              0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  sprintf(buf,"%12.4e",1.0);
  p_rateText = new ewxStaticText(this,ID_RATE,buf);
  rateSizer->Add(p_rateText, 0,
                   wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  doPlot();
}

void PlotMeta3D::doPlot()
{
  // clear any plot(s) that might exist
//  p_plotCtrl->DeleteCurve(-1, true); // -1 deletes all, true emits event

  // get property data
  int xsize, ysize, nsize;
  int i, j, icnt;
  PropTable *prop;
  PropTSVector *keprop;
  if (p_dataNames.size() > 0) {
    for (i=0; i<p_dataNames.size(); i++) {
      if (p_dataNames[i] == "METATABLE") {
        prop = dynamic_cast<PropTable*>(
            getCalculation()->getProperty(p_dataNames[i].c_str()));
        const Property_Ref *propRef =
          PropFactory::getPropRef(p_dataNames[i].c_str());
        p_ylabel = propRef->long_name;
      } else if (p_dataNames[i] == "IKCPVEC") {
        keprop = dynamic_cast<PropTSVector*>(
            getCalculation()->getProperty(p_dataNames[i].c_str()));
        vector<double> &tmprtrData = keprop->values();
        int ksize = tmprtrData.size();
        p_Temperature = 0.0;
        for (j=0; j<ksize; j++) {
          p_Temperature += tmprtrData[j];
        }
        p_Temperature /= ((double)ksize);
        p_Temperature *= 2.0/(k_B*((double)p_DOF));
        char buf[13];
        sprintf(buf,"%12.3f",p_Temperature);
        p_tmprtrText->SetLabel(buf);
        p_tmprtrCtrl->setValueAsDouble(p_Temperature);
      }
    }
  }
  xsize = prop->columns();
  ysize = prop->rows();
  vector<double> &vectorData = prop->values();
  nsize = vectorData.size();
  vector<string> *y_axis = prop->colLabels();
  if (p_ylabel.IsEmpty()) p_ylabel = prop->columnLabel();
  ysize = y_axis->size();
  vector<string> *x_axis = prop->rowLabels();
  if (p_xlabel.IsEmpty()) p_xlabel = prop->rowLabel();
  xsize = x_axis->size();
  if (nsize == xsize*ysize &&  nsize > 0) {
    if (p_xvalues) delete p_xvalues;
    p_xvalues = new double[xsize];
    if (p_yvalues) delete p_yvalues;
    p_yvalues = new double[ysize];
    if (p_values) delete p_values;
    p_values = new double[nsize];
    copy(vectorData.begin(), vectorData.end(), p_values);
    if (p_kvalues) delete p_kvalues;
    p_kvalues = new double[nsize];
    for (i=0; i<xsize; i++) {
      p_xvalues[i] = atof((*x_axis)[i].c_str());
    }
    for (i=0; i<ysize; i++) {
      p_yvalues[i] = atof((*y_axis)[i].c_str());
    }
    // Find min and max values of free energy
    icnt = 0;
    for (j=0; j<ysize; j++) {
      for (i=0; i<xsize; i++) {
        p_values[icnt] *= -1.0;
        if (i == 0 && j == 0) {
          p_xmin = p_xvalues[i];
          p_xmax = p_xvalues[i];
          p_ymin = p_yvalues[j];
          p_ymax = p_yvalues[j];
          p_amin = p_values[icnt];
          p_amax = p_values[icnt];
        } else {
          if (p_amin > p_values[icnt]) {
            p_xmin = p_xvalues[i];
            p_ymin = p_yvalues[j];
            p_amin = p_values[icnt];
          } else if (p_amax < p_values[icnt]) {
            p_xmax = p_xvalues[i];
            p_ymax = p_yvalues[j];
            p_amax = p_values[icnt];
          }
        }
        icnt++;
      }
    }
    for (i=0; i<nsize; i++) {
      p_values[i] -= p_amin;
      // Convert from atomic units to kcal/mol
      p_values[i] *= HtoKcal;
    }
    p_amin *= HtoKcal;
    p_amax *= HtoKcal;
    p_amax = p_amax - p_amin;
    p_amin = 0.0;
    // Evaluate logarithm of rate constant curve
    if (p_Temperature > 0.0) {
      for (i=0; i<nsize; i++) {
        p_kvalues[i] = (k_B_ov_h*p_Temperature)
                     * exp(-(p_values[i]-p_amin)/(R_gas*p_Temperature));
      }
    }
  }
  if (p_rateText) {
    char rbuf[15];
    double rate = (k_B_ov_h*p_Temperature)
      * exp(-(p_amax-p_amin)/(R_gas*p_Temperature));
    sprintf(rbuf,"%14.4e",rate);
    p_rateText->SetLabel(rbuf);
  }
  if (p_aminText && p_amaxText) {
    char ybuf[13];
    sprintf(ybuf,"%12.4f",p_amin);
    p_aminText->SetLabel(ybuf);
    sprintf(ybuf,"%12.4f",p_amax);
    p_amaxText->SetLabel(ybuf);
  }
  p_xsize = xsize;
  p_ysize = ysize;
  p_nsize = nsize;
  p_xminCtrl->setValueAsDouble(p_xmin);
  p_xmaxCtrl->setValueAsDouble(p_xmax);
  p_yminCtrl->setValueAsDouble(p_ymin);
  p_ymaxCtrl->setValueAsDouble(p_ymax);

  // create our curve and add to plot control
  // Gary: This is where the 3D plot needs to be generated
//  wxPlotData *plotData = new wxPlotData(p_xvalues, p_yvalues, p_size);
//  p_plotCtrl->AddCurve(plotData, true, true);

  // add and show the axes labels
//  p_plotCtrl->SetXAxisLabel(p_xlabel);
//  p_plotCtrl->SetYAxisLabel(p_ylabel);
//  p_plotCtrl->SetPlotTitle(p_xlabel + " vs " + p_ylabel);
}

// Refresh plot if xmin or xmax values have been changed
void PlotMeta3D::refreshPlot(wxCommandEvent &event)
{
  if (p_xminCtrl && p_xmaxCtrl && p_yminCtrl && p_ymaxCtrl) {
    double xmin = p_xminCtrl->getValueAsDouble();
    double xmax = p_xmaxCtrl->getValueAsDouble();
    double ymin = p_yminCtrl->getValueAsDouble();
    double ymax = p_ymaxCtrl->getValueAsDouble();
    int imin_m, imax_m, imin_p, imax_p;
    int jmin_m, jmax_m, jmin_p, jmax_p;
    double x_lower, x_upper, y_lower, y_upper;
    double dx, dy;
    // Check to see if values are legitimate
    x_lower = p_xvalues[0];
    x_upper = p_xvalues[p_xsize-1];
    if (xmin < x_lower) {
      xmin = x_lower;
      p_xminCtrl->setValueAsDouble(xmin);
    }
    if (xmin > x_upper) {
      xmin = x_upper;
      p_xminCtrl->setValueAsDouble(xmin);
    }
    if (xmax > x_upper) {
      xmax = x_upper;
      p_xmaxCtrl->setValueAsDouble(xmax);
    }
    if (xmax < x_lower) {
      xmax = x_lower;
      p_xmaxCtrl->setValueAsDouble(xmax);
    }
    y_lower = p_yvalues[0];
    y_upper = p_yvalues[p_ysize-1];
    if (ymin < y_lower) {
      ymin = y_lower;
      p_yminCtrl->setValueAsDouble(ymin);
    }
    if (ymin > y_upper) {
      ymin = y_upper;
      p_yminCtrl->setValueAsDouble(ymin);
    }
    if (ymax > y_upper) {
      ymax = y_upper;
      p_ymaxCtrl->setValueAsDouble(ymax);
    }
    if (ymax < y_lower) {
      ymax = y_lower;
      p_ymaxCtrl->setValueAsDouble(ymax);
    }
    // Find indice that bound new values of xmin and xmax
    imin_m = 0;
    imin_p = 0;
    imax_m = 0;
    imax_p = 0;
    jmin_m = 0;
    jmin_p = 0;
    jmax_m = 0;
    jmax_p = 0;
    int i;
    for (i=0; i<p_xsize; i++) {
      if (p_xvalues[i]<=xmin) imin_m = i;
      if (p_xvalues[i]<=xmax) imax_m = i;
    }
    for (i=p_xsize-1; i>=0; i--) {
      if (p_xvalues[i]>=xmin) imin_p = i;
      if (p_xvalues[i]>=xmax) imax_p = i;
    }
    for (i=0; i<p_ysize; i++) {
      if (p_yvalues[i]<=ymin) jmin_m = i;
      if (p_yvalues[i]<=ymax) jmax_m = i;
    }
    for (i=p_ysize-1; i>=0; i--) {
      if (p_yvalues[i]>=ymin) jmin_p = i;
      if (p_yvalues[i]>=ymax) jmax_p = i;
    }
    // Do linear interpolation to get exact value of p_ymin, p_ymax
    if (imin_m != imin_p) {
      dx = (xmin-p_xvalues[imin_m])/(p_xvalues[imin_p]-p_xvalues[imin_m]);
    } else {
      dx = 0.0;
    }
    if (jmin_m != jmin_p) {
      dy = (ymin-p_yvalues[jmin_m])/(p_yvalues[jmin_p]-p_yvalues[jmin_m]);
    } else {
      dy = 0.0;
    }
    p_amin = (1.0-dx)*(1.0-dy)*p_values[jmin_m*p_xsize+imin_m]
           + (1.0-dx)*dy*p_values[jmin_p*p_xsize+imin_m]
           + dx*(1.0-dy)*p_values[jmin_m*p_xsize+imin_p]
           + dx*dy*p_values[jmin_p*p_xsize+imin_p];
    if (imax_m != imax_p) {
      dx = (xmax-p_xvalues[imax_m])/(p_xvalues[imax_p]-p_xvalues[imax_m]);
    } else {
      dx = 0.0;
    }
    if (jmax_m != jmax_p) {
      dy = (ymax-p_yvalues[jmax_m])/(p_yvalues[jmax_p]-p_yvalues[jmax_m]);
    } else {
      dy = 0.0;
    }
    p_amax = (1.0-dx)*(1.0-dy)*p_values[jmax_m*p_xsize+imax_m]
           + (1.0-dx)*dy*p_values[jmax_p*p_xsize+imax_m]
           + dx*(1.0-dy)*p_values[jmax_m*p_xsize+imax_p]
           + dx*dy*p_values[jmax_p*p_xsize+imax_p];
    char buf[13];
    sprintf(buf,"%12.4f",p_amin);
    p_aminText->SetLabel(buf);
    sprintf(buf,"%12.4f",p_amax);
    p_amaxText->SetLabel(buf);

// Gary: Might not need to do anything here.
//  TODO: this code keeps crashing, but it would be nice if it didn't
//    p_plotCtrl->DeleteCurve(-1, true);
//    wxPlotData *plotData = new wxPlotData(p_xvalues, p_yvalues, p_size);
//    p_plotCtrl->AddCurve(plotData, true, true);
  }
  refreshTemp(event);
}

// Refresh Rate constant plot if temperature value has been changed
void PlotMeta3D::refreshTemp(wxCommandEvent &event)
{
  if (p_tmprtrCtrl && p_rateText) {
    double tmprtr = p_tmprtrCtrl->getValueAsDouble();
    if (p_Temperature > 0.0) {
      p_Temperature = tmprtr;
      int i;
      for (i=0; i<p_nsize; i++) {
        p_kvalues[i] = (k_B_ov_h*p_Temperature)
                     * exp(-(p_values[i]-p_amin)/(R_gas*p_Temperature));
      }
      double rate = (k_B_ov_h*p_Temperature)
                  * exp(-(p_amax-p_amin)/(R_gas*p_Temperature));
      char buf[15];
      sprintf(buf,"%14.4e",rate);
      p_rateText->SetLabel(buf);
    }
  }
}
