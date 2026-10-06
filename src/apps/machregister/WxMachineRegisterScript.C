/**
 * @file
 *
 *  Test hook for Register Machines; see the header.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dcmemory.h"
#include "wx/dcscreen.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include "wx/listbox.h"
#include "wx/spinctrl.h"

#include "wx/collpane.h"
#include "wx/radiobut.h"
#include "wx/stattext.h"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxTextCtrl.H"
#include "WxMachineRegister.H"
#include "WxMachineRegisterScript.H"

using std::string;
using std::vector;

//  Words split on blanks; '...' and "..." keep their blanks.
static vector<string> words(const string& line)
{
    vector<string> out;
    size_t i = 0;
    while (i < line.size())
    {
        while (i < line.size() && isspace((unsigned char)line[i]))
            i++;
        if (i >= line.size() || line[i] == '#')
            break;
        string w;
        char q = 0;
        for (; i < line.size(); i++)
        {
            char c = line[i];
            if (q) { if (c == q) q = 0; else w += c; }
            else if (c == '\'' || c == '"') q = c;
            else if (isspace((unsigned char)c)) break;
            else w += c;
        }
        out.push_back(w);
    }
    return out;
}


//  A backslash-n is a line break, for the multi-line boxes.
static string unescape(string v)
{
    for (size_t at; (at = v.find("\\n")) != string::npos; )
        v.replace(at, 2, "\n");
    return v;
}


MachRegScript::MachRegScript(WxMachineRegister* frame, const string& file)
    : p_frameRef(frame), p_frame(frame), p_timer(this), p_failures(0),
      p_loaded(false), p_size(0, 0)
{
    std::ifstream in(file.c_str());
    p_loaded = in.good();
    string line;
    while (std::getline(in, line))
        if (!words(line).empty())
            p_cmds.push_back(line);
    Bind(wxEVT_TIMER, &MachRegScript::onTimer, this);
}


void MachRegScript::start()
{
    if (!p_loaded)
        fail("cannot read the script");
    p_timer.StartOnce(300);
}


void MachRegScript::fail(const string& what)
{
    p_failures++;
    fprintf(stderr, "[MACHREG] FAIL %s\n", what.c_str());
}


void MachRegScript::expectEq(const string& what, const string& got,
                             const string& want)
{
    if (got != want)
        fail("expect " + what + ": got '" + got + "', wanted '" + want + "'");
    else
        fprintf(stderr, "[MACHREG] ok %s = '%s'\n", what.c_str(), got.c_str());
}


string MachRegScript::get(const string& name)
{
    wxWindow* w = p_frame->field(name);
    if (w == NULL)
        return "<no such field>";
    if (wxTextCtrl* t = dynamic_cast<wxTextCtrl*>(w))
        return (string)t->GetValue();
    if (wxSpinCtrl* s = dynamic_cast<wxSpinCtrl*>(w))
        return std::to_string(s->GetValue());
    if (wxSpinCtrlDouble* d = dynamic_cast<wxSpinCtrlDouble*>(w))
        return (string)wxString::Format("%g", d->GetValue());
    if (wxListBox* lb = dynamic_cast<wxListBox*>(w))
    {
        string all;
        for (unsigned i = 0; i < lb->GetCount(); i++)
            all += (i ? "," : "") + (string)lb->GetString(i);
        return all;
    }
    if (wxChoice* c = dynamic_cast<wxChoice*>(w))
        return (string)c->GetStringSelection();
    if (wxCheckBox* b = dynamic_cast<wxCheckBox*>(w))
        return b->IsChecked() ? "1" : "0";
    if (wxRadioButton* rb = dynamic_cast<wxRadioButton*>(w))
        return rb->GetValue() ? "1" : "0";
    if (wxButton* b = dynamic_cast<wxButton*>(w))
        return (string)b->GetLabel();
    if (wxStaticText* t = dynamic_cast<wxStaticText*>(w))
        return (string)t->GetLabel();
    return "<unsupported field>";
}


//  The button's own handler, as a click would reach it; a disabled button
//  does nothing, as for a user.
bool MachRegScript::click(const string& name)
{
    if (wxCheckBox* c = dynamic_cast<wxCheckBox*>(p_frame->field(name)))
    {
        c->SetValue(!c->IsChecked());
        wxCommandEvent ev(wxEVT_CHECKBOX, c->GetId());
        ev.SetEventObject(c);
        ev.SetInt(c->IsChecked());
        c->GetEventHandler()->ProcessEvent(ev);
        return true;
    }
    wxButton* b = dynamic_cast<wxButton*>(p_frame->field(name));
    if (b == NULL)
    {
        fail("no button " + name);
        return false;
    }
    if (!b->IsEnabled())
        return true;
    wxCommandEvent ev(wxEVT_BUTTON, b->GetId());
    ev.SetEventObject(b);
    b->GetEventHandler()->ProcessEvent(ev);
    return true;
}


bool MachRegScript::shot(const string& file, bool dialog)
{
    wxWindow* win = dialog ? static_cast<wxWindow*>(p_frame->p_rawDlg ?
                             p_frame->p_rawDlg : p_frame->p_wordsDlg)
                           : p_frame;
    if (win == NULL)
        return false;
    win->Raise();
    for (int i = 0; i < 3; i++)
    {
        win->Update();
        wxTheApp->Yield(true);
        wxMilliSleep(50);
    }
    wxSize sz = win->GetClientSize();
    wxClientDC screen(win);
    wxBitmap bmp(sz.x, sz.y);
    wxMemoryDC mem(bmp);
    mem.Blit(0, 0, sz.x, sz.y, &screen, 0, 0);
    mem.SelectObject(wxNullBitmap);
    return bmp.ConvertToImage().SaveFile(file, wxBITMAP_TYPE_PNG);
}


void MachRegScript::finish()
{
    fprintf(stderr, "[MACHREG] script done failures=%d\n", p_failures);
    fflush(stderr);
    _exit(p_failures ? 1 : 0);
}


void MachRegScript::onTimer(wxTimerEvent&)
{
    if (p_cmds.empty() || p_frameRef.get() == NULL)
    {
        finish();
        return;
    }
    string line = p_cmds.front();
    p_cmds.pop_front();
    fprintf(stderr, "[MACHREG] > %s\n", line.c_str());
    int delay = runCommand(words(line));
    p_timer.StartOnce(delay);
}


//  Returns the delay before the next command.
int MachRegScript::runCommand(const vector<string>& w)
{
    WxMachineRegister* f = p_frame;
    const string& cmd = w[0];
    size_t n = w.size();
    static const char* tabs[] = { "machine", "connection", "codes", "job",
                                  "queues" };

    if (cmd == "select" && n == 2)
    {
        if (f->findRow(w[1]) < 0) fail("select " + w[1] + ": not in the list");
        else f->selectMachine(w[1]);
    }
    else if (cmd == "new") click("new");
    else if (cmd == "save") click("save");
    else if (cmd == "delete") click("delete");
    else if (cmd == "close") f->Close();
    else if (cmd == "tab" && n == 2)
    {
        if (!f->showPage(w[1])) fail("tab " + w[1]);
    }
    else if (cmd == "set" && n >= 2)
    {
        string value = n > 2 ? w[2] : "";
        wxWindow* win = f->field(w[1]);
        if (win == NULL) { fail("no field " + w[1]); return 100; }
        if (wxTextCtrl* t = dynamic_cast<wxTextCtrl*>(win))
        {
            t->SetValue(unescape(value));
        }
        else if (wxSpinCtrl* s = dynamic_cast<wxSpinCtrl*>(win))
            s->SetValue(atoi(value.c_str()));
        else if (wxSpinCtrlDouble* d = dynamic_cast<wxSpinCtrlDouble*>(win))
            d->SetValue(atof(value.c_str()));
        else if (wxCheckBox* b = dynamic_cast<wxCheckBox*>(win))
            b->SetValue(value == "1");
        else if (wxRadioButton* rb = dynamic_cast<wxRadioButton*>(win))
            rb->SetValue(value == "1");
        else if (wxChoice* c = dynamic_cast<wxChoice*>(win))
        {
            if (!c->SetStringSelection(value)) fail("no choice " + value);
            else if (w[1] == "queue") f->showQueue(value);
        }
        //  Programmatic changes send no change event for most controls.
        f->updateDirty();
    }
    else if (cmd == "undo" && n == 2)
    {
        wxWindow* w2 = f->field("undo:" + w[1]);
        if (w2 == NULL) fail("no undo for " + w[1]);
        else if (w2->IsShown()) f->cfgUndo(w[1]);
        else fail("undo " + w[1] + " is not shown");
    }
    else if (cmd == "click" && n == 2) click(w[1]);
    else if (cmd == "words") f->showWords();
    else if (cmd == "code" && n == 2)
    {
        if (!f->selectCodeByName(w[1])) fail("code " + w[1] + ": not listed");
    }
    else if (cmd == "focus" && n == 2)
    {
        wxTextCtrl* t = dynamic_cast<wxTextCtrl*>(f->field(w[1]));
        if (t == NULL) fail("focus " + w[1]);
        else { t->SetFocus(); f->p_lastText = t; }
    }
    else if (cmd == "cursor" && n == 3)
    {
        wxTextCtrl* t = dynamic_cast<wxTextCtrl*>(f->field(w[1]));
        if (t == NULL) fail("cursor " + w[1]);
        else t->SetInsertionPoint(atoi(w[2].c_str()));
    }
    else if ((cmd == "words-pick" || cmd == "words-activate") && n == 2)
    {
        wxListCtrl* l = f->p_wordsList;
        long row = -1;
        for (long i = 0; l && i < l->GetItemCount(); i++)
            if ((string)l->GetItemText(i) == w[1]) row = i;
        if (row < 0) fail(cmd + " " + w[1] + ": no such placeholder");
        else if (cmd == "words-pick")
            l->SetItemState(row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        else
        {
            wxListEvent ev(wxEVT_LIST_ITEM_ACTIVATED, l->GetId());
            ev.SetEventObject(l);
            ev.m_itemIndex = row;
            l->GetEventHandler()->ProcessEvent(ev);
        }
    }
    else if (cmd == "mark-size") { p_size = f->GetSize(); }
    else if (cmd == "queue-apply") click("queue-apply");
    else if (cmd == "queue-remove") click("queue-remove");
    else if (cmd == "queue-clear") click("queue-clear");
    else if (cmd == "answer" && n == 2)
    {
        f->p_answers.push_back(w[1] == "yes" ? wxID_YES
                             : w[1] == "no" ? wxID_NO : wxID_CANCEL);
    }
    else if (cmd == "wait" && n == 2)
        return atoi(w[1].c_str());
    else if (cmd == "dump")
    {
        fprintf(stderr, "[MACHREG] dump loaded='%s' from='%s' dirty=%d "
                "save=%d delete=%d title='%s' rows=%d\n",
                f->p_loadedName.c_str(), f->p_loadedFrom.c_str(),
                (int)f->isDirty(), (int)f->p_saveButton->IsEnabled(),
                (int)f->p_deleteButton->IsEnabled(),
                (const char*)f->GetTitle().c_str(), (int)f->p_rows.size());
    }
    else if (cmd == "snapshot" && n == 2)
    {
        static const char* names[] = { "machine", "connection", "codes",
                                       "job-script", "queues" };
        for (int i = 4; i >= 0; i--)
        {
            p_cmds.push_front(string("shot ") + w[1] + "/" +
                              std::to_string(i + 1) + "-" + names[i] + ".png");
            p_cmds.push_front("wait 800");
            p_cmds.push_front(string("tab ") + tabs[i]);
        }
    }
    else if ((cmd == "shot" || cmd == "shot-dialog") && n == 2)
    {
        if (!shot(w[1], cmd == "shot-dialog")) fail("could not write " + w[1]);
        else fprintf(stderr, "[MACHREG] wrote %s\n", w[1].c_str());
    }
    else if (cmd == "expect" && n >= 3)
    {
        const string& what = w[1];
        string v = w[2];
        if (what == "dirty")
            expectEq("dirty", f->isDirty() ? "1" : "0", v);
        else if (what == "save-enabled")
            expectEq("save-enabled", f->p_saveButton->IsEnabled() ? "1" : "0", v);
        else if (what == "delete-enabled")
            expectEq("delete-enabled", f->p_deleteButton->IsEnabled() ? "1" : "0", v);
        else if (what == "title-star")
            expectEq("title-star", f->GetTitle().StartsWith("*") ? "1" : "0", v);
        else if (what == "banner")
            expectEq("banner", f->p_info->IsShown() ? "1" : "0", v);
        else if (what == "field" || what == "label")
            expectEq(what + " " + w[2], get(w[2]), unescape(n > 3 ? w[3] : ""));
        else if (what == "contains" && n >= 4)
        {
            string got = get(w[2]), want = unescape(w[3]);
            if (got.find(want) == string::npos)
                fail("expect " + w[2] + " to contain '" + want + "', got '" +
                     got + "'");
            else
                fprintf(stderr, "[MACHREG] ok %s contains '%s'\n",
                        w[2].c_str(), want.c_str());
        }
        else if (what == "size-kept")
            expectEq("size-kept", f->GetSize().x >= p_size.x &&
                     f->GetSize().y >= p_size.y ? "1" : "0", v);
        else if (what == "raw-dialog")
            expectEq("raw-dialog", f->rawDialogOpen() ? "1" : "0", v);
        else if (what == "advanced")
            expectEq("advanced", f->p_advanced && f->p_advanced->IsShown()
                                 ? "1" : "0", v);
        else if (what == "tooltip" && n >= 4)
        {
            wxWindow* win = f->field(w[2]);
            string got = win ? (string)win->GetToolTipText() : "";
            if (got.find(unescape(w[3])) == string::npos)
                fail("expect tooltip of " + w[2] + " to contain '" +
                     unescape(w[3]) + "', got '" + got + "'");
            else
                fprintf(stderr, "[MACHREG] ok tooltip %s\n", w[2].c_str());
        }
        else if (what == "help-ref" && n >= 4)
            expectEq("help-ref " + w[2], WxMachineRegister::helpRef(w[2]), w[3]);
        else if (what == "shown" && n >= 4)
        {
            wxWindow* win = f->field(w[2]);
            expectEq("shown " + w[2], win && win->IsShown() ? "1" : "0", w[3]);
        }
        else if (what == "enabled" && n >= 4)
        {
            wxWindow* win = f->field(w[2]);
            expectEq("enabled " + w[2], win && win->IsEnabled() ? "1" : "0", w[3]);
        }
        else if (what == "code-listed" && n >= 4)
        {
            bool listed = false;
            for (size_t i = 0; i < f->p_codeShown.size(); i++)
                if (f->p_codeNames[f->p_codeShown[i]] == w[2]) listed = true;
            expectEq("code-listed " + w[2], listed ? "1" : "0", w[3]);
        }
        else if (what == "tab")
        {
            int sel = f->p_book->GetSelection();
            expectEq("tab", sel >= 0 && sel < 5 ? tabs[sel] : "?", w[2]);
        }
        else if (what == "list" && n == 4)
        {
            string from = "<absent>";
            for (size_t i = 0; i < f->p_rows.size(); i++)
                if (f->p_rows[i].name == w[2]) from = f->p_rows[i].from;
            expectEq("list " + w[2], from, w[3]);
        }
        else if (what == "message")
        {
            if (f->p_lastMessage.find(w[2]) == string::npos)
                fail("expect message '" + w[2] + "' in '" + f->p_lastMessage + "'");
            else
                fprintf(stderr, "[MACHREG] ok message %s\n", w[2].c_str());
        }
        else fail("bad expect: " + what);
    }
    else if (cmd == "quit")
        finish();
    else
        fail("bad command: " + cmd);

    return 150;
}
