/**
 * @file
 *
 *  Test hook for the Launcher; see the header.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dcmemory.h"

#include "WxLauncher.H"
#include "WxLauncherScript.H"
#include "wxgui/WindowShot.H"

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


LauncherScript::LauncherScript(WxLauncher* frame, const string& file)
    : p_frameRef(frame), p_frame(frame), p_timer(this), p_failures(0),
      p_loaded(false)
{
    std::ifstream in(file.c_str());
    p_loaded = in.good();
    string line;
    while (std::getline(in, line))
        if (!words(line).empty())
            p_cmds.push_back(line);
    Bind(wxEVT_TIMER, &LauncherScript::onTimer, this);
}


void LauncherScript::start()
{
    if (!p_loaded)
        fail("cannot read the script");
    p_timer.StartOnce(500);
}


void LauncherScript::fail(const string& what)
{
    p_failures++;
    fprintf(stderr, "[LAUNCHER] FAIL %s\n", what.c_str());
}


bool LauncherScript::shot(const string& file)
{
    return ecceWindowShot(p_frame, file);
}


void LauncherScript::finish()
{
    fprintf(stderr, "[LAUNCHER] script done failures=%d\n", p_failures);
    fflush(stderr);
    _exit(p_failures ? 1 : 0);
}


void LauncherScript::onTimer(wxTimerEvent&)
{
    if (p_cmds.empty() || p_frameRef.get() == NULL)
    {
        finish();
        return;
    }
    string line = p_cmds.front();
    p_cmds.pop_front();
    fprintf(stderr, "[LAUNCHER] > %s\n", line.c_str());
    int delay = runCommand(words(line));
    p_timer.StartOnce(delay);
}


int LauncherScript::runCommand(const vector<string>& w)
{
    const string& cmd = w[0];
    size_t n = w.size();

    if (cmd == "machine" && n == 2)
    {
        if (!p_frame->selectMachine(w[1]))
            fail("machine " + w[1] + ": not in the list");
    }
    else if (cmd == "pick" && n == 2)
    {
        if (!p_frame->pickMachine(w[1]))
            fail("pick " + w[1] + ": not in the list");
    }
    else if (cmd == "editable" && n == 2)
        p_frame->hookEditAllowed(w[1] == "1");
    else if (cmd == "queue" && n == 2)
    {
        if (!p_frame->selectQueue(w[1]))
            fail("queue " + w[1] + ": not in the list");
    }
    else if (cmd == "click" && n == 2 && w[1] == "machine-settings")
    {
        if (!p_frame->clickMachineSettings())
            fail("the Machine settings button is missing or disabled");
    }
    else if (cmd == "click" && n == 2 && w[1] == "advanced")
        p_frame->toggleAdvanced();
    else if (cmd == "username" && n == 2)
        p_frame->setUsername(w[1]);
    else if (cmd == "launch" && n == 1)
    {
        if (!p_frame->clickLaunch())
            fail("the Launch button is missing or disabled");
    }
    else if (cmd == "procs" && n == 2)
        p_frame->setProcessors(atoi(w[1].c_str()));
    else if (cmd == "mpicheck" && n == 2)
        fprintf(stderr, "[LAUNCHER] mpicheck %s: %s\n", w[1].c_str(),
                p_frame->confirmMpiLauncher(w[1]) ? "go" : "cancel");
    else if (cmd == "sizecheck" && n == 1)
        fprintf(stderr, "[LAUNCHER] sizecheck: %s\n",
                p_frame->confirmQmSize() ? "go" : "cancel");
    else if (cmd == "launchwarn" && n == 2)
    {
        // a literal \n in the word stands for a newline
        string t = w[1];
        for (size_t at; (at = t.find("\\n")) != string::npos; )
            t.replace(at, 2, "\n");
        fprintf(stderr, "[LAUNCHER] launchwarn: %s\n",
                p_frame->confirmLaunchWarning(t) ? "go" : "cancel");
    }
    else if (cmd == "rundir" && n == 2)
        p_frame->setRunDirectory(w[1]);
    else if (cmd == "account" && n == 2)
        p_frame->setAccount(w[1]);
    else if (cmd == "profile" && n == 2)
    {
        if (!p_frame->hookProfile(w[1]))
            fail("profile " + w[1] + ": not in the list");
    }
    else if (cmd == "profile-save" && n == 2)
    {
        if (!p_frame->hookSaveProfile(w[1]))
            fail("profile-save " + w[1] + ": refused");
    }
    else if (cmd == "profile-delete" && n == 2)
    {
        if (!p_frame->hookDeleteProfile(w[1]))
            fail("profile-delete " + w[1] + ": no such profile");
    }
    else if (cmd == "reload")
        p_frame->machRegChanged();
    else if (cmd == "exec" && n >= 2)
    {
        int rc = system(w[1].c_str());
        if (rc != 0)
            fail("exec '" + w[1] + "' returned " + std::to_string(rc));
    }
    else if (cmd == "expect" && n >= 3)
    {
        string got = p_frame->hookGet(w[1]);
        string want = w[2];
        if (got != want)
            fail("expect " + w[1] + ": got '" + got + "', wanted '" + want + "'");
        else
            fprintf(stderr, "[LAUNCHER] ok %s = '%s'\n", w[1].c_str(), got.c_str());
    }
    else if (cmd == "print" && n == 2)
        fprintf(stderr, "[LAUNCHER] %s = %s\n", w[1].c_str(),
                p_frame->hookGet(w[1]).c_str());
    else if (cmd == "wait" && n == 2)
        return atoi(w[1].c_str());
    else if (cmd == "shot" && n == 2)
    {
        if (!shot(w[1]))
            fail("shot " + w[1]);
    }
    else if (cmd == "quit")
        finish();
    else
        fail("bad command: " + cmd);
    return 200;
}
