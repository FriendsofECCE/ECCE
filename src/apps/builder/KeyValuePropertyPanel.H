#ifndef KEYVALUEPROPERTYPANEL_H_
#define KEYVALUEPROPERTYPANEL_H_

#include <string>
#include <vector>

#include "PropertyPanel.H"

class KeyValuePropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(KeyValuePropertyPanel)

  public:
    KeyValuePropertyPanel();
    KeyValuePropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="KeyValuePropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="KeyValuePropertyPanel");
    virtual ~KeyValuePropertyPanel();

    virtual void refresh();
    virtual bool isRelevant(IPropCalculation *propCalc) const;

    // The rows as drawn, one string per row ("H ..." heading, "R ..." value),
    // for tests; toggleGroup() is what a click on a group heading does.
    std::vector<std::string> rowDump() const { return p_rows; }
    bool toggleGroup(const std::string& group);
    // Per row "<key> | <value right edge> | <unit right edge> | <panel width>"
    // in panel pixels, so a test can see whether a unit is cut off.
    std::vector<std::string> rowFit() const;
    // What the right-click menu copies (#408): one row's value, with or
    // without its unit, or every row as "key<TAB>value<TAB>unit" lines.
    wxString copyText(const std::string& key, bool unit) const;
    wxString copyAllText() const;

  protected:
    virtual void initialize();

  private:
    bool isFolded(const std::string& group) const;

    std::vector<std::string> p_rows;
    struct RowWindows { std::string key; wxWindow *value, *unit;
                        std::string valueText, unitText; };
    void copyMenu(const std::string& key);
    std::vector<RowWindows> p_rowWindows;
};

#endif // KEYVALUEPROPERTYPANEL_H_
