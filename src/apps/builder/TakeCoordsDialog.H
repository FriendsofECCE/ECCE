// Take Coordinates From... (#620): another calculation, the atoms to take,
// and whether to fit it onto the open structure first.
#ifndef TAKECOORDSDIALOG_H
#define TAKECOORDSDIALOG_H

#include <string>
#include <vector>
#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxTextCtrl;

class TakeCoordsDialog : public wxDialog
{
  public:
    TakeCoordsDialog(wxWindow *parent, const std::vector<std::string>& names,
                     const std::string& atoms);
    int source() const;          ///< index into names
    std::string atoms() const;   ///< 1-based list as typed
    bool align() const;

  private:
    wxChoice *p_calc;
    wxTextCtrl *p_atoms;
    wxCheckBox *p_align;
};

#endif
