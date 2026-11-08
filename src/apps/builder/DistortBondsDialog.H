// Distort Bonds... (#619): one change in length per bond to the selected atom.
#ifndef DISTORTBONDSDIALOG_H
#define DISTORTBONDSDIALOG_H

#include <functional>
#include <string>
#include <vector>
#include <wx/dialog.h>

class Fragment;
class TAtm;
class wxStaticText;
class wxButton;
class wxCommandEvent;
class wxChoice;
class wxTextCtrl;

class DistortBondsDialog : public wxDialog
{
  public:
    /// Modeless: Builder shows it with Show() and calls refresh() whenever the
    /// structure or selection may have changed; onApply runs the distortion.
    DistortBondsDialog(wxWindow *parent, const Fragment& frag, int centre,
                       std::function<void()> onApply);

    /// Re-reads the structure; Apply is enabled only while the centre atom
    /// still exists, is still selected (sel = selected atom indices) and has
    /// the same neighbours.
    void refresh(const Fragment *frag, const std::vector<int>& sel);

    /// Index of the centre atom now, or -1 when Apply is disabled.
    int centre() const { return p_ok ? p_centre : -1; }

    /// Test hooks.
    void fillAll(const std::string& v);
    bool applyEnabled() const;
    std::string note() const;

    /// The change per neighbour as the "list" parameter of DistortBondsCmd
    /// ("3:0.1,5:-0.05", atom numbers from 1), empty if all are zero.
    std::string list() const;

  private:
    void onAll(wxCommandEvent&);
    void onJT(wxCommandEvent&);
    void onApplyBtn(wxCommandEvent&);
    void onCloseBtn(wxCommandEvent&);
    void setNames();
    const Fragment *p_frag;
    std::string p_sym;
    int p_centre;
    bool p_ok;
    std::function<void()> p_apply;
    std::vector<int> p_nb;
    std::vector<std::string> p_nbSyms;
    std::vector<wxStaticText*> p_name, p_len;
    wxStaticText *p_note;
    wxButton *p_applyBtn;
    std::vector<wxTextCtrl*> p_dr;
    wxTextCtrl *p_all, *p_delta;
    wxChoice *p_axis;
};

#endif
