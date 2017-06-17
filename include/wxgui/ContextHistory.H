#ifndef CONTEXTHISTORY_H_
#define CONTEXTHISTORY_H_

#include <vector>
  using std::vector;

#include <wx/docview.h>
#include <wx/string.h>

class wxMenu;

/**
 * Manages a list of context URLs meant to display within a wxMenu.
 * For better or worse, functions follow wx standard CamelCase conventions.
 */
class ContextHistory : public wxFileHistory
{
  public:
    ContextHistory(size_t maxFiles = 9,
                   wxWindowID idBase = wxID_FILE1,
                   bool useNumbers = false);
    virtual ~ContextHistory();

    virtual bool SetContext(const wxWindowID& id);
    virtual bool SetContext(const wxString& context);
    virtual wxString GetContext() const { return p_context; }
    virtual bool RemoveContext(const wxString& context);
    virtual bool RenameContext(const wxString& oldName,
                               const wxString& newName);

  protected:
    virtual void RefreshNames();

  private:
    wxString p_context;
    bool p_useNumbers;
};

#endif // CONTEXTHISTORY_H_
