// Distort Bonds... (#619): one change in length per bond to the selected atom.
#ifndef DISTORTBONDSDIALOG_H
#define DISTORTBONDSDIALOG_H

#include <string>
#include <vector>
#include <wx/dialog.h>

class Fragment;
class wxChoice;
class wxTextCtrl;

class DistortBondsDialog : public wxDialog
{
  public:
    DistortBondsDialog(wxWindow *parent, const Fragment& frag, int centre);

    /// The change per neighbour as the "list" parameter of DistortBondsCmd
    /// ("3:0.1,5:-0.05", atom numbers from 1), empty if all are zero.
    std::string list() const;

  private:
    void onAll(wxCommandEvent&);
    void onJT(wxCommandEvent&);
    const Fragment& p_frag;
    int p_centre;
    std::vector<int> p_nb;
    std::vector<wxTextCtrl*> p_dr;
    wxTextCtrl *p_all, *p_delta;
    wxChoice *p_axis;
};

#endif
