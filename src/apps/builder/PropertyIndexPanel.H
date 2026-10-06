#ifndef PROPERTYINDEXPANEL_H_
#define PROPERTYINDEXPANEL_H_

#include <functional>
#include <string>
#include <vector>

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/treectrl.h>

/**
 * The top of the Builder's Properties tab in the list + detail layout: a
 * compact header (calculation, code and theory, total energy) over a
 * grouped list of the property panels the calculation has.  Choosing a
 * panel calls the select handler; the Builder shows that panel below.
 */
class PropertyIndexPanel : public wxPanel
{
  public:
    struct Entry {
      std::string name;    // the panel's name, which is its pane's name
      std::string group;   // heading from PropertyPanelDescriptor.xml
    };

    PropertyIndexPanel(wxWindow *parent);

    void setHeader(const wxString& title, const wxString& codeTheory,
                   const wxString& energy);
    void setEntries(const std::vector<Entry>& entries);
    void setSelected(const std::string& name);
    void setSelectHandler(std::function<void(const std::string&)> f)
    { p_onSelect = f; }

    // Test hook: what the list holds, one "group/name" line per panel.
    std::vector<std::string> listing() const;

  private:
    void OnSelChanged(wxTreeEvent& event);

    wxStaticText *p_title;
    wxStaticText *p_codeTheory;
    wxStaticText *p_energy;
    wxTreeCtrl *p_tree;
    std::function<void(const std::string&)> p_onSelect;
    bool p_quiet;   // selection set by the program, not the user
    std::vector<Entry> p_entries;
};

#endif
