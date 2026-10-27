#include <string>
#include <vector>

#include "RemoteFileChooser.H"
#include "RemoteListing.H"

using std::string;
using std::vector;

RemoteFileChooser::RemoteFileChooser(wxWindow* parent, const wxString& title,
                                     const SchedulerQuery::Connection& conn,
                                     const string& current)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      p_remote(new SchedulerQuery::Remote(conn)), p_opened(false)
{
    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    p_up = new wxButton(this, wxID_ANY, "Up");
    p_pathBox = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                               wxDefaultSize, wxTE_PROCESS_ENTER);
    row->Add(p_up, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
    row->Add(p_pathBox, 1, wxALIGN_CENTER_VERTICAL);
    top->Add(row, 0, wxEXPAND | wxALL, 8);
    p_list = new wxListBox(this, wxID_ANY, wxDefaultPosition,
                           wxSize(460, 300));
    top->Add(p_list, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    p_status = new wxStaticText(this, wxID_ANY, "");
    top->Add(p_status, 0, wxEXPAND | wxALL, 8);
    top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
             wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizerAndFit(top);

    p_up->Bind(wxEVT_BUTTON, &RemoteFileChooser::onUp, this);
    p_pathBox->Bind(wxEVT_TEXT_ENTER, &RemoteFileChooser::onEnterPath, this);
    p_list->Bind(wxEVT_LISTBOX_DCLICK, &RemoteFileChooser::onActivate, this);
    Bind(wxEVT_BUTTON, &RemoteFileChooser::onOK, this, wxID_OK);

    string err;
    {
        wxBusyCursor busy;
        p_opened = p_remote->open(err);
    }
    if (!p_opened)
    {
        if (err.empty())
            err = p_remote->error();
        p_status->SetLabel("Could not connect: " +
            wxString::FromUTF8(err.c_str()) +
            "\nType the path in the Program box instead, or use Find.");
        p_up->Disable();
        p_pathBox->Disable();
        p_list->Disable();
        FindWindow(wxID_OK)->Disable();
    }
    else
    {
        string home;
        if (current.empty() || current[0] != '/')
        {
            wxBusyCursor busy;
            home = p_remote->home();
        }
        list(RemoteListing::startDir(current, home));
    }
    Layout();
}

RemoteFileChooser::~RemoteFileChooser()
{
    delete p_remote;
}

//  One ssh round trip per directory change, never per keystroke.
void RemoteFileChooser::list(const string& dir)
{
    vector<string> argv;
    argv.push_back("sh");
    argv.push_back("-c");
    argv.push_back(RemoteListing::command(dir));
    string out;
    bool ok;
    {
        wxBusyCursor busy;
        ok = p_remote->run(argv, out, 30);
    }
    if (!ok)
    {
        //  Stay on the folder listed last; the text says why.
        p_status->SetLabel(wxString::FromUTF8(
            (out.empty() ? "Cannot list " + dir : out).c_str()));
        p_pathBox->SetValue(wxString::FromUTF8(p_dir.c_str()));
        Layout();
        return;
    }
    p_dir = dir;
    p_pathBox->SetValue(wxString::FromUTF8(dir.c_str()));
    RemoteListing::Entries e = RemoteListing::parse(out);
    p_list->Clear();
    p_names.clear();
    p_isDir.clear();
    for (size_t i = 0; i < e.dirs.size(); i++)
    {
        p_list->Append(wxString::FromUTF8((e.dirs[i] + "/").c_str()));
        p_names.push_back(e.dirs[i]);
        p_isDir.push_back(true);
    }
    for (size_t i = 0; i < e.files.size(); i++)
    {
        p_list->Append(wxString::FromUTF8(e.files[i].c_str()));
        p_names.push_back(e.files[i]);
        p_isDir.push_back(false);
    }
    p_status->SetLabel(p_names.empty() ? "Empty folder." : "");
    Layout();
}

void RemoteFileChooser::onEnterPath(wxCommandEvent&)
{
    string typed = string(p_pathBox->GetValue().ToUTF8());
    if (typed.empty() || typed[0] != '/')
    {
        p_status->SetLabel("Give an absolute path, starting with /.");
        return;
    }
    list(RemoteListing::normalize(typed));
}

void RemoteFileChooser::onUp(wxCommandEvent&)
{
    list(RemoteListing::parent(p_dir));
}

//  True when the item was a file, now chosen; a folder is entered.
bool RemoteFileChooser::pick(int i)
{
    if (i < 0 || i >= (int)p_names.size())
        return false;
    string full = RemoteListing::join(p_dir, p_names[i]);
    if (p_isDir[i])
    {
        list(full);
        return false;
    }
    p_chosen = full;
    return true;
}

void RemoteFileChooser::onActivate(wxCommandEvent&)
{
    if (pick(p_list->GetSelection()))
        EndModal(wxID_OK);
}

void RemoteFileChooser::onOK(wxCommandEvent&)
{
    if (pick(p_list->GetSelection()))
        EndModal(wxID_OK);
}
