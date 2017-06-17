#ifndef PLOTMETAXY_H_
#define PLOTMETAXY_H_

#include <map>
  using std::map;

#include "PropertyPanel.H"

class ewxPlotCtrl;
class ewxTextCtrl;
class wxString;

class PlotMetaXY : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(PlotMetaXY)
  DECLARE_EVENT_TABLE()

  public:
    PlotMetaXY();
    PlotMetaXY(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PlotMetaXY");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PlotMetaXY");
    virtual ~PlotMetaXY();

    virtual void refresh();

  protected:
    virtual void initialize();

    virtual void doPlot();
    virtual void refreshPlot(wxCommandEvent &event);
    virtual void refreshTemp(wxCommandEvent &event);

  private:
    ewxPlotCtrl *p_plotCtrl;
    wxString p_dimension;
    vector<wxString> p_dataNames;
    ewxTextCtrl *p_xminCtrl;
    ewxTextCtrl *p_xmaxCtrl;
    ewxStaticText *p_tmprtrText;
    ewxStaticText *p_rateText;
    ewxStaticText *p_yminText;
    ewxStaticText *p_ymaxText;
    ewxTextCtrl *p_tmprtrCtrl;
    double p_xmin, p_xmax;
    double p_ymin, p_ymax;
    double *p_xvalues;
    double *p_yvalues;
    double *p_kvalues;
    int p_size;
    int p_DOF;
    double p_Temperature;
    wxString p_xlabel;
    wxString p_ylabel;

    static const wxWindowID ID_XMIN;
    static const wxWindowID ID_XMAX;
    static const wxWindowID ID_YMIN;
    static const wxWindowID ID_YMAX;
    static const wxWindowID ID_SIMTMP;
    static const wxWindowID ID_TMPRTR;
    static const wxWindowID ID_RATE;
};

#endif // PLOTMETAXY_H_
