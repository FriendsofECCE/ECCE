#ifndef VIZPROPERTYPANEL_H_
#define VIZPROPERTYPANEL_H_

#include <map>
  using std::map;
#include <set>
  using std::set;

#include "wxviz/WxVizTool.H"

#include "PropertyPanel.H"


class wxCustomButton;


class VizPropertyPanel : public PropertyPanel,
                         public WxVizTool
{
  DECLARE_DYNAMIC_CLASS(VizPropertyPanel)

  public:
    VizPropertyPanel();
    VizPropertyPanel(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="VizPropertyPanel");

    virtual bool Create(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="VizPropertyPanel");

    virtual ~VizPropertyPanel();

  // getters
    bool hasFocus() const;
    bool isPinned() const;

    static set<VizPropertyPanel*> getPanels(const string& url="",
                                            const string& name="");

  // setters
    void setFocus(const bool& value);
    void setPinned(const bool& value);
    
  protected:
    virtual void receiveFocus();
    virtual void loseFocus();

    void doFocus(const bool& value);
    void doPin(const bool& value);

    void selectFragStep(int index);
    bool isRelevent(IPropCalculation *propCalc,
                    const string& code, 
                    const string& runtype,
                    const set<string>& pnames) const;


  private:
    static set<VizPropertyPanel*> p_allVizPropertyPanels;
    static map<string, set<VizPropertyPanel*> > p_vizPropertyPanels;

    bool p_hasFocus;
    bool p_isPinned;
};

#endif // VIZPROPERTYPANEL_H_
