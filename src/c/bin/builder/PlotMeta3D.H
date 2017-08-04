#ifndef PLOTMETA3D_H_
#define PLOTMETA3D_H_

#include <map>
  using std::map;

#include "PropertyPanel.H"

class ewxPlotCtrl;
class ewxTextCtrl;
class wxString;

class PlotMeta3D : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(PlotMeta3D)
  DECLARE_EVENT_TABLE()

  public:
    PlotMeta3D();
    PlotMeta3D(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PlotMeta3D");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PlotMeta3D");
    virtual ~PlotMeta3D();

    virtual void refresh();

  protected:
    virtual void initialize();

    virtual void doPlot();
    virtual void refreshPlot(wxCommandEvent &event);
    virtual void refreshTemp(wxCommandEvent &event);

  private:
    ewxPlotCtrl *p_plotCtrl;
    vector<wxString> p_dataNames;
    ewxTextCtrl *p_xminCtrl;
    ewxTextCtrl *p_xmaxCtrl;
    ewxTextCtrl *p_yminCtrl;
    ewxTextCtrl *p_ymaxCtrl;
    ewxStaticText *p_tmprtrText;
    ewxStaticText *p_rateText;
    ewxStaticText *p_aminText;
    ewxStaticText *p_amaxText;
    ewxTextCtrl *p_tmprtrCtrl;
    double p_xmin, p_xmax;
    double p_ymin, p_ymax;
    double p_amin, p_amax;
    double *p_xvalues;
    double *p_yvalues;
    double *p_values;
    double *p_kvalues;
    int p_xsize, p_ysize, p_nsize;
    int p_DOF;
    double p_Temperature;
    wxString p_xlabel;
    wxString p_ylabel;

    static const wxWindowID ID_XMIN;
    static const wxWindowID ID_XMAX;
    static const wxWindowID ID_YMIN;
    static const wxWindowID ID_YMAX;
    static const wxWindowID ID_AMIN;
    static const wxWindowID ID_AMAX;
    static const wxWindowID ID_SIMTMP;
    static const wxWindowID ID_TMPRTR;
    static const wxWindowID ID_RATE;
};

#endif // PLOTMETA3D_H_
