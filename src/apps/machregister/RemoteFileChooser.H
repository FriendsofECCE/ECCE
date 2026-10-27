//  Modal file chooser for a remote machine: Browse in Register Machines
//  when the program is not on this computer.  Lists with `ls -1Ap` over the
//  machine's existing login (SchedulerQuery::Remote).
#ifndef REMOTEFILECHOOSER_HH
#define REMOTEFILECHOOSER_HH

#include <string>
#include <vector>

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/dialog.h"
#include "wx/listbox.h"

#include "SchedulerQuery.H"

class RemoteFileChooser : public wxDialog
{
public:
    RemoteFileChooser(wxWindow* parent, const wxString& title,
                      const SchedulerQuery::Connection& conn,
                      const std::string& current);
    ~RemoteFileChooser();

    /// The chosen absolute path on the machine (valid after wxID_OK).
    std::string path() const { return p_chosen; }

private:
    void list(const std::string& dir);
    void onEnterPath(wxCommandEvent&);
    void onUp(wxCommandEvent&);
    void onActivate(wxCommandEvent&);
    void onOK(wxCommandEvent&);
    bool pick(int index);

    SchedulerQuery::Remote* p_remote;
    bool p_opened;
    wxTextCtrl* p_pathBox;
    wxListBox* p_list;
    wxStaticText* p_status;
    wxButton* p_up;
    std::string p_dir;
    std::string p_chosen;
    std::vector<bool> p_isDir;
    std::vector<std::string> p_names;
};

#endif
