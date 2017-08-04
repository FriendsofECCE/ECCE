#include <algorithm>
  using std::copy;
  using std::fill;

#include <wx/link.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "tdat/PropVector.H"
#include "tdat/PropTSVector.H"
#include "tdat/Fragment.H"
#include "tdat/QMMMModel.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"
#include "dsm/ChemistryTask.H"

#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"

#include "PlotMetaXY.H"


wxFORCE_LINK_THIS_MODULE(PlotMetaXY)

const wxWindowID PlotMetaXY::ID_XMIN = wxNewId();
const wxWindowID PlotMetaXY::ID_XMAX = wxNewId();
const wxWindowID PlotMetaXY::ID_YMIN = wxNewId();
const wxWindowID PlotMetaXY::ID_YMAX = wxNewId();
const wxWindowID PlotMetaXY::ID_SIMTMP = wxNewId();
const wxWindowID PlotMetaXY::ID_TMPRTR = wxNewId();
const wxWindowID PlotMetaXY::ID_RATE = wxNewId();

static double k_B = 3.166815e-6;  // Boltzmann constant
static double R_gas = 1.9858775e-3; // Ideal gas constant
static double k_B_ov_h = 2.083665e10; // Boltzmann constant over Planck's constant
static double HtoKcal = 627.510; // Hartree to Kcal/mol conversion

IMPLEMENT_DYNAMIC_CLASS(PlotMetaXY, PropertyPanel)

BEGIN_EVENT_TABLE(PlotMetaXY, PropertyPanel)
  EVT_TEXT_ENTER(ID_XMIN, PlotMetaXY::refreshPlot)
  EVT_TEXT_ENTER(ID_XMAX, PlotMetaXY::refreshPlot)
  EVT_TEXT_ENTER(ID_TMPRTR, PlotMetaXY::refreshTemp)
END_EVENT_TABLE()

PlotMetaXY::PlotMetaXY()
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  p_xvalues = NULL;
  p_yvalues = NULL;
  p_kvalues = NULL;
}


PlotMetaXY::PlotMetaXY(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
  p_xvalues = NULL;
  p_yvalues = NULL;
  p_kvalues = NULL;
}


bool PlotMetaXY::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PlotMetaXY creation failed") );
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


PlotMetaXY::~PlotMetaXY()
{
  if (p_xvalues) delete p_xvalues;
  if (p_yvalues) delete p_yvalues;
  if (p_kvalues) delete p_kvalues;
}


void PlotMetaXY::refresh()
{
  doPlot();
}


void PlotMetaXY::initialize()
{
  set<string> names;
  set<string>::iterator name;
  PropVector *prop = NULL;

  p_xvalues=NULL;
  p_yvalues=NULL;
  p_size = 0;

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
    prop = dynamic_cast<PropVector*>(getCalculation()->getProperty(*name));
    if (prop) {
      const Property_Ref *propRef = PropFactory::getPropRef(*name);
      if (propRef) {
        p_dataNames.push_back(*name);
        if (p_dimension.IsEmpty())
          p_dimension = prop->rowLabel();
      }
    }
  }
               
  // create our wxPlotCtrl and add to sizer
  p_plotCtrl = new ewxPlotCtrl(this);
  p_plotCtrl->SetMinSize(wxSize(300,200));
  p_plotCtrl->SetShowXAxisLabel(true);
  p_plotCtrl->SetShowYAxisLabel(true);
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  //Create boxes for values of minimum and maximum locations
  //and corresponding free energies
  wxBoxSizer *xminSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(xminSizer, 0, wxEXPAND);
  xminSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Position of Free Energy Minimum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_xminCtrl = new ewxTextCtrl(this,ID_XMIN);
  xminSizer->Add(p_xminCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxBoxSizer *yminSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(yminSizer, 0, wxEXPAND);
  yminSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Value of Free Energy Minimum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_yminText = new ewxStaticText(this,ID_YMIN);
  yminSizer->Add(p_yminText, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxBoxSizer *xmaxSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(xmaxSizer, 0, wxEXPAND);
  xmaxSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Position of Free Energy Maximum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_xmaxCtrl = new ewxTextCtrl(this,ID_XMAX);

  wxBoxSizer *ymaxSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(ymaxSizer, 0, wxEXPAND);
  ymaxSizer->Add(new wxStaticText(this,wxID_STATIC,
                 "Value of Free Energy Maximum: "),
                 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  p_ymaxText = new ewxStaticText(this,ID_YMAX);
  ymaxSizer->Add(p_ymaxText, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  xmaxSizer->Add(p_xmaxCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  //Create labels for simulation temperature
  wxBoxSizer *tmprtrSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(tmprtrSizer, 0, wxEXPAND);
  tmprtrSizer->Add(new wxStaticText(this,wxID_STATIC,
              "Average Simulation Temperature: "),
              0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  char buf[12];
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

void PlotMetaXY::doPlot()
{
  // clear any plot(s) that might exist
  p_plotCtrl->DeleteCurve(-1, true); // -1 deletes all, true emits event

  // get property data
  int size, xsize;
  int i;
  PropVector *prop;
  PropTSVector *keprop;
  if (p_dataNames.size() > 0) {
    for (i=0; i<p_dataNames.size(); i++) {
      if (p_dataNames[i] == "METAVEC") {
        prop = dynamic_cast<PropVector*>(
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
        int j;
        for (j=0; j<ksize; j++) {
          p_Temperature += tmprtrData[j];
        }
        p_Temperature /= ((double)ksize);
        p_Temperature *= 2.0/(k_B*((double)p_DOF));
        char buf[12];
        sprintf(buf,"%12.3f",p_Temperature);
        p_tmprtrText->SetLabel(buf);
        p_tmprtrCtrl->setValueAsDouble(p_Temperature);
      }
    }
  }
  size = prop->rows();
  vector<double> &vectorData = prop->values();
  vector<string> *x_axis = prop->rowLabels();
  if (p_xlabel.IsEmpty()) p_xlabel = prop->rowLabel();
  p_ylabel << " Kcal/mole";
  xsize = x_axis->size();
  if (size == xsize &&  size > 0) {
    if (p_yvalues) delete p_yvalues;
    p_yvalues = new double[size];
    copy(vectorData.begin(), vectorData.end(), p_yvalues);
    if (p_xvalues) delete p_xvalues;
    p_xvalues = new double[size];
    if (p_kvalues) delete p_kvalues;
    p_kvalues = new double[size];
    for (i=0; i<size; i++) {
      p_xvalues[i] = atof((*x_axis)[i].c_str());
    }
    for (i=0; i<size; i++) {
      p_yvalues[i] *= -1.0;
      if (i == 0) {
        p_xmin = p_xvalues[i];
        p_xmax = p_xvalues[i];
        p_ymin = p_yvalues[i];
        p_ymax = p_yvalues[i];
      } else {
        if (p_ymin > p_yvalues[i]) {
          p_ymin = p_yvalues[i];
          p_xmin = p_xvalues[i];
        } else if (p_ymax < p_yvalues[i]) {
          p_ymax = p_yvalues[i];
          p_xmax = p_xvalues[i];
        }
      }
    }
    for (i=0; i<size; i++) {
      p_yvalues[i] -= p_ymin;
      // Convert from atomic units to kcal/mol
      p_yvalues[i] *= HtoKcal;
    }
    p_ymin *= HtoKcal;
    p_ymax *= HtoKcal;
    p_ymax = p_ymax - p_ymin;
    p_ymin = 0.0;
    // Evaluate logarithm of rate constant curve
    if (p_Temperature > 0.0) {
      for (i=0; i<size; i++) {
        p_kvalues[i] = (k_B_ov_h*p_Temperature)
                     * exp(-(p_yvalues[i]-p_ymin)/(R_gas*p_Temperature));
      }
    }
  }
  if (p_rateText) {
    char rbuf[14];
    double rate = (k_B_ov_h*p_Temperature)
      * exp(-(p_ymax-p_ymin)/(R_gas*p_Temperature));
    sprintf(rbuf,"%14.4e",rate);
    p_rateText->SetLabel(rbuf);
  }
  if (p_yminText && p_ymaxText) {
    char ybuf[12];
    sprintf(ybuf,"%12.4f",p_ymin);
    p_yminText->SetLabel(ybuf);
    sprintf(ybuf,"%12.4f",p_ymax);
    p_ymaxText->SetLabel(ybuf);
  }
  p_size = xsize;
  p_xminCtrl->setValueAsDouble(p_xmin);
  p_xmaxCtrl->setValueAsDouble(p_xmax);

  // create our curve and add to plot control
  wxPlotData *plotData = new wxPlotData(p_xvalues, p_yvalues, p_size);
  p_plotCtrl->AddCurve(plotData, true, true);

  // add and show the axes labels
  p_plotCtrl->SetXAxisLabel(p_xlabel);
  p_plotCtrl->SetYAxisLabel(p_ylabel);
  p_plotCtrl->SetPlotTitle(p_xlabel + " vs " + p_ylabel);
}

// Refresh plot if xmin or xmax values have been changed
void PlotMetaXY::refreshPlot(wxCommandEvent &event)
{
  if (p_xminCtrl && p_xmaxCtrl) {
    double xmin = p_xminCtrl->getValueAsDouble();
    double xmax = p_xmaxCtrl->getValueAsDouble();
    int imin_m, imax_m, imin_p, imax_p;
    double x_lower, x_upper;
    // Check to see if values are legitimate
    x_lower = p_xvalues[0];
    x_upper = p_xvalues[p_size-1];
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
    // Find indice that bound new values of xmin and xmax
    imin_m = 0;
    imin_p = 0;
    imax_m = 0;
    imax_p = 0;
    int i;
    for (i=0; i<p_size; i++) {
      if (p_xvalues[i]<=xmin) imin_m = i;
      if (p_xvalues[i]<=xmax) imax_m = i;
    }
    for (i=p_size-1; i>=0; i--) {
      if (p_xvalues[i]>=xmin) imin_p = i;
      if (p_xvalues[i]>=xmax) imax_p = i;
    }
    // Do linear interpolation to get exact value of p_ymin, p_ymax
    if (imin_m != imin_p) {
      p_ymin = ((p_yvalues[imin_p]-p_yvalues[imin_m])
           / (p_xvalues[imin_p]-p_xvalues[imin_m]))
           * (xmin-p_xvalues[imin_m]) + p_yvalues[imin_m];
    } else {
      p_ymin = p_yvalues[imin_m];
    }
    if (imax_m != imax_p) {
      p_ymax = ((p_yvalues[imax_p]-p_yvalues[imax_m])
           / (p_xvalues[imax_p]-p_xvalues[imax_m]))
           * (xmax-p_xvalues[imax_m]) + p_yvalues[imax_m];
    } else {
      p_ymax = p_yvalues[imax_m];
    }
    char buf[12];
    sprintf(buf,"%12.4f",p_ymin);
    p_yminText->SetLabel(buf);
    sprintf(buf,"%12.4f",p_ymax);
    p_ymaxText->SetLabel(buf);

//  TODO: this code keeps crashing, but it would be nice if it didn't
//    p_plotCtrl->DeleteCurve(-1, true);
//    wxPlotData *plotData = new wxPlotData(p_xvalues, p_yvalues, p_size);
//    p_plotCtrl->AddCurve(plotData, true, true);
  }
  refreshTemp(event);
}

// Refresh Rate constant plot if temperature value has been changed
void PlotMetaXY::refreshTemp(wxCommandEvent &event)
{
  if (p_tmprtrCtrl && p_rateText) {
    double tmprtr = p_tmprtrCtrl->getValueAsDouble();
    if (p_Temperature > 0.0) {
      p_Temperature = tmprtr;
      int i;
      for (i=0; i<p_size; i++) {
        p_kvalues[i] = (k_B_ov_h*p_Temperature)
                     * exp(-(p_yvalues[i]-p_ymin)/(R_gas*p_Temperature));
        printf("x[%d]: %f k[%d]: %f\n",i,p_xvalues[i],i,log10(p_kvalues[i]));
      }
      double rate = (k_B_ov_h*p_Temperature)
                  * exp(-(p_ymax-p_ymin)/(R_gas*p_Temperature));
      char buf[14];
      sprintf(buf,"%14.4e",rate);
      p_rateText->SetLabel(buf);
    }
  }
}
