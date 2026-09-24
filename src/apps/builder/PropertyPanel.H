#ifndef PROPERTYPANEL_H_
#define PROPERTYPANEL_H_

#include <map>
  using std::map;
#include <set>
  using std::set;
#include <string>
  using std::string;

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxPanel.H"


class IPropCalculation;
class PropertyPanelDescription;
class wxCustomButton;


/**
 * The main interface for any GUI displaying calculation properties.
 */
class PropertyPanel : public ewxPanel
{
  DECLARE_DYNAMIC_CLASS(PropertyPanel)

  public:
    PropertyPanel();
    PropertyPanel(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PropertyPanel");

    virtual bool Create(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PropertyPanel");
    virtual ~PropertyPanel();

    // wxWindow virtuals
    virtual bool Show(bool show=true);

    // PropertyPanelDescription wrappers
    string getName() const;
    string getClass() const;
    bool isConsumer() const;
    bool allowPin() const;
    set<string> getPropertyNames() const; 

    // when things change
    virtual void refresh();
    virtual void stateChange(const ResourceDescriptor::RUNSTATE& state);
    virtual void propertyUpdate(const string &propKey);
    virtual void attributeUpdate(const string &attrKey);

    virtual bool isRelevant(IPropCalculation *propCalc) const;

    /**
     * Should this panel open as a floating window rather than docked?
     *
     * The side docks are built for a narrow column -- addPropertyPanel()
     * floors their width at 200 and the panels themselves ask for 400 --
     * which suits a scalar readout or a table and does not suit anything
     * wide.  A correlation diagram is three columns side by side and is
     * unreadable in a dock.
     *
     * Opt-in, so no existing panel changes.  The pane is still an AUI
     * pane and can be docked by hand afterwards; only where it starts
     * differs.
     */
    virtual bool prefersFloating() const;

    /** Size for a panel that opens floating. */
    virtual wxSize preferredFloatingSize() const;

    // getters
    PropertyPanelDescription* getDescription() const;
    IPropCalculation* getCalculation() const;
    bool needsInitialization() const;
    bool needsRefresh() const;
    static set<PropertyPanel*> getPanels(const string& url="",
                                         const string& name="");
    static set<PropertyPanel*> getPanelsForPropertyName(const string& url="",
                                                        const string& key="");
    static void removePanels(const string& url="", const string& name="");
    static bool panelExists(const string& url, const string& name);

    // setters
    void setDescription(PropertyPanelDescription *description);

  protected:
    virtual void initialize();
    void ensureInitialized();

  private:
    PropertyPanelDescription *p_description;
    IPropCalculation *p_calculation;

    bool p_needsInitialization;
    bool p_needsRefresh;

    static set<PropertyPanel*> p_allPropertyPanels;
    static map<string, set<PropertyPanel*> > p_propertyPanels;
};

#endif // PROPERTYPANEL_H_
