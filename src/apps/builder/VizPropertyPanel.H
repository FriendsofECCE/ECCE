#ifndef VIZPROPERTYPANEL_H_
#define VIZPROPERTYPANEL_H_

#include <map>
  using std::map;
#include <set>
  using std::set;

#include "wxviz/WxVizTool.H"

#include "PropertyPanel.H"


class wxCustomButton;
class wxCheckBox;


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

    virtual bool Show(bool show=true);

  // getters
    bool hasFocus() const;
    bool isPinned() const;

    //  Whether taking viz focus puts anything in the 3-D viewer.  A panel
    //  that draws nothing there (the MO diagram) must not take it: taking
    //  it calls loseFocus() on whichever panel DOES own the viewer, and
    //  for the MOs panel that switches the orbital just shown off again.
    virtual bool drawsInViewer() const { return true; }

    //  The panel's "Show in viewer" box (0 until the panel is shown).
    wxCheckBox *viewerToggle() const { return p_viewerToggle; }

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
    void addViewerToggle();
    bool isRelevent(IPropCalculation *propCalc,
                    const string& code, 
                    const string& runtype,
                    const set<string>& pnames) const;


  private:
    static set<VizPropertyPanel*> p_allVizPropertyPanels;
    static map<string, set<VizPropertyPanel*> > p_vizPropertyPanels;

    bool p_hasFocus;
    bool p_isPinned;
    wxCheckBox *p_viewerToggle;
};

#endif // VIZPROPERTYPANEL_H_
